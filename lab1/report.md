<div align="center">

**MINISTRY OF EDUCATION, CULTURE AND RESEARCH OF THE REPUBLIC OF MOLDOVA**
**Technical University of Moldova**
Faculty of Computers, Informatics and Microelectronics
Department of Software Engineering and Automation

# REPORT
### Laboratory Work No. 1
**Operating Systems**
*Topic: The OS as a Resource Manager*

**Elaborated by:** st. gr. FAF-242, Biriucov Alexandr
**Checked by:** Reițman Patricia

Chișinău – 2026

</div>

---

## Purpose of the work

To explore how an operating system acts as a **resource manager** by inspecting, from the Linux command line, its four main areas of responsibility: **files, processes, memory, and devices/storage**.

| Part | Resource | Key commands |
|------|----------|--------------|
| 0 | System identity | `whoami`, `uname -a`, `uptime` |
| 1 | Files & directories | `ls -l`, `chmod`, `rm` |
| 2 | Processes | `ps`, `top`, `sleep`, `jobs`, `kill`, `/proc` |
| 3 | Memory | `free`, `/proc/meminfo`, `VmRSS` |
| 4 | Devices & storage | `df`, `lsblk`, `du`, `/dev`, `mount` |

---

## Part 0. Who and where am I?

```console
alex@ubuntu:~/os-lab1$ whoami
alex
```
`whoami` prints the user currently logged in — here, `alex`.

```console
alex@ubuntu:~/os-lab1$ uname -a
Linux ubuntu 7.0.0-38-generic #38-Ubuntu SMP PREEMPT_DYNAMIC Fri Sep 4 09:10:14 UTC 2026 x86_64 GNU/Linux
```
`uname -a` shows kernel and OS information: Ubuntu, Linux kernel 7.0.0-38, 64-bit (`x86_64`).

```console
alex@ubuntu:~/os-lab1$ uptime
16:02:55 up 8 min, 1 user, load average: 0.22, 0.15, 0.08
```
`uptime` shows how long the system has been running, the number of logged-in users, and the load average.

> **Comment:** The three load-average numbers are the average number of runnable processes over the last 1, 5 and 15 minutes. Values far below 1.0 on a machine mean the CPU is almost idle.

---

## Part 1. Files and directories

The user (me) owns the files I create, and the owning group has the same name as the user.

```console
alex@ubuntu:~/os-lab1/demo$ ls -l
total 8
-rw-rw-r-- 1 alex alex 24 Oct 4 16:16 note.txt
-rw-rw-r-- 1 alex alex 24 Oct 4 16:16 renamed.txt
alex@ubuntu:~/os-lab1/demo$ rm renamed.txt
```

> **Comment:** `renamed.txt` has the same size and timestamp as `note.txt` — it is a copy/renamed version used to practice `cp`/`mv`, and then removed with `rm`. The default permissions `rw-rw-r--` come from the `umask 0002` (visible later in `/proc/<PID>/status`).

### Reading the permission string

After `chmod`, the 10 characters of `ls -l` mean:

| Position | Meaning | Example (`-rw-r--r--`) |
|----------|---------|------------------------|
| 1 | File type (`-` file, `d` directory, `l` link, `c` char device, `b` block device) | `-` |
| 2–4 | Permissions for the **user** (owner) | `rw-` |
| 5–7 | Permissions for the **group** | `r--` |
| 8–10 | Permissions for **everyone else** | `r--` |

```console
alex@ubuntu:~/os-lab1/demo$ ls -l note.txt
-rw-r--r-- 1 alex alex 24 Oct 4 16:16 note.txt
```

> **Comment:** The mode shown, `rw-r--r--`, is `644` (owner read/write, others read-only). `chmod 600` would give `-rw-------`, so this listing reflects a `chmod 644`. Octal digits: r = 4, w = 2, x = 1.

**Directory layout:** `~/os-lab1/` holds the `demo/` directory, which contains `note.txt`.

---

## Part 2. Process management

### 2.1 Listing processes

```console
alex@ubuntu:~$ ps aux | head
USER   PID %CPU %MEM   VSZ   RSS TTY STAT START TIME COMMAND
root     1  0.0  0.4 25268 16624 ?   Ss   15:54 0:01 /usr/lib/systemd/systemd --switched-root --system --deserialize=51
root     2  0.0  0.0     0     0 ?   S    15:54 0:00 [kthreadd]
root     3  0.0  0.0     0     0 ?   S    15:54 0:00 [pool_workqueue_release]
root     4  0.0  0.0     0     0 ?   I<   15:54 0:00 [kworker/R-rcu_gp]
root     5  0.0  0.0     0     0 ?   I<   15:54 0:00 [kworker/R-sync_wq]
...
```

