#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "./jobs.h"

int jid;  // global jid var

/**
 * Prints the prompt and returns the number of bytes read in using read().
 *
 * Parameters:
 * - buf: input buffer
 * Returns:
 * -the number of bytes read into buffer
 */
ssize_t get_a_line(char buf[1024]) {  // returns num of lines successfully read
    memset(buf, 0, 1024);             // clear buffer
#ifdef PROMPT
    if (printf("33sh> ") < 0) {  // prompt
        perror("printing command prompt failed");
    }
    if (fflush(stdout) < 0) {
        perror("flushing for print of command prompt failed");
    }
#endif
    ssize_t byt_read = read(STDIN_FILENO, buf, 1024);  // read in to buffer
    if (byt_read < 0) {
        perror("read failed");
    }
    return byt_read;
}

/*
 * parse()
 *
 * - Description: creates the token and argv arrays from the buffer character
 *   array
 *
 * - Arguments: buffer: a char array representing user input, tokens: the
 *   tokenized input, argv: the argument array eventually used for execv()
 */
int parse(char buffer[1024], char *tokens[512], char *argv[512]) {
    int i = 0;
    int last_item = 0;
    const char delim[] = " \t\n";  // space, tab, new line

    tokens[i] = strtok(buffer, delim);
    argv[i] = tokens[i];  // can't call strtok again bc it won't populaate this
    while (tokens[i] != NULL) {
        if (!strcmp(tokens[i], ">") || !strcmp(tokens[i], "<") ||
            !strcmp(tokens[i], ">>")) {  // don't add redirections to argv
            argv[i] = tokens[i];  // should be the same unless there's a slash
        }
        argv[i] = tokens[i];
        i++;  // so we populate the whole array
        last_item++;
        tokens[i] = strtok(NULL, delim);
    }
    if (tokens[0] != NULL) {
        if (strrchr(tokens[0], '/') !=
            NULL) {  // strrchr will get the occurance of a character
            argv[0] = strrchr(tokens[0], '/') +
                      1;  // add one so it doesn't include the slash
        }
    }
    if (last_item > 0 && strcmp(tokens[last_item - 1], "&") == 0) {
        tokens[last_item - 1] =
            NULL;  // make this the end because we shouldn't have & in there
        return 1;
    }
    argv[i] = NULL;  // ensure null terminated
    return 0;
}

/**
 * Helper method to check what the command is in execute command. Helps make
 * code more readable.
 *
 * Parameters:
 * - tokens: input tokens
 * Returns:
 * -the true if command is exit
 */
int check_command(char *tokens[512],
                  char *str) {  // helper to check what the command is - makes
                                // code more readable
    if (tokens[0] != NULL) {
        return ((int)strcmp(tokens[0], str)) == 0;
    }
    return 0;
}

/**
 * Helper method to check that the first char of the token is % for fg/bg
 * commands.
 *
 * Parameters:
 * - tokens: input tokens
 * Returns:
 * -true if % is the first char
 */
int check_argument(char *tokens[512]) {  // helper checks token
    const char *token = tokens[1];
    return (token[0] == '%');
}

/**
 * Helper method to handle behavior for the fg command. It sends the
 * SIGCONT signal updates the jid andsets terminal control then waits before
 * restoring terminal control. It also handles signals and prints messages as
 * appropriate.
 *
 * Parameters:
 * - tokens: input tokens
 * - job_list: pointer to the job list
 * Returns:
 * - nothing
 */
void fg_command_handler(char *tokens[512], job_list_t *job_list) {
    int curr_jid;
    pid_t pid;
    if ((curr_jid = atoi(&tokens[1][1])) == 0) {
        perror("atoi failed");
        return;
    }
    if ((pid = get_job_pid(job_list, curr_jid)) < 0) {
        printf("job not found\n");
        return;
    }
    int status;
    if (kill(-pid, SIGCONT) < 0) {  // negative indicates sending signal to
                                    // group
        perror("kill failed\n");
    }
    if (update_job_jid(job_list, curr_jid, RUNNING) <
        0) {  // update status in job list
        printf("job not found\n");
    }
    if (tcsetpgrp(STDIN_FILENO, getpgid(pid)) < 0) {  // set terminal control
        perror("set terminal control fail");
    }
    if (waitpid(pid, &status, WUNTRACED) <
        0) {  // wait for process to finish running
        perror("waitpid fail");
    }
    if (tcsetpgrp(STDIN_FILENO, getpgid(0)) < 0) {  // restore terminal control
        perror("set terminal control fail");
    }

    if (WIFSTOPPED(status)) {  // check signals in fg
        if (update_job_jid(job_list, curr_jid, STOPPED) < 0) {
            printf("update job failed");
        }
        if (printf("[%d] (%d) suspended by signal %d\n", curr_jid, pid,
                   WSTOPSIG(status)) < 0) {
            printf("printing suspended message failed");
        }
    } else if (WIFSIGNALED(status)) {  // if signaled it's terminated so remove
                                       // it and print message
        if (remove_job_pid(job_list, pid) < 0) {
            printf("remove_job failed");
        }
        if (printf("(%d) terminated by signal %d\n", pid, WTERMSIG(status)) <
            0) {
            printf("printed terminated message failed");
        }
    } else if (WIFEXITED(status)) {  // if it's exited it should be removed from
                                     // the job list
        if (remove_job_pid(job_list, pid) < 0) {
            printf("remove_job failed");
        }
    }
}

