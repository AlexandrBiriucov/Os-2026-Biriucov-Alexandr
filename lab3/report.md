<div align="center">

**Technical University of Moldova**
Faculty of Computers, Informatics and Microelectronics
Department of Software Engineering and Automation

# REPORT
### Laboratory Work No. 3
**Course:** Operating Systems
**Topic:** The Command Line: Shell, Pipes and Text Filters

**Done by:** st. gr. FAF-242, Biriucov Alexandr
**Checked by:** Reițman Patricia

Chișinău, 2026

</div>

---

## Purpose of the work

To learn to work fast and confidently in the Linux **bash** shell:

- use tab completion, history and built-in help;
- predict how the shell expands a command line (globbing, quoting, variable and command substitution) *before* the command runs;
- redirect standard input, output and error streams, and chain commands with `;`, `&&` and `||`;
- answer questions about data with one-line pipelines of small text filters (`grep`, `cut`, `sort`, `uniq`, `wc`, `head`, `tail`, `tr`);
- turn working one-liners into a first shell script.

---

## Part 1. Moving fast and finding help

### 1.1 Tab completion and history

```bash
ls /usr/sh<Tab><Tab>
cd /et<Tab>
```

**Observation:** `/usr/sh` has only one match, so Tab completed it to `/usr/share/` (when several names match, pressing Tab twice lists them all). With `cd /et`, one Tab completed it to `/etc/`. Tab completion saves typing and avoids typos.

*Figure 1.1 – Tab completion: `ls /usr/share/` and `cd /etc/` (screenshots in the original report).*

```bash
date
whoami
pwd
# (press Ctrl+R, type "dat", press Enter)
```

**Observation:** `Ctrl+R` opens *reverse-i-search*: as you type, the shell shows the most recent history entry containing those letters; Enter runs it (pressing `Ctrl+R` again goes to an older match). Here the letters `dat` found `date`, which ran again.

### 1.2 Built-in or external, and the PATH

```bash
type cd ; type ls ; type echo ; which ls
echo $PATH
echo $PATH | tr ':' '\n'
echo $PATH | tr ':' '\n' | wc -l
nosuchprog ; echo "exit code: $?"
```

```console
vboxuser@ubuntu:~$ type cd ; type ls ; type echo ; which ls
cd is a shell builtin
ls is aliased to `ls --color=auto'
echo is a shell builtin
/usr/bin/ls

vboxuser@ubuntu:~$ echo $PATH
/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/snap/bin

vboxuser@ubuntu:~$ echo $PATH | tr ':' '\n'
/usr/local/sbin
/usr/local/bin
/usr/sbin
/usr/bin
/sbin
/bin
/usr/games
/usr/local/games
/snap/bin
/snap/bin

vboxuser@ubuntu:~$ echo $PATH | tr ':' '\n' | wc -l
10

vboxuser@ubuntu:~$ nosuchprog ; echo "exit code: $?"
nosuchprog: command not found
exit code: 127
```

>  **Comment:** `ls` is reported as an **alias** (it adds colors) and `which ls` shows the real external program, `/usr/bin/ls`. `/snap/bin` appears twice in `PATH` — harmless, usually caused by two config files both adding it. The shell searches the 10 entries from left to right and runs the first match.

### 1.3 Help without a browser

```bash
man ls                  # inside: /sort then n for next, q to quit
ls --help | head
man -k "list directory"
help cd
```

| Command | What it does |
|---------|--------------|
| `man` | shows the full manual page of a program |
| `--help` | prints a short summary |
| `man -k` | searches manual-page descriptions by keyword |
| `help` | bash's help for **built-in** commands such as `cd` |

```console
vboxuser@ubuntu:~$ ls --help | head
List directory contents.
Ignore files and directories starting with a '.' by default
Usage: ls [OPTION]... [FILE]...
...

vboxuser@ubuntu:~$ man -k "list directory"
dir (1)      - List directory contents. Ignore files and directories ...
gnuls (1)    - list directory contents
ls (1)       - List directory contents. Ignore files and directories ...
ntfsls (8)   - list directory contents on an NTFS filesystem
rust-ls (1)  - List directory contents. ...
vdir (1)     - List directory contents. ...

vboxuser@ubuntu:~$ help cd
cd: cd [-L|[-P [-e]]] [-@] [dir]
    Change the shell working directory.
    ...
    Exit Status:
    Returns 0 if the directory is changed, ...
