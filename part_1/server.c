#include <stdio.h>
#include <zconf.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/file.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/loadavg.h> 
#include <sys/ipc.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <signal.h>
#include <sys/file.h>
#include "server.h"

void check_errno(char *strerr) {
    if (errno) {
        fprintf(stderr, "%s. Error: %s\n", strerr, strerror(errno));
        exit(1);
    }
}

void set_ids(struct server_param *server_param) {
    time(&server_param->start_time);
    server_param->pid = getpid();
    server_param->uid = getuid();
    server_param->gid = getgid();
}

int mem_id;
struct server_param *shared_memory_param() {
    errno = 0;
    server_signal();
    mem_id = shmget(IPC_PRIVATE, sizeof(struct server_param), IPC_CREAT | PERM);
    if (mem_id < 0) {
        fprintf(stderr, "Error: shmget.\n");
        exit(1);
    }

    struct server_param *server_param = (struct server_param *) shmat(mem_id, NULL, 0);
    if (server_param == NULL) {
        fprintf(stderr, "Error: shmat.\n");
        exit(1);
    }

    check_errno("Cant create memory segment");

    set_ids(server_param);

    printf("Server works.\npid = %ld, uid = %ld, gid = %ld\n", server_param->pid, server_param->uid,
           server_param->gid);
    printf("Client-Server works via SHARED_MEMORY, mem_id = %d\n", mem_id);
    return server_param;
}

struct server_param *message_queue_param(int *mem_id_q) {
    errno = 0;
    *mem_id_q = msgget(IPC_PRIVATE, IPC_CREAT | PERM);

    check_errno("Cant create the queue of messages");

    struct server_param *server_param = malloc(sizeof(struct server_param));

    set_ids(server_param);

    printf("Server works.\npid = %ld, uid = %ld, gid = %ld\n", server_param->pid, server_param->uid,
           server_param->gid);
    printf("Client-Server works via Message Queue, mem_id = %d\n", *mem_id_q);
    return server_param;
}

int file;
struct server_param *mmap_file(char *filename) {
    errno = 0;
    file = open(filename, O_CREAT | O_RDWR, PERM);
    server_signal();
    check_errno("Cant create/open the file");
    
     struct flock lock;
     lock.l_type = F_WRLCK;
     lock.l_whence = SEEK_SET;
     lock.l_start = 0;
     lock.l_len = 0;
     fcntl(file, F_SETLK, &lock);
		          
    
    ftruncate(file, sizeof(struct server_param));
    check_errno("Cant open the file");


    struct server_param *server_param = (struct server_param *) mmap(NULL, sizeof(struct server_param), PROT_WRITE,
                                                                     MAP_SHARED, file, 0);
    check_errno("Cant print the file");

    set_ids(server_param);
    printf("Server works.\npid = %ld, uid = %ld, gid = %ld\n", server_param->pid, server_param->uid,
           server_param->gid);
    printf("Using file, filename = \"%s\"\n", filename);
    return server_param;
}

void start_server(int argc, char *argv[]) {
    char* filename = malloc(256 * sizeof(char));
    flag = parse_flag(argc, argv, filename);
    int mem_id = 0;
    struct server_param *server_param;
    if (flag & SHARED_MEMORY) {
        server_param = shared_memory_param();
    } else if (flag & MESSAGE_QUEUE) {
        server_param = message_queue_param(&mem_id);
    } else if (flag & MMAP_FILE) {
        server_param = mmap_file(filename);
    }
    while (true) {
        sleep(1);
        set_param(server_param);
        if (flag & MESSAGE_QUEUE) {
            struct msgbuff msg;
            errno = 0;
            msgrcv(mem_id, &msg, 0, MSGTYPE_QUERY, 0);
            check_errno("Cant create a message");

            msg.mtype = MSGTYPE_REPLY;
            memcpy(msg.mtext, server_param, sizeof(struct server_param));
            msgsnd(mem_id, &msg, sizeof(struct server_param), 0);
            check_errno("Cant write a message");
        }
        printf("work_time = %ld, 1min = %.2f, 5min = %.2f, 15min = %.2f\n", server_param->work_time,
               server_param->loadavg[0],
               server_param->loadavg[1], server_param->loadavg[2]);
    }
}

void set_param(struct server_param *server_param) {
    time_t now;
    time(&now);
    server_param->work_time = now - server_param->start_time;
    getloadavg(server_param->loadavg, 3);
}

unsigned int parse_flag(int argc, char *argv[], char* filename) {
    unsigned int flag = 0, opt = 0;
    while ((opt = getopt(argc, argv, "sqf")) != -1) {
        switch (opt) {
            case 's':
                flag |= SHARED_MEMORY;
                if (flag ^ SHARED_MEMORY) {
                    fprintf(stderr, "Got more than 1 flag.\n");
                    exit(1);
                }
                printf("Client-Server works via Shared Memory.\n");
                break;
            case 'q':
                flag |= MESSAGE_QUEUE;
                if (flag ^ MESSAGE_QUEUE) {
                    fprintf(stderr, "Got more than 1 flag.\n");
                    exit(1);
                }
                printf("Client-Server works via System V message queue.\n");
                break;
            case 'f':
                flag |= MMAP_FILE;
                if (flag ^ MMAP_FILE) {
                    fprintf(stderr, "Got more than 1 flag.\n");
                    exit(1);
                }
                if (optind == argc) {
                    fprintf(stderr, "No filename.\n");
                    exit(1);
                }
                strcpy(filename, argv[optind]);
                optind++;
                printf("Client-Server works via mmap.\n");
                break;
            default:
                fprintf(stderr, "Use: \n\t./server -s|-q|-f filename.\n");
                exit(1);
        }
    }
    if (!flag) {
        fprintf(stderr,
                "\nUse: \n\t./server -s|-q|-f filename.\n");
        exit(1);
    }
    return flag;
}


void *die() {
    printf("Destroyed.\n");
    shmdt((const void *) mem_id);
    lockf(file, F_ULOCK, 0);
    exit(0);
}

void set_signal(int sig, void *func) {
    struct sigaction action;
    sigset_t *mask = (sigset_t*) malloc(sizeof(sigset_t));
    action.sa_handler = func;
    action.sa_flags = 0;
    action.sa_mask = *mask;
    sigemptyset(mask);
    sigaction(sig, &action, NULL);
    check_errno("Cant create segaction");
}

void server_signal(){
    errno = 0;
    set_signal(SIGINT, die);
}
