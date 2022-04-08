#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define MAXLINE 1024
#define MAXJOBS 1024
#define TEN_MS 10000000

pid_t MASTER_PID;

char **environ;

// TODO: you will need some data structure(s) to keep track of jobs
struct Job {
    int job_id;
    pid_t pid;
    char status[10];    // status string will not exceed 10 chars
    char cmd[30];       
    bool terminated;
    bool suspended;
    bool fg;
};

volatile struct Job FG_JOB;

volatile struct Job JOBS[MAXJOBS + 1];

volatile int JOB_ID = 1;

int get_jid (pid_t pid) {
    for (int i = 1; i < JOB_ID; i++) {
        if (JOBS[i].pid == pid) {
            return JOBS[i].job_id;
        }
    }

    return -1;
}

bool no_pid(pid_t pid) {
    for (int i = 1; i < JOB_ID; ++i) {
        if (!JOBS[i].terminated) {
            if (JOBS[i].pid == pid) {
                return false;
            }
        }
    }

    return true;
}

int get_fg_jid(pid_t pid) {
    return FG_JOB.job_id;
}

// counts number of digits in integer
int num_int_dig (int integer) {
    int count = 0;
    do {
        integer = integer / 10;
        count++;
    } while (integer != 0);

    return count;
}

void num_to_string(int integer, int num_dig, char string[]) {
    char digits[] = "0123456789";
    int remainder = 0;
    int ptr = num_dig - 1;

    string[num_dig] = '\0';

    while (num_dig > 0) {
        remainder = integer % 10;
        string[ptr] = digits[remainder];
        integer = integer / 10;
        ptr--;
        num_dig--;
    }
}

void remove_fg_job () {
    FG_JOB.terminated = true;
}

void print_fg_terminated (pid_t pid) {
    int p = (int) pid;
    int j = get_fg_jid(pid);
    
    int num_pid_dig = num_int_dig(p);
    int num_jid_dig = num_int_dig(j);

    char cmd[30];
    strcpy(cmd, FG_JOB.cmd);
    char str_pid[num_pid_dig + 1];
    char str_jid[num_jid_dig + 1];
    
    num_to_string(p, num_pid_dig, str_pid);
    num_to_string(j, num_jid_dig, str_jid);

    char msg[30];
    strcpy(msg, "[");
    strcat(msg, str_jid);
    strcat(msg, "] (");
    strcat(msg, str_pid);
    strcat(msg, ")  killed  ");
    strcat(msg, cmd);
    strcat(msg, "\n");
    int total_len = strlen(str_pid) + strlen(str_jid) + strlen(cmd) + 16;
    write(1, msg, total_len);
}

void print_fg_suspended(pid_t pid) {
    int p = (int) pid;
    int j = get_fg_jid(pid);
    
    int num_pid_dig = num_int_dig(p);
    int num_jid_dig = num_int_dig(j);

    char cmd[30];
    strcpy(cmd, FG_JOB.cmd);
    char str_pid[num_pid_dig + 1];
    char str_jid[num_jid_dig + 1];
    
    num_to_string(p, num_pid_dig, str_pid);
    num_to_string(j, num_jid_dig, str_jid);

    char msg[30];
    strcpy(msg, "[");
    strcat(msg, str_jid);
    strcat(msg, "] (");
    strcat(msg, str_pid);
    strcat(msg, ")  suspended  ");
    strcat(msg, cmd);
    strcat(msg, "\n");
    int total_len = strlen(str_pid) + strlen(str_jid) + strlen(cmd) + 19;
    write(1, msg, total_len);
}

void print_suspended(pid_t pid) {
    pid_t p = pid;
    int j = get_jid(pid);
    int num_pid_dig = num_int_dig(p);
    int num_jid_dig = num_int_dig(j);

    char cmd[30];
    strcpy(cmd, FG_JOB.cmd);
    char str_pid[num_pid_dig + 1];
    char str_jid[num_jid_dig + 1];
    
    num_to_string(p, num_pid_dig, str_pid);
    num_to_string(j, num_jid_dig, str_jid);

    char msg[30];
    strcpy(msg, "[");
    strcat(msg, str_jid);
    strcat(msg, "] (");
    strcat(msg, str_pid);
    strcat(msg, ")  suspended  ");
    strcat(msg, cmd);
    strcat(msg, "\n");
    int total_len = strlen(str_pid) + strlen(str_jid) + strlen(cmd) + 19;
    write(1, msg, total_len);
}

