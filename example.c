#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "dpopen.h"

#define MAXLINE 80

int main()
{
    char line[MAXLINE];
    FILE *fp;

    signal(SIGPIPE, SIG_IGN);
    fp = dpopen("sort");
    if (fp == NULL) {
        perror("dpopen error");
        exit(1);
    }

    fprintf(fp, "orange\n");
    fprintf(fp, "apple\n");
    fprintf(fp, "pear\n");

    if (dphalfclose(fp) < 0) {
        perror("dphalfclose error");
        exit(1);
    }

    for (;;) {
        if (fgets(line, MAXLINE, fp) == NULL) {
            break;
        }
        fputs(line, stdout);
    }

    int status = dpclose(fp);
    if (status < 0) {
        perror("dpclose error");
        exit(1);
    }
    if (status != 0) {
        if (WIFEXITED(status)) {
            fprintf(stderr, "Command exited with status %d\n",
                    WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "Command terminated by signal %d\n",
                    WTERMSIG(status));
        } else {
            fprintf(stderr, "Command stopped with signal %d\n",
                    WSTOPSIG(status));
        }
    }
    return 0;
}
