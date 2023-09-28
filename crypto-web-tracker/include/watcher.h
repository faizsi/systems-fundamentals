#include <stdlib.h>
#include <signal.h>
#include <ticker.h>
#include <stdio.h>


typedef struct watcher {
    pid_t pid;
    int watcherID;
    int type;
    int fdin;
    int fdout;
    int tracing;
    int serial; //for tracing
    struct watcher *prev;
    struct watcher *next;
    char** args;
    char* buf;
    size_t bufPosition;
    size_t bufSize;

} WATCHER;

extern WATCHER *head;