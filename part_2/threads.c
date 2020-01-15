#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <zconf.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include "threads.h"


struct time_threads time_threads = {1000000, 1000000, 1000000, 1000000};
const char *format = "./threads -1\n"
                     "\t./threads -2\n"
                     "\t./threads -3 [TIME | TIME_MAIN TIME1 TIME2]\n"
                     "\t./threads -4 [TIME | TIME_MAIN TIME1 TIME2 TIME3]";

void check_errno(char *strerr) {
    if (errno) {
        fprintf(stderr, "%s. Error: %s\n", strerr, strerror(errno));
        exit(1);
    }
}

void print_array() {
    for (int i = 0; i < SIZE; ++i) {
        printf("%c ", array[i]);
    }
    printf("\n");
}

void parse_flag(int argc, char *argv[]) {
    unsigned int opt = 0;
    if (argc == 1) {
        fprintf(stderr,
                "No task nubmer.\nUse: \n\t.%s.\n", format);
        exit(1);
    }
    while ((opt = getopt(argc, argv, "1234")) != -1) {
        switch (opt) {
            case '1':
                if (argc > 2) {
                    fprintf(stderr, "Got more than 1 flag. Use: \n\t%s\n", format);
                    exit(1);
                }
                printf("First subtask.\n");
                first_task();
                break;
            case '2':
                if (argc > 2) {
                    fprintf(stderr, "Got more than 1 flag. Use: \n\t%s\n", format);
                    exit(1);
                }
                printf("Second subtask.\n");
                second_task();
                break;
            case '3':
                errno = 0;
                printf("Third subtask.\n");
                if (argc == optind) {
                    printf("Count of time was not set.\nTIME by default = %ld\n",
                           time_threads.time_main);
                } else if (optind + 1 == argc) {
                    char *p;
                    unsigned long tmp = (unsigned long) strtol(argv[optind], &p, 10);
                    if (errno || *p != '\0' || time < 0) {
                        fprintf(stderr, "Wrong format. Use: \n\t%s\n", format);
                        exit(1);
                    }
                    time_threads.time_main = tmp;
                    time_threads.time_change = tmp;
                    time_threads.time_reverse = tmp;
                } else if (optind + 3 == argc) {
                    char *p;
                    for (int i = 0; i < 3; ++i) {
                        *(&time_threads.time_main + i) = (unsigned long) strtol(argv[optind++], &p, 10);
                        if (errno || *p != '\0' || time < 0) {
                            fprintf(stderr,
                                    "Wrong format. Use: \n\t%s\n", format);
                            exit(1);
                        }
                    }
                } else {
                    fprintf(stderr,
                            "Wrong format. Use: \n\t%s\n", format);
                    exit(1);
                }
                third_task();
                break;
            case '4':
                printf("Fourth subtask:\n");
                if (argc == optind) {
                    printf("Count of time was not set.\n TIME by default = %ld\n",
                           time_threads.time_main);
                } else if (optind + 1 == argc) {
                    char *p;
                    unsigned long tmp = (unsigned long) strtol(argv[optind], &p, 10);
                    if (errno || *p != '\0' || time < 0) {
                        fprintf(stderr, "Wrong format. Use: \n\t%s\n", format);
                        exit(1);
                    }
                    for (int i = 0; i < 4; ++i) {
                        *(&time_threads.time_main + i) = tmp;
                    }
                } else if (optind + 4 == argc) {
                    char *p;
                    for (int i = 0; i < 4; ++i) {
                        *(&time_threads.time_main + i) = (unsigned long) strtol(argv[optind++], &p, 10);
                        if (errno || *p != '\0' || time < 0) {
                            fprintf(stderr,
                                    "Wrong format. Use: \n\t%s\n", format);
                            exit(1);
                        }
                    }
                } else {
                    fprintf(stderr,
                            "Wrong format. Use: \n\t.%s\n", format);
                    exit(1);
                }
                forth_task();
                break;
            default:
                fprintf(stderr, "Wrong key. Use: \n\t%s\n", format);
                exit(1);
        }
    }
}

void start(int argc, char *argv[]) {
    for (int i = 0; i < SIZE; ++i) {
        array[i] = i + 'a';
    }
    parse_flag(argc, argv);
}

sem_t sem;

void first_task() {
    errno = 0;
    pthread_t thread1, thread2;
    sem_init(&sem, 0, 1);
    check_errno("Cant run sem_init");

    pthread_create(&thread1, NULL, task1_thread1, NULL);
    pthread_create(&thread2, NULL, task1_thread2, NULL);
    check_errno("Cant run thread");

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
}

int sem_id;
struct sembuf *sembuf;

