<div align="center">

**Ministerul Educației și Cercetării al Republicii Moldova**
**Universitatea Tehnică a Moldovei**
Facultatea Calculatoare, Informatică și Microelectronică
Program: Software Engineering

# Operating Systems
### Laboratory Work No. 2
**Meet the OS You Will Build**
*Track B: Building and Exploring xv6*

**Elaborated by:** st. gr. FAF-242, Biriucov Alexandr
**Verified by:** Reițman Patricia

Chișinău – 2026

</div>

---

## 1. Introduction

The purpose of this laboratory work is to become familiar with **xv6**, a small Unix-like teaching operating system.

During the work, xv6 was built and launched using QEMU, basic Unix commands were executed inside the xv6 shell, and the source code was inspected to understand the difference between **user programs** and **kernel code**. The work also demonstrates how user programs interact with the kernel through **system calls**.

---

## 2. Building and Running xv6

The xv6 source code is located in `~/xv6-riscv`. Tool versions used:

| Tool | Version |
|------|---------|
| RISC-V compiler | `riscv64-linux-gnu-gcc 13.3.0` |
| Emulator | `QEMU 8.2.2` |

The OS was built and started with:

```bash
make qemu
```

After a successful compilation, xv6 booted and displayed:

```console
xv6 kernel is booting

hart 2 starting
hart 1 starting
init: starting sh
$
```

>  **Comment:** A *hart* is a RISC-V hardware thread (core). Each "hart N starting" line is one CPU coming online. `init: starting sh` means the first user process (`init`) launched the shell, and `$` is the xv6 prompt — the kernel booted correctly. To leave QEMU press **Ctrl-A, then X**.

---

## 3. Using xv6 as the Unix It Is

Commands executed inside the xv6 shell:

```console
$ ls
$ cat README
$ echo hello xv6
$ ls | grep c
$ wc README
```

### 3.1 Observations

1. **Three programs included with xv6:** `ls`, `cat`, `echo`.
2. **What a pipe such as `ls | grep c` requires:** the OS must support **processes** and **inter-process communication**. The shell runs both commands as separate processes and connects the standard output of the first to the standard input of the second through a pipe.
3. **Compared to Bash:** the xv6 shell is much simpler, but basic Unix concepts — `ls`, `cat`, `echo`, pipes, standard input/output — work the same way.

### 3.2 Example command output

`ls` lists the programs available inside xv6:

```console
$ ls
cat        echo       forktest   grep       init       kill
ln         ls         mkdir      rm         sh         stressfs
usertests  grind      wc         zombie     logstress  forphan
dorphan    sync
```

```console
$ echo hello xv6
hello xv6
```

>  **Comment:** Every command here is a separate *program file* in the xv6 filesystem — there are no built-in `ls` or `cat` inside the kernel. The shell simply `fork()`s and `exec()`s them. Programs like `forktest`, `usertests`, `stressfs` and `zombie` are test programs used to stress the kernel (process creation, filesystem, zombies and orphans).

---

## 4. Reading the Source

The user programs and kernel system-call handlers were inspected with:

```bash
sed -n '1,40p' user/ls.c
sed -n '1,30p' user/cat.c
cat user/user.h
grep -n "sys_read\|sys_write" kernel/sysfile.c
```

### 4.1 System calls used by `cat`

`user/cat.c` uses these system calls:

| System call | What it asks the kernel to do |
|-------------|-------------------------------|
| `read()` | read data from a file descriptor into a buffer |
| `write()` | write data from a buffer to a file descriptor |
| `open()` | open a file |
| `close()` | close an open file descriptor |
| `exit()` | terminate the current process |

The main loop of `cat`:

```c
while ((n = read(fd, buf, sizeof(buf))) > 0) {
  if (write(1, buf, n) != n) {
    fprintf(2, "cat: write error\n");
    exit(1);
  }
}
```

The program repeatedly **reads** data from a file descriptor and **writes** it to standard output (descriptor `1`).

>  **Comment:** File descriptors `0`, `1`, `2` are standard input, standard output and standard error. That is why `cat` needs no special code for pipes or redirection — it just reads from `fd` and writes to `1`; the shell rewires what those numbers point to.

### 4.2 Kernel implementation

The implementations were found in `kernel/sysfile.c`:

| Function | Starts at line |
|----------|----------------|
| `sys_read(void)` | 69 |
| `sys_write(void)` | 83 |

This shows that a user-level call such as `read()` ultimately makes the kernel execute the matching system-call implementation.

### 4.3 Kernel space vs. user space

| Directory | Contents |
|-----------|----------|
| `kernel/` | privileged OS code that manages resources and implements system calls |
| `user/` | ordinary programs that run outside the kernel and request services through system calls |

This separation **protects the kernel and system resources** from direct access by user programs.

---

## 5. Creating a User Program

A new program, `sleep`, was added to xv6 in `user/sleep.c`:

```c
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc != 2){
    fprintf(2, "usage: sleep <ticks>\n");
    exit(1);
  }
  pause(atoi(argv[1]));
  exit(0);
}
```

In this xv6 version, the system call that suspends a process for a number of clock ticks is named **`pause()`**.

It was registered in the `UPROGS` section of the Makefile:

```make
$U/_sleep\
```

After rebuilding:

```bash
make clean
make qemu
```

the new program became available in the xv6 shell.

### 5.1 Testing

```console
$ sleep 10
$                       <- returns after a short pause
$ sleep
usage: sleep <ticks>
```

- `sleep 10` paused briefly, then returned to the prompt.
- Calling it **without an argument** produced the expected error message.

>  **Comment:** One tick is a timer interrupt (about 1/10 of a second in xv6), so `sleep 10` is ~1 second. `exit(1)` signals failure to the parent shell, while `exit(0)` signals success; `fprintf(2, ...)` writes the usage message to **stderr** (fd 2).

This confirmed that the program compiled, was added to the xv6 filesystem, and ran successfully.

---

## 6. Conclusion

In this laboratory work, xv6 was successfully built and executed on the QEMU RISC-V emulator. Basic Unix commands, including pipes and standard input/output, were used to explore the xv6 user environment. The source code was then examined to understand the split between user programs and the kernel and to see how programs reach OS functionality through system calls (`read`, `write`, `open`, `close`, `exit`). Finally, a new user-level program, `sleep`, was written, added to the build system, compiled and run successfully.

The work gave a practical introduction to the internal structure of a Unix-like operating system and to the relationship between **user programs, system calls, and kernel implementations**.