# Quick Start Guide

## 60-Second Setup

### Prerequisites Check

```bash
# Check you're on Linux
uname -s  # Should output: Linux

# Check sudo access
sudo -v   # Verify you can run sudo commands

# Check kernel version (5.0+)
uname -r
```

### Installation (3 Steps)

```bash
# 1. Clone/download the project
cd /path/to/rootkit

# 2. Make the script executable
chmod +x bash.sh

# 3. Run with interactive menu
sudo ./bash.sh
```

## Menu-Based Quick Start

After running `sudo ./bash.sh`, you'll see:

```
╔════════════════════════════════════════════════╗
║    MANAGER PROCESSUS (deploy_manager.sh)       ║
╚════════════════════════════════════════════════╝
1) Installation Complète (Full Install)
2) Test Interactif (Interactive Test)
3) Vérifier statut (Check Status)
4) Logs
5) Désinstaller tout (Uninstall All)
6) Quitter (Exit)
```

### Option 1: Full Installation (Permanent)

```bash
# Select: 1
# This will:
# ✓ Check dependencies
# ✓ Compile kernel module
# ✓ Compile user program
# ✓ Load module
# ✓ Create service
# ✓ Display PCB info
```

**After installation**, the service runs at boot:

```bash
# Check service status
sudo systemctl status mon_processus_resident

# View logs
tail -f /tmp/processus_resident.log

# Stop service
sudo systemctl stop mon_processus_resident
```

### Option 2: Interactive Test (Temporary)

```bash
# Select: 2
# This will:
# ✓ Compile both components
# ✓ Load module temporarily
# ✓ Create and hide a test process
# ✓ Show process hiding working
# ✓ Clean up automatically
```

**Best for learning** - No persistence, safe to experiment.

## Command-Line Quick Start

### One-Liner Installation

```bash
cd rootkit && sudo ./bash.sh --install
```

### One-Liner Test

```bash
cd rootkit && sudo ./bash.sh --test
```

### Uninstall

```bash
cd rootkit && sudo ./bash.sh --uninstall
```

## Immediate Test (≈2 minutes)

### Step 1: Check Prerequisites

```bash
# Verify kernel version
uname -r
# Output should be 5.0 or higher

# Check for build tools
gcc --version
# Output should show gcc version

# Check for kernel headers
ls /lib/modules/$(uname -r)/build/
# Output should show kernel build directory
```

### Step 2: Run Interactive Test

```bash
cd /path/to/rootkit
sudo ./bash.sh --test
```

### Step 3: Observe the Demo

```
=== Création d'un Processus avec Modification du PCB ===

Processus parent PID: 3421
→ Processus enfant créé! PID: 3422

==================================================
   PID DU PROCESSUS FILS (ENFANT) : 3422
==================================================

→ Parent: Envoi du signal 64 pour cacher le PID 3422
✓ Signal 64 envoyé. Le processus 3422 devrait être caché.

======================================================
    PROCESS CONTROL BLOCK (PCB) - PID: 3422
======================================================
    1. IDENTIFICATION
    PID       : 3422
    PPID      : 3421
    ...
```

The process `3422` is now hidden from `ps` and `top`!

## Troubleshooting Quick Fixes

### Problem: "Erreur: Ce script doit être exécuté avec sudo"

**Solution**: Use sudo!

```bash
sudo ./bash.sh          # Correct
./bash.sh               # Wrong
```

### Problem: "Headers introuvables" (No kernel headers)

**Solution**: Let script install for you (option 1), or manually:

```bash
# Ubuntu/Debian
sudo apt-get install -y linux-headers-$(uname -r)

# Fedora/RHEL
sudo dnf install -y kernel-devel-$(uname -r)

# Arch
sudo pacman -S linux-headers
```

### Problem: Module won't load

**Check kernel logs**:

```bash
sudo dmesg | tail -20
```

**Possible causes**:

- Kernel too old (< 5.0)
- Secure Boot enabled
- SELinux blocking

### Problem: Process still visible after hiding

**Verify module is loaded**:

```bash
lsmod | grep hide_process
```

Should output:

```
hide_process           XXXX  0
```

**Check logs**:

```bash
sudo dmesg | grep hide_process
```

Should see:

```
hide_process: Module loaded
hide_process: Hooks installed successfully
hide_process: Hiding PID 3422
```

## System Information Display

The script automatically displays **Process Control Block (PCB)** information:

```
======================================================
    PROCESS CONTROL BLOCK (PCB) - PID: 3422
======================================================
    1. IDENTIFICATION
    -----------------
    PID       : 3422         (Process ID)
    PPID      : 3421         (Parent Process ID)
    User ID   : 0            (root user)
    Group ID  : 0            (root group)

    2. ORDONNANCEMENT (PRIORITÉ)
    --------------------------
    Real Priority : 39       (Dynamic priority, lower=more CPU)
    Nice Value    : -10      (Request hint, -20 to +19)
======================================================
```

## What Just Happened?

1. **Kernel module loaded** - `hide_process.ko` installed
2. **Child process created** - PID 3422 started in infinite loop
3. **Signal 64 sent** - Marked process 3422 as "hidden"
4. **Syscall hooked** - `getdents64()` now filters out PID 3422
5. **Process hidden** - No longer visible in `ps`, `top`, `/proc/[PID]`

**Try this** while the process runs:

```bash
# In another terminal:
ps aux | grep 3422          # Won't show!
ls -la /proc | grep 3422    # Won't show!
top                         # Won't show process!
```

## Next Steps

### To Learn More

- 📖 Read [README.md](README.md) for detailed documentation
- 💻 Check [hide_process.c](hide_process.c) for kernel module code
- 📝 See [programme.c](programme.c) for user-space code
- 🔧 Review [bash.sh](bash.sh) for deployment script

### To Experiment

- Modify signal number in the code
- Add support for multiple hidden PIDs
- Create detection tools
- Study kernel logging output

### To Clean Up

**After testing**, completely remove everything:

```bash
sudo ./bash.sh          # Select option 5 (Uninstall)
# or
sudo ./bash.sh --uninstall
```

This removes:

- Kernel module
- Systemd service
- Compiled binaries
- Build files

## Common Commands Reference

```bash
# Start the interactive menu
sudo ./bash.sh

# Run full installation
sudo ./bash.sh --install

# Run interactive test (recommended first time)
sudo ./bash.sh --test

# Uninstall everything
sudo ./bash.sh --uninstall

# Check service status
sudo systemctl status mon_processus_resident

# View application logs
tail -f /tmp/processus_resident.log

# View kernel module logs
sudo dmesg | grep hide_process

# Stop the service
sudo systemctl stop mon_processus_resident

# View all hidden process attempts
sudo dmesg | grep "Hiding PID"

# Check if module is loaded
lsmod | grep hide_process

# Unload module (if stopped)
sudo rmmod hide_process
```

## Recommended Learning Path

1. ✅ Run `sudo ./bash.sh --test` first
2. ✅ Observe output and logs
3. ✅ Read inline code comments
4. ✅ Check kernel logs with `dmesg`
5. ✅ Examine source code
6. ✅ Modify and recompile
7. ✅ Create variations and improvements

## Support

- **Issues**: Check the [full README](README.md) troubleshooting section
- **How it works**: Read [README.md](README.md) "How It Works" section
- **Advanced topics**: See [README.md](README.md) "Technical Details"

---

**Ready? Run this:**

```bash
sudo ./bash.sh --test
```

Then explore the code and learn how Linux kernel security works!