```

>  **Comment:** The `ls --help` text here (`Arguments: [paths]...`, `rust-ls` in the `man -k` list) suggests this Ubuntu uses the **Rust `uutils coreutils`** rewrite rather than GNU `ls`, which is why the help text differs from the traditional format.

### Answers to the Observe questions

1. **Which of `cd`, `ls`, `echo` are built-ins, and why must `cd` be one?**
   `cd` and `echo` are shell built-ins; `ls` is an external program stored on disk (`which ls`). `cd` must be a built-in because every program runs as a separate **child process** with its own working directory. An external `cd` would change only its own child's directory and then exit, leaving the shell where it was. Only the shell itself can change its own working directory.
2. **How many directories are in PATH and where is `ls`?** `PATH` is a colon-separated list of directories searched left to right for external commands. It contains **10 entries** (9 unique), and `ls` is in `/usr/bin/ls`.
3. **Exit code of a command that is not found:** **127** (0 = success, non-zero = error; 127 specifically means "command not found").
4. **Which option of `ls` sorts by modification time?** `-t` (newest first); `ls -lt` adds details and `-r` reverses the order (`ls -ltr`).

---

## Part 2. What the shell does to your line

### 2.1 Setup

```console
vboxuser@ubuntu:~$ mkdir -p ~/os-lab3/play && cd ~/os-lab3/play
vboxuser@ubuntu:~/os-lab3/play$ touch a.c b.c file1.txt file2.txt notes.txt "alfa beta.txt"
vboxuser@ubuntu:~/os-lab3/play$ ls
a.c  'alfa beta.txt'  b.c  file1.txt  file2.txt  notes.txt
```

### 2.2 Predictions and real results

| Command | My prediction | Real output | Explanation |
|---------|---------------|-------------|-------------|
| `echo *.c` | print all `.c` files | `a.c b.c` | The shell expanded the glob `*.c` into matching names **before** `echo` ran. |
| `echo file?.txt` | `?` matches exactly one character | `file1.txt file2.txt` | `?` matches exactly one character. |
| `echo [a-b]*` | files starting with a or b | `a.c alfa beta.txt b.c` | Any name starting with a or b; the name with a space stays **one** argument because it came from expansion. |
| `echo *.xyz` | I don't know | `*.xyz` | No file matches, so bash leaves the pattern as literal text. |
| `ls alfa beta.txt` | list files called alfa and beta | `ls: cannot access 'alfa': No such file or directory`<br>`ls: cannot access 'beta.txt': ...` | The shell split the line at the space into **two** arguments. |
| `ls "alfa beta.txt"` | keep it as a single argument | `'alfa beta.txt'` | Double quotes keep the name as one argument. |
| `echo '$HOME' "$HOME" \$HOME` | literal, expanded, literal | `$HOME /home/vboxuser $HOME` | Single quotes: literal. Double quotes: variable expanded. Backslash: next char literal. |
| `echo {1..5} ; echo $((7 * 6))` | 1–5 and 42 | `1 2 3 4 5` / `42` | Brace expansion generates the sequence; `$(( ))` is arithmetic expansion. |
| `echo "today is $(date +%A), I am $(whoami)"` | today's weekday and my username | `today is Monday, I am vboxuser` | `$( )` runs the command and substitutes its output. |
| `touch log{1..3}.txt && ls log*` | I don't know | `log1.txt log2.txt log3.txt` | Brace expansion created the three files; `log*` then matched them. |

**Real terminal output:**

```console
vboxuser@ubuntu:~/os-lab3/play$ echo *.c
a.c b.c
vboxuser@ubuntu:~/os-lab3/play$ echo file?.txt
file1.txt file2.txt
vboxuser@ubuntu:~/os-lab3/play$ echo [a-b]*
a.c alfa beta.txt b.c
vboxuser@ubuntu:~/os-lab3/play$ echo *.xyz
*.xyz
vboxuser@ubuntu:~/os-lab3/play$ ls alfa beta.txt
ls: cannot access 'alfa': No such file or directory
ls: cannot access 'beta.txt': No such file or directory
vboxuser@ubuntu:~/os-lab3/play$ ls "alfa beta.txt"
'alfa beta.txt'
vboxuser@ubuntu:~/os-lab3/play$ ls alfa\ beta.txt
'alfa beta.txt'
vboxuser@ubuntu:~/os-lab3/play$ echo '$HOME' "$HOME" \$HOME
$HOME /home/vboxuser $HOME
vboxuser@ubuntu:~/os-lab3/play$ echo {1..5} ; echo $((7 * 6))
1 2 3 4 5
42
vboxuser@ubuntu:~/os-lab3/play$ echo "today is $(date +%A), I am $(whoami)"
today is Monday, I am vboxuser
vboxuser@ubuntu:~/os-lab3/play$ touch log{1..3}.txt && ls log*
log1.txt log2.txt log3.txt
```

>  **Comment:** The two surprises were `echo *.xyz` (an unmatched glob is passed through unchanged, so `echo` prints the pattern) and `ls alfa beta.txt` (word splitting). Both show the same rule: **expansion is done by the shell, and the command only sees the final list of arguments.**

### Answers to the Observe questions

1. See the table above.
2. **Who produced the file names for `echo *.c`?** The **shell**. Globbing happens before the command starts: bash replaces `*.c` with `a.c b.c`, and `echo` just prints its arguments. `echo` knows nothing about files.
3. **Why did `ls alfa beta.txt` fail, and two fixes?** The shell splits words at spaces, so `ls` received two arguments, neither existing. **Fix 1:** quote it — `ls "alfa beta.txt"`. **Fix 2:** escape the space — `ls alfa\ beta.txt` (Tab completion inserts the backslash automatically).
4. **Single vs. double quotes:** inside single quotes everything is literal (`$HOME` stays `$HOME`). Inside double quotes spaces and wildcards are protected, but `$variables`, `$( )` and backslash are still interpreted (`$HOME` → `/home/vboxuser`).

**Why does `ls` show `'alfa beta.txt'` in quotes?** When writing to a terminal, `ls` quotes names that contain special characters such as spaces, so you can copy and paste them back as one argument. The quotes are not part of the real file name (`ls | cat` prints it without them).

---

## Part 3. Redirection, chaining and jobs

### 3.1 Separate stdout and stderr

```console
vboxuser@ubuntu:~/os-lab3$ ls /etc /nope > out.txt 2> err.txt
vboxuser@ubuntu:~/os-lab3$ wc -l out.txt ; cat err.txt
238 out.txt
ls: cannot access '/nope': No such file or directory
```

### 3.2 Order of redirections

```console
vboxuser@ubuntu:~/os-lab3$ ls /etc /nope > all.txt 2>&1 ; tail -2 all.txt
xml
zsh_command_not_found