> **Comment:** PID 1 is `systemd`, the first user-space process and ancestor of everything else. PID 2 `kthreadd` spawns kernel threads — names in `[brackets]` are kernel threads with **0 VSZ/RSS** because they own no user memory.

```console
alex@ubuntu:~$ ps aux | wc -l
214
```
There are 214 lines (213 processes + 1 header line).

### 2.2 `top`

```console
top - 16:43:06 up 48 min, 1 user, load average: 0.16, 0.31, 0.23
Tasks: 212 total, 1 running, 211 sleeping, 0 stopped, 0 zombie
%Cpu(s): 0.8 us, 0.8 sy, 0.0 ni, 98.4 id, 0.0 wa, 0.0 hi, 0.0 si
MiB Mem : 3302.8 total, 402.5 free, 1538.6 used, 1624.5 buff/cache
MiB Swap:   0.0 total,   0.0 free,    0.0 used. 1764.2 avail Mem
```

> **Comment:** 211 of 212 tasks are *sleeping* and the CPU is 98.4 % idle — a typical idle desktop VM. The one *running* task is `top` itself. `PR`/`NI` columns show scheduling priority (`NI = -20` is the highest priority, `rt` is real-time).

### 2.3 Background jobs, `kill`, and the `/proc` filesystem

```console
alex@ubuntu:~$ sleep 300 &
[1] 5790
alex@ubuntu:~$ jobs
[1]+  Running                 sleep 300 &
alex@ubuntu:~$ ps -ef | grep sleep
alex  5790  4745  0 16:43 pts/0  00:00:00 sleep 300
alex  5794  4745  0 16:44 pts/0  00:00:00 grep --color=auto sleep
```

The first process is `sleep 300` with **PID 5790**; its parent (PPID) is **4745**, the bash shell.

**A mistake worth keeping in the report:**

```console
alex@ubuntu:~$ kill <PID>
bash: syntax error near unexpected token `newline'
alex@ubuntu:~$ kill <5790>
bash: syntax error near unexpected token `5790'
alex@ubuntu:~$ kill 5790
[1]+  Terminated              sleep 300
```

> **Comment:** `<PID>` in documentation is a *placeholder*. In bash, `<` is the **input-redirection** operator, so the shell tried to read from a file named `PID` and failed on syntax. Only the plain number works: `kill 5790`.

Using `$!` (PID of the last background job) and inspecting `/proc`:

```console
alex@ubuntu:~$ sleep 300 &
[1] 5810
alex@ubuntu:~$ PID=$!
alex@ubuntu:~$ ls /proc/$PID/
arch_status  fdinfo   net        setgroups   attr   gid_map   ns   smaps
cmdline      cwd      environ    exe         fd     limits    maps status ...
alex@ubuntu:~$ cat /proc/$PID/status | head -20
Name:   sleep
Umask:  0002
State:  S (sleeping)
Tgid:   5810
Pid:    5810
PPid:   4745
Uid:    1000 1000 1000 1000
Gid:    1000 1000 1000 1000
Groups: 4 24 27 30 46 100 111 114 1000
VmPeak: 16112 kB
VmSize: 16112 kB
alex@ubuntu:~$ kill $PID
[1]+  Terminated              sleep 300
```

> **Comment:** `/proc` is a *virtual* filesystem — nothing is stored on disk; the kernel generates these files on demand to expose each process's state. `cmdline`, `cwd`, `exe` and `fd` tell what the process runs, where it is, which binary it is, and which files it has open.

### Observe

- The first process is `sleep 300`, **PID 5790**.
- The `ps aux | wc -l` count was **213** on the second run (214 on the first) — the number fluctuates as short-lived processes start and end.
- `State: S (sleeping)` means the process is **alive but waiting** (here, for its timer) and uses no CPU.

---

## Part 3. Memory

```console
alex@ubuntu:~$ free -h
               total   used   free  shared  buff/cache  available
Mem:           3.2Gi  1.5Gi  378Mi    36Mi       1.6Gi      1.7Gi
Swap:             0B     0B     0B

alex@ubuntu:~$ cat /proc/meminfo | head -6
MemTotal:        3382032 kB
MemFree:          312768 kB
MemAvailable:    1734320 kB
Buffers:           54100 kB
Cached:          1575860 kB
SwapCached:            0 kB

alex@ubuntu:~$ sleep 300 & PID=$!
[1] 6087
alex@ubuntu:~$ grep VmRSS /proc/$PID/status
VmRSS:      7508 kB
alex@ubuntu:~$ kill $PID
[1]+  Terminated              sleep 300
```