void print_background_job(pid_t pid) {
    for(int i = 0; i < JOB_ID; ++i) {
        if (JOBS[i].pid == pid) {
            printf("[%d] (%d)  %s\n", JOBS[i].job_id, pid, JOBS[i].cmd);
            fflush(stdout);
            return;
        }
    }
}

void print_terminated(pid_t pid) {
    int p = (int) pid;
    int j = get_jid(pid);
    if (j == -1) {
        write(1, "ERROR: failed to get job id in signal handler\n", 47);
    }
    int num_pid_dig = num_int_dig(p);
    int num_jid_dig = num_int_dig(j);

    char cmd[30];
    strcpy(cmd, JOBS[j].cmd);
    char str_pid[num_pid_dig + 1];
    char str_jid[num_jid_dig + 1];
    
    num_to_string(p, num_pid_dig, str_pid);
    num_to_string(j, num_jid_dig, str_jid);

    char msg[30];
    strcpy(msg, "[");
    strcat(msg, str_jid);
    strcat(msg, "] (");
    strcat(msg, str_pid);
    strcat(msg, ")  killed  ");
    strcat(msg, cmd);
    strcat(msg, "\n");
    int total_len = strlen(str_pid) + strlen(str_jid) + strlen(cmd) + 16;
    write(1, msg, total_len);
}

void record_bg_to_fg_job (pid_t pid) {
    int jid = get_jid(pid);
    FG_JOB.terminated = false;
    FG_JOB.suspended = JOBS[jid].suspended;
    FG_JOB.pid = pid;
    FG_JOB.job_id = jid;
    memset(FG_JOB.cmd, '\0', sizeof(FG_JOB.cmd));
    strcpy(FG_JOB.cmd, JOBS[jid].cmd);
    memset(FG_JOB.status, '\0', sizeof(FG_JOB.status));
    strcpy(FG_JOB.status, "running");

    return;
}
void record_fg_to_bg_job (pid_t pid) {
    int jid = FG_JOB.job_id;
    JOBS[jid].job_id = jid;
    JOBS[jid].pid = pid;
    memset(JOBS[jid].cmd, '\0', sizeof(JOBS[jid].cmd));   // init cmd
    strcpy(JOBS[jid].cmd, FG_JOB.cmd);
    memset(JOBS[jid].status, '\0', sizeof(JOBS[jid].status));   // init status 
    strcpy(JOBS[jid].status, "suspended");
    JOBS[jid].terminated = false;
    JOBS[jid].suspended = FG_JOB.suspended;

    return;
}

void bg_running_to_suspended(pid_t pid) {
    int j = get_jid(pid);
    strcpy(JOBS[j].status, "suspended");
    JOBS[j].terminated = false;
    JOBS[j].suspended = true;
}

void bg_suspended_to_running(pid_t pid) {
    int j = get_jid(pid);
    strcpy(JOBS[j].status, "running");
    JOBS[j].terminated = false;
    JOBS[j].suspended = false;
}

void bg_print_suspended(pid_t pid) {
    pid_t p = pid;
    int j = get_jid(pid);
    int num_pid_dig = num_int_dig(p);
    int num_jid_dig = num_int_dig(j);

    char cmd[30];
    strcpy(cmd, FG_JOB.cmd);
    char str_pid[num_pid_dig + 1];
    char str_jid[num_jid_dig + 1];
    
    num_to_string(p, num_pid_dig, str_pid);
    num_to_string(j, num_jid_dig, str_jid);

    char msg[30];
    strcpy(msg, "[");
    strcat(msg, str_jid);
    strcat(msg, "] (");
    strcat(msg, str_pid);
    strcat(msg, ")  suspended  ");
    strcat(msg, cmd);
    strcat(msg, "\n");
    int total_len = strlen(str_pid) + strlen(str_jid) + strlen(cmd) + 19;
    write(1, msg, total_len);
}