void second_task() {
    errno = 0;
    pthread_t thread1, thread2;
    sembuf = malloc(sizeof(struct sembuf));
    sembuf->sem_flg = 0;
    sembuf->sem_num = 0;
    sem_id = semget(IPC_PRIVATE, 2, IPC_CREAT | PERM);
    check_errno("Cant run semget");
    semctl(sem_id, 0, SETVAL, 1);

    pthread_create(&thread1, NULL, task2_thread1, NULL);
    pthread_create(&thread2, NULL, task2_thread2, NULL);
    check_errno("Cant run thread");

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    semctl(sem_id, 0, IPC_CREAT);
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void third_task() {
    errno = 0;
    pthread_t thread1, thread2;
    pthread_mutex_init(&mutex, NULL);
    check_errno("Cant initialize mutex");

    pthread_create(&thread1, NULL, task3_thread1, NULL);
    pthread_create(&thread2, NULL, task3_thread2, NULL);
    check_errno("Cant run a thread");

    while (true) {
        pthread_mutex_lock(&mutex);
        check_errno("Cant run pthread_mutex_lock");
        print_array();
        pthread_mutex_unlock(&mutex);
        check_errno("Cant run pthread_mutex_lock");
        usleep(time_threads.time_main);
    }
}

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

void forth_task() {
    errno = 0;
    pthread_t thread1, thread2, thread3;

    pthread_create(&thread1, NULL, task4_thread1, NULL);
    pthread_create(&thread2, NULL, task4_thread2, NULL);
    pthread_create(&thread3, NULL, task4_thread3, NULL);
    check_errno("Cant run a thread");

    while (true) {
        errno = 0;
        usleep(time_threads.time_main);
        pthread_rwlock_rdlock(&rwlock);
        check_errno("Cant block resource");
        print_array();
        pthread_rwlock_unlock(&rwlock);
        check_errno("Cant block resource");
    }
}

void change_reg() {
    for (int i = 0; i < SIZE; ++i) {
        if (array[i] >= 'A' && array[i] <= 'Z') {
            array[i] = array[i] - 'A' + 'a';
        } else {
            array[i] = array[i] - 'a' + 'A';
        }
    }
}

void reverse() {
    for (int i = 0; i < SIZE / 2; ++i) {
        char tmp = array[i];
        array[i] = array[SIZE - i - 1];
        array[SIZE - i - 1] = tmp;
    }
}

u_short count_letters() {
    int res = 0;
    for (int i = 0; i < SIZE; ++i) {
        if (array[i] >= 'A' && array[i] <= 'Z') {
            res++;
        }
    }
    return res;
}

void *task1_thread1() {
    while (true) {
        errno = 0;
        sem_wait(&sem);
        check_errno("Cant block resource (in sem_wait)");
        change_reg();
        print_array();
        sleep(1);
        sem_post(&sem);
        sem_post(&sem);
        check_errno("Cant block resource (in sem_post)");
        sleep(1);
    }
}

void *task1_thread2() {
    while (true) {
        sem_wait(&sem);
        check_errno("Cant block resource (in sem_wait)");
        reverse();
        print_array();
        sleep(1);
        sem_post(&sem);
        check_errno("Cant block resource (in sem_post)");
        sleep(1);
    }
}

void *task2_thread1() {
    while (true) {
        errno = 0;
        sembuf->sem_op = -1;
        semop(sem_id, sembuf, 1);
        check_errno("Cant block resource (in sembuf)");
        change_reg();
        print_array();
	sleep(1);
        sembuf->sem_op = 1;
        semop(sem_id, sembuf, 1);
        check_errno("Cant block resource (in sembuf)");
	sleep(1);
    }
}

void *task2_thread2() {
    while (true) {
        errno = 0;
        sembuf->sem_op = -1;
        semop(sem_id, sembuf, 1);
        check_errno("Cant block resource (in semop)");
        reverse();
        print_array();
	sleep(1);
        sembuf->sem_op = 1;
        semop(sem_id, sembuf, 1);
        check_errno("Cant block resource (in semop)");
	sleep(1);
    }
}

void *task3_thread1() {
    while (true) {
        pthread_mutex_lock(&mutex);
        check_errno("Cant block resource (in pthread_mutex_lock)");
        change_reg();
        pthread_mutex_unlock(&mutex);
        check_errno("Cant block resource (in  pthread_mutex_unlock)");
        usleep(time_threads.time_change);
    }
}

void *task3_thread2() {
    while (true) {
        pthread_mutex_lock(&mutex);
        check_errno("Cant block resource (pthread_mutex_lock)");
        reverse();
        pthread_mutex_unlock(&mutex);
        check_errno("Cant block resource (pthread_mutex_lock)");
        usleep(time_threads.time_reverse);
    }
}

void *task4_thread1() {
    while (true) {
        usleep(time_threads.time_change);
        pthread_rwlock_wrlock(&rwlock);
        check_errno("Cant block 1st thread");
        change_reg();
        pthread_rwlock_unlock(&rwlock);
        check_errno("Cant unblock 1st thread");
    }
}

void *task4_thread2() {
    while (true) {
        usleep(time_threads.time_reverse);
        pthread_rwlock_wrlock(&rwlock);
        check_errno("Cant unblock 2nd thread");
        reverse();
        pthread_rwlock_unlock(&rwlock);
        check_errno("Cant unblock 2nd thread");
    }
}

void *task4_thread3() {
    while (true) {
        usleep(time_threads.count_letter);
        pthread_rwlock_rdlock(&rwlock);
        check_errno("Cant block the 3rd thread");
        printf("Count of uppercase symbols: %d\n", count_letters());
        pthread_rwlock_unlock(&rwlock);
        check_errno("Cant unblock 3rd thread");
    }
}

