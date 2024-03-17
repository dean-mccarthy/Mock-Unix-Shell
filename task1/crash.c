#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#define MAXLINE 1024
void quit(const char **toks) {
    if (toks[1] != NULL) {
            const char *msg = "ERROR: quit takes no arguments\n";
            write(STDERR_FILENO, msg, strlen(msg));
    } else {
            exit(0);
    }
}

void jobs() {
    //list jobs;
}

void nuke(const char **toks) {
    if (toks[1] == NULL) {
        //KILL all
    } else {
        int i = 1;
        while (toks[i] != NULL)
        {
            char *process = toks[1];
            if (process[0] == '%')
            {
                //kill immediately
            } else {
                //KILL process iff shell has not exited
            }
            i++;
        }
    }
}

void foreground(const char **toks) {
    if (toks[1] == NULL) {
        const char *msg = "ERROR: fg requires at least one argument\n";
        write(STDERR_FILENO, msg, strlen(msg));
    } else {
        char *process = toks[1];
        if (process[0] == '%')
        {
            //puts job %n in foreground
        } else {
            //puts process n in foreground
        }
    }
}

void background(const char **toks) {
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

void runProcess(const char **toks, bool bg) {
    char *process = toks[0];
    int i = 1;
    char *args[] = {};
    while (toks[i] != NULL)
    {
        //do something with arg
        args[i-1] = toks[i];
        i++;
    }  
    if (bg)
    {
        //run process in bg
    } else {
        execvp(toks[0], args);
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

int main(int argc, char **argv) {
    return repl();
}