void remove_job(pid_t pid) {
    for (int i = 0; i < JOB_ID; ++i) {
        if (JOBS[i].pid == pid) {
            JOBS[i].terminated = true;
            return;
        }
    }
    return;
}

void handle_sigchld(int sig) {
    // TODO
    if (getpid() != MASTER_PID) {
        // this is child process
        return;
    }
    pid_t pid;
    int status;

    // This will reap all of dead children processors
    while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        if (WIFSIGNALED(status) != 0) {
            print_terminated(pid);
        }

        if (WIFSTOPPED(status)) {
            if (WSTOPSIG(status) != SIGTSTP) {
                bg_print_suspended(pid);
                bg_running_to_suspended(pid);
                continue;
            } else {
                if(!FG_JOB.terminated) {
                    print_fg_suspended(pid);
                    record_fg_to_bg_job(pid);
                    remove_fg_job();
                    continue;
                } else {
                    bg_print_suspended(pid);
                    bg_running_to_suspended(pid);
                    continue;
                }
            }
        }

        if (WIFCONTINUED(status)) {
            bg_suspended_to_running(pid);
            print_background_job(pid);
            continue;
        }
        remove_job(pid);
    } 
    
    return;
}

void handle_sigtstp(int sig) {
    if (getpid() != MASTER_PID) {
        // this is child process
        return;
    }

    if (!FG_JOB.terminated) {
        pid_t pid = FG_JOB.pid;
        FG_JOB.suspended = true;
        print_fg_suspended(pid);
        record_fg_to_bg_job(pid);
        remove_fg_job();
        
        kill(pid, SIGTSTP); 
    }
}

void handle_sigint(int sig) {
    if (getpid() != MASTER_PID) {
        // this is child process
        return;
    }

    if (!FG_JOB.terminated) {
        pid_t pid = FG_JOB.pid;
        remove_fg_job();
        kill(pid, SIGINT);
    }
}

void handle_sigquit(int sig) {
    if (getpid() != MASTER_PID) {
        // this is child process
        return;
    }

    if (FG_JOB.terminated) {
        exit(0);
    } else {
        pid_t pid = FG_JOB.pid;
        remove_fg_job();
        kill(pid, SIGQUIT);
    }
}

void install_signal_handlers() {
    // TODO
    FG_JOB.terminated = true;
    MASTER_PID = getpid();  // pid of this shell's init (master) process

    // sigaction for SIGCHLD
    struct sigaction sigchld_act;
    sigemptyset(&sigchld_act.sa_mask);
    sigchld_act.sa_handler = handle_sigchld;
    sigchld_act.sa_flags = SA_RESTART;   
    sigaction(SIGCHLD, &sigchld_act, NULL);

    // sigaction for SIGINT
    struct sigaction sigint_act;
    sigemptyset(&sigint_act.sa_mask);
    // sigaddset(&sigint_act.sa_mask, SIGINT);
    // sigaddset(&sigint_act.sa_mask, SIGQUIT);
    // sigaddset(&sigint_act.sa_mask, SIGTSTP);
    sigaddset(&sigint_act.sa_mask, SIGCHLD);
    sigint_act.sa_handler = handle_sigint;
    sigint_act.sa_flags = SA_RESTART;   
    sigaction(SIGINT, &sigint_act, NULL); 

    // sigaction for SIGQUIT
    struct sigaction sigquit_act;
    sigemptyset(&sigquit_act.sa_mask);
    // sigaddset(&sigquit_act.sa_mask, SIGINT);
    // sigaddset(&sigquit_act.sa_mask, SIGQUIT);
    // sigaddset(&sigquit_act.sa_mask, SIGTSTP);
    sigaddset(&sigquit_act.sa_mask, SIGCHLD);
    sigquit_act.sa_handler = handle_sigquit;
    sigquit_act.sa_flags = SA_RESTART;   
    sigaction(SIGQUIT, &sigquit_act, NULL);  

    // sigaction for SIGTSTP
    struct sigaction sigtstp_act;
    sigemptyset(&sigtstp_act.sa_mask);
    // sigaddset(&sigtstp_act.sa_mask, SIGINT);
    // sigaddset(&sigtstp_act.sa_mask, SIGQUIT);
    sigtstp_act.sa_handler = handle_sigtstp;
    sigtstp_act.sa_flags = SA_RESTART;   
    sigaction(SIGTSTP, &sigtstp_act, NULL);
}