/**
 * Helper method to handle behavior for the bg command. It sends the
 * SIGCONT signal updates the jid.
 *
 * Parameters:
 * - tokens: input tokens
 * - job_list: pointer to the job list
 * Returns:
 * - nothing
 */
void bg_command_handler(char *tokens[512], job_list_t *job_list) {
    int curr_jid;
    pid_t pid;
    if ((curr_jid = atoi(&tokens[1][1])) == 0) {
        perror("atoi failed");
        return;
    }
    if ((pid = get_job_pid(job_list, curr_jid)) < 0) {
        printf("job not found\n");
        return;  // want to return so we don't print mult error messages and it
                 // just makes sense to not continue the program
    }
    if (kill(-pid, SIGCONT) < 0) {  // should continue running
        perror("kill failed\n");
    }
    if (update_job_jid(job_list, curr_jid, RUNNING) <
        0) {  // update status in job list
        printf("job not found\n");
        return;
    }
}
/**
 * Handles functionality for executing built in commands. Utilizes the helper to
 * check what the command is. For exit it returns 2 so in the while loop in
 * main it can actually exit. It also checks syntax errors for cd. Also it
 * returns 1 if a command was in tokens so that in the main while loop we go
 * into the fork else if. It will also execute the built in function for
 * chdir,unlink(rm), and link. If there is no built in command to execute it
 * returns 0 so the whole process can be forked. It also returns 3 if it's a
 * newline. It also handles fg and bg commands.
 *
 * Parameters:
 * - tokens: input tokens
 *
 * Returns:
 * - value that determines main function logic
 */
int execute_builtin(char *tokens[512], job_list_t *job_list) {
    if (tokens[0] ==
        NULL) {  // if tokens is null then nothing has been inputted -> \n
        return 3;
    } else if (check_command(tokens,
                             "exit")) {  // returns true if command is exit
        return 2;
    } else if (check_command(tokens, "cd")) {
        if (tokens[1] == NULL || tokens[2] != NULL) {
            perror("cd: syntax error");
            return 1;  // better way to do this
        }
        if (chdir(tokens[1]) < 0) {
            perror("chdir failed");
        }
        return 1;
    } else if (check_command(tokens, "rm")) {  // if it's rm then unlink
        if (unlink(tokens[1]) < 0) {
            perror("unlink failed");
        }
        return 1;
    } else if (check_command(tokens, "ln")) {
        if (link(tokens[1], tokens[2]) < 0) {
            perror("link failed");
        }
        return 1;
    } else if (check_command(tokens, "jobs")) {
        jobs(job_list);  // void ret no error checking
        return 1;
    } else if (check_command(tokens, "fg")) {
        if (check_argument(tokens)) {
            fg_command_handler(tokens, job_list);
            return 1;
        }
    } else if (check_command(tokens, "bg")) {
        if (check_argument(tokens)) {
            bg_command_handler(tokens, job_list);
            return 1;
        }
    }
    return 0;
}

/**
 * This is a helper to handle opening and closing files. This was partly
 * born out of laziness so I don't have to do tons of error checking.
 *
 * Parameters:
 * token - the token the loop is on
 * file_no - file macro (STDIN etc)
 * flags - flags to be used
 * permissions - file permissions to use
 */
void files_handler(char *token, int file_no, int flags, int permissions) {
    if (close(file_no) < 0) {
        perror("close failed");
    }
    if (open(token, flags, permissions) < 0) {
        perror("open failed");
    }
}

