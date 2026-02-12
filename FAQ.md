# Frequently Asked Questions (FAQ)

## General Questions

### Q: What exactly is this project?

**A:** This is an **educational Linux kernel module** that demonstrates advanced process hiding techniques using ftrace syscall hooking. It's designed to teach kernel-level concepts and security vulnerabilities, not for malicious use.

### Q: Is this a malware/virus?

**A:** No. This code:

- ✅ Requires manual compilation
- ✅ Requires explicit root execution
- ✅ Doesn't auto-infect systems
- ✅ Requires kernel headers and build tools
- ✅ Includes extensive documentation
- ✅ Is clearly educational in nature

It's similar to teaching how SQL injection works - explaining the technique is different from exploiting it maliciously.

### Q: Is this legal to use?

**A:**

- ✅ **Legal** for educational use on systems you own
- ✅ **Legal** for authorized security testing (with written permission)
- ✅ **Legal** for security research in controlled environments
- ❌ **Illegal** for unauthorized system access (Computer Fraud and Abuse Act - CFAA)

**Always get explicit written permission before testing on any system you don't own.**

### Q: Where can I learn more about kernel programming?

**A:**

- Linux Kernel Documentation: https://www.kernel.org/doc/html/latest/
- LKM Programming Guide: https://linux.die.net/lkmpg/
- ftrace Documentation: https://www.kernel.org/doc/html/latest/trace/ftrace.html
- "Understanding the Linux Kernel" book

---

## Installation & Setup

### Q: Can I run this on macOS or Windows?

**A:** No. This project requires:

- **Linux kernel** (specifically)
- **x86-64 architecture**
- **Kernel 5.0 or higher**

Windows and macOS use different kernels with different syscall interfaces.

### Q: What are the minimum system requirements?

**A:**

- **OS**: Linux (any distribution)
- **Kernel**: 5.0+ (tested on 5.7+)
- **Architecture**: x86-64 only
- **RAM**: 512 MB minimum
- **Disk**: 200 MB free
- **Access**: Root/sudo required

### Q: Do I need to know C/kernel programming?

**A:**

- **To run it**: No, just follow the Quick Start
- **To understand it**: Some C knowledge helpful
- **To modify it**: Kernel programming knowledge needed

### Q: Which Linux distributions are supported?

**A:** Any distribution with:

- A 5.0+ kernel
- gcc compiler
- Package manager (apt, pacman, dnf, yum)

**Tested on**:

- ✅ Ubuntu 20.04+
- ✅ Debian 10+
- ✅ Fedora 30+
- ✅ CentOS 8+
- ✅ Arch Linux
- ✅ Kali Linux

### Q: Do I need internet access during installation?

**A:** Only if kernel headers need to be downloaded, which requires package manager access.

### Q: What does the script do automatically?

**A:** The `bash.sh` script automatically:

1. Checks if you're running with sudo
2. Detects Linux distribution
3. Checks for kernel headers
4. Installs missing dependencies
5. Compiles the kernel module
6. Compiles the user-space program
7. Loads the module
8. Creates/manages systemd service
9. Displays system information

---

## Usage & Functionality

### Q: What does "signal 64" mean?

**A:** Signal 64 is an arbitrary signal number chosen as a "magic signal" to trigger process hiding. It's defined in the code and doesn't have standard meaning in Linux:

```c
if (sig == 64)  // Magic signal for hiding
{
    // Hide this PID
}
```

To hide a process: `kill -64 <PID>`

### Q: Can I hide multiple processes?

**A:** **Not with current code** - it stores only one PID at a time.

**To hide multiple**: Extend `hide_pid` to an array:

```c
// Instead of:
static char hide_pid[NAME_MAX];

// Use:
#define MAX_HIDDEN 100
static char hide_pids[MAX_HIDDEN][NAME_MAX];
static int num_hidden = 0;
```

### Q: What does the PCB (Process Control Block) display mean?

**A:** The PCB shows:

- **PID**: Process identifier (your hidden process)
- **PPID**: Parent process ID
- **UID/GID**: User and group (0 = root)
- **Priority**: Dynamic kernel priority (lower = more CPU)
- **Nice Value**: User hint to scheduler (-20 to +19)

### Q: Can I hide processes other people created?

**A:** Only if you can:

1. Run as root
2. Know their PID
3. Have permission to send signals

In practice, you generally hide your own processes.

### Q: How is the process hidden?

**A:** By removing it from directory listings:

1. Kernel module intercepts `getdents64()` syscall
2. Removes entries from `/proc` directory listing
3. So `ps aux` doesn't see it
4. But process still exists and runs!

### Q: Is the process actually hidden or just invisible?

**A:** **Just invisible** - it's not hidden from:

- Direct memory inspection
- Kernel-level tools
- Root user's investigation
- Network traffic analysis
- The processor itself

It won't be listed in `ps` or `/proc`, but forensic analysis could find it.

---

## Technical Details

### Q: How is this different from regular rootkits?

**A:**
| Aspect | This Project | Traditional Rootkit |
|--------|-------------|-------------------|
| Purpose | Educational | Malicious persistence |
| Loading | Manual sudo | Auto-loading, hidden |
| Removal | Easy cleanup | Designed to hide |
| Code availability | Fully documented | Obfuscated |
| Logging | Verbose kernel logs | Silent operation |

### Q: Does this work on newer kernels?

**A:** The code supports **5.7+ kernels** with dynamic symbol lookup:

```c
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    // Use kprobes for newer kernels
    register_kprobe(&kp);
#endif
```

**May need updating for kernel 6.0+** - kernel API changes over time.

### Q: Why use ftrace instead of syscall table hooking?

**A:** ftrace is better because:

- ✅ More stable across kernel versions
- ✅ Designed for kernel instrumentation
- ✅ Built-in feature (not bypassing security)
- ✅ Works with newer kernel protections
- ❌ Older methods: direct table modification (unstable)

### Q: What's the performance impact?

**A:** Minimal:

- **Per syscall**: ~300-500 CPU cycles overhead
- **Memory**: ~100 KB kernel module + buffers
- **Impact**: Invisible unless 1000s of getdents calls/second

### Q: Is there a way to detect this?

**A:** Yes, multiple detection methods:

1. **Module detection**: `lsmod | grep hide_process`
2. **Kernel logs**: `dmesg | grep hide_process`
3. **Behavioral**: Process CPU usage without visibility
4. **Memory inspection**: Kernel memory dumps
5. **Advanced**: Kernel integrity checking (IMA, EVM)

---

## Troubleshooting

### Q: "Headers du noyau introuvables" error?

**A:**

```bash
# Let script auto-install, OR manually:
sudo apt-get install -y linux-headers-$(uname -r)
```

Then retry the installation.

### Q: "Module fails to load" with insmod error?

**A:**

```bash
# Check what's wrong:
sudo dmesg | tail -20

# Likely causes:
# 1. Kernel headers mismatch
# 2. Secure Boot enabled
# 3. SELinux restrictions
# 4. Kernel too old
```

### Q: Process still visible after hiding?

**A:**

```bash
# Verify module loaded:
lsmod | grep hide_process

# Check kernel logs:
sudo dmesg | grep "Hiding PID"

# Verify signal 64 was sent:
sudo dmesg | tail -20
```

### Q: Getting "Permission denied" errors?

**A:** Must use `sudo`:

```bash
sudo ./bash.sh          # Correct
./bash.sh               # Wrong - permission denied
```

### Q: Script hangs during compilation?

**A:**

- May take several minutes on slow systems
- Press Ctrl+C to cancel
- Check disk space: `df -h`
- Check CPU: `top` in another terminal

### Q: Service won't start?

**A:**

```bash
# Check service status:
sudo systemctl status mon_processus_resident

# View error logs:
sudo journalctl -u mon_processus_resident -n 50

# Check if module loaded:
lsmod | grep hide_process

# Restart:
sudo systemctl restart mon_processus_resident
```

### Q: Can't uninstall/unload module?

**A:**

```bash
# Module may be in use
# First, stop the service:
sudo systemctl stop mon_processus_resident

# Wait a moment:
sleep 2

# Then unload:
sudo rmmod hide_process

# Or use the script:
sudo ./bash.sh    # Select option 5
```

### Q: "Kernel headers mismatch" error?

**A:** Your headers don't match running kernel:

```bash
# Check kernel version:
uname -r

# Install correct headers:
sudo apt-get install -y linux-headers-$(uname -r)

# Reboot if headers installed:
sudo reboot
```

---

## Security & Safety

### Q: Is it safe to run this?

**A:**

- ✅ Safe on systems you own
- ✅ Safe in virtual machines (easy to revert)
- ✅ Safe on test systems
- ❌ NOT safe on production systems
- ❌ NOT safe on systems you don't control

**Best practice**: Test in virtual machine first!

### Q: Can this harm my system?

**A:** Possible issues:

- System instability if kernel module bugs
- Unexpected process behavior
- Requires reboot to fully unload
- Data loss if not properly removed

**Mitigation**:

- Use virtual machine
- Keep backups
- Test in isolated environment
- Have physical access/recovery method

### Q: What if I can't uninstall?

**A:** You can force removal:

```bash
# Kill the service:
sudo systemctl kill mon_processus_resident

# Force unload module:
sudo rmmod -f hide_process

# Remove service file:
sudo rm /etc/systemd/system/mon_processus_resident.service

# Reload systemd:
sudo systemctl daemon-reload

# In worst case: Reboot
sudo reboot
```