vboxuser@ubuntu:~/os-lab3$ ls /etc /nope 2>&1 > all.txt ; tail -2 all.txt
ls: cannot access '/nope': No such file or directory
xml
zsh_command_not_found
```

>  **Comment:** In the first command the error message is *inside* `all.txt` (it was sorted into the file, so `tail -2` doesn't show it), while in the second it appeared on the **screen** and `all.txt` contains only the listing. Redirections are processed **left to right**.

### 3.3 Append and input redirection

```console
vboxuser@ubuntu:~/os-lab3$ echo first > notes.md ; echo second >> notes.md ; cat notes.md
first
second
vboxuser@ubuntu:~/os-lab3$ wc -l < access.log
500
vboxuser@ubuntu:~/os-lab3$ wc -l access.log
500 access.log
```

### 3.4 Operators `;` `&&` `||`

```console
vboxuser@ubuntu:~/os-lab3$ ls /nope 2> /dev/null || echo "failed with code $?"
failed with code 2
vboxuser@ubuntu:~/os-lab3$ mkdir -p tmp && cd tmp && echo "I am in $(pwd)" ; cd ~/os-lab3
I am in /home/vboxuser/os-lab3/tmp
vboxuser@ubuntu:~/os-lab3$ false ; echo "A: runs after ;"
A: runs after ;
vboxuser@ubuntu:~/os-lab3$ false && echo "B: runs after &&"
vboxuser@ubuntu:~/os-lab3$ false || echo "C: runs after ||"
C: runs after ||
```

>  **Comment:** `ls /nope` exits with code **2** (a "serious trouble" status for `ls`), different from 127 for a missing command. Sending `2> /dev/null` discards the error message — `/dev/null` swallows everything written to it.

### 3.5 Background jobs

```console
vboxuser@ubuntu:~/os-lab3$ sleep 100 &
[1] 6199
vboxuser@ubuntu:~/os-lab3$ jobs ; kill %1 ; jobs
[1]+  Running                 sleep 100 &
[1]+  Running                 sleep 100 &
[1]+  Terminated              sleep 100
```

The trailing `&` starts the command in the background, so the prompt returns immediately. `jobs` lists background jobs, `kill %1` terminates job number 1, and the last `jobs` shows it as `Terminated`.

>  **Comment:** The second `Running` line appears because `kill` only *sends* the termination signal; bash reports the new status on the next prompt/`jobs` call.

### Answers to the Observe questions

1. **Which file got the error and which the listing?** The `/etc` listing (**stdout**, stream 1) went to `out.txt`; the error message (**stderr**, stream 2) went to `err.txt`.
2. **`> all.txt 2>&1` vs. `2>&1 > all.txt`:** the shell processes redirections left to right.
   - `> all.txt 2>&1`: stdout → file, then stderr becomes a copy of stdout (the file) ⇒ **both** go to `all.txt`.
   - `2>&1 > all.txt`: stderr becomes a copy of stdout (still the terminal), *then* stdout → file ⇒ the error stays on the **screen**, only the listing is in `all.txt`.
3. **Why does `wc -l < access.log` print only a number?** With `<` the shell opens the file and gives it to `wc` as standard input; `wc` never learns a file name. With `wc -l access.log`, `wc` opens the file itself and prints its name.
4. **Operators:**
   - `;` — run the next command regardless of the result.
   - `&&` — run the next command only if the previous **succeeded** (exit code 0).
   - `||` — run the next command only if the previous **failed** (non-zero exit code).

---

## Part 4. Answering questions with one-liners

Each line of `access.log` has five space-separated fields: **date, time, user, result (OK/FAIL), page**. The data set has 500 lines (`wc -l access.log` → `500`). All commands were saved in `oneliners.sh`, one per question.

### Q1. How many requests failed?

```console
$ grep -c FAIL access.log
100
```
`grep -c` counts the lines containing `FAIL`.

### Q2. How many different pages were requested, and which?

```console
$ cut -d' ' -f5 access.log | sort -u | wc -l ; cut -d' ' -f5 access.log | sort -u
4
/page0
/page1
/page2
/page3
```
`cut -d' ' -f5` takes the 5th field (page); `sort -u` sorts and removes duplicates; `wc -l` counts what is left.

### Q3. How many requests did each page get?

```console
$ cut -d' ' -f5 access.log | sort | uniq -c
    125 /page0
    125 /page1
    125 /page2
    125 /page3
