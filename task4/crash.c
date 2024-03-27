#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#define MAXLINE 1024
#define MAXJOBS 65536
#define KILLED 2
#define RUNNING 1
#define FINISHED 0
#define SUSPENDED -1

typedef struct {
    pid_t PID;
    int jobNum;
    int status; //1:running, 0:finished, -1:suspended 2:killed
    const char *name;
    bool valid;
} job;

static int currJob = 1;
static job *jobList[MAXJOBS];

static int getJobNum(int PID) {
    for (size_t i = currJob; i > 0; i--)
    {
        pid_t currPID;
        job *jobSearch = jobList[i];
        if (jobSearch) currPID = jobSearch->PID;
        if (jobSearch && jobSearch->PID == PID) {
            return jobSearch->jobNum;
        }
    }
    return -1;
    
}

static void quit(const char **toks) {
    if (toks[1] != NULL) {
            const char *msg = "ERROR: quit takes no arguments\n";
            write(STDERR_FILENO, msg, strlen(msg));
    } else {
            exit(0);
    }
}

static void printJob(int i) {
    job *currJob = jobList[i];
        const char *status;
        switch(currJob->status) {
            case 1:
                status = "running";
                break;
            case 0:
                status = "finished";
                break;
            case -1:
                status = "suspended";
                break;
            case 2:
                status = "killed";
                break;    
            default:
                status = NULL;
        }
        const char* msg = (char*)malloc(MAXLINE);
        const char *name = currJob->name;
        snprintf(msg, MAXLINE, "[%d] (%d)  %s  %s\n", i, currJob->PID, status, currJob->name);
        write(STDOUT_FILENO, msg, strlen(msg));

}

static void jobs() {
    for (size_t i = 1; i < currJob; i++)
    {
        if (jobList[i]->valid)
        {
            printJob(i);
        }
    }
}

static void nuke(const char **toks) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    if (toks[1] == NULL) {
        //KILL all
        for (size_t i = 1; i < currJob; i++)
        {
            job *killJob = jobList[i];
            if (killJob && killJob->status == 1)
            {
                killJob->status = 2;
                pid_t killJobPID = killJob->PID; 
                kill(killJobPID, SIGKILL);
            }
            
        }
        
    } else {
        int i = 1;
        while (toks[i] != NULL)
        {
            char * process = toks[i];
            if (process[0] == '%')
            {
                int jobNumKill = 0;
                int numReader = 1;
                while (process[numReader] != '\0')
                {
                    jobNumKill = jobNumKill*10 + (process[numReader] - '0');
                    numReader++;
                }
                
                job * killJob = jobList[jobNumKill];
                if (!killJob || !killJob->valid) {
                    const char* msg = (char*)malloc(MAXLINE);
                    snprintf(msg, MAXLINE,"ERROR: no job %d\n", jobNumKill);
                    write(STDOUT_FILENO, msg, strlen(msg));
                }
                if (killJob && killJob->status == 1)
                {
                    killJob->status = 2;
                    pid_t curPID = killJob->PID;
                    kill(killJob->PID, SIGKILL);
                }
            } else {
                //KILL process iff shell has not exited
                pid_t currPID = strtol(process,NULL,0);
                int jobNumKill = getJobNum(currPID);
                if (jobNumKill != -1)
                {
                    job * killJob = jobList[jobNumKill];
                    if (!killJob || !killJob->valid) {
                        const char* msg = (char*)malloc(MAXLINE);
                        snprintf(msg, MAXLINE,"ERROR: no job %d\n", currPID);
                        write(STDOUT_FILENO, msg, strlen(msg));
                    }
                    if (killJob && killJob->status == 1)
                    {
                        killJob->status = 2;
                        pid_t curPID = killJob->PID;
                        kill(killJob->PID, SIGKILL);
                    }
                } else {
                    const char* msg = (char*)malloc(MAXLINE);
                    snprintf(msg, MAXLINE,"ERROR: no job %d\n", currPID);
                    write(STDOUT_FILENO, msg, strlen(msg));
                }
                
            }
            i++;
        }
    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

static void foreground(const char **toks) {
    if (toks[1] == NULL) {
        const char *msg = "ERROR: fg requires at least one argument\n";
        write(STDERR_FILENO, msg, strlen(msg));
    } else {
        bool error = false;
        char *process = toks[1];
        pid_t fgJob = 0;
        if (process[0] == '%')
        {
            int jobNumFG = 0;
            int numReader = 1;
            while (process[numReader] != '\0')
            {
                jobNumFG = jobNumFG*10 + (process[numReader] - '0');
                numReader++;
            }
            job *pushJob = jobList[jobNumFG];
            if (!pushJob || !pushJob->valid) {
                const char* msg = (char*)malloc(MAXLINE);
                snprintf(msg, MAXLINE,"ERROR: no job %d\n", jobNumFG);
                write(STDOUT_FILENO, msg, strlen(msg));
                error = true;
            } else {fgJob = pushJob->PID;}

        } else {
            fgJob = strtol(toks[1],NULL,0);
            int jobNum = getJobNum(fgJob);
            job *pushJob = jobList[jobNum];
            if (!pushJob || !pushJob->valid) {
                const char* msg = (char*)malloc(MAXLINE);
                snprintf(msg, MAXLINE,"ERROR: no job %d\n", fgJob);
                write(STDOUT_FILENO, msg, strlen(msg));
                error = true;
            }
        }
        if (!error)
        {
            pid_t pidOut;
            int status;
            
            waitpid(fgJob, &status, 0);
            int jobNum = getJobNum(fgJob);
            job *deadJob = jobList[jobNum];
            deadJob->valid = false;
            if (deadJob->status != KILLED) deadJob->status = FINISHED;
            printJob(jobNum);
        }
        
    }
}

static void background(const char **toks) {
    if (toks[1] == NULL) {
        const char *msg = "ERROR: bg requires at least one argument\n";
        write(STDERR_FILENO, msg, strlen(msg));
    } else {
        int i = 1;
        while (toks[i] != NULL)
        {
            char *process = toks[1];
            if (process[0] == '%')
            {
                //send job %n to background
            } else {
                //send job n to background
            }
            i++;
        }
    }
}

static void runProcess(const char **toks, bool bg) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);    
    sigprocmask(SIG_BLOCK, &mask, NULL);
    char *process = toks[0];
    int i = 0;
    char *args[MAXLINE] = {};
    while (toks[i] != NULL)
    {
        args[i] = toks[i];
        i++;
    }  
    args[i] = NULL;
    pid_t child = fork();
    if (child == 0) {
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        int run = execvp(toks[0], args);
        if (run == -1)
        {
            const char* msg = (char*)malloc(MAXLINE);
            snprintf(msg, MAXLINE,"ERROR: cannot run %s\n", strdup(process));
            write(STDOUT_FILENO, msg, strlen(msg));
            exit(EXIT_FAILURE);
        }
    } else {

        job *childJob = malloc(sizeof(job));
        childJob->PID = child;
        childJob->jobNum = currJob++;
        childJob->status = RUNNING;
        childJob->name = strdup(process);
        childJob->valid = true;
        jobList[currJob - 1] = childJob;
        if (!bg) { //if foreground, wait for death
            pid_t pidOut;
            int status;
            
            pidOut = waitpid(child, &status, 0);
            int jobNum = getJobNum(pidOut);
            job *deadJob = jobList[jobNum];
            deadJob->valid = false;
            if (deadJob->status != KILLED) deadJob->status = FINISHED;
            printJob(jobNum);
        } else {
            printJob(currJob - 1);
        }
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
    } 
}

