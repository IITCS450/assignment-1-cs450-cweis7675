#include "common.h"
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void usage(const char *a) { 
    fprintf(stderr, "Usage: %s <cmd> [args]\n", a); 
    exit(1); 
}

static double d(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

int main(int c, char** v) {
    if (c < 2) usage(v[0]);

    struct timespec start, end;
    pid_t pid;
    int status;

    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) DIE("clock_gettime start");

    pid = fork();
    if (pid < 0) {
        DIE("fork");
    } else if (pid == 0) {
        // Child process: execute the command
        // v[1] is the command, &v[1] is the array of command + args
        execvp(v[1], &v[1]);
        DIE("execvp failed");
    } else {
        // Parent process: wait for child
        if (waitpid(pid, &status, 0) == -1) DIE("waitpid");

        if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) DIE("clock_gettime end");

        int exit_val = WIFEXITED(status) ? WEXITSTATUS(status) : WTERMSIG(status);

        printf("pid=%d elapsed=%.3f exit=%d\n", pid, d(start, end), exit_val);
    }

    return 0;
}