```
`uniq` only collapses **adjacent** duplicates, which is why `sort` comes first; `-c` prefixes each line with its count. **Check:** 4 × 125 = 500 ✔

### Q4. Which user(s) have the most failed requests, and how many?

```console
$ grep FAIL access.log | cut -d' ' -f3 | sort | uniq -c | sort -rn | awk 'NR==1{m=$1} $1==m'
     15 user5
     15 user3
```
The pipeline counts failures per user in descending order; `awk` keeps every line whose count equals the maximum, so **ties** are shown.

| user0 | user1 | user2 | user3 | user4 | user5 | user6 | Total |
|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|
| 14 | 14 | 14 | **15** | 14 | **15** | 14 | 100 ✔ (= Q1) |

### Q5. How many requests did `user3` make, and how many failed?

```console
$ grep -c 'user3 ' access.log ; grep 'user3 ' access.log | grep -c FAIL
72
15
```
`user3` made 72 requests, 15 of which failed. The **trailing space** in the pattern avoids matching names that merely begin with `user3`.

### Q6. Last 3 failed requests, showing only time and user

```console
$ grep FAIL access.log | tail -3 | cut -d' ' -f2,3
10:10 user0
10:15 user5
10:20 user3
```

### Q7. Which login shells appear in `/etc/passwd`, and how many accounts use each?

```bash
cut -d: -f7 /etc/passwd | sort | uniq -c | sort -rn
```
The 7th colon-separated field is the login shell. Accounts with `/usr/sbin/nologin` or `/bin/false` are system accounts that cannot log in interactively. *(Result in the full run below: 40 × `nologin`, 6 × `/bin/false`, 2 × `/bin/bash`, 1 × `/bin/sync`.)*

### Q8. Why do `ls /etc | wc -l` and `ls -l /etc | wc -l` differ by exactly one?

```bash
ls /etc | wc -l
ls -l /etc | wc -l
ls -l /etc | head -3
```
The long format prints one line per entry **plus a header line** `total N` (disk blocks used by the listed files). `wc -l` counts that header, so the second number is exactly one larger.

### Full run of `oneliners.sh`

```console
vboxuser@ubuntu:~/os-lab3$ bash oneliners.sh
100
4
/page0
/page1
/page2
/page3
    125 /page0
    125 /page1
    125 /page2
    125 /page3
     15 user5
     15 user3