void eval(const char **toks, bool bg) { // bg is true iff command ended with &
    assert(toks);
    if (*toks == NULL) return;
    if (strcmp(toks[0], "quit") == 0) {
        quit(toks);
    } else if (strcmp(toks[0], "jobs") == 0) {
        jobs();
    } else if (strcmp(toks[0], "nuke") == 0) {
        nuke(toks);
    } else if (strcmp(toks[0], "fg") == 0) {
        foreground(toks);
    } else if (strcmp(toks[0], "bg") == 0) {
        background(toks);
    } else {
        runProcess(toks, bg);
    }
}

void parse_and_eval(char *s) {
    assert(s);
    const char *toks[MAXLINE+1];
    
    while (*s != '\0') {
        bool end = false;
        bool bg = false;
        int t = 0;

        while (*s != '\0' && !end) {
            while (*s == '\n' || *s == '\t' || *s == ' ') ++s;
            if (*s != ';' && *s != '&' && *s != '\0') toks[t++] = s;
            while (strchr("&;\n\t ", *s) == NULL) ++s;
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

void prompt() {
    const char *prompt = "crash> ";
    ssize_t nbytes = write(STDOUT_FILENO, prompt, strlen(prompt));
}

int repl() {
    char *buf = NULL;
    size_t len = 0;
    while (prompt(), getline(&buf, &len, stdin) != -1) {
        parse_and_eval(buf);
    }

    if (buf != NULL) free(buf);
    if (ferror(stdin)) {
        perror("ERROR");
        return 1;
    }
    return 0;
}

static void handler(int num) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    
    pid_t pidOut;
    int status;

    while ((pidOut = waitpid(-1, &status, WNOHANG)) > 0) {
        int jobNum = getJobNum(pidOut);
        job *deadJob = jobList[jobNum];
        deadJob->valid = false;
        if (deadJob->status != KILLED) deadJob->status = FINISHED;
        printJob(jobNum);
    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int main(int argc, char **argv) {
    struct sigaction sigact;
    sigact.sa_handler = handler;
    sigact.sa_flags = SA_RESTART;
    sigemptyset(&sigact.sa_mask);
    sigaction(SIGCHLD, &sigact, NULL);
    
    return repl();
} 