/**
 * This method loops through the tokens and checks each redirection and then
 * calls the helper to handle it. It also has an else statement to handle
 * populating the argv array so nothing redirection related is populated
 * into that.
 *
 * Parameters:
 * - tokens: input tokens
 * - argv: arguments
 * Returns:
 * -nothing
 */
void redirections(char *tokens[512], char *argv[512]) {
    memset(argv, 0,
           512);  // reset argv so we can handle redirections populated into it
    int j = 0;
    int out_redir = 0;  // var to handle too many args
    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], ">") == 0) {  // check redir
            files_handler(tokens[i + 1], STDOUT_FILENO,
                          O_WRONLY | O_CREAT | O_TRUNC, 0644);
            i++;
            out_redir++;
        } else if (strcmp(tokens[i], "<") == 0) {  // check redir
            files_handler(tokens[i + 1], STDIN_FILENO, O_RDONLY, 0);
            i++;
        } else if (strcmp(tokens[i], ">>") == 0) {  // check redir
            files_handler(tokens[i + 1], STDOUT_FILENO,
                          O_WRONLY | O_CREAT | O_APPEND, 0644);
            i++;
            out_redir++;
        } else {
            argv[j++] = tokens[i];
        }
    }
    if (strrchr(tokens[0], '/') !=
        NULL) {  // strrchr will get the occurance of a character
        argv[0] = strrchr(tokens[0], '/') +
                  1;  // add one so it doesn't include the slash
    }

    if (out_redir > 1) {
        perror("syntax error: multiple output files");
    }
    argv[j] = NULL;  // null terminate the array
}

/**
 * This is a helper to get the filepath for tokens. This ensures the filepath
 * is passed in even when it isn't always the first argument.
 *
 * Parameters:
 * tokens - the tokens
 *
 * Returns:
 * - filepath for execv
 */
char *get_filepath(char *tokens[512]) {
    int i = 0;
    while (tokens[i] != NULL) {
        if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], ">>") == 0 ||
            strcmp(tokens[i], "<") == 0) {  // if it's a redirection
            i++;  // increment twice to pass by redirection and file name
            i++;
        } else {
            return tokens[i];  // next thing will be the filepath
        }
    }
    return NULL;
}

/**
 * Helper method to set signals in the main function.
 *
 * Parameters:
 * sigs - the signal to be set
 *
 * Returns:
 * nothing
 */
void set_sigs(sighandler_t sigs) {
    if (signal(SIGINT, sigs) == SIG_ERR) {
        perror("signal");
    }
    if (signal(SIGTSTP, sigs) == SIG_ERR) {
        perror("signal");
    }
    if (signal(SIGTTOU, sigs) == SIG_ERR) {
        perror("signal");
    }
}

/**
 * Reaps the background by waiting for all child processes to change state and
 * check if they terminated normally or were suspended.
 *
 * Parameters:
 * job_list - job list to maintain
 *
 * Returns:
 * nothing
 */
void reap_background(job_list_t *job_list) {
    // printf("reaps background");
    pid_t pid;
    int status;
    int curr_jid;

    // loop checks if any processes have changed state
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        if (pid < 0) {
            perror("waitpid failed\n");
        }
        if ((curr_jid = get_job_jid(job_list, pid)) < 0) {
            printf("job not found\n");
        }
        if (WIFEXITED(status)) {  // check if child process terminated normally
            if (printf("[%d] (%d) terminated with exit status %d\n", curr_jid,
                       pid, WEXITSTATUS(status)) < 0) {
                printf("printing terminated message failed");
            }
            if (remove_job_pid(job_list, pid) <
                0) {  // remove job -> job completed
                printf("job not found\n");
            }
        } else if (WIFSIGNALED(status)) {  // check if child process terminated
                                           // DELETE?!? by a signal
            if (printf("[%d] (%d) terminated by signal %d\n", curr_jid, pid,
                       WTERMSIG(status)) < 0) {
                printf("printing terminated failed\n");
            }
            if (remove_job_pid(job_list, pid) <
                0) {  // remove job -> completed by signal
                printf("job not found\n");
            }
        } else if (WIFSTOPPED(status)) {  // check if process stopped by a
                                          // signal
            if (update_job_pid(job_list, pid, STOPPED) < 0) {
                printf("job not found\n");
            }  // job stopped by signal -> suspend
            if (printf("[%d] (%d) suspended by signal %d\n", curr_jid, pid,
                       WSTOPSIG(status)) < 0) {
                printf("printing suspended failed\n");
            }
        } else if (WIFCONTINUED(status)) {  // check if child resumed execution
            if (printf("[%d] (%d) resumed\n", get_job_jid(job_list, pid), pid) <
                0) {
                printf("printing resumed failed");
            }
            if (update_job_pid(job_list, pid, RUNNING) < 0) {
                printf("job not found\n");
            };  // mark as running again
        }
    }
}

