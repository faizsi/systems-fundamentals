#include <stdio.h>
#include <stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

int main(int argc, char **argv)
{

    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options == HELP_OPTION)
        USAGE(*argv, EXIT_SUCCESS);

    FILE *diff_file = fopen(diff_filename, "r");
    if(patch(stdin, stdout, diff_file) == 0) {
        return EXIT_SUCCESS;
    }

    else {
        EXIT_FAILURE;
    }

    return EXIT_FAILURE; 
}