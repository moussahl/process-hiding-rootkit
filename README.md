# Process Hiding Rootkit - Educational Project

A Linux kernel module (LKM) demonstration that illustrates advanced kernel-level process manipulation using **ftrace syscall hooking**. This project showcases how processes can be hidden from user-space utilities and modified in the Process Control Block (PCB).

**⚠️ DISCLAIMER**: This is an **educational and research project only**. It demonstrates kernel security concepts for learning purposes. Unauthorized access to computer systems is illegal. Use only on systems you own or have explicit permission to test.

---

## Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Usage](#usage)
- [How It Works](#how-it-works)
- [Technical Details](#technical-details)
- [License](#license)

---

## Features

### 🔒 Core Capabilities

- **Syscall Hooking**: Uses ftrace to intercept and modify system calls at kernel level
- **Process Hiding**: Hides processes from `ps`, `top`, and `/proc` enumeration
- **Dynamic PCB Modification**: Modifies Process Control Block attributes (priority, scheduling)
- **Signal-Based Activation**: Uses signal 64 as a "magic signal" to trigger process hiding
- **Systemd Integration**: Can be deployed as a persistent systemd service
- **Kernel Version Compatibility**: Supports kernels 5.7+ with dynamic symbol resolution via kprobes

### 📋 Components

1. **Kernel Module** (`hide_process.c`)
   - Ftrace-based syscall hooking
   - Hooks: `__x64_sys_kill`, `__x64_sys_getdents64`, `__x64_sys_getdents`
   - Process hiding from directory enumeration

2. **User-Space Program** (`programme.c`)
   - Forks child processes with modified priorities
   - Creates systemd service for persistence
   - PCB monitoring and display

3. **Deployment Script** (`bash.sh`)
   - Automated compilation and installation
   - Dependency checking
   - Service management
   - Interactive menu system

---

## Project Structure

```
rootkit/
├── hide_process.c        # Linux kernel module (ftrace hooks)
├── programme.c           # User-space program (PCB manipulation)
├── bash.sh              # Deployment and management script
├── Makefile             # Kernel module build configuration (auto-generated)
└── README.md            # This file
```

---

## Prerequisites

### System Requirements

- **Linux Kernel**: 5.0+ (tested on 5.7+)
- **Architecture**: x86-64
- **Root/sudo access** (required for kernel module operations)

### Dependencies

The deployment script automatically checks and installs:

- `build-essential` - GCC compiler and build tools
- `linux-headers-$(uname -r)` - Kernel header files
- `kmod` - Kernel module utilities (insmod, rmmod, depmod)

**Automatic installation supported on**:

- Debian/Ubuntu (apt-get)
- Arch Linux (pacman)
- RHEL/CentOS (dnf/yum)
- Other distributions with compatible package managers

---

## Installation

### Quick Start

```bash
# Clone or download the project
cd rootkit

# Make the deployment script executable
chmod +x bash.sh

# Run with sudo for interactive menu
sudo ./bash.sh
```

### Menu Options

```
1) Installation Complète (Full Install)    - Compile, load, and install persistently
2) Test Interactif (Interactive Test)      - Compile and test without persistence
3) Vérifier statut (Check Status)          - Display service status
4) Logs                                    - View system logs
5) Désinstaller tout (Uninstall All)       - Remove all components
6) Quitter (Exit)                          - Exit menu
```

### Full Installation (Option 1)

```bash
sudo ./bash.sh     # Select option 1
```

This will:

1. Check kernel headers and dependencies
2. Compile the kernel module (`hide_process.ko`)
3. Compile the user-space program (`programme`)
4. Load the module into kernel
5. Create and enable systemd service
6. Display PCB (Process Control Block) information

### Interactive Testing (Option 2)

```bash
sudo ./bash.sh     # Select option 2
```

Useful for:

- Development and debugging
- Testing without system persistence
- Educational demonstration
- Kernel module testing before full installation

---

## Usage

### Method 1: Via Deployment Script

```bash
# Interactive menu
sudo ./bash.sh

# Direct commands
sudo ./bash.sh --install        # Install persistently
sudo ./bash.sh --test           # Run interactive test
sudo ./bash.sh --uninstall      # Remove all components
```

### Method 2: Direct Program Execution

```bash
# Compile manually
gcc -o programme programme.c

# Create and manage a hidden process
sudo ./programme

# Install as service
sudo ./programme --install

# Run in daemon mode (used by systemd)
sudo ./programme --daemon

# Display help
./programme --help
```

### Method 3: Systemd Service Management

After installation:

```bash
# Check service status
sudo systemctl status mon_processus_resident

# View service logs
sudo journalctl -u mon_processus_resident -f

# Stop service
sudo systemctl stop mon_processus_resident

# Restart service
sudo systemctl restart mon_processus_resident
```

---

## How It Works

### Process Hiding Mechanism

1. **Deploy Kernel Module**: Load `hide_process.ko` to install ftrace hooks
2. **Send Magic Signal**: Send signal 64 to target process to mark it for hiding
3. **Syscall Interception**: Kernel module intercepts `getdents`/`getdents64` calls
4. **Directory Filtering**: Removes target PID from directory listings
5. **Result**: Process is hidden from `ps`, `top`, `/proc` enumeration

### Signal 64 Activation

The kernel module uses signal 64 as a "magic signal" for activation:

```c
// In kernel module hook_kill():
if (sig == 64)
{
    printk(KERN_INFO "hide_process: Hiding PID %d\n", pid);
    snprintf(hide_pid, NAME_MAX, "%d", pid);
    return 0;  // Hide the signal
}
```

When a process is targeted:

```bash
kill -64 <PID>    # Mark process for hiding
```

### PCB Modification

The program modifies process scheduling properties:

```c
setpriority(PRIO_PROCESS, pid, -10);  // Increase priority (lower nice value)
```

---

## Technical Details

### Ftrace Syscall Hooking

The kernel module uses **ftrace** (Function Tracer) to intercept system calls:

- **Why ftrace?** More stable and detectable than direct syscall table manipulation
- **Target syscalls**:
  - `__x64_sys_kill` - Signal delivery interception
  - `__x64_sys_getdents64` - Modern directory enumeration (64-bit)
  - `__x64_sys_getdents` - Legacy directory enumeration

### Kernel Version Compatibility

Handles differences in kernel symbol availability:

```c
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    // Use kprobe to locate kallsyms_lookup_name
#else
    // Direct symbol export
#endif
```

### Memory Protection

Disables/enables CPU write protection during memory operations:

```c
unprotect_memory();    // Clear WP flag in CR0
// ... memory modification ...
protect_memory();      // Set WP flag in CR0
```

### Directory Entry Filtering

Filters out target PID from directory listings:

```c
// Iterate through directory entries
if (memcmp(hide_pid, current_dir->d_name, strlen(hide_pid)) != 0)
{
    // Include entry (not hidden)
    copy_to_user(...);
}
```

---

## Kernel Module Details

### Module Parameters

- **`hide_pid[NAME_MAX]`** - Stores the PID string to hide (globally accessible)

### Exported Functions

- `lookup_name()` - Safe symbol lookup using kprobes
- `fh_install_hooks()` - Install all ftrace hooks
- `fh_remove_hooks()` - Clean up hooks on module unload

### Hook Installation

```
Module Load → lookup_name() → ftrace_set_filter_ip() → register_ftrace_function()
```

---

## Monitoring and Logs

### System Logs

```bash
# View kernel module logs
sudo dmesg | grep hide_process

# Follow live kernel logs
sudo journalctl -k -f
```

### Application Logs

```bash
# View process logs
tail -f /tmp/processus_resident.log

# Clear logs
sudo rm /tmp/processus_resident.log
```

### PCB Display

The script displays Process Control Block information:

```
======================================================
    PROCESS CONTROL BLOCK (PCB) - PID: 1234
======================================================
    1. IDENTIFICATION
    -----------------
    PID       : 1234
    PPID      : 1
    User ID   : 0 (root=0)
    Group ID  : 0 (root=0)

    2. ORDONNANCEMENT (PRIORITÉ)
    --------------------------
    Real Priority : 39 (Dynamic Scheduler Priority)
    Nice Value    : -10 (Static Hint)
======================================================
```

---

## Troubleshooting

### Issue: "Headers du noyau introuvables" (Kernel headers not found)

**Solution**: Let the script auto-install or manually install:

```bash
sudo apt-get update
sudo apt-get install -y linux-headers-$(uname -r)
```

### Issue: Module fails to load (insmod error)

**Possible causes**:

- Missing kernel headers
- Kernel too old (< 5.0)
- Secure Boot enabled
- SELinux restrictions

**Debug**:

```bash
sudo dmesg | tail -20          # Check kernel messages
uname -r                        # Check kernel version
```

### Issue: Process still visible after hiding

**Verify**:

1. Module is loaded: `lsmod | grep hide_process`
2. Signal 64 was sent successfully
3. Child process is running: Check logs in `/tmp/processus_resident.log`

---

## Educational Value

This project demonstrates:

✅ **Kernel Module Development** - Compilation, loading, unloading  
✅ **Ftrace Hooking** - Modern syscall interception technique  
✅ **Process Management** - Understanding process lists and enumeration  
✅ **Privilege Escalation** - Kernel-level access and capabilities  
✅ **Memory Protection** - CPU privilege levels (ring 0/3)  
✅ **Systemd Integration** - Service persistence and management  
✅ **Linux Internals** - Process Control Blocks, scheduler, syscalls

---

## Security Implications

### Detection Methods

- **Behavioral**: Process CPU usage without visibility
- **Forensic**: Kernel module presence in memory
- **Network**: Unexpected network traffic from hidden processes
- **Advanced**: Memory dumps, kernel integrity checks (AIDE, Tripwire)

### Defense Mechanisms

- Kernel Module Signing (CONFIG_MODULE_SIG)
- Secure Boot
- SELinux / AppArmor policies
- Rootkit detection tools (rkhunter, chkrootkit)
- Kernel security modules (LSM)

---

## Limitations

1. **Not invisible to root** - Root user can find processes via multiple methods
2. **Single PID hiding** - Hides one PID at a time (can be extended)
3. **Syscall-level only** - Doesn't hide from kernel-level tools or direct memory inspection
4. **Requires module reload** - Can't change target PID without unloading
5. **Kernel-specific** - Requires kernel rebuild for different versions

---

## Files Reference

### [hide_process.c](hide_process.c)

Linux kernel module implementing ftrace-based syscall hooking.

**Key sections**:

- Lines 1-40: Kernel compatibility setup
- Lines 70-100: `hook_kill()` - Signal interception
- Lines 107-150: `hook_getdents64()` - Hide from modern directory listing
- Lines 160-210: `hook_getdents()` - Hide from legacy directory listing
- Lines 250-300: Ftrace hook installation and management

### [programme.c](programme.c)

User-space program for process creation, modification, and service management.

**Key sections**:

- Lines 1-50: Logging infrastructure
- Lines 55-120: Systemd service file creation
- Lines 125-170: Process priority modification
- Lines 175-220: Daemon process loop
- Lines 230-350: Interactive child process creation and hiding

### [bash.sh](bash.sh)

Automated deployment and management script with dependency resolution.

**Key sections**:

- Lines 1-100: Environment setup and color definitions
- Lines 150-250: Dependency checking with multi-distro support
- Lines 300-400: Kernel module compilation
- Lines 450-550: Interactive menu and service management

---

## License

This project is provided **for educational and research purposes only**.

- Inspect, study, and modify the source code
- Test in controlled environments you own
- **Do NOT** use for unauthorized system access

---

## Contributing

For educational improvements:

1. Add support for multiple target PIDs
2. Implement additional syscall hooks
3. Add eBPF-based detection evasion
4. Create detection/defense demonstrations
5. Add comprehensive documentation

---

## References

- **Kernel Module**: [Linux Kernel Module Programming Guide](https://linux.die.net/lkmpg/)
- **Ftrace**: [Ftrace Documentation](https://www.kernel.org/doc/html/latest/trace/ftrace.html)
- **System Calls**: [Linux Syscall Reference](https://man7.org/linux/man-pages/man2/syscalls.2.html)
- **Security**: [Linux Security Internals](https://www.kernel.org/doc/html/latest/security/)

---

## Author

Educational Rootkit Project - 2024-2026

---

## ⚠️ Legal Notice

**This software is provided strictly for educational purposes.**

Users are responsible for complying with all applicable laws and regulations. Unauthorized access to computer systems is illegal under the Computer Fraud and Abuse Act (CFAA) and similar laws worldwide.

Always obtain explicit written permission before testing on any system you do not own.

---

**Questions or improvements?** See the source files for detailed code comments and inline documentation.