72
15
10:10 user0
10:15 user5
10:20 user3
     40 /usr/sbin/nologin
      6 /bin/false
      2 /bin/bash
      1 /bin/sync
50
```

### Bonus. Failed requests whose minute ends in 0

```console
$ grep -E ' 10:[0-5]0 .*FAIL' access.log | wc -l
50
```
The extended regex `10:[0-5]0` matches `10:00, 10:10, … 10:50`, and `.*FAIL` requires the word `FAIL` later on the same line.

---

## Part 5. Your first script

`report.sh` was written from the lab template, with the TODO completed as a `for` loop over `user0 … user6`:

```bash
#!/bin/bash
# usage: ./report.sh access.log
log=$1
if [ ! -f "$log" ]; then
  echo "no such file: $log" >&2
  exit 1
fi
total=$(wc -l < "$log")
fails=$(grep -c FAIL "$log")
echo "$total requests, $fails failed"
for n in 0 1 2 3 4 5 6; do
  x=$(grep -c "user$n FAIL" "$log")
  echo "user$n: $x failures"
done
```

```console
vboxuser@ubuntu:~/os-lab3$ ./report.sh access.log
bash: ./report.sh: Permission denied          <- before chmod
vboxuser@ubuntu:~/os-lab3$ chmod +x report.sh
vboxuser@ubuntu:~/os-lab3$ ./report.sh access.log
500 requests, 100 failed
user0: 14 failures
user1: 14 failures
user2: 14 failures
user3: 15 failures
user4: 14 failures
user5: 15 failures
user6: 14 failures
vboxuser@ubuntu:~/os-lab3$ ./report.sh nope.log ; echo "exit code: $?"
no such file: nope.log
exit code: 1
```

>  **Comment:** The script output matches the one-liners: 500 total, 100 failures, and the per-user numbers equal the table from Q4. The error path shows the correct behaviour — message on stderr and a non-zero exit code.

### Answers to the Observe questions

1. **What does `#!/bin/bash` do, and what if you run `./report.sh` before `chmod +x`?** The first line (**shebang**) tells the kernel which interpreter must run the file. Before `chmod +x` the file has no execute permission, so the shell refuses: `Permission denied`.
2. **Why `./report.sh` and not `report.sh`?** Without a slash the shell searches only the directories in `PATH`, and the current directory is deliberately *not* in `PATH` (security: a malicious file named like a standard command could be run by accident). `./` gives an explicit path, so no PATH lookup is done.
3. **Why `>&2` and `exit 1`?** `>&2` sends the message to **standard error**, so errors do not mix with normal output when it is redirected or piped. `exit 1` ends the script with a **non-zero exit code**, which callers (`$?`, `&&`, `||`, cron) use to detect failure. Exit code 0 means success.

### Bonus: running the script with cron

```bash
crontab -e
# add this line (replace USER with your username):
* * * * * /home/USER/os-lab3/report.sh /home/USER/os-lab3/access.log >> /home/USER/os-lab3/report.log 2>&1
# wait 2 minutes, then:
crontab -l
cat ~/os-lab3/report.log
# finally remove the line:
crontab -e      # (or crontab -r to delete the whole crontab)
```

>  **Comment:** `* * * * *` means "every minute". Cron runs with a minimal environment, so **absolute paths** are used, and `>> … 2>&1` appends both stdout and stderr to a log file.

---

## Conclusions

In this laboratory work I worked with the Linux bash shell from the command line. I used Tab completion, `Ctrl+R` history search and the built-in help (`man`, `--help`, `help`) to work faster, and learned that commands are either shell **built-ins** (`cd`, `echo`) or **external programs** found through the directories of `PATH`; a command that is not found returns exit code **127**.

I saw that the shell — not the command — performs globbing, brace expansion, variable and command substitution and quote handling *before* the command starts, and that single quotes, double quotes and backslash control this expansion. Predicting results first made the differences between my expectations and the real behaviour clear.

I redirected stdout and stderr to files, understood why the order of redirections matters (`> file 2>&1` vs. `2>&1 > file`), chained commands with `;`, `&&` and `||`, and ran and stopped a background job. Finally, I answered eight questions about a 500-line log with one-line pipelines of `grep`, `cut`, `sort`, `uniq`, `wc`, `head` and `tail`, cross-checked the results (Q3 sums to 500, per-user failures sum to the 100 of Q1), and turned them into the script `report.sh` with a loop, an argument check, an error message on stderr and a meaningful exit code.