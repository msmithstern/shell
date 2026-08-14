```
 ____  _          _ _  
/ ___|| |__   ___| | |
\___ \| '_ \ / _ \ | | 
 ___) | | | |  __/ | |  
|____/|_| |_|\___|_|_| 
```


A Unix shell written in C. Reads and parses command lines, executes programs in child
processes, handles input/output redirection, responds to signals, and manages foreground
and background jobs with full terminal control.
 
Built in two milestones: a core execution shell, then job control layered on top.
 
## Features
 
**Execution**
- Built-in commands handled in-process; external programs executed via `fork` and `execv`
- Input and output redirection (`<`, `>`, `>>`), applied in the child before `exec`
- Clean exit handling distinguished from normal command completion
**Job control**
- Job list tracking every running and stopped job with its process group and state
- `fg` and `bg` built-ins to resume jobs in the foreground or background
- Terminal control transferred between the shell and foreground process groups
- Background jobs launched with `&`
- Terminated children reaped so no system resources are left held
**Signals**
- `SIGINT`, `SIGTSTP`, and `SIGQUIT` forwarded to the foreground job rather than killing the shell
- `SIGCHLD` handled to keep the job list in sync with actual process state
## Build
 
```
make clean all
```
 
This produces two binaries:
 
| Binary | Description |
|---|---|
| `33sh` | Interactive shell with a prompt |
| `33noprompt` | Same shell without the prompt, for scripted input and testing |
 
Either can be built on its own with `make 33sh` or `make 33noprompt`.
 
## Usage
 
```
./33sh
```
 
Then use it as you would any shell:
 
```
ls -la > output.txt        # redirect stdout
wc -l < source.txt         # redirect stdin
sleep 30 &                 # run in background
jobs                       # list jobs
fg %1                      # bring job 1 to the foreground
```
 
## Implementation notes
 
**Main loop.** The shell reads a line, tokenizes it, and dispatches to either a built-in
or an external command. Command execution returns a status code the loop uses to decide
whether to continue or terminate — a distinct code for `exit` keeps that path explicit
rather than relying on control-flow keywords scattered through the loop.
 
**Redirection.** Handled inside the child after `fork` and before `execv`, by scanning the
token list for redirection operators and rewiring the relevant file descriptors. Doing this
in the child leaves the parent shell's descriptors untouched.
 
**Job control.** The job list is the source of truth for what is running. Each job carries
its process group ID and state so the shell can signal an entire group, hand the terminal
over to a foreground job, and take it back when that job stops or exits. Reaping runs
against this list so terminated children are cleaned up and their entries removed.
 
**Parsing.** The tokenizer is adapted from an earlier exercise, extended to recognize the
trailing `&` that marks a background job.
 
Function-level and inline comments in the source cover the details.
 
## Files
 
| File | Contents |
|---|---|
| `sh.c` | Main loop, parsing, redirection, command dispatch, signal handling |
| `sh.h` | Shell declarations |
| `jobs.c` | Job list implementation |
| `jobs.h` | Job list interface |
| `Makefile` | Build targets for both binaries |
 