void record_fg_job (const char **toks, const pid_t pid) {
    FG_JOB.terminated = false;
    FG_JOB.pid = pid;
    FG_JOB.job_id = JOB_ID;
    memset(FG_JOB.cmd, '\0', sizeof(FG_JOB.cmd));
    strcpy(FG_JOB.cmd, toks[0]);
    memset(FG_JOB.status, '\0', sizeof(FG_JOB.status));
    strcpy(FG_JOB.status, "running");
    FG_JOB.suspended = false;
    JOB_ID++;
    return;
}

void record_job(const char **toks, const pid_t pid) {
    JOBS[JOB_ID].job_id = JOB_ID;
    JOBS[JOB_ID].pid = pid;
    memset(JOBS[JOB_ID].cmd, '\0', sizeof(JOBS[JOB_ID].cmd));   // init cmd
    strcpy(JOBS[JOB_ID].cmd, toks[0]);
    memset(JOBS[JOB_ID].status, '\0', sizeof(JOBS[JOB_ID].status));   // init status 
    strcpy(JOBS[JOB_ID].status, "running");
    JOBS[JOB_ID].terminated = false;
    JOB_ID++;

    return;
}

// returns process id corresponds to job id, return -1 if no corresponding pid
pid_t get_pid(int job_id) {
    for (int i = 1; i < JOB_ID; ++i) {
        if (JOBS[i].job_id == job_id) {
            return JOBS[i].pid;
        }
    }

    return -1;
}
// returns -1 if pid is not valid, return pid otherwise
pid_t valid_pid(const char **toks) {
    // format of pid = 1111
    long pid;
    pid_t invalid = -1;
    char* endptr;
    pid = strtol(toks[1], &endptr, 10);

    if (*endptr != '\0') {
        return invalid;
    }

    if (pid <= 0) {
        return invalid;
    }

    return (pid_t) pid;
}

// returns -1 if job id is not valid, return job id otherwise
int valid_jobid(const char** toks) {
    // job id format = %1111
    char* endptr;
    long jid;
    int invalid = -1;
    char* arg[sizeof(toks[1])];
    memset(arg, '\0', sizeof(arg));
    strcpy(arg, toks[1] + 1);

    jid = strtol(arg, &endptr, 10);

    if (*endptr != '\0') {
        // entire string was NOT converted
        return invalid;
    }

    if (jid <= 0) {
        return invalid;
    }

    return (int) jid;
}

bool no_job(int jid) {
    for (int i = 1; i < JOB_ID; ++i) {
        if (!JOBS[i].terminated) {
            if (JOBS[i].job_id == jid) {
                return false;
            }
        }
    }

    return true;
}

// send SIGTERM kill signal to process
// returns true if success, false if failed
bool kill_process (pid_t pid) {
    int result = kill(pid, SIGKILL);

    if (result == 0) {
        return true;
    } else {
        return false;
    }
}
  

void foreground_spawn(const char **toks) {
    int status;
    
    pid_t pid1 = fork();   // create copy of current process

    if (pid1== -1) {
        // failed to fork
        fprintf(stderr, "ERROR: failed to fork current process\n");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) {
        setpgid(0, 0);
        // this is child process
        execvp(toks[0], toks);
        // if failed to exec, process will reach this line (error)
        fprintf(stderr, "ERROR: cannot run %s\n", toks[0]); 
        exit(EXIT_FAILURE);
    } else {
        // this is parent(original) process
        // runs in foreground => wait until child to finish job
        record_fg_job(toks, pid1);
        pid_t pid2 = waitpid(pid1, &status, WUNTRACED);

        // if (WIFSTOPPED(status)) {
        //     FG_JOB.suspended = true;
        //     print_fg_suspended(pid1);
        //     fprintf(stderr, "in fg spawn WIFSTOPPED\n");
        // }

        remove_fg_job();
    } 
}