### Observe

- The VM has **3.2 GiB** of RAM, of which **378 MiB** is *free*.
- **Swap** is disk space the OS uses as overflow when RAM fills up; inactive memory pages are moved there. It is much slower than RAM. It shows `0B` because this VM has no swap configured.
- The idle `sleep` process uses **7508 kB** of resident RAM, which was surprising since the process does nothing but wait.

> **Comment:** Two things explain that number. (1) `VmRSS` counts every page currently resident, including the **shared C library** and the program image mapped into the process — these are shared with other processes, not private to `sleep`. (2) Even a "do-nothing" process needs a stack, heap, and dynamic-loader data. Also note that "free" is only 378 Mi but "available" is 1.7 Gi: the kernel uses spare RAM for **cache** (1.6 Gi) and releases it whenever a program needs memory. *Free* is therefore a misleading number; **available** is the one that matters.

---

## Part 4. Devices and storage

```console
alex@ubuntu:~$ df -h
Filesystem   Size  Used Avail Use% Mounted on
tmpfs        661M  2.1M  659M   1% /run
/dev/sda2    9.8G  6.8G  2.6G  74% /
tmpfs        1.7G     0  1.7G   0% /dev/shm
tmpfs        1.7G  8.0K  1.7G   1% /tmp
tmpfs        331M   80K  331M   1% /run/user/1000
/dev/sr0      51M   51M     0 100% /run/media/alex/VBox_GAs_7.2.20
```

```console
alex@ubuntu:~$ lsblk
NAME   MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
loop0    7:0    0     4K  1 loop /snap/bare/5
loop4    7:4    0 260.3M  1 loop /snap/firefox/8763
...
sda      8:0    0  10.1G  0 disk
├─sda1   8:1    0     1M  0 part
└─sda2   8:2    0    10G  0 part /
sr0     11:0    1  50.7M  1 rom  /run/media/alex/VBox_GAs_7.2.20
```

```console
alex@ubuntu:~$ du -sh ~/os-lab1
12K     /home/alex/os-lab1

alex@ubuntu:~$ ls -l /dev | head
crw-r--r--  1 root root 10, 235 Oct 4 15:54 autofs
drwxr-xr-x  2 root root      460 Oct 4 16:48 block
crw-------  1 root root 10, 234 Oct 4 15:54 btrfs-control
lrwxrwxrwx+ 1 root root        3 Oct 4 15:54 cdrom -> sr0
crw-------  1 root tty    5,   1 Oct 4 15:54 console
lrwxrwxrwx  1 root root       11 Oct 4 15:54 core -> /proc/kcore

alex@ubuntu:~$ mount | head
/dev/sda2 on / type ext4 (rw,relatime)
tmpfs on /run type tmpfs (rw,nosuid,nodev,size=676408k,...)
devtmpfs on /dev type devtmpfs (rw,nosuid,size=731316k,...)
sysfs on /sys type sysfs (rw,nosuid,nodev,noexec,relatime)
cgroup2 on /sys/fs/cgroup type cgroup2 (...)
```

### Observe

- My **root filesystem is mounted from `/dev/sda2`** (type `ext4`).
- Examples of `/dev` entries:

| Entry | Meaning |
|-------|---------|
| `/dev/sda` | the first physical (or virtual) hard disk |
| `/dev/null` | discards anything written to it |
| `/dev/tty` | the current terminal |

- **"Everything is a file":** disks, terminals and even process information (`/proc`) appear as entries in the filesystem, so the same operations — `open`, `read`, `write` — work on all of them, instead of needing a different interface for each kind of resource.



---

## Conclusion

In this laboratory work I examined the operating system as a **resource manager** from the Linux command line. I identified the system and kernel (`whoami`, `uname`, `uptime`), inspected file ownership and permissions, and observed how the OS tracks **processes** through `ps`, `top` and the `/proc` filesystem, including how a background process is created and terminated. I looked at **memory** usage with `free` and `/proc/meminfo` and learned the role of cache and swap, and finally explored **devices and storage** with `df`, `lsblk`, `mount` and `/dev`. The main takeaway is the Unix principle that "everything is a file": files, processes, memory information and hardware are all exposed through one uniform filesystem interface.