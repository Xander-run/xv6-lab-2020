//
// Created by xabi on 2/14/21.
//
#include "kernel/types.h"
#include "user/user.h"

#define BUFFER_SIZE 64

int main(int argc, char **argv) {
    if (argc != 1) {
        fprintf(2, "Usage: primes\n");
    }
    int terminated = 0;
    int fd[2];
    int cnt = 0;
    int numbers[BUFFER_SIZE];
    for (int i = 2; i <= 35; i++) {
        numbers[cnt++] = i;
    }
    int sel_index = 0;
    while(!terminated) {
        pipe(fd);
        if (fork() == 0) {
            // child
            close(fd[1]);
            read(fd[0], numbers, sizeof(numbers[0]) * cnt);
            close(fd[0]);
            int sel = numbers[sel_index];
            fprintf(1, "prime %d\n", sel);
            int new_cnt = cnt;
            for(int left = sel_index + 1, right = sel_index + 1; right < cnt; right++) {
                if ((numbers[right] % sel) == 0) {
                    new_cnt--;
                } else {
                    numbers[left] = numbers[right];
                    left++;
                }
            }
            cnt = new_cnt;
            sel_index++;
            if (sel_index >= cnt) {
                terminated = 1;
            }
        } else {
            // parent
            close(fd[0]);
            write(fd[1], numbers, sizeof(numbers[0]) * cnt);
            close(fd[1]);
            terminated = 1;
            wait(0); // wait child terminated and return to shell
        }
    }
    exit(0);
}