void background_spawn(const char** toks) {
    int status;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    pid_t pid1 = fork();   // create copy of current process

    if (pid1 == -1) {
        // failed to fork
        fprintf(stderr, "ERROR: failed to fork current process\n");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) {
        setpgid(0, 0);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        // this is child process
        execvp(toks[0], toks);
        // if failed to exec, process will reach this line (error)
        fprintf(stderr, "ERROR: cannot run %s\n", toks[0]); 
        exit(EXIT_FAILURE);
    } else {
        // this is parent(original) process
        // runs in background => don't wait child to finish
        record_job(toks, pid1);
        print_background_job(pid1);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        // pid_t pid2;
        // while(pid2 = waitpid(pid1, &status, WNOHANG) > 0) {
        //     sigprocmask(SIG_BLOCK, &mask, NULL);
        //     remove_job(pid1);
        //     sigprocmask(SIG_UNBLOCK, &mask, NULL);
        // }
    }
}

void spawn(const char **toks, bool bg) { // bg is true iff command ended with &
    // TODO
    if (bg) {
        // runs in background
        background_spawn(toks);
    } else {
        // runs in foreground
        foreground_spawn(toks);
    }
}

void cmd_jobs(const char **toks) {
    // TODO
    // no args for jobs command
    if (toks[1] != NULL) {
        fprintf(stderr, "ERROR: jobs takes no arguments\n");
    } else {
        for (int i = 1; i < JOB_ID ; ++i) {
            if (!JOBS[i].terminated && JOBS[i].job_id != 0) {
                printf("[%d] (%d)  %s  %s\n", JOBS[i].job_id, JOBS[i].pid, JOBS[i].status, JOBS[i].cmd);
                fflush(stdout);
            }
        }
    }
}

void wait_job(pid_t pid) {
    int status;
    int jid = get_pid(pid);
    kill(pid, SIGCONT);

    while(waitpid(pid, &status, WNOHANG | WUNTRACED) == 0 && !FG_JOB.suspended)  {
        nanosleep((const struct timespec[]){{0, 10000000}}, NULL);
    }

    
    // if (WIFSIGNALED(status)) {
    //     print_fg_terminated(pid);
    // }

    // if (WIFSTOPPED(status)) {
    //     print_fg_suspended(pid);
    // }
}

void cmd_fg(const char **toks) {
    // if fg cmd takes zero or more than one args => error
    if (toks[1] == NULL || toks[2] != NULL) {
        fprintf(stderr, "ERROR: fg takes exactly one arguments\n");
    } else {
        if (toks[1][0] == '%') {
            // arg is job id
            int jid = valid_jobid(toks);
            if (jid == -1) {
                fprintf(stderr, "ERROR: bad argument for fg: %s\n", toks[1]);
            } else if (no_job(jid)) {
                // not fg job id
                fprintf(stderr, "ERROR: no job %s\n", toks[1]);
            } else {
                pid_t p = get_pid(jid);
                record_bg_to_fg_job(p);
                remove_job(p);
                FG_JOB.terminated = false;
                FG_JOB.suspended = false;
                wait_job(p);
            }
        } else {
            // arg is pid
            pid_t pid = valid_pid(toks);

            if (pid == -1) {
                fprintf(stderr, "ERROR: bad argument for fg: %s\n", toks[1]);
            } else if (no_pid(pid)) {
                // not fg pid
                fprintf(stderr, "ERROR: no PID %s\n", toks[1]);
            } else {
                record_bg_to_fg_job(pid);
                remove_job(pid);
                FG_JOB.terminated = false;
                FG_JOB.suspended = false;
                wait_job(pid);
            }
        }
    }
}

