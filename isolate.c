#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>

#include <stdio.h>

#define LOG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)

static const uid_t CHILD_UID = 1000;

struct child_args {
    int fds[2];
    char **argv;
};

void cat_proc_file(pid_t pid, const char *file)
{
    char command[64];
    snprintf(command, sizeof(command), "cat /proc/%d/%s", pid, file);
    system(command);
}

int map_write(pid_t pid, const char *map, int from, int to, int range)
{
    char mapfile[64];
    snprintf(mapfile, sizeof(mapfile), "/proc/%d/%s", pid, map);

    FILE *fp = fopen(mapfile, "w");
    if (!fp)
    {
        perror("fopen");
        return 1;
    }

    if (fprintf(fp, "%d %d %d\n", from, to, range) < 0)
    {
        perror("fprintf");
        return 1;
    }

    fclose(fp);
    return 0;
}

int map_ids(pid_t child_pid, uid_t child_uid)
{
    static const int ROOT = 0;
    static const int RANGE = 1000;

    if (map_write(child_pid, "uid_map", ROOT, child_uid, RANGE))
    {
        return 1;
    }

    if (map_write(child_pid, "gid_map", ROOT, child_uid, RANGE))
    {
        return 1;
    }

    return 0;
}

int signal_child(int pipe)
{
    char x = 'x';
    int rv = write(pipe, &x, sizeof(x));
    close(pipe);

    return (rv == sizeof(x)) ? 0 : 1;
}

int wait_for_parent_signal(int pipe)
{
    char x;
    int rv = read(pipe, &x, sizeof(x));
    close(pipe);

    return (rv == sizeof(x)) ? 0 : 1;
}

int set_mntns()
{
    return system("mount -t proc proc /proc");
}

int set_utsns()
{
    static const char *HOSTNAME = "Isolated";
    return sethostname(HOSTNAME, strlen(HOSTNAME));
}

int child_set_netns()
{
    return system("ip link set lo up") ||
           system("ip link set peer0 up") ||
           system("ip addr add 10.11.12.14/24 dev peer0");
}

int child(void *args)
{
    struct child_args *child_args = (struct child_args *)args;

    close(child_args->fds[1]);
    wait_for_parent_signal(child_args->fds[0]);

    if (set_mntns())
    {
        LOG("Child failed to set mount namespace");
        return 1;
    }

    if (set_utsns())
    {
        LOG("Child failed to set uts namespace");
        return 1;
    }

    if (child_set_netns())
    {
        LOG("Child failed to set net namespace");
        return 1;
    }

    setuid(CHILD_UID);

    execv(child_args->argv[0], child_args->argv);
    perror("execve");
    return 1;
}

int set_userns(pid_t pid)
{
    return map_ids(pid, 0);
}

int set_netns(pid_t pid)
{
    char command[128];
    snprintf(command, sizeof(command), "ip link set peer0 netns /proc/%d/ns/net", pid);

    return system("ip link add veth0 type veth peer name peer0") ||
           system("ip link set veth0 up") ||
           system("ip addr add 10.11.12.13/24 dev veth0") ||
           system(command);
}

int main(int argc, char **argv)
{
    static const size_t STACK_SIZE = (1024*1024);
    char child_stack[STACK_SIZE];

    struct child_args child_args;
    child_args.argv = &argv[1];

    if (argc < 2)
    {
        LOG("Usage: %s PROGRAM [ARGS...]", argv[0]);
        return 1;
    }

    if (pipe(child_args.fds))
    {
        perror("pipe");
        return 1;
    }

    int flags = SIGCHLD | \
                CLONE_NEWUSER | CLONE_NEWNET | CLONE_NEWUTS | \
                CLONE_NEWIPC| CLONE_NEWNS | CLONE_NEWPID;
    pid_t pid = clone(child, child_stack + sizeof(child_stack), flags, &child_args);
    if (pid == -1)
    {
        perror("clone");
        return 1;
    }

    if (set_userns(pid))
    {
        LOG("Parent failed to set user namespace");
        return 1;
    }

    if (set_netns(pid))
    {
        LOG("Parent failed to set network namespace");
        return 1;
    }

    close(child_args.fds[0]);
    if (signal_child(child_args.fds[1]))
    {
        LOG("Parent couldn't signal child");
        return 1;
    }

    int status;
    if (waitpid(pid, &status, 0) != pid)
    {
        perror("waitpid");
        return 1;
    }

    if (!WIFEXITED(status))
    {
        LOG("Child didn't terminate gracefully");
        return 1;
    }

    return WEXITSTATUS(status);
}