/**
 * Perhaps not aptly named but I got confused with the reaping section but
 * essentially handles signals for the foreground.
 *
 * Parameters:
 * job_list - job list to maintain
 *
 * Returns:
 * nothing
 */
void reap_foreground(job_list_t *job_list, char buffer[1024], pid_t pid) {
    // printf("we reap");
    int status;
    if (waitpid(pid, &status, WUNTRACED) < 0) {
        perror("waitpid");
    }
    if (WIFSIGNALED(status)) {  // check if foreground job is terminated
                                // by a signal
        if (printf("(%d) terminated by signal %d\n", pid, WTERMSIG(status)) <
            0) {
            printf("printing terminated by signal failed\n");
        }
    } else if (WIFSTOPPED(status)) {  // check if process stopped by a signal
        jid++;
        if (add_job(job_list, jid, pid, STOPPED,
                    buffer) < 0) {  // job not alr in job list so add it
            printf("job not found\n");
        }
        if (printf("[%d] (%d) suspended by signal %d\n", jid, pid,
                   WSTOPSIG(status)) < 0) {
            printf("printing terminated by signal failed\n");
        }
    } else if (WIFCONTINUED(status)) {  // check if child resumed execution
        if (update_job_pid(job_list, pid, RUNNING) <
            0) {  // mark as running again
            printf("job not found\n");
        }
        if (printf("[%d] (%d) resumed\n", jid, pid) < 0) {
            printf("printing resumed failed\n");
        }
    }
}

/**
 * This is the main function.
 */

int main() {
    char buf[1024];
    char *tokens[512];
    char *argv[512];
    ssize_t line;
    pid_t pid;
    jid = 0;
    // int status;
    int is_background = 0;
    job_list_t *job_list = init_job_list();
    set_sigs(SIG_IGN);  // ignore signals in parent process
    while ((line = get_a_line(buf)) != 0) {
        memset(tokens, 0, sizeof(tokens));  // for safety memset these
        memset(argv, 0, sizeof(argv));
        is_background = parse(buf, tokens,
                              argv);  // parse tells us if it's a background job
        int cmd_bool = execute_builtin(tokens, job_list);
        if (cmd_bool) {           // if cmd is executed
            if (cmd_bool == 2) {  // if cmd is exit then exit by breaking
                break;
            } else if (cmd_bool == 3) {
                reap_background(job_list);
                continue;
            }
        } else if ((pid = fork()) < 0) {  // error check
            perror("fork failed");
        } else if (!pid) {
            // now in child process
            pid_t curr_pid;
            if ((curr_pid = getpid()) < 0) {
                perror("getpid");
            }
            if (setpgid(curr_pid, curr_pid) < 0) {  // error check
                perror("setpgid");
            }
            if (!is_background) {  // if no the background reset terminal
                                   // control
                if (tcsetpgrp(STDIN_FILENO, curr_pid) < 0) {
                    perror("tcsetpgrp");
                }
            }
            set_sigs(SIG_DFL);  // reset signals
            redirections(tokens, argv);
            execv(get_filepath(tokens), argv);
            perror("execv");  // we won’t get here unless execv failed
            cleanup_job_list(job_list);  // cleanup before every exit
            exit(1);
        } else {
            pid_t g_pid;
            if ((g_pid = getpgrp()) < 0) {
                perror("getpgrp");
            }
            if (is_background) {
                jid++;
                if (add_job(job_list, jid, pid, RUNNING,
                            buf) < 0) {  // ensure jobs are added even if
                                         // signals are reset in parent process
                    printf("job not found");
                }
                if (printf("[%d] (%d)\n", jid, pid) < 0) {
                    perror("printing jid and pid failed");
                }
            } else {
                reap_foreground(job_list, buf, pid);
                if (tcsetpgrp(STDOUT_FILENO, g_pid) <
                    0) {  // restore terminal control
                    perror("tcsetpgrp");
                }
            }
        }
        reap_background(job_list);  // reaping background
    }
    cleanup_job_list(job_list);  // cleanup before every exit
    return 0;
}