### Q: Is my kernel source exposed?

**A:** No - we never access/modify kernel source files. We only:

- Use kernel headers (public)
- Use standard kernel APIs
- Load compiled module

---

## Advanced Questions

### Q: Can I modify this for other purposes?

**A:** Yes, for **educational purposes**:

- Hide different syscalls (not getdents)
- Monitor system calls
- Implement your own hooks
- Create detection tools

**Prohibited modifications**:

- For malicious use
- For unauthorized access
- Without educational intent
- Without proper warnings

### Q: How would this be detected?

**A:**

1. **Kernel logs**: Module loads leave traces
2. **Process analysis**: Hidden process still uses CPU/memory
3. **Network traffic**: Hidden process still networks
4. **Forensics**: Memory dumps reveal module
5. **Advanced**: Kernel integrity checking (IMA)

### Q: Can I add this to a real rootkit?

**A:** Technically maybe, but:

- ❌ It's illegal without authorization
- ❌ This code is designed for education, not stealth
- ❌ It's obvious when loaded (lots of logging)
- ❌ Easy to detect (kernel modules visible)

### Q: What's the persistence mechanism?

**A:** Systemd service:

```bash
/etc/systemd/system/mon_processus_resident.service
├─ Starts at boot via WantedBy=multi-user.target
├─ Restarts on crash (Restart=always)
├─ Runs with elevated privileges (root)
└─ Can be stopped/disabled (not truly persistent)
```

To truly persist, you'd need:

- Bootloader modification
- Filesystem-level hiding
- Initial boot code injection

...which is beyond this educational project.

---

## Learning & Education

### Q: How do I learn from this?

**A:**

1. **Run it** - See the code working
2. **Read docs** - Understand architecture
3. **Read code** - Study implementation
4. **Modify it** - Change parameters, add logging
5. **Break it** - Intentionally cause errors
6. **Fix it** - Debug and understand
7. **Extend it** - Add new features

### Q: What concepts does this teach?

**A:** Linux kernel concepts:

- ✅ Module loading and unloading
- ✅ Syscall interception (ftrace)
- ✅ Process management
- ✅ Privilege levels (ring 0/3)
- ✅ Memory protection
- ✅ System call interface
- ✅ Kernel data structures
- ✅ Systemd service management

### Q: Can I use this in school?

**A:**

- ✅ For learning kernel programming
- ✅ For security course projects
- ✅ For demonstration of concepts
- ❌ For actual system compromise

**Check with your professor first** - ethical guidelines vary by institution.

### Q: Where can I find source code similar to this?

**A:**

- Linux Kernel source: https://github.com/torvalds/linux
- LKMPG: https://linux.die.net/lkmpg/
- Kernel module examples: Various GitHub repositories
- Academic papers: Kernel security research

---

## Contact & Support

### Q: I found a bug, where do I report it?

**A:** Open an issue on GitHub with:

- Clear description
- Kernel version and distro
- Steps to reproduce
- Error messages/logs

### Q: Can I contribute improvements?

**A:** Yes! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Q: Is there commercial support?

**A:** This is an educational open-source project. No commercial support available.

### Q: How do I get help?

**A:**

1. Read [README.md](README.md)
2. Check [QUICKSTART.md](QUICKSTART.md)
3. Review [ARCHITECTURE.md](ARCHITECTURE.md)
4. Check code comments
5. Search existing issues
6. Open a new issue if needed

---

## Last Resort Questions

### Q: What if nothing works?

**A:** Step-by-step recovery:

```bash
# 1. Bad state? Use clean slate:
sudo ./bash.sh --uninstall

# 2. Module stuck? Force removal:
sudo rmmod -f hide_process

# 3. Still broken? Reboot:
sudo reboot

# 4. After reboot, try again:
cd /path/to/rootkit
sudo ./bash.sh --test
```

### Q: What if I accidentally broke something?

**A:** Likely scenarios:

- **Service won't stop**: Use `-f` flag
- **Module won't unload**: Reboot
- **Files stuck**: Use `rm -r` carefully
- **Systemd broken**: Rescue mode boot

### Q: Can I delete everything and start fresh?

**A:** Yes, completely safe:

```bash
# Full removal:
sudo ./bash.sh --uninstall

# Reboot to clear kernel state:
sudo reboot

# Download fresh copy if needed:
git clone <repository>
```

---

## Summary

**This is an educational tool for learning Linux kernel concepts.**

Use it responsibly, learn from it thoroughly, and understand that the techniques it demonstrates have serious legal and ethical implications in real-world scenarios.

**Questions?** Check the full documentation, review the code comments, and feel free to experiment in a safe environment!
