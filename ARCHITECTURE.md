# Architecture & Design Overview

## Project Architecture

```
┌─────────────────────────────────────────────────────┐
│         User-Space Applications                      │
├─────────────────────────────────────────────────────┤
│  programme.c (Process Manager & Service)            │
│  ├─ creer_et_gerer_processus()                      │
│  ├─ create_systemd_service()                        │
│  ├─ modifier_priorite()                             │
│  └─ daemon_process()                                │
└──────────────────────┬──────────────────────────────┘
                       │ fork, kill, setpriority
                       │
┌──────────────────────▼──────────────────────────────┐
│       System Call Interface (syscall layer)         │
├─────────────────────────────────────────────────────┤
│  __x64_sys_kill()          ← Signal delivery        │
│  __x64_sys_getdents64()    ← Modern dir listing     │
│  __x64_sys_getdents()      ← Legacy dir listing     │
└──────────────────────┬──────────────────────────────┘
                       │ ftrace interception
                       │
┌──────────────────────▼──────────────────────────────┐
│       Kernel Module (LKM): hide_process.c           │
├─────────────────────────────────────────────────────┤
│  hide_process.ko                                    │
│  ├─ hook_kill()        [catches signal 64]         │
│  ├─ hook_getdents64()  [filters /proc entries]     │
│  ├─ hook_getdents()    [filters legacy entries]    │
│  ├─ ftrace_thunk()     [ftrace dispatcher]         │
│  └─ fh_install_hooks() [hook registration]         │
└──────────────────────┬──────────────────────────────┘
                       │ ftrace framework
                       │
┌──────────────────────▼──────────────────────────────┐
│            Linux Kernel (5.7+)                      │
├─────────────────────────────────────────────────────┤
│  Ftrace Infrastructure                              │
│  Process Scheduling                                 │
│  System Call Dispatcher                             │
│  Filesystem / /proc interface                       │
└─────────────────────────────────────────────────────┘
```

## Component Interaction Flow

### Process Hiding Sequence

```
1. User creates process
   └─ fork() → new child process created

2. Parent sends magic signal
   └─ kill(pid, 64) → signal delivery requested

3. Kernel intercepts syscall
   └─ sys_kill() → ftrace hook activated

4. Kernel module processes signal
   └─ hook_kill() → signal 64 detected
   └─ stores PID in global hide_pid variable
   └─ returns 0 (consumed, no further processing)

5. Later: getdents syscall called
   └─ userspace: ls /proc or ps command
   └─ kernel: sys_getdents64() called
   └─ ftrace intercepts → hook_getdents64() called

6. Directory filtering applied
   └─ kernel buffer copied to kernel space
   └─ entries compared against hide_pid
   └─ matching entries removed
   └─ modified buffer copied back to user space

7. Result
   └─ process invisible via /proc, ps, top, ls
   └─ but still running, consuming CPU, memory, etc.
```

## Key Data Structures

### Global State

```c
// In kernel module:
static char hide_pid[NAME_MAX];  // Currently hidden PID as string
                                 // e.g., "3421"
                                 // Only one PID at a time!
```

### Directory Entry Structures

```c
// Modern format (getdents64):
struct linux_dirent64
{
    u64        d_ino;      // Inode number
    s64        d_off;      // Offset in directory
    unsigned short d_reclen; // Record length
    unsigned char d_type;    // File type
    char       d_name[];     // Filename (variable length)
                             // This is compared against hide_pid!
};

// Legacy format (getdents):
struct linux_dirent
{
    unsigned long d_ino;     // Inode number
    unsigned long d_off;     // Offset
    unsigned short d_reclen; // Record length
    char           d_name[]; // Filename (compared here!)
};
```

### Ftrace Hook Structure

```c
struct ftrace_hook
{
    const char *name;           // Symbol name: "__x64_sys_kill"
    void *function;             // Hook function: hook_kill
    void *original;             // Pointer to save original: &orig_kill
    unsigned long address;      // Runtime symbol address
    struct ftrace_ops ops;      // Ftrace operations
};
```

## Compilation & Dependency Flow

```
bash.sh (Deployment Script)
├─ check_root()                 ← Verify sudo
├─ check_dependencies()         ← Install headers if needed
│  ├─ apt-get / pacman / dnf
│  └─ linux-headers installation
├─ compile_program()            ← gcc programme.c → programme
├─ compile_kernel_module()      ← make hide_process.ko
│  ├─ generate_makefile()
│  ├─ make -C /lib/modules/..   ← Kernel build system
│  └─ gcc + kernel headers
└─ load_kernel_module_temp()    ← insmod hide_process.ko
```

## Memory Layout During Execution

### In Kernel Space (Module Loaded)

```
┌─────────────────────────────────────┐
│     Kernel Module Memory            │
├─────────────────────────────────────┤
│ hide_pid[NAME_MAX]   ← Global state │
│ hooks[] array        ← Hook configs │
│ ftrace_ops          ← Ftrace data   │
│ hook functions      ← Our code      │
│  - hook_kill        │
│  - hook_getdents64  │
│  - hook_getdents    │
│  - ftrace_thunk     │
│  - etc.             │
└─────────────────────────────────────┘
```

### During getdents64 Hook

