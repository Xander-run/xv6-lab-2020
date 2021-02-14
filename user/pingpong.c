//
// Created by xabi on 2/14/21.
//
#include "kernel/types.h"
#include "user/user.h"

#define BUFFER_SIZE 16

int main (int argc, char* argv[]) {
    if (argc != 1) {
        fprintf(2, "Usage: pingpong..\n");
        exit(1);
    }
    int parent_write_pipe[2];
    int child_write_pipe[2];
    pipe(parent_write_pipe);
    pipe(child_write_pipe);

    if (fork() == 0) {
        // child
        char buffer[BUFFER_SIZE];
        close(parent_write_pipe[1]);
        close(child_write_pipe[0]);
        read(parent_write_pipe[0], buffer, BUFFER_SIZE);
        fprintf(1, "%d: received %s\n", getpid(), buffer);
        strcpy(buffer, "pong");
        write(child_write_pipe[1], buffer, BUFFER_SIZE);
        close(child_write_pipe[1]);
    } else {
        // parent
        char buffer[BUFFER_SIZE];
        close(child_write_pipe[1]);
        close(parent_write_pipe[0]);
        strcpy(buffer, "ping");
        write(parent_write_pipe[1], buffer, BUFFER_SIZE);
        close(parent_write_pipe[1]);
        read(child_write_pipe[0], buffer, BUFFER_SIZE);
        fprintf(1, "%d: received %s\n", getpid(), buffer);
    }

    exit(0);
}
