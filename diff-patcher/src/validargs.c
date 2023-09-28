#include <stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 * @modifies global variable "diff_filename" to point to the name of the file
 * containing the diffs to be used.
 */

int validargs(int argc, char **argv) {
    global_options = 0;
    //int nOption = 0;
    //int qOption = 0;
    if(argc < 2){// minimum of two args, fliki and filename
        return -1;
    }
    else if(argc == 2){
        if(**(argv+1)=='-'){
             if(*(*(argv+1)+1) == 'h'){
                if(*(*(argv+1)+2) != '\0'){
                   global_options=0;
                    return -1;  
                }
                global_options = global_options | HELP_OPTION;
                return 0;
             }
             else{
                return -1;
             }
        }
        diff_filename = *(argv+1);
        return 0;
    }
    else{
    //int validLoopCounter = 0;
    for(int i = 1; i < argc ; i++) {
        char *a = *(argv + i);
        //validLoopCounter++;
        char* tempChar = a;
        if(i == 1){
            if((*tempChar == '-') && (*(tempChar+1) == 'h') && (*(tempChar+2) == '\0') ){
                global_options = global_options | HELP_OPTION;
                return 0;
        }}
        if(i != argc-1){ //handling for non last arg
            // check if first char is a dash
            if(*tempChar == '-'){
                tempChar++; //increment tempchar to get to next char
                if(*tempChar == 'n'){
                    global_options = global_options | NO_PATCH_OPTION;
                }
                else if(*tempChar == 'q'){
                    global_options = global_options | QUIET_OPTION;}
                else{ //invalid flag
                    global_options=0;
                    return -1; 
                }
                //make sure that flag is only two chars long
                tempChar++;
                if(*tempChar != '\0'){
                    global_options=0;
                    return -1; 
                }
            }
            else{
            //non last arg, assuming >1 arg, can only be a flag
            global_options=0;
            return -1; }
            }
            
        else{ //handling for last arg, should be a filename assuming no h or error
            if(*tempChar == '-'){
                global_options=0;
                return -1; //last arg should not be a flag
            }
            //while(*tempChar != '\0');    May need to parse file name further?
            diff_filename = tempChar; //loop will terminate after this
        }
    }
    //global options and diff filename should be set by now
    return 0;
    }
    //abort();
}
