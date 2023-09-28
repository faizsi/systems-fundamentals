#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "ticker.h"
#include "bitstamp.h"
#include "debug.h"
#include "helpers.h"
#include <string.h>
#include "store.h"
#include "argo.h"
#include <fcntl.h>
#include <time.h>

WATCHER *bitstamp_watcher_start(WATCHER_TYPE *type, char *args[]) {
   
    WATCHER *watcher = (WATCHER *) malloc(sizeof(WATCHER));

    int to_parent[2], to_child[2];
    
    if (pipe(to_parent) == -1) {
        free(args);
        free(watcher);
        return NULL;
    }
    if (pipe(to_child) == -1){
        free(args);
        free(watcher);
        return NULL;
    } 

            if(fcntl(to_parent[0], F_SETFL, O_ASYNC | O_NONBLOCK) == -1){
               free(args);
        free(watcher);
        return NULL;
            }
            if(fcntl(to_parent[0], F_SETOWN, head->pid) == -1){
                    free(args);
        free(watcher);
        return NULL;
            }
            fcntl(to_parent[0], F_SETSIG, SIGIO);

    pid_t childpid = fork();
    if (childpid < 0) {
      free(args);
        free(watcher);
        return NULL;
    }

    //CHILD
    if (childpid == 0) {
        //close pipes
        close(to_parent[0]);
        close(to_child[1]);

        dup2(to_child[0], STDIN_FILENO);
        close(to_child[0]);

        dup2(to_parent[1], STDOUT_FILENO);
        close(to_parent[1]);


            if (fcntl(STDIN_FILENO, F_SETFL, O_ASYNC | O_NONBLOCK) == -1) {
                        free(args);
                    free(watcher);
                    return NULL;
            }
            if (fcntl(STDIN_FILENO, F_SETOWN, head->pid) == -1) {
                        free(args);
                    free(watcher);
                    return NULL;
            }
            if (fcntl(STDIN_FILENO, F_SETSIG, SIGIO) == -1) {
                       free(args);
                    free(watcher);
                    return NULL;
            }

        //start uwsc
        execvp(watcher_types[BITSTAMP_WATCHER_TYPE].argv[0], watcher_types[BITSTAMP_WATCHER_TYPE].argv);
    }

    //PARENT
    else {
        //close unused pipes
        close(to_parent[1]);
        close(to_child[0]);

        // create watcher struct
        watcher->pid = childpid;
        watcher->type = BITSTAMP_WATCHER_TYPE;
        watcher->fdin = to_parent[0];
        watcher->fdout = to_child[1];
        watcher->tracing = 0;
        watcher->serial = 0;
        watcher->next = NULL;
        watcher->args = args;
        watcher->buf = NULL;
        watcher->bufSize = 0;

        WATCHER *ptr = head;
        while (ptr->next != NULL) {
            ptr = ptr->next;
        }
        ptr->next = watcher;
        watcher->prev = ptr;
    
        watcher->watcherID = ptr->watcherID+1;

        // send subscription command 
         char subscribe_command[100];
        snprintf(subscribe_command, sizeof(subscribe_command),
                 "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"%s\" } }\n", args[0]);
        write(to_child[1], subscribe_command, strlen(subscribe_command));
        watcher_types[head->type].send(head,"");

    }
    return watcher;
}

int bitstamp_watcher_stop(WATCHER *wp) {
    bitstamp_watcher_send(wp,"!q");
    kill(wp->pid,SIGTERM);
    
    return 0;
}

int bitstamp_watcher_send(WATCHER *wp, void *arg) {
    if(arg!=NULL){
        write(wp->fdout,arg,sizeof(arg));
        write(wp->fdout,"\n",1);
        return 0;
    }
    else{
        return -1;
    }

}

int bitstamp_watcher_recv(WATCHER *wp, char *txt) {
    wp->serial++;
    char *prefix = "> \b\bServer message: '";
    
    if (strncmp(txt, prefix, strlen(prefix)) == 0) {
        txt = txt + strlen(prefix);
        txt[strlen(txt)-1]='\0';
        FILE *stream = fmemopen(txt, strlen(txt), "r");
        ARGO_VALUE *readVal = argo_read_value(stream);
        fclose(stream);

        ARGO_VALUE* storeVal = argo_value_get_member(readVal,"event");

        if(storeVal!=NULL){
            char* tradeCheck = argo_value_get_chars(storeVal);
            if(strcmp(tradeCheck,"trade")==0){
                
                //char* currencyPair = (wp->args[0]) + 12; 
               ARGO_VALUE* storePair = argo_value_get_member(readVal,"channel");
               char * channel = argo_value_get_chars(storePair);
               char *currencyPair = strstr(channel, "_");
               currencyPair++;
                currencyPair = strstr(currencyPair, "_");
                if (currencyPair != NULL) {
                   currencyPair = currencyPair + 1;
                   
                }
                char* keyStr1 = malloc(strlen("bitstamp.net:live_trades_") + strlen(currencyPair) + strlen(":price") + 1);
                sprintf(keyStr1, "bitstamp.net:live_trades_%s:price", currencyPair);

                ARGO_VALUE* data = argo_value_get_member(readVal,"data");
                ARGO_VALUE* newPrice = argo_value_get_member(data,"price");
                double priceHolder = 0;
                int priceCheckReturn = argo_value_get_double(newPrice,&priceHolder);
                    if(priceCheckReturn==-1){
                        argo_free_value(readVal);
                        free(tradeCheck);
                        free(channel);
                        free(keyStr1);
                        return -1;
                    }
               
                struct store_value priceStore = {
                    .type = STORE_DOUBLE_TYPE,
                    .content.double_value = priceHolder
                }; 
                store_put(keyStr1,&priceStore);
                free(keyStr1);

                char* keyStr2 = calloc(1,strlen("bitstamp.net:live_trades_") + strlen(currencyPair) + strlen(":volume") + 1);
                sprintf(keyStr2, "bitstamp.net:live_trades_%s:volume", currencyPair);    
                ARGO_VALUE *newVolume = argo_value_get_member(data,"amount");
                double newVolumeDbl = 0;
                int volCheckReturn = argo_value_get_double(newVolume,&newVolumeDbl);
                    if(volCheckReturn == -1){
                        free(keyStr2);
                        argo_free_value(readVal);
                        free(tradeCheck);
                        free(channel);
                        return -1;
                    }
                struct store_value *volStore = store_get(keyStr2);
                if(volStore==NULL){
                    struct store_value volStoreFirst = 
                    { .type = STORE_DOUBLE_TYPE, 
                    .content.double_value = newVolumeDbl };
                    store_put(keyStr2,&volStoreFirst);
                    free(keyStr2);
                }
                else{
                    newVolumeDbl += (volStore->content.double_value);
                    volStore->type=STORE_DOUBLE_TYPE;
                    volStore->content.double_value=newVolumeDbl;
                    store_put(keyStr2,volStore);
                    //argo_free_value(newVolume);
                    store_free_value(volStore);
                    free(keyStr2);

                }
            free(channel);
            }
            free(tradeCheck);
            
            argo_free_value(readVal);

        }
    }
    if(wp->tracing){
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    fprintf(stderr, "\n[%ld.%06ld][%s][%2d][%5d]: %s\n", time.tv_sec, (time.tv_nsec/1000), watcher_types[wp->type].name, wp->fdin, wp->serial, txt);
    fflush(stderr);
    }
return 0;
}

int bitstamp_watcher_trace(WATCHER *wp, int enable) {
    if(enable==0){
        wp->tracing=0;
    }
    else{
        wp->tracing=1;
    }
    return 0;
}
