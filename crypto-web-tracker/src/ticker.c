#include <stdlib.h>
#include <signal.h>
#include "ticker.h"
#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "helpers.h"
#include "debug.h"

WATCHER *head=NULL;

WATCHER* get_watcher(int id){

    WATCHER *ptr = head;

    while (ptr != NULL) {
        if (ptr->watcherID == id) {
            return ptr;
        }
        ptr = ptr->next;
    }
    return NULL;
}


void tick_watchers(WATCHER *cli_ptr){

    WATCHER *ptr = head;
    while (ptr != NULL) {
    WATCHER_TYPE *type = &watcher_types[ptr->type];
    if(ptr==head){fprintf(stdout, "%d\t%s(%d,%d,%d)\n", ptr->watcherID, type->name, -1, (int)ptr->fdin, (int)ptr->fdout);}
     else{  fprintf(stdout, "%d\t%s(%d,%d,%d) ", ptr->watcherID, type->name, (int)ptr->pid, (int)ptr->fdin, (int)ptr->fdout);}
        fflush(stdout);
            int count = 0;
            if(type->argv != NULL){
                while (type->argv[count] != NULL) {
                    fprintf(stdout, "%s ", type->argv[count]);
                    fflush(stdout);
                    count++;
                }
            fprintf(stdout, "[");
            fflush(stdout);
            count = 0;
            while (ptr->args[count] != NULL) {
                if(ptr->args[count+1]==NULL){
                fprintf(stdout, "%s", ptr->args[count]);
                fflush(stdout);
                break;}
                else{
                    fprintf(stdout, "%s ", ptr->args[count]);
                }
                fflush(stdout);
                count++;
            }
            if(ptr->next==NULL){
                fprintf(stdout, "]");
            fflush(stdout);
            }
            else{
            fprintf(stdout, "]\n");
            fflush(stdout);}
            }
        ptr=ptr->next;
    }
    watcher_types[head->type].send(head,"");
}

void sigint_handler(int sig){
    sigset_t mask_set;
    sigemptyset(&mask_set);
    sigaddset(&mask_set, SIGCHLD);
    sigaddset(&mask_set, SIGIO);
    sigprocmask(SIG_BLOCK, &mask_set, NULL);

  exit(0);

    sigprocmask(SIG_UNBLOCK, &mask_set, NULL);
}


void sigchld_handler(int sig, siginfo_t *info, void *ucontext){
    sigset_t mask_set;
    sigemptyset(&mask_set);
    sigaddset(&mask_set, SIGIO);
    sigprocmask(SIG_BLOCK, &mask_set, NULL);
    
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        WATCHER *wp;
        for (wp = head; wp != NULL; wp = wp->next) {
            if (wp->pid == pid) {
                if (wp->prev == NULL) {
                    head = wp->next;
                } else {
                    wp->prev->next = wp->next;
                }
                if (wp->next != NULL) {
                    wp->next->prev = wp->prev;
                }
                free(wp->args);
                free(wp->buf);
                free(wp);
                break;
            }
        }
    }

    sigprocmask(SIG_UNBLOCK, &mask_set, NULL);
}

void sigio_handler(int sig, siginfo_t *info, void *ucontext) {
    sigset_t mask_set;
    sigemptyset(&mask_set);
    sigaddset(&mask_set, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask_set, NULL);

    WATCHER *ptr = head;

    while (ptr != NULL) {
        if (ptr->buf == NULL) {
            ptr->bufSize  = 1024;
            ptr->buf = malloc(ptr->bufSize);
            ptr->bufPosition = 0;
        }

        ssize_t bytes_read = 0;
        while ((bytes_read = read(ptr->fdin, ptr->buf + ptr->bufPosition, ptr->bufSize - ptr->bufPosition)) != -1) {
            if(bytes_read==0){
                WATCHER *ptr = head->next;

             while (ptr != NULL) {
                WATCHER *temp = ptr->next;
                watcher_types[ptr->type].stop(ptr);
                ptr=temp;
            }
            free(head->buf);
            free(head);
            raise(SIGINT);
            break;
            }
            
            ptr->bufPosition += bytes_read;
            if (ptr->bufPosition == ptr->bufSize) {
                ptr->bufSize *= 2;
                ptr->buf = realloc(ptr->buf, ptr->bufSize);
            }

            char *newline = NULL;
            char *buf_end = ptr->buf + ptr->bufPosition;
            char *line_start = ptr->buf;
            while ((newline = memchr(line_start, '\n', buf_end - line_start)) != NULL) {
                *newline = '\0';
                debug("%s     %s", ptr->buf, newline);
                watcher_types[ptr->type].recv(ptr, line_start);
                line_start = newline + 1;
            }

            ptr->bufPosition = buf_end - line_start;
            memmove(ptr->buf, line_start, ptr->bufPosition);
        }
        ptr = ptr->next;
    }

    sigprocmask(SIG_UNBLOCK, &mask_set, NULL);
}


int ticker(void) {

     struct sigaction sa_int = {0};
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sigaddset(&sa_int.sa_mask, SIGINT);

    if (sigaction(SIGINT, &sa_int, NULL) < 0) {
        printf("error");
        exit(1);
    }

    struct sigaction sa_chld = {0};
    sa_chld.sa_flags = SA_SIGINFO; 
    sa_chld.sa_sigaction = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);

    if (sigaction(SIGCHLD, &sa_chld, NULL) < 0) {
        perror("sigaction");
        exit(1);
    }

    struct sigaction sa_io = {0};
    sa_io.sa_flags = SA_SIGINFO; 
    sa_io.sa_sigaction = sigio_handler;
    sigemptyset(&sa_io.sa_mask);

    if (sigaction(SIGIO, &sa_io, NULL) < 0) {
        perror("sigaction");
        exit(1);
    }


   

    if(fcntl(STDIN_FILENO, F_SETFL, O_ASYNC | O_NONBLOCK) == -1){}
    if(fcntl(STDIN_FILENO, F_SETOWN ,getpid()) == -1){}
    fcntl(STDIN_FILENO, F_SETSIG , SIGIO);


    WATCHER* cli_inst = malloc(sizeof(WATCHER));
    cli_inst->pid = getpid();
    cli_inst->watcherID = 0;
    cli_inst->type = CLI_WATCHER_TYPE;
    cli_inst->fdin = STDIN_FILENO;
    cli_inst->fdout = STDOUT_FILENO;
    cli_inst->tracing = 0;
    cli_inst->serial = 0;
    cli_inst->prev = NULL;
    cli_inst->next = NULL;
    cli_inst->args = NULL;
    cli_inst->buf = NULL;
    cli_inst->bufPosition = 0;
    cli_inst->bufSize=0;

    head = cli_inst;
    char *arg = "ticker> ";
    write(STDOUT_FILENO, arg, strlen(arg));
    raise(SIGIO);

     sigset_t mask_set;
        sigfillset(&mask_set);
        sigdelset(&mask_set, SIGCHLD);
        sigdelset(&mask_set, SIGIO);
        sigdelset(&mask_set, SIGINT);

    while (1) {
        sigsuspend(&mask_set);
   
    }
    return 0;
}