    define STACK_SIZE (1024*1024)
    char stack_child[STACK_SIZE];

    int create_namespaces()
    {
        int fds[2];
        int flags;
        pid_t pid;

        pipe(fds);
        
        // recieve signal on child process termination
        flags = SIGCHLD | \
                CLONE_NEWUSER | CLONE_NEWNET | CLONE_NEWUTS | \
                CLONE_NEWIPC| CLONE_NEWNS | CLONE_NEWPID;

        // the child stack growns downwards 
        pid = clone(child_func, stack_child + STACK_SIZE, flags, fds);
        if (pid == -1) {
            fprintf(stderr,"clone: %s", strerror(errno));
            exit(1);
        }
        
        setup_userns(pid);
        setup_netns(pid);

        write(c->fd[1], &pid, sizeof(int));
        close(c->fd[1]);
        waitpid(pid, NULL, 0);
    }

    void int child_func(void *args)
    {
        int fds[2] = args;
        pid_t pid;

        read(fds[0], &pid, sizeof(int));
        close(fds[0]);

        setup_mntns();
        setup_utsns();

        write(c->fd[1], &pid, sizeof(int));
        close(c->fd[1]);
        waitpid(pid, NULL, 0);
    }

    void int child_func(void *args)
    {
        int fds[2] = args;
        pid_t pid;

        read(fds[0], &pid, sizeof(int));
        close(fds[0]);

        execvp(...);
    }
