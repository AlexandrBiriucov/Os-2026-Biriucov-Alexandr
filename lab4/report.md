<div align="center">

**Technical University of Moldova**
Faculty of Computers, Informatics and Microelectronics
Department of Software Engineering and Automation

# REPORT
### on Laboratory Work No. 4
**Course:** Operating Systems
**Topic:** The xv6 Shell from the Outside

**Done by:** st. gr. FAF-242, Biriucov Alexandr
**Checked by:** Patricia Reițman

Chișinău, 2026

</div>

---

## Purpose of the work

To study the shell by comparing the very small **xv6 shell** (`user/sh.c`, about 500 lines) with **bash**:

- discover which features a shell *must* provide (running programs, redirection, pipes, sequences, background execution);
- identify which features are *extras* added by bash (globbing, variables, quoting, PATH search, history and completion);
- adapt the `sleep` program to the renamed system call `pause()`;
- write a small program that prints its arguments, which shows exactly **who expands `*` and who removes quotes: the shell, not the program or the kernel.**

---

## Part 1. Fix `sleep` for the current xv6

In August 2025 the xv6 authors renamed the system call `sleep()` to `pause()` (to avoid confusion with the kernel's internal `sleep(chan, lock)`). A `sleep` program from Lab 2 that calls `sleep()` therefore no longer builds on the current tree.

### 1.1 Which name does the tree use?

```console
vboxuser@ubuntu:~$ cd ~/xv6-riscv
vboxuser@ubuntu:~/xv6-riscv$ grep -n "pause\|sleep" user/user.h
25:int pause(int);
```

*Figure 1.1 – The grep result in `user.h`.*

>  **Comment:** Only `pause` is declared (line 25), and no user-level `sleep` prototype exists any more — so the old code fails to compile with an "implicit declaration" error.

### 1.2 The updated `user/sleep.c`

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
  pause(atoi(argv[1]));   // was sleep() before Aug 2025
  exit(0);
}
```

```console
vboxuser@ubuntu:~/xv6-riscv$ grep -n "_sleep" Makefile
133:        $U/_sleep\
```

*Figure 1.2 – The updated `sleep.c` and its `UPROGS` line.*

### 1.3 Build, boot and test

```console
$ make qemu
...
$ sleep 20
$ sleep
usage: sleep <ticks>
```
(exit QEMU with **Ctrl-A, then X**)

`sleep 20` waits 20 ticks and returns to the prompt; `sleep` without an argument prints the usage message.

*Figure 1.3 – `sleep` running in xv6, with and without an argument.*

---

## Part 2. bash versus the xv6 shell

### 2.1 Setup

```bash
mkdir ~/shtest && cd ~/shtest && touch README
```

Every command in the comparison table was run twice: once in **bash** in this folder, and once at the **xv6** `$` prompt.

#### bash output

```console
vboxuser@ubuntu:~/shtest$ echo hello > f ; cat < f
hello
vboxuser@ubuntu:~/shtest$ ls | grep READ
README
vboxuser@ubuntu:~/shtest$ (echo a; echo b) | wc
      2       2       4
