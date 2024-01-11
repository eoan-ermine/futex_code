#include <errno.h>
#include <linux/futex.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

void wait_on_futex_value(int *futex_addr, int val) {
    while (1) {
        int futex_rc =
            syscall(SYS_futex, futex_addr, FUTEX_WAIT, val, NULL, NULL, 0);
        if (futex_rc == -1) {
            if (errno != EAGAIN) {
                perror("futex");
                exit(1);
            }
        } else if (futex_rc == 0) {
            if (*futex_addr == val) {
                return;
            }
        } else {
            abort();
        }
    }
}

void wake_futex_blocking(int *futex_addr) {
    while (1) {
        int futex_rc =
            syscall(SYS_futex, futex_addr, FUTEX_WAKE, 1, NULL, NULL, 0);
        if (futex_rc == -1) {
            perror("futex wake");
            exit(1);
        } else if (futex_rc > 0) {
            return;
        }
    }
}

int main(int argc, char **argv) {
    int shm_id = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    int *shared_data = (int*)shmat(shm_id, NULL, 0);
    *shared_data = 0;

    int forkstatus = fork();
    if (forkstatus < 0) {
        perror("fork");
        exit(1);
    }

    if (forkstatus == 0) {
        // Дочерний процесс

        printf("child waiting for A\n");
        wait_on_futex_value(shared_data, 0xA);
        /* Таймаут на 500 мс
        printf("child waiting for A\n");
        struct timespec timeout = {.tv_sec = 0, .tv_nsec = 500000000};
        while (1) {
            unsigned long long t1 = time_ns();
            int futex_rc = futex(shared_data, FUTEX_WAIT, 0xA, &timeout, NULL,
        0); printf("child woken up rc=%d errno=%s, elapsed=%llu\n", futex_rc,
                    futex_rc ? strerror(errno) : "", time_ns() - t1);
            if (futex_rc == 0 && *shared_data == 0xA) {
                break;
            }
        }
        */

        printf("child writing B\n");
        // Записываем 0xB в разделяемый слот памяти
        // и ждём ответа родителя
        *shared_data = 0xB;
        wake_futex_blocking(shared_data);
    } else {
        // Родительский процесс

        printf("parent writing A\n");
        // Записываем 0xA в разделяемый слот памяти
        // и ждём ответа ребёнка
        *shared_data = 0xA;
        wake_futex_blocking(shared_data);

        printf("parent waiting for B\n");
        wait_on_futex_value(shared_data, 0xB);

        // Wait for the child to terminate
        wait(NULL);
        shmdt(shared_data);
    }
}