```
┌─ System Memory ──────────────────────────┐
│ User-space buffer  (in user memory)      │
│ └─ original /proc entries                │
│    (containing "3422" directory)         │
│                                          │
│ Kernel buffer (kmalloc'd)               │
│ └─ copy of entries (from copy_from_user)│
│    └─ filtered by hook_getdents64()     │
│       └─ "3422" removed!                 │
│                                          │
│ Modified buffer returned (copy_to_user) │
│ └─ user gets filtered list              │
│    └─ no "3422" visible in userspace!   │
└──────────────────────────────────────────┘
```

## Execution Timeline

### Time T=0: Module Load

```
insmod hide_process.ko
  ├─ hide_process_init()
  ├─ fh_install_hooks()
  │  ├─ lookup_name("__x64_sys_kill")
  │  ├─ lookup_name("__x64_sys_getdents64")
  │  ├─ lookup_name("__x64_sys_getdents")
  │  ├─ ftrace_set_filter_ip() × 3
  │  └─ register_ftrace_function() × 3
  └─ Hooks ACTIVE ✓

Kernel logs:
  [INFO] hide_process: Module loaded
  [INFO] hide_process: Hooks installed successfully
```

### Time T=1s: Child Process Created

```
programme → fork()
  ├─ Parent PID: 3421
  └─ Child PID: 3422 (infinite loop sleep)

Output:
  PID DU PROCESSUS FILS (ENFANT) : 3422
```

### Time T=2s: Signal Sent

```
Parent → kill(3422, 64)
  ├─ Kernel intercepts syscall
  ├─ ftrace_thunk() called
  ├─ hook_kill() intercepted
  ├─ Checks signal == 64 ✓
  ├─ snprintf(hide_pid, "3422")
  └─ returns 0 (consumed)

Kernel logs:
  [INFO] hide_process: Hiding PID 3422
```

### Time T=3s: Directory Listing

```
User: ps aux
  ├─ calls getdents64("/proc")
  ├─ ftrace_thunk() called
  ├─ hook_getdents64() intercepted
  │  ├─ copy_from_user() - get original entries
  │  ├─ Iterate through entries
  │  │  └─ if (strcmp(hide_pid, d_name) == 0)
  │  │     └─ SKIP this entry! ✓
  │  └─ copy_to_user() - return filtered list
  └─ ps shows everything EXCEPT 3422 ✓
```

## Kernel Version Compatibility

### Before Kernel 5.7

```c
#define KALLSYMS_LOOKUP_NAME kallsyms_lookup_name
// Direct export available - simpler
unsigned long addr = kallsyms_lookup_name("__x64_sys_kill");
```

### Kernel 5.7+

```c
#include <linux/kprobes.h>

// kallsyms_lookup_name not exported
// Use kprobe to find it dynamically
struct kprobe kp = {.symbol_name = "kallsyms_lookup_name"};
register_kprobe(&kp);
unsigned long (*kallsyms_lookup_name_ptr)(const char*) = kp.addr;
unregister_kprobe(&kp);
```

## Service Architecture

### Systemd Service Integration

```ini
[/etc/systemd/system/mon_processus_resident.service]

[Unit]
Description=Processus Résident avec Priorité Modifiée
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/mon_processus_resident --daemon
    ↓
    fork()
    ↓
    daemon_process()
    ├─ kill(getpid(), 64)  ← Try to hide self
    ├─ setpriority(-5)      ← Increase CPU priority
    └─ loop:
        ├─ log_message()
        └─ sleep(30)

Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

## Error Handling Flow

```
Error Detection
├─ Hook installation fails
│  └─ ftrace_set_filter_ip() returns error
│  └─ printk(KERN_ERR "...")
│  └─ fh_install_hooks() returns error code
│  └─ hide_process_init() fails
│  └─ Module won't load
│
├─ Memory allocation fails
│  └─ kmalloc() returns NULL
│  └─ getdents hook returns original untouched
│  └─ Process NOT hidden (safe fallback)
│
└─ Copy operations fail
   └─ copy_from_user() / copy_to_user() fail
   └─ Return original buffer unchanged
   └─ Process NOT hidden (safe fallback)
```

## Performance Considerations

### Hook Overhead

Every syscall goes through:

```
Normal syscall
  → ftrace check: is IP in filtered set?

If in filtered set:
  → ftrace_thunk() called
  → Hook function decides action
  → ~200-500 CPU cycles per call

Impact: Minimal for syscalls we care about
```

### Memory Usage

```
Kernel module:     ~50-100 KB
hide_pid string:   256 bytes
ftrace hooks:      ~1 KB per hook × 3
Per-operation:     kmalloc(getdents_result_size) during syscall
                   → freed immediately after
```

### Hiding Quality

❌ **NOT hidden from**:

- Direct memory inspection
- Root user (kernel tools)
- eBPF programs
- Kernel module listing (lsmod)

✅ **Hidden from**:

- User-space process enumeration (ps, top)
- /proc directory listing
- Standard inspection tools
- Non-root users

---

## Next Steps

- Explore individual files for detailed documentation
- Review inline code comments
- Test different kernel versions
- Extend for multiple hidden PIDs
- Develop detection mechanisms
