# Contributing to Process Hiding Rootkit

Thank you for your interest in this educational project! We welcome contributions that enhance the learning value and educational clarity of this kernel module demonstration.

## Code of Conduct

This is an **educational and research project**. All contributions must:

- ✅ Be used **only for legitimate education and authorized testing**
- ✅ Include clear documentation of how the code works
- ✅ Maintain ethical standards and legal compliance
- ✅ Include appropriate warnings and disclaimers
- ❌ **Never** be used for unauthorized system access or malicious purposes

## How to Contribute

### 1. **Reporting Issues**

Found a bug or have a question? Please open an issue with:

- **Clear title** describing the problem
- **Detailed description** of the issue
- **Kernel version** and environment (uname -r, distro)
- **Steps to reproduce** (if applicable)
- **Expected vs actual behavior**

Example:

```
Title: Module fails to compile on Kernel 5.15

Description: When compiling on Ubuntu 22.04 with Kernel 5.15, the module fails...
Environment: Ubuntu 22.04, Kernel 5.15.0-56-generic
Steps: 1. sudo ./bash.sh, 2. Select option 2, 3. Observe error...
Error message: [paste error output]
```

### 2. **Submitting Improvements**

Educational enhancements are welcome! Consider contributing:

#### Code Improvements

- Bug fixes with explanations
- Performance optimizations with comments
- Better error handling
- Code documentation and comments
- Multi-PID hiding support
- Additional syscall hooks documentation

#### Documentation

- Clearer explanations of kernel concepts
- More inline code comments
- Enhanced README sections
- Troubleshooting guides
- Architecture diagrams

#### Features (educational value)

- Support for hiding multiple processes
- Additional syscall hook examples
- PCB modification demonstrations
- Detection evasion techniques (documented)
- Logging and monitoring examples

### 3. **Pull Request Process**

1. **Fork the repository** and create your feature branch:

   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes** with clear, documented commits:

   ```bash
   git commit -m "Add: Clear description of what and why"
   # Examples:
   # git commit -m "Add: Multi-PID hiding support with documentation"
   # git commit -m "Fix: Module compilation error on Kernel 5.15"
   # git commit -m "Docs: Add detailed explanation of ftrace hooking"
   ```

3. **Test thoroughly**:
   - Test on multiple kernel versions if possible
   - Test both compilation and runtime
   - Document any compatibility issues discovered

4. **Write/update documentation**:
   - Update README.md if user-visible changes
   - Add inline code comments explaining complex logic
   - Document any new features or options
   - Include troubleshooting info for new features

5. **Push and create a Pull Request**:

   ```bash
   git push origin feature/your-feature-name
   ```

6. **Describe your changes**:
   - Clear title and description
   - Motivation: Why this improvement is valuable
   - Testing: How you tested the changes
   - Documentation: What documentation was updated
   - Educational value: What this teaches about Linux kernels

### 4. **Code Style Guidelines**

#### C Code

```c
// Comments should explain WHY, not just WHAT
// Good:
if (sig == 64)  // Magic signal for process hiding
{
    // Mark this PID for hiding in directory enumeration
    snprintf(hide_pid, NAME_MAX, "%d", pid);
    return 0;  // Hide the signal from normal processing
}

// Avoid:
if (sig == 64)
{
    snprintf(hide_pid, NAME_MAX, "%d", pid);
    return 0;
}
```

#### Bash Script

```bash
# Clear function names and descriptions
check_kernel_headers() {
    # This function verifies kernel build files are available
    # They're required for module compilation
    if [ -d "/lib/modules/$(uname -r)/build" ]; then
        return 0
    else
        return 1
    fi
}
```

#### Documentation

- Use clear, simple language
- Explain concepts before diving into code
- Include example commands and output
- Add warnings for potentially dangerous operations

## Areas for Contribution

### High Priority

- [ ] Fix any reported bugs
- [ ] Improve kernel version compatibility
- [ ] Better error messages and diagnostics
- [ ] Enhanced documentation of security concepts

### Good For Learning

- [ ] Add support for hiding multiple PIDs
- [ ] Document syscall hooking mechanics
- [ ] Create architecture diagrams
- [ ] Add more inline code comments

### Advanced Topics

- [ ] eBPF-based syscall hooking (comparison)
- [ ] Detection methodology documentation
- [ ] Defense mechanism explanations
- [ ] Performance analysis and optimization

## Development Setup

### Prerequisites

```bash
# Debian/Ubuntu
sudo apt-get install -y build-essential linux-headers-$(uname -r) git

# RHEL/CentOS
sudo dnf install -y kernel-devel-$(uname -r) gcc git

# Arch
sudo pacman -S linux-headers base-devel git
```

### Testing Your Changes

```bash
# Compile the kernel module
gcc -o programme programme.c
# (Makefile is auto-generated by bash.sh)

# Run tests
sudo ./bash.sh --test

# Check kernel logs
sudo dmesg | grep hide_process
```

## Documentation Standards

All contributions should include:

1. **Clear comments** explaining WHY code does something
2. **Function documentation** describing inputs, outputs, and side effects
3. **Error handling** with explanatory messages
4. **Update README** if user-facing changes are made

### Example Documentation

```c
/**
 * hook_kill - Intercept kill() syscalls
 * @regs: x86-64 CPU registers from syscall entry
 *
 * This hook intercepts the sys_kill syscall to detect the
 * magic "signal 64" which marks a process for hiding.
 *
 * When signal 64 is detected:
 * - The target PID is stored globally for later filtering
 * - The signal is hidden (return 0) so user code doesn't see it
 *
 * Return: 0 if signal was the magic "hide" signal, otherwise
 *         forwards to original sys_kill implementation
 */
static asmlinkage int hook_kill(const struct pt_regs *regs)
```

## Questions or Need Help?

- **Documentation questions**: Check README.md and inline comments
- **Kernel concepts**: Refer to Linux kernel documentation
- **Technical issues**: Open an issue with detailed information
- **General questions**: Open a discussion

## Legal Considerations

By contributing to this project, you agree that:

1. Your contributions are for **educational purposes only**
2. You understand this code demonstrates security **vulnerabilities and evasion techniques**
3. You will **not use or distribute this code** for unauthorized system access
4. Contributors are responsible for ensuring their work complies with applicable laws

## Recognition

Contributors will be recognized in:

- The CONTRIBUTORS.md file
- Commit history
- Release notes (if applicable)

Thank you for contributing to Linux kernel education!