void cmd_bg(const char **toks) {
    // if bg cmd takes zero or more than one args => error
    if (toks[0] == NULL || toks[2] != NULL) {
        fprintf(stderr, "ERROR: bg takes exactly one arguments\n");
    } else {
        if (toks[1][0] == '%') {
            // arg is job id
            int jid = valid_jobid(toks);
            if (jid == -1) {
                fprintf(stderr, "ERROR: bad argument for fg: %s\n", toks[1]);
            } else if (no_job(jid)) {
                // not fg job id
                fprintf(stderr, "ERROR: no job %s\n", toks[1]);
            } else {
                int p = get_pid(jid);
                kill(p, SIGCONT);
            }
        } else {
            // arg is pid
            pid_t pid = valid_pid(toks);

            if (pid == -1) {
                fprintf(stderr, "ERROR: bad argument for fg: %s\n", toks[1]);
            } else if (no_pid(pid)) {
                // not fg pid
                fprintf(stderr, "ERROR: no PID %s\n", toks[1]);
            } else {
                kill(pid, SIGCONT);
            }
        }
    }
}

void cmd_slay(const char **toks) {
    // if slay cmd takes zero or more than one args => error
    if (toks[0] == NULL || toks[2] != NULL) {
        fprintf(stderr, "ERROR: slay takes exactly one arguments\n");
        exit(EXIT_FAILURE);
    } else {
        // TODO: do actual slay cmd
        if (toks[1][0] == '%') {
            // arg is job id
            int jid = valid_jobid(toks);
            if (jid == -1) {
                fprintf(stderr, "ERROR: bad argument for slay: %s\n", toks[1]);
            } else if (no_job(jid)) {
                fprintf(stderr, "ERROR: no job %s\n", toks[1]);
            } else {
                // perform actual slay cmd
                if (!kill_process(get_pid(jid))) {
                    fprintf(stderr, "ERROR: no job %s\n", toks[1]);
                } 
            }
        } else {
            // arg is pid
            pid_t pid = valid_pid(toks);

            if (pid == -1) {
                fprintf(stderr, "ERROR: bad argument for slay: %s\n", toks[1]);
            } else {
                // perform actual slay cmd
                if (!kill_process(pid)) {
                    fprintf(stderr, "ERROR: no PID %s\n", toks[1]);
                }
            }
        }
    } 
}

void cmd_quit(const char **toks) {
    if (toks[1] != NULL) {
        fprintf(stderr, "ERROR: quit takes no arguments\n");
    } else {
        exit(0);
    }
}

void eval(const char **toks, bool bg) { // bg is true iff command ended with &
    assert(toks);

    if (*toks == NULL) return;
    if (strcmp(toks[0], "quit") == 0) {
        cmd_quit(toks);
    // TODO: other commands
    } else if (strcmp(toks[0], "jobs") == 0) {
        cmd_jobs(toks);    
    } else if (strcmp(toks[0], "slay") == 0) {
        cmd_slay(toks);
    } else if (strcmp(toks[0], "fg") == 0) {
        cmd_fg(toks);
    } else if (strcmp(toks[0], "bg") == 0) {
        cmd_bg(toks);
    } else {
        spawn(toks, bg);
    }
}

// you don't need to touch this unless you are submitting the bonus task
void parse_and_eval(char *s) {
    assert(s);
    const char *toks[MAXLINE+1];
    
    while (*s != '\0') {
        bool end = false;
        bool bg = false;
        int t = 0;

        while (*s != '\0' && !end) {
            while (*s == ' ' || *s == '\t' || *s == '\n') ++s;
            if (*s != '&' && *s != ';' && *s != '\0') toks[t++] = s;
            while (strchr("&; \t\n", *s) == NULL) ++s;
            switch (*s) {
            case '&':
                bg = true;
                end = true;
                break;
            case ';':
                end = true;
                break;
            }
            if (*s) *s++ = '\0';
        }
        toks[t] = NULL;
        eval(toks, bg);
    }
}

// you don't need to touch this unless you are submitting the bonus task
void prompt() {
    printf("junsUnix> ");
    fflush(stdout);
}

// you don't need to touch this unless you are submitting the bonus task
int repl() {
    char *line = NULL;
    size_t len = 0;
    while (prompt(), getline(&line, &len, stdin) != -1) {
        parse_and_eval(line);
    }

    if (line != NULL) free(line);
    if (ferror(stdin)) {
        perror("ERROR");
        return 1;
    }
    return 0;
}

// you don't need to touch this unless you want to add debugging options
int main(int argc, char **argv) {
    FG_JOB.terminated = true;
    install_signal_handlers();
    return repl();
}
