#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <string.h>

#define STACK_SIZE (1024 * 1024)

static char stack[STACK_SIZE];

typedef struct {
    char rootfs[256];
    char command[256];
} config_t;

int container_main(void *arg) {
    config_t *cfg = (config_t *)arg;

    printf("inside container\n");

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        // This becomes PID 1 inside container

        if (chroot(cfg->rootfs) != 0) {
            perror("chroot failed");
            return 1;
        }

        if (chdir("/") != 0) {
            perror("chdir failed");
            return 1;
        }

        if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
            perror("mount failed");
            return 1;
        }

        execl(cfg->command, cfg->command, NULL);

        perror("exec failed");
        return 1;
    }

    wait(NULL);
    return 0;
}

int main() {
    printf("starting container...\n");

    config_t cfg;
    strcpy(cfg.rootfs, "../rootfs-alpha");
    strcpy(cfg.command, "/bin/sh");

    int flags = CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD;

    int pid = clone(container_main, stack + STACK_SIZE, flags, &cfg);

    if (pid < 0) {
        perror("clone failed");
        return 1;
    }

    waitpid(pid, NULL, 0);

    printf("container exited\n");
    return 0;
}