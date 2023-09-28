#include <stdlib.h>
#include "ticker.h"
#include "store.h"
#include "cli.h"
#include "debug.h"
#include "helpers.h"
#include <string.h>


WATCHER *cli_watcher_start(WATCHER_TYPE *type, char *args[]) {
    cli_watcher_send(NULL,"???\n");
    return NULL;
}

int cli_watcher_stop(WATCHER *wp) {
    if(cli_watcher_send(wp,"???\n") == -1){
        return -1;
    }
    return 0;
}

int cli_watcher_send(WATCHER *wp, void *arg) {
    char* shellString = "ticker> ";
    if(arg == NULL){
        if(write(STDOUT_FILENO, shellString , strlen(shellString))){
        return -1;
        }
        else return 0;
    }
    if(arg==NULL){
        return -1;
    }
    arg = (char*)arg;
    if(write(STDOUT_FILENO,arg, strlen(arg))== -1 || write(STDOUT_FILENO, shellString , strlen(shellString))){
        return -1;
    }

    return 0;
}

int cli_watcher_recv(WATCHER *wp, char *txt) {
    //parse txt

    char *argsPtr = txt;
    int argsCount = 1;

    while (*argsPtr != '\0') {
        if (*argsPtr == ' ') {
            argsCount++;
        }
        argsPtr++;
    }

    char** args = (char**)(malloc(argsCount * sizeof(char*))); //dynamically allocate array of pointers to args
    if (args == NULL) {
        return -1;
    }

    argsPtr = txt;
    args[0] = argsPtr;
    int counter = 1;
    while (*argsPtr != '\0') {
        if (*argsPtr == ' ' || *argsPtr == '\t') {
            *argsPtr = '\0';
            args[counter] = argsPtr + 1;
            counter++;
        }
        argsPtr++;
    }


    //Identify command
    
        if (strcmp(args[0], "quit") == 0) {         //QUIT
            if (argsCount != 1) {
                cli_watcher_send(wp,"???\n");
                free(args);
                return -1;
            }
            WATCHER *ptr = head->next;

             while (ptr != NULL) {
                WATCHER *temp = ptr->next;
                watcher_types[ptr->type].stop(ptr);
                ptr=temp;
            }
            free(args);
            free(head->buf);
             cli_watcher_send(wp,"");
            free(head);
           
            raise(SIGINT);
            
            return 0;
        } else if (strcmp(args[0], "watchers") == 0) {      //WATCHERS
            if (argsCount != 1) {
                cli_watcher_send(wp,"???\n");
                free(args);
                return -1;
            }
            //print watchers table
            tick_watchers(wp);
            free(args);
            return 0;
        }  
        else if (strcmp(args[0], "start") == 0) {       //START
            //loop through watcher types table to find watcher type that fits args[1]
            if (argsCount < 3) {
                free(args);
                cli_watcher_send(wp,"???\n");
                return -1;
            }
            int typeCount = 0;
            while(watcher_types[typeCount].name != NULL){
                if(strcmp(args[1],watcher_types[typeCount].name) == 0){
                        int args_size = argsCount-2;
                        char** newArgsArr = malloc(args_size * sizeof(char*));
                        for (int i = 0; i < args_size; i++) {
                            newArgsArr[i] = args[i + 2];
                        }
                        watcher_types[typeCount].start(&watcher_types[typeCount],newArgsArr);
                        break;
                }
                typeCount++;
            }
            return 0;
        }
        else if (strcmp(args[0], "stop") == 0) {        //STOP
            if (argsCount != 2) {
                cli_watcher_send(wp,"???\n");
                free(args);
                return -1;
            }
            //stop a watcher instance
           char *endptr;
            int id = (int)strtol(args[1], &endptr, 10);
            if (*endptr != '\0') {
                cli_watcher_send(wp,"???\n");
                free(args);
                return -1;
            }
            WATCHER *watchPtr = get_watcher(id);
            if(watchPtr==NULL){
                cli_watcher_send(wp,"???\n");
                free(args);
                return -1;
            }
            watcher_types[watchPtr->type].stop(watchPtr);
            free(args);
            return 0;
        }
        else if (strcmp(args[0], "trace") == 0) {       //TRACE
            if (argsCount != 2) {
               cli_watcher_send(wp,"???\n");
               free(args);
                return -1;
            }
            //enable tracing for watcher
            char *endptr;
            int id = (int)strtol(args[1], &endptr, 10);
            if (*endptr != '\0') {
                cli_watcher_send(wp,"???\n");
                free(args);
                return -1;
            }
            WATCHER *watchPtr = get_watcher(id);
            if(watchPtr==NULL){
                cli_watcher_send(wp,"???\n");
                free(args);
                return -1;
            }
            watcher_types[watchPtr->type].trace(watchPtr,1);
            free(args);
            return 0;
        }
        else if (strcmp(args[0], "untrace") == 0) {     //UNTRACE
            if (argsCount != 2) {
                cli_watcher_send(wp,"???\n");
                free(args);
                return -1;
            }
            //disable tracing for watcher
                char *endptr;
            int id = (int)strtol(args[1], &endptr, 10);
            if (*endptr != '\0') {
                cli_watcher_send(wp,"???\n");
                free(args);
                return -1;
            }
            WATCHER *watchPtr = get_watcher(id);
            if(watchPtr==NULL){
               cli_watcher_send(wp,"???\n");
               free(args);
                return -1;
            }
            watcher_types[watchPtr->type].trace(watchPtr,0);
            free(args);
            return 0;
        }
        else if (strcmp(args[0], "show") == 0) {        //SHOW
            if (argsCount != 2) {
                cli_watcher_send(wp,"???\n");
                free(args);
                return -1;
            }
            struct store_value *vp = store_get(args[1]);
            if(vp == NULL){
                    cli_watcher_send(wp,"???\n");
                    free(args);
                    return -1;
            }
            else{
               fprintf(stdout,"%s\t%lf",args[1],vp->content.double_value);
               fflush(stdout);
               cli_watcher_send(wp, "\n");
               store_free_value(vp);
               free(args);
               return 0;
            }
        }
        else {
            // Unknown command
                cli_watcher_send(wp,"???\n");
                free(args);
                return -1;
        }
    free(args);
    return 0;
}

int cli_watcher_trace(WATCHER *wp, int enable) {
    if(enable==0){
        wp->tracing=0;
    }
    else{
        wp->tracing=1;
    }
    return 0;
}