vboxuser@ubuntu:~/shtest$ echo one ; echo two
one
two
vboxuser@ubuntu:~/shtest$ echo bg &
[1] 5448
bg
[1]+  Done                    echo bg
vboxuser@ubuntu:~/shtest$ ls *
README  f
vboxuser@ubuntu:~/shtest$ echo $HOME
/home/vboxuser
vboxuser@ubuntu:~/shtest$ echo "a b"
a b
vboxuser@ubuntu:~/shtest$ mkdir d ; cd d ; ls ..
README  d  f
vboxuser@ubuntu:~/shtest/d$
```
*Figure 2.1 – The commands run in bash.*

#### xv6 output

```console
$ echo hello > f ; cat < f
hello
$ ls | grep READ
README          2 2 2441
$ (echo a; echo b) | wc
2 2 4
$ echo one ; echo two
one
two
$ echo bg &
bg
$ ls *
ls: cannot open *
$ echo $HOME
$HOME
$ echo "a b"
"a b"
$ mkdir d ; cd d ; ls ..
exec cd failed
.              1 1 1024
..             1 1 1024
README         2 2 2441
sleep          2 3 35280
cat            2 4 36728
...
console        3 24 0
f              2 25 6
d              1 26 32
$
```
*Figure 2.2 – The same commands run in xv6.*

>  **Comment:** xv6 `ls` prints three numbers per file: **type** (1 = directory, 2 = file, 3 = device), **inode number**, and **size in bytes**. The garbled output (e.g. `README222441`) in the raw console capture is because the xv6 console doesn't handle spacing/echo the way a modern terminal does.

#### Things noticed

- `ls *` prints an error, and `echo $HOME` prints `$HOME` itself.
- `echo "a b"` **keeps the quote characters**.
- After `echo bg &` the prompt may look odd — just press Enter.
- On the last line, `cd d` fails with `exec cd failed`. That is expected.

### 2.2 Comparison table

| Command | Tests | bash | xv6 | Why |
|---------|-------|------|-----|-----|
| `echo hello > f ; cat < f` | redirection | `hello` | `hello` | Both implement `>` and `<` (xv6: `REDIR`) and `;` (`LIST`). |
| `ls \| grep READ` | pipe | `README` | `README 2 <inum> <size>` | Both have pipes. xv6 `ls` prints more per file: name, type, inode number, size. |
| `(echo a; echo b) \| wc` | grouping, list | `2 2 4` | `2 2 4` | Both understand `( )` grouping; only the output format of `wc` differs. |
| `echo one ; echo two` | sequence | `one` / `two` | `one` / `two` | The `;` operator (`LIST`) runs commands one after another. |
| `echo bg &` | background | `[1] <PID>`, `bg`, `[1]+ Done ...` | `bg` | Both run in the background (xv6: `BACK`). Only bash does job control and prints `[1]` and `Done`. |
| `ls *` | globbing | `README f` | `ls: cannot open *` | bash expands `*` into file names. xv6 passes a literal `*` to `ls`, which tries to open a file named `*`. |
| `echo $HOME` | variables | `/home/<user>` | `$HOME` | xv6 `sh` has no variables; `echo` just prints the text it received. |
| `echo "a b"` | quotes | `a b` | `"a b"` | bash removes the quotes and keeps `a b` as one word. xv6 splits at the space and keeps the quote characters. |
| `mkdir d ; cd d ; ls ..` | PATH | `README d f` | `mkdir` ok; `exec cd failed`; `ls ..` lists root | In xv6 `cd` works only when the line *starts* with `cd` (see Part 4), and `ls` is found only in the current directory (no PATH). |
| arrow up, Tab, Ctrl+R | history, completion | previous command, completion, history search | raw characters (`^[[A`), a tab character, nothing | The xv6 console only has Backspace, Ctrl-U (kill line), Ctrl-D (end of input) and Ctrl-P (process list). |

Because `cd d` must be tested on its own in xv6, the three lines were typed separately:

```console
$ mkdir d
$ cd d
$ ls
exec ls failed
$ /ls
.              1 26 32
..             1 1 1024
$ /ls ..
.              1 1 1024
..             1 1 1024
README         2 2 2441
...
```
*Figure 2.3 – `ls` failing after `cd d`, and the working alternative `/ls`.*

>  **Comment:** After `cd d` the shell is *inside* `d`, and `exec("ls")` looks for `d/ls`, which doesn't exist. Typing `/ls` gives an absolute path from the root directory where all xv6 programs live. The garbled lines in the raw capture (`exec ls fa\iled`) come from console echo glitches, not from different errors.

### Answers to the Observe questions

1. **Features of both shells and features only bash has.**
   - **Both:** run programs (`fork` + `exec`), input/output redirection (`<`, `>`), pipes (`|`), sequences (`;`), background execution (`&`), grouping with parentheses.
   - **Only bash:** globbing (`*`, `?`, `[ ]`), variables (`$HOME`) and command substitution, quoting (`" "`, `' '`), PATH search, history, arrow keys, Tab completion, `Ctrl+R`, job control (`jobs`, `fg`), `&&` and `||`, stderr redirection (`2>`), and many built-ins.
2. **Why does `ls` stop working after `cd d` in xv6, and how to run it anyway?** xv6 has **no PATH**. When you type `ls`, the shell calls `exec("ls")` and the kernel looks for a file named `ls` relative to the current directory. All xv6 programs live in the root `/`, so after `cd d` the file `d/ls` does not exist: `exec ls failed`. The fix is a **full path**: `/ls` (or `/ls ..`). `cd ..` also works.
3. **Which missing feature would you miss most, and which is easiest to add?** I would miss **history/Tab completion** and **globbing** most. The easiest to add is a simple **PATH search**: if `exec(name)` fails, try `exec("/" + name)` in `runcmd` (a few lines). Variables or globbing would need a new expansion step in the parser.

---

## Part 3. Who expands `*`? A program that prints its `argv`

### 3.1 The xv6 program `args.c`

```c
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("argc = %d\n", argc);
  for(int i = 0; i < argc; i++)
    printf("argv[%d] = [%s]\n", i, argv[i]);
  exit(0);
}
```

Saved as `user/args.c` and added to `UPROGS` (`$U/_args\`). It prints how many arguments it received and each one between square brackets.

```console
vboxuser@ubuntu:~/xv6-riscv$ grep -n "_args" Makefile
134:        $U/_args\
```
*Figure 3.1 – `args.c` and the `UPROGS` line.*

### 3.2 The Linux program `args_linux.c`

Same program, with `#include <stdio.h>` instead of the two xv6 headers and `return 0` instead of `exit(0)`:

```c
#include <stdio.h>

int
main(int argc, char *argv[])
{
  printf("argc = %d\n", argc);
  for(int i = 0; i < argc; i++)
    printf("argv[%d] = [%s]\n", i, argv[i]);
  return 0;
}
```

```bash
mkdir -p ~/os-lab4 && cd ~/os-lab4
gcc -o args args_linux.c
touch a.c b.c        # some .c files for the globbing test
```

### 3.3 Running the same lines in both shells

Test lines: `args one two three` (with several spaces), `args *.c "x y"`, `args $HOME`, `args 'single quoted'`.

#### xv6

```console
$ args one two three
argc = 4
argv[0] = [args]
argv[1] = [one]
argv[2] = [two]
argv[3] = [three]
$ args *.c "x y"
argc = 4
argv[0] = [args]
argv[1] = [*.c]
argv[2] = ["x]
argv[3] = [y"]
$ args $HOME
argc = 2
argv[0] = [args]
argv[1] = [$HOME]
$ args 'single quoted'
argc = 3
argv[0] = [args]
argv[1] = ['single]
argv[2] = [quoted']
```
*Figure 3.2 – `args` output in xv6.*

#### bash

```console
vboxuser@ubuntu:~/os-lab4$ ./args one two three
argc = 4
argv[0] = [./args]
argv[1] = [one]
argv[2] = [two]
argv[3] = [three]
vboxuser@ubuntu:~/os-lab4$ ./args *.c "x y"
argc = 5
argv[0] = [./args]
argv[1] = [a.c]
argv[2] = [args_linux.c]
argv[3] = [b.c]
argv[4] = [x y]
vboxuser@ubuntu:~/os-lab4$ ./args $HOME
argc = 2
argv[0] = [./args]
argv[1] = [/home/vboxuser]
vboxuser@ubuntu:~/os-lab4$ ./args 'single quoted'
argc = 2
argv[0] = [./args]
argv[1] = [single quoted]
```
*Figure 3.3 – `args_linux` output in bash.*

#### Side-by-side summary

| Line | bash (`./args`; folder has `a.c b.c args_linux.c`) | xv6 (`args`) |
|------|------|------|
| `one two three` | `argc = 4`: `[./args] [one] [two] [three]` | `argc = 4`: `[args] [one] [two] [three]` |
| `*.c "x y"` | `argc = 5`: `[./args] [a.c] [args_linux.c] [b.c] [x y]` | `argc = 4`: `[args] [*.c] ["x] [y"]` |
| `$HOME` | `argc = 2`: `[./args] [/home/vboxuser]` | `argc = 2`: `[args] [$HOME]` |
| `'single quoted'` | `argc = 2`: `[./args] [single quoted]` | `argc = 3`: `[args] ['single] [quoted']` |

>  **Comment:** `argv[0]` differs only because bash was launched as `./args` while xv6 was launched as `args`. The `argc` column tells the whole story: the *same program* got 5 arguments in one shell and 4 in the other, with identical input text — so the difference must come from the shell.

### Answers to the Observe questions

1. **Both outputs for `args *.c "x y"`.** In **bash** the program received **5** arguments: `./args`, the three `.c` files (`a.c`, `args_linux.c`, `b.c`) and the single argument `x y`. In **xv6** it received **4**: `args`, the literal `*.c`, and two pieces `"x` and `y"` (quote characters kept, string split at the space).
2. **Who turns `*.c` into file names and removes the quotes?** The **shell**. The program only receives the finished `argv` array; the kernel (`exec`) just copies that array into the new program without looking at it. bash expands `*.c` and strips quotes before starting the program; the xv6 shell does neither, so the program sees `*.c` and the quotes exactly as typed (and the xv6 root directory has no `.c` files anyway).
3. **What happened to the extra spaces in `one two three`?** Nothing is left of them: both shells use whitespace only as a **separator** between words, and any number of spaces counts as one separator. The program received exactly four arguments with no spaces inside. To keep spaces in an argument in bash, quote it: `"one  two"`.

---

## Part 4. Read the shell

```console
vboxuser@ubuntu:~/xv6-riscv$ wc -l user/sh.c
499 user/sh.c
vboxuser@ubuntu:~/xv6-riscv$ grep -n "#define" user/sh.c
8:#define EXEC  1
9:#define REDIR 2
10:#define PIPE  3
11:#define LIST  4
12:#define BACK  5
14:#define MAXARGS 10
```
*Figure 4.1 – The command kinds defined in `sh.c`.*

```console
vboxuser@ubuntu:~/xv6-riscv$ grep -n "case " user/sh.c
75:  case EXEC:
83:  case REDIR:
93:  case LIST:
101: case PIPE:
125: case BACK:
284: case 0:
286: case '|':
287: case '(':
288: case ')':
289: case ';':
290: case '&':
291: case '<':
294: case '>':
391: case '<':
394: case '>':
397: case '+':    // >>
469: case EXEC:
475: case REDIR:
481: case PIPE:
487: case LIST:
493: case BACK:
```
*Figure 4.2 – Where each kind is run.*

>  **Comment:** The `case` lines come in three groups: **lines 75–125** are `runcmd()` (executing each command kind), **lines 284–397** are the tokenizer/parser (recognising `| ( ) ; & < >` and `>>`), and **lines 469–493** are `nulterminate()`, which finalises the parsed tree. The shell is a classic *parse → build a command tree → execute* design.

```console
vboxuser@ubuntu:~/xv6-riscv$ grep -n "cd" user/sh.c | head
170:    fprintf(2, "cannot cd %s\n", cmd + 3);
```
*Figure 4.3 – The code that handles `cd`.*

```console
vboxuser@ubuntu:~/xv6-riscv$ grep -n "'\*'\|'\$'" user/sh.c
vboxuser@ubuntu:~/xv6-riscv$
```
*Figure 4.4 – Searching `sh.c` for `*` and `$`: no result.*

### Answers to the Observe questions

1. **The five kinds of command in the xv6 shell** (defined with `#define` in `sh.c`):

| Kind | Syntax | Meaning |
|------|--------|---------|
| `EXEC` | plain words | a simple command: program name and its arguments |
| `REDIR` | `<` `>` | redirection of input or output to a file |
| `PIPE` | `\|` | connects the output of one command to the input of another |
| `LIST` | `;` | run one command, then the next |
| `BACK` | `&` | run the command in the background (the shell does not wait) |

   Parentheses `( )` only group commands into a block; they do not create a sixth kind.

2. **Where is `cd` handled, and why is it not a separate program?** In `main()` of `sh.c`, in the loop that reads input lines: if the line starts with `"cd "`, the shell calls `chdir()` **itself** and continues to the next line; only other lines are run with `fork()` + `runcmd()`. The source comment says `chdir` must be called by the parent, not the child. A separate program would run in a child process and change only the child's directory, which disappears when it exits. This is also why `cd` in the middle of a line (`mkdir d ; cd d`) fails in xv6: it runs in a child as a program called `cd`, which doesn't exist (`exec cd failed`).

3. **Code that handles `*` or `$`, and what it explains about Part 2.** There is **none** — searching `sh.c` finds nothing, because the xv6 shell has no globbing and no variables. This explains why `ls *` printed an error, why `echo $HOME` printed `$HOME`, and why `args *.c` in Part 3 showed `[*.c]`. The same applies to quotes, history and PATH: the xv6 shell is only ~500 lines because it provides just the five kinds of commands above; everything else in bash is an addition.

---

## Conclusions

In this laboratory work I updated my `sleep` program to the renamed system call `pause()` so that it builds on the current xv6, and compared bash with the xv6 shell. Both shells can run programs, redirect input and output, build pipes, run command sequences and background commands, and group commands with parentheses. bash additionally offers globbing, variables, quoting, a PATH search, history and Tab completion, and job control; the xv6 shell has none of them, which I confirmed by experiment (`ls *`, `echo $HOME`, `echo "a b"`, `ls` after `cd d`).

With the `args` program I saw that a program never expands `*` or removes quotes: it only receives an array of strings from the shell through `exec`. In bash the shell prepares that array (globbing, variables, quotes); in xv6 it only splits the line at spaces. Reading `user/sh.c` showed that the xv6 shell knows only five kinds of commands (`EXEC`, `REDIR`, `PIPE`, `LIST`, `BACK`) and that `cd` must be implemented **inside** the shell, because it changes the working directory of the shell process itself.