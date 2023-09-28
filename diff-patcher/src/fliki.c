#include <stdlib.h>
#include <stdio.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

// Hunk next
static int hunkCount = 0;
static int start_of_line = 1;

// Hunk get c
static int char_count = 0;
static char c_test = '<';       // < if first section, else > if second section

static char* deletion_buffer_byte_ptr = hunk_deletions_buffer;
static char* deletion_buffer_ptr = hunk_deletions_buffer + 2;

static char* append_buffer_byte_ptr = hunk_additions_buffer;
static char* append_buffer_ptr = hunk_additions_buffer + 2;

static int c_append_section = -1;
static int line_count = 0;
static int new_hunk_flag = -1;
static int eos_flag = -1;

// Hunk show
static int hunk_check = -1;




/**
 * @brief Get the header of the next hunk in a diff file.
 * @details This function advances to the beginning of the next hunk
 * in a diff file, reads and parses the header line of the hunk,
 * and initializes a HUNK structure with the result.
 *
 * @param hp  Pointer to a HUNK structure provided by the caller.
 * Information about the next hunk will be stored in this structure.
 * @param in  Input stream from which hunks are being read.
 * @return 0  if the next hunk was successfully located and parsed,
 * EOF if end-of-file was encountered or there was an error reading
 * from the input stream, or ERR if the data in the input stream
 * could not be properly interpreted as a hunk.
 */

static int checkFormat(int type, int old_start, int old_end, int new_start, int new_end) {
    /* 
    XaX
    XaX,X

    XdX
    X,XdX

    XcX
    XcX,X
    X,XcX
    X,XcX,X */

    if(type == 1) {
        if(old_start != old_end) {
            return -1;
        }
        return 1;
    }

    else if(type == 2) {
        if(new_start != new_end) {
            return -1;
        }
        return 1;
    }

    else if(type == 3) {
        return 1;
    }

    return -1;
}

static int inFile_c(FILE *in) {
    char c;

    while((c = fgetc(in)) != EOF) {
        return c;
    }

    return ERR;
}



int hunk_next(HUNK *hp, FILE *in) {
    // Patch calls hunk_next with a pointer in the file and the function
    // Parses the file according to the current pointer and finds the next hunk
    // While parsing hunk_next() checks for the correct format in the hunk body (<, >, \n---)
    // Also checks for correct format in hunk header

    c_test = '<';
    eos_flag = -1;

    // Clear buffer
    char *clear_delete_ptr = hunk_deletions_buffer;
    char *clear_append_ptr = hunk_additions_buffer;
    for(int i = 0; i < 512; i++) {  
        *clear_append_ptr = 0;
        *clear_delete_ptr = 0;
        clear_append_ptr++;
        clear_delete_ptr++;
    }

    deletion_buffer_byte_ptr = hunk_deletions_buffer;
    deletion_buffer_ptr = hunk_deletions_buffer + 2;

    append_buffer_byte_ptr = hunk_additions_buffer;
    append_buffer_ptr = hunk_additions_buffer + 2;


    // Move pointers back to original spots


    // printf("HUNK NEXT CALLED\n");
    // fflush(stdout);

    char t;
    int counter = 0;
    // Find next hunk
    while((t = fgetc(in)) != EOF) {
        // printf("Parsing hunk body: %c\n", t);
        // fflush(stdout);

        if(start_of_line >= 0) {
            if(t == '>' || t == '<' || t == '-') {
                start_of_line = -1;
                continue;
            }
            ungetc(t, in);
            break;
        }

        if(t == '\n') {
            // printf("FOUND NEWLINE \n");
            // fflush(stdout);
            start_of_line = 1;
            
            // Check for ---\n
            if(counter == 3) {
                counter = 0;
                continue;
            }


            // Check for \n< and \n> + space
           t = fgetc(in);

           if(t == '<' || t == '>') {
                t = fgetc(in);
                start_of_line = -1;

                if(t == ' ') {
                    continue;
                }

                else {
                    return ERR;
                }
            }

           else if(t == '-') {
                start_of_line = -1;
                counter++;
           }

            // Is the start of new hunk header
            else {
                ungetc(t, in);
                break;
            }
        }

        // Check for single -
        else if(t == '-') {
            counter++;
        }
        
    }


    // If end-of-file was encountered or error reading from input stream
    if (ferror(in) || feof(in)) {
        return EOF;
    }

    // Find the old range
    // a = 97, c = 99, d = 100
    int temp = 0;
    int curr;
    char c;
    while ((c = fgetc(in)) != EOF) {
        // printf("HEADER: %c\n", c);
        // fflush(stdout);

        // Check if the current character is 'a', 'c', or 'd'
        if (c == 'a' || c == 'c' || c == 'd') {
            if(c == 'a') {
                hp->type = HUNK_APPEND_TYPE;
            }
            else if(c == 'c') {
                hp->type = HUNK_CHANGE_TYPE;
            }
            else if(c == 'd') {
                hp->type = HUNK_DELETE_TYPE;
            }
            else {
                hp->type = HUNK_NO_TYPE;
            }
            temp = temp % 10;

            hp->old_start = temp;
            hp->old_end = temp;

            break;
            }

        // , = 44
        else if (c == ',') {
            hp->old_start = temp;

            // Get the old_end
            temp = 0;
            while((c = fgetc(in)) != EOF) {
                // printf("CHARACTER AFTER ',' (LINE 140): %c\n", c);
                // fflush(stdout);

                if (c == 'a' || c == 'c' || c == 'd') {
                    if(c == 'a') {
                        hp->type = HUNK_APPEND_TYPE;
                    }
                    else if(c == 'c') {
                        hp->type = HUNK_CHANGE_TYPE;
                    }
                    else if(c == 'd') {
                        hp->type = HUNK_DELETE_TYPE;
                    }   
                    else {
                        hp->type = HUNK_NO_TYPE;
                    }
                hp->old_end = temp;
                break;
            }   
                
                 if (!(c >= '0' && c <= '9')) {
                    return ERR;
                }

                curr = c - '0';
                temp = temp * 10 + curr;
            }
            break;
        }

        if (!(c >= '0' && c <= '9')) {
            return ERR;
        }

        curr = c - '0';
        temp = temp * 10 + curr;
    }

    // If end-of-file was encountered or error reading from input stream
    if (ferror(in) || feof(in)) {
        return EOF;
    }

    // New Range
    temp = 0;
    curr = 0;
    int flag = -1;              // Check if has a a ,
    while ((c = fgetc(in)) != EOF) {
        if(c == ',') {
            hp->new_start = temp;
            temp = 0;
            flag = 1;

            while((c = fgetc(in)) != EOF) {
                if(c == '\n') {
                    break;
                }

                 if (!(c >= '0' && c <= '9')) {
                    return ERR;
                    }

                curr = c - '0';
                temp = temp * 10 + curr;
            }
        }    


        if(c == '\n') {
            if(flag < 0) {
                 hp->new_start = temp;
                hp->new_end = temp;
                break;
            }

            else if(flag > 0) {
                hp->new_end = temp;
                break;
            }
        }

         if (!(c >= '0' && c <= '9')) {
            return ERR;
        }

        curr = c - '0';
        temp = temp * 10 + curr;
    }

    // If end-of-file was encountered or error reading from input stream
    if (ferror(in) || feof(in)) {
        return EOF;
    }


    // Check correct format
    if(checkFormat(hp->type, hp->old_start, hp->old_end, hp->new_start, hp->new_end) == -1) {
        return ERR;
    }

    hp->serial = ++hunkCount;
    
    // TEST RUN
    // printf("Type: %d\n", hp->type);
    //     fflush(stdout);
    // printf("Serial: %d\n", hp->serial);
    //     fflush(stdout);
    // printf("Old Start: %d\n", hp->old_start);
    //     fflush(stdout);
    // printf("Old End: %d\n", hp->old_end);
    //     fflush(stdout);
    // printf("New Start: %d\n", hp->new_start);
    //     fflush(stdout);
    // printf("New End: %d\n", hp->new_end);
    //     fflush(stdout);


    return 0;
    // abort();
}

/**
 * @brief  Get the next character from the data portion of the hunk.
 * @details  This function gets the next character from the data
 * portion of a hunk.  The data portion of a hunk consists of one
 * or both of a deletions section and an additions section,
 * depending on the hunk type (delete, append, or change).
 * Within each section is a series of lines that begin either with
 * the character sequence "< " (for deletions), or "> " (for additions).
 * For a change hunk, which has both a deletions section and an
 * additions section, the two sections are separated by a single
 * line containing the three-character sequence "---".
 * This function returns only characters that are actually part of
 * the lines to be deleted or added; characters from the special
 * sequences "< ", "> ", and "---\n" are not returned.
 * @param hdr  Data structure containing the header of the current
 * hunk.
 *
 * @param in  The stream from which hunks are being read.
 * @return  A character that is the next character in the current
 * line of the deletions section or additions section, unless the
 * end of the section has been reached, in which case the special
 * value EOS is returned.  If the hunk is ill-formed; for example,
 * if it contains a line that is not terminated by a newline character,
 * or if end-of-file is reached in the middle of the hunk, or a hunk
 * of change type is missing an additions section, then the special
 * value ERR (error) is returned.  The value ERR will also be returned
 * if this function is called after the current hunk has been completely
 * read, unless an intervening call to hunk_next() has been made to
 * advance to the next hunk in the input.  Once ERR has been returned,
 * then further calls to this function will continue to return ERR,
 * until a successful call to call to hunk_next() has successfully
 * advanced to the next hunk.
 */

int hunk_getc(HUNK *hp, FILE *in) {
    // FUNCTION RETURNS character OR EOS OR ERR
    // EOS returned after each hunk section. 2 EOS will be returned for a CHANGE hunk section
    // can return '\n' as character when reading
    // <, >, -, can be within the lines of the hunk and should be returned if that's the case
    // Does not return space after < or >
    // Check that # of lines matches with the header


    //////////////////////////////////////////// TODO: Can return ERR if the hunk section has been completely read ////////////////////////////////////////////
    // implement EOS when it reaches end of file after while loop
    // last line of file ends with \n (fgetc(in) returns EOF)

    // printf("HUNK GET C CALLED\n");
    // fflush(stdout);

    int type = hp->type;
    char c;
    int counter = 0;

    int old_range = (hp->old_end + 1) - hp->old_start;
    int new_range = (hp->new_end + 1) - hp->new_start;

    hunk_check = -1;

    if(eos_flag == 1) {
        return ERR;
    }

    eos_flag = -1;


   // If hunk is append, or delete type
    if(type != 3) {

        // Ignore all <, >, space, ---\n
        while((c = fgetc(in)) != EOF) {
            // printf("READS: %c\n", c);
            // fflush(stdout);
        
            // If reads > + ' '
            if(c == '>' && type == 1 && start_of_line == 1) {
                c = fgetc(in);
                new_hunk_flag = -1;
                //  printf("Check space: %c\n", c);
                //     fflush(stdout);

                if(c != ' ') {
                    return ERR;
                }
                
                else {
                    start_of_line = -1;
                    continue;
                }
            }

            // If reads < + ' '
            else if(c == '<' && type == 2 && start_of_line == 1) {
                c = fgetc(in);
                new_hunk_flag = -1;

                if(c != ' ') {
                    return ERR;
                }
                
               else {
                    start_of_line = -1;
                    continue;
                }
            }

            else {
                // printf("LINE 373\n");
                // fflush(stdout);
                // start_of_line = -1;

                if(new_hunk_flag == 1 && c != '<' && c != '>' && c != '-') {
                     // Store char count in buffer 
                        unsigned short char_count_temp = ((unsigned char) char_count) | (((unsigned char) (char_count >> 8)) << 8);
                        unsigned char byte1 = char_count_temp & 0xff;
                        unsigned char byte2 = (char_count_temp >> 8) & 0xff;
                    
                        if(type == 1 && append_buffer_ptr - hunk_additions_buffer <= 512) {
                            *append_buffer_byte_ptr = byte1;
                            append_buffer_byte_ptr++;

                            *append_buffer_byte_ptr = byte2;
                            append_buffer_byte_ptr = append_buffer_ptr;
                            append_buffer_ptr = append_buffer_ptr + 2;
                        }
                        else if(type == 2 && deletion_buffer_ptr - hunk_deletions_buffer <= 512) {
                            *deletion_buffer_byte_ptr = byte1;
                            deletion_buffer_byte_ptr++;

                            *deletion_buffer_byte_ptr = byte2;
                            deletion_buffer_byte_ptr = deletion_buffer_ptr;
                            deletion_buffer_ptr = deletion_buffer_ptr + 2;
                        }

                    char_count = 0;
                    new_hunk_flag = -1;
                    eos_flag = 1;
                    ungetc(c, in);
                    return EOS;
                }

                char_count++;

                // Store character in buffer
                if(type == 1) {
                    // Append type
                    *append_buffer_ptr = c;
                    append_buffer_ptr++;

                    // Check for full buffer
                    if(append_buffer_ptr - hunk_additions_buffer == 512) {
                        *(hunk_additions_buffer + 510) = 0x0;
                        *(hunk_additions_buffer + 511) = 0x0;
                    }
                }

                else if(type == 2) {
                    // Delete type
                    *deletion_buffer_ptr = c;
                    deletion_buffer_ptr++;

                    // Check for full buffer
                    if(deletion_buffer_ptr - hunk_deletions_buffer == 512) {
                        *(hunk_deletions_buffer + 510) = 0x0;
                        *(hunk_deletions_buffer + 511) = 0x0;
                    }
                }

                

                if(c == '\n') {
                    char temp = fgetc(in);
                    line_count++;

                    // End of hunk section is reached
                    if(temp != '<' && temp != '>' && temp != '-' && new_hunk_flag >= 0) {
                        // printf("LINE COUNT LINE 476: %d\n", line_count);
                        if(type == 1 && line_count != new_range) {
                            return ERR;
                        }

                        else if(type == 2 && line_count != old_range) {
                            return ERR;
                        }

                        // Set last two bytes to indicate end of data
                        if(type == 1 && append_buffer_ptr - hunk_additions_buffer <= 512) {
                            *append_buffer_ptr = 0x0;
                            append_buffer_ptr++;
                            *append_buffer_ptr = 0x0;
                            append_buffer_ptr++;
                        }

                        else if(type == 2 && deletion_buffer_ptr - hunk_deletions_buffer <= 512) {
                            *deletion_buffer_ptr = 0x0;
                            deletion_buffer_ptr++;
                            *deletion_buffer_ptr = 0x0;
                            deletion_buffer_ptr++;
                        }

                        start_of_line = 1;      // Pointer at the start of a line
                        line_count = 0;
                        hunk_check = 1;
                        // printf("LINE 377 FINAL CHAR COUNT: %d\n", char_count);              

                        // Store char count in buffer 
                        unsigned short char_count_temp = ((unsigned char) char_count) | (((unsigned char) (char_count >> 8)) << 8);
                        unsigned char byte1 = char_count_temp & 0xff;
                        unsigned char byte2 = (char_count_temp >> 8) & 0xff;
                    
                        if(type == 1 && append_buffer_ptr - hunk_additions_buffer <= 512) {
                            *append_buffer_byte_ptr = byte1;
                            append_buffer_byte_ptr++;

                            *append_buffer_byte_ptr = byte2;
                            append_buffer_byte_ptr = append_buffer_ptr;
                            append_buffer_ptr = append_buffer_ptr + 2;
                        }
                        else if(type == 2 && deletion_buffer_ptr - hunk_deletions_buffer <= 512) {
                            *deletion_buffer_byte_ptr = byte1;
                            deletion_buffer_byte_ptr++;

                            *deletion_buffer_byte_ptr = byte2;
                            deletion_buffer_byte_ptr = deletion_buffer_ptr;
                            deletion_buffer_ptr = deletion_buffer_ptr + 2;
                        }

                        ungetc(temp, in);
                        // printf("EOS RETURNED LINE 322\n");
                        eos_flag = 1;
                        return EOS;
                    }

                else {
                    ungetc(temp, in);
                }

                new_hunk_flag = 1;
                start_of_line = 1;
                
                // Store \n in buffer
                if(type == 1 && append_buffer_ptr - hunk_additions_buffer <= 512) {
                    // Append type
                    *append_buffer_ptr = c;
                    append_buffer_ptr++;
                }

                else if(type == 2 && deletion_buffer_ptr - hunk_deletions_buffer <= 512) {
                    // Delete type
                    *deletion_buffer_ptr = c;
                    deletion_buffer_ptr++;
                }
                return c;
            }

                return c;
                }
        }
    }

    // Hunk is CHANGE type
    else {
        while((c = fgetc(in)) != EOF) {
            // printf("CHAR: %c\n", c);

            // If reads ---\n
            if(counter == 3 && c == '\n') {
                // printf("LINE COUNT LINE 528: %d\n", line_count);
                // Line count check
                // printf("LINE COUNT LINE 549: %d\n", line_count);
                if(line_count != old_range) {
                    return ERR;
                }
                line_count = 0;
                counter = 0;
                c_test = '>';
                c_append_section = 1;
                new_hunk_flag = -1;       
                
                // Next section char count starts
                // Store char count in buffer 
                unsigned short char_count_temp = ((unsigned char) char_count) | (((unsigned char) (char_count >> 8)) << 8);
                unsigned char byte1 = char_count_temp & 0xff;
                unsigned char byte2 = (char_count_temp >> 8) & 0xff;

                // printf("BYTE1: %c\n", byte1);
                // printf("BYTE2: %c\n", byte2);
                // printf("COUNT: %d\n", char_count);
                if(deletion_buffer_ptr - hunk_deletions_buffer <= 512) {
                    *deletion_buffer_byte_ptr = byte1;
                    deletion_buffer_byte_ptr++;

                    *deletion_buffer_byte_ptr = byte2;
                    deletion_buffer_byte_ptr = deletion_buffer_ptr;         // Move to next byte storage
                    deletion_buffer_ptr = deletion_buffer_ptr + 2;
                }

                char_count = 0;   
                
                // printf("EOS RETURNED LINE 381\n");
                return EOS;         // First section is parsed
            }


            // Check '< ' and '> ' for each section
            if(c == c_test) {
                // printf("HERE\n");
                new_hunk_flag = -1;
                 c = fgetc(in);

                if(c != ' ') {
                    return ERR;
                }
                
                else {
                    start_of_line = -1;
                    continue;
                }
            }


            // If reads -
            else if(c == '-') {
                counter++;
                continue;
            }

            
            // Reads character or \n
            else {
                // printf("CHAR C: %c\n", c);
                
                // printf("TEMPL %c\n", temp);

                // End of hunk section is reached
                    if(c != '<' && c != '>' && c != '-' && new_hunk_flag >= 0) {
                        // printf("LINE COUNT LINE 476: %d\n", line_count);
                        if(type == 1 && line_count != new_range) {
                            return ERR;
                        }

                        else if(type == 2 && line_count != old_range) {
                            return ERR;
                        }

                        // Set last two bytes to indicate end of data
                        if(type == 1 && append_buffer_ptr - hunk_additions_buffer <= 512) {
                            *append_buffer_ptr = 0x0;
                            append_buffer_ptr++;
                            *append_buffer_ptr = 0x0;
                            append_buffer_ptr++;
                        }

                        else if(type == 2 && deletion_buffer_ptr - hunk_deletions_buffer <= 512) {
                            *deletion_buffer_ptr = 0x0;
                            deletion_buffer_ptr++;
                            *deletion_buffer_ptr = 0x0;
                            deletion_buffer_ptr++;
                        }

                        start_of_line = 1;      // Pointer at the start of a line
                        line_count = 0;
                        hunk_check = 1;
                        // printf("LINE 377 FINAL CHAR COUNT: %d\n", char_count);              

                        // Store char count in buffer 
                        unsigned short char_count_temp = ((unsigned char) char_count) | (((unsigned char) (char_count >> 8)) << 8);
                        unsigned char byte1 = char_count_temp & 0xff;
                        unsigned char byte2 = (char_count_temp >> 8) & 0xff;
                    
                        if(type == 1 && append_buffer_ptr - hunk_additions_buffer <= 512) {
                            *append_buffer_byte_ptr = byte1;
                            append_buffer_byte_ptr++;

                            *append_buffer_byte_ptr = byte2;
                            append_buffer_byte_ptr = append_buffer_ptr;
                            append_buffer_ptr = append_buffer_ptr + 2;
                        }
                        else if(type == 2 && deletion_buffer_ptr - hunk_deletions_buffer <= 512) {
                            *deletion_buffer_byte_ptr = byte1;
                            deletion_buffer_byte_ptr++;

                            *deletion_buffer_byte_ptr = byte2;
                            deletion_buffer_byte_ptr = deletion_buffer_ptr;
                            deletion_buffer_ptr = deletion_buffer_ptr + 2;
                        }

                        ungetc(c, in);
                        // printf("REUTRNED EOS\n");
                        return EOS;
                    }

                // printf("CHAR: %c\n", c);
                char_count++;
                char temp = fgetc(in);


                if(c == '\n') {
                    start_of_line = 1;      // Pointer at the start of a line
                    line_count++;

                    // Store \n 
                    if(c_append_section == 1 && append_buffer_ptr - hunk_additions_buffer <= 512) {
                        *append_buffer_ptr = c;
                        append_buffer_ptr++;

                        // Check for full buffer
                    if(append_buffer_ptr - hunk_additions_buffer == 512) {
                        *(hunk_additions_buffer + 510) = 0x0;
                        *(hunk_additions_buffer + 511) = 0x0;
                        }
                    }

                    else if(deletion_buffer_ptr - hunk_deletions_buffer <= 512) {
                        *deletion_buffer_ptr = c;
                        deletion_buffer_ptr++;

                        // Check for full buffer
                        if(deletion_buffer_ptr - hunk_deletions_buffer == 512) {
                            *(hunk_deletions_buffer + 510) = 0x0;
                            *(hunk_deletions_buffer + 511) = 0x0;
                        }
                    }

                    // printf("COUNT: %d\n", char_count);


                    // Next section char count starts
                    // Store char count in buffer 
                    unsigned short char_count_temp = ((unsigned char) char_count) | (((unsigned char) (char_count >> 8)) << 8);
                    unsigned char byte1 = char_count_temp & 0xff;
                    unsigned char byte2 = (char_count_temp >> 8) & 0xff;
                    
                    // printf("CHAR COUNT AFTER: %d\n", char_count_temp);
                    // printf("BYTE1: %d\n", byte1);
                    // printf("BYTE2: %d\n", byte2);
                    
                    if(c_append_section == 1 && append_buffer_ptr - hunk_additions_buffer <= 512) {
                        *append_buffer_byte_ptr = byte1;
                        append_buffer_byte_ptr++;

                        *append_buffer_byte_ptr = byte2;
                        append_buffer_byte_ptr = append_buffer_ptr;
                        append_buffer_ptr = append_buffer_ptr + 2;
                    }

                    else if(deletion_buffer_ptr - hunk_deletions_buffer <= 512) {
                        *deletion_buffer_byte_ptr = byte1;  
                        deletion_buffer_byte_ptr++;

                        *deletion_buffer_byte_ptr = byte2;
                        deletion_buffer_byte_ptr = deletion_buffer_ptr;         // Move to next byte storage
                         deletion_buffer_ptr = deletion_buffer_ptr + 2;
                     }
                       
                    char_count = 0; 


                    // End of hunk section is reached
                    if(temp != '<' && temp != '>' && temp != '-' && c_test == '>' && new_hunk_flag >= 0) {
                    
                        if(c_test == '>') {
                            // printf("LINE COUNT LINE 679: %d\n", line_count);

                            // New line check
                            if(line_count != new_range) {
                                return ERR;
                            }

                            new_hunk_flag = -1;
                            line_count = 0;
                            hunk_check = 1;
                            ungetc(temp, in);
                            // printf("EOS RETURNED LINE 393\n");
                            return EOS;
                        }
                    }   
                    
                    else {
                        // printf("UNGET CALLED %c\n", temp);
                        ungetc(temp, in);
                    }
                    new_hunk_flag = 1;
                }

                else {
                    ungetc(temp, in);
                    if(c_append_section == 1 && append_buffer_ptr - hunk_additions_buffer <= 512) {
                        *append_buffer_ptr = c;
                        append_buffer_ptr++;
                        }
                    else if(deletion_buffer_ptr - hunk_deletions_buffer <= 512){
                        // printf("HERE LINE 604 %c\n", *deletion_buffer_ptr);
                        *deletion_buffer_ptr = c;
                        deletion_buffer_ptr++;
                        }
                    }
                
                return c;
            }

        }
          
    }

    if(c == EOF) {
        eos_flag = 1;
        return EOS;
    }

    
    return ERR;
    // abort();
}

/**
 * @brief  Print a hunk to an output stream.
 * @details  This function prints a representation of a hunk to a
 * specified output stream.  The printed representation will always
 * have an initial line that specifies the type of the hunk and
 * the line numbers in the "old" and "new" versions of the file,
 * in the same format as it would appear in a traditional diff file.
 * The printed representation may also include portions of the
 * lines to be deleted and/or inserted by this hunk, to the extent
 * that they are available.  This information is defined to be
 * available if the hunk is the current hunk, which has been completely
 * read, and a call to hunk_next() has not yet been made to advance
 * to the next hunk.  In this case, the lines to be printed will
 * be those that have been stored in the hunk_deletions_buffer
 * and hunk_additions_buffer array.  If there is no current hunk,
 * or the current hunk has not yet been completely read, then no
 * deletions or additions information will be printed.
 * If the lines stored in the hunk_deletions_buffer or
 * hunk_additions_buffer array were truncated due to there having
 * been more data than would fit in the buffer, then this function
 * will print an elipsis "..." followed by a single newline character
 * after any such truncated lines, as an indication that truncation
 * has occurred.
 *
 * @param hp  Data structure giving the header information about the
 * hunk to be printed.
 * @param out  Output stream to which the hunk should be printed.
 */

void hunk_show(HUNK *hp, FILE *out) {
    /* If a line has 260 characters, that would be 256 + 4 and in binary: 0000 0001 0000 0100
    * Since 1 char = 1 byte, in hex that would be 0x1 and 0x4.
    * However little endian has the smallest byte first and the largest byte last.
    * So in the buffer you'd put them as 0x4 and 0x1.
    * 
    * fputc('//char', out)
    * 
    * If there is no current hunk,
    * or the current hunk has not yet been completely read, then no
    * deletions or additions information will be printed.
*/  
    // The length is the length of the data stored in the buffer.
    // Write hunk header to out


    // No current hunk or hunk has not been completely read
    if(hunk_check == 1) {
        if(hp->old_start == hp->old_end) {
        fputc((char) (hp->old_start + '0'), out);
        }
        else {
            fputc((char) (hp->old_start + '0'), out);
            fputc(',', out);
            fputc((char) (hp->old_end + '0'), out);
        }

        if(hp->type == 1) {
            fputc('a', out);
        }
        else if(hp->type == 2) {
            fputc('d', out);
        }    
        else if(hp->type == 3) {
            putc('c', out);
        }

        if(hp->new_start == hp->new_end) {
        fputc((char) (hp->new_start + '0'), out);
            }
        else {
            fputc((char) (hp->new_start + '0'), out);
            fputc(',', out);
            fputc((char) (hp->new_end + '0'), out);
        }

        fputc('\n', out);
        char *append_ptr = hunk_additions_buffer;
        char *delete_ptr = hunk_deletions_buffer;
        // char bracket;

        // Figure out the type of hunk
        if(hp->type == 1) {
            // bracket = '>';

            for(int i = 0; i < 511; i++) {

             if(*(append_ptr + i) == 0x0) {
                if(*(append_ptr + i + 1) == 0x0) {
                    break;
                }
            }


            if (*(append_ptr + i) >= 0x01 && *(append_ptr + i) <= 0xFF && *(append_ptr + i + 1) == 0x00  && *(append_ptr + i) != 0x0A) {
                continue;
            }

            else {
                char t = *(append_ptr + i);
                if(i == 2) {
                    fputc('>', out);
                    fputc(' ', out);
                }

                if(*(append_ptr + i) == '\n') {
                    fputc('\n', out);

                     if (*(append_ptr + i + 1) >= 0x01 && *(append_ptr + i + 1) <= 0xFF && *(append_ptr + i + 1) != 0x0A) {
                        fputc('>', out);
                        fputc(' ', out);
                    }
                    // printf("BYTE: %d\n", *delete_ptr + i + 1);
                }

                else {
                    fputc(*(append_ptr + i), out);
                }
                
            
            }
        }

        if(append_ptr - hunk_additions_buffer == 512) {
            fputc('.', out);
            fputc('.', out);
            fputc('.', out);
            fputc('\n', out);
        } 
    }

    else if(hp->type == 2) {

       for(int i = 0; i < 511; i++) {

             if(*(delete_ptr + i) == 0x0) {
                if(*(delete_ptr + i + 1) == 0x0) {
                    break;
                }
            }


            if (*(delete_ptr + i) >= 0x01 && *(delete_ptr + i) <= 0xFF && *(delete_ptr + i + 1) == 0x00  && *(delete_ptr + i) != 0x0A) {
                continue;
            }

            else {
                char t = *(delete_ptr + i);
                if(i == 2) {
                    fputc('<', out);
                    fputc(' ', out);
                }

                if(*(delete_ptr + i) == '\n') {
                    fputc('\n', out);


                     if (*(delete_ptr + i + 1) >= 0x01 && *(delete_ptr + i + 1) <= 0xFF && *(delete_ptr + i + 1) != 0x0A) {
                        fputc('<', out);
                        fputc(' ', out);
                    }
                    // printf("BYTE: %d\n", *delete_ptr + i + 1);
                }

                else {
                    fputc(*(delete_ptr + i), out);
                }
                
            
            }
        }

         if(delete_ptr - hunk_deletions_buffer == 512) {
            fputc('.', out);
            fputc('.', out);
            fputc('.', out);
            fputc('\n', out);
        } 
    }

    else if(hp->type == 3) {
        int num_of_line = 0;
        int num_del_line = (hp->old_end + 1) - hp->old_start;
        int num_app_line = (hp->new_end + 1) - hp->new_start;


        // First section (delete)
        for(int i = 0; i < 511; i++) {
            char h = *(delete_ptr + i);
             if(*(delete_ptr + i) == 0x0) {
                if(*(delete_ptr + i + 1) == 0x0) {
                    break;
                }
            }

            if(num_of_line == num_del_line) {
                break;
            }

            if (*(delete_ptr + i) >= 0x01 && *(delete_ptr + i) <= 0xFF && *(delete_ptr + i + 1) == 0x00  && *(delete_ptr + i) != 0x0A) {
                continue;
            }

            else {
                char t = *(delete_ptr + i);
                if(i == 2) {
                    fputc('<', out);
                    fputc(' ', out);
                }

                if(*(delete_ptr + i) == '\n') {
                    num_of_line++;
                    fputc('\n', out);

                    char tem = *(delete_ptr + i + 1);

                     if (*(delete_ptr + i + 1) >= 0x01 && *(delete_ptr + i + 1) <= 0xFF && *(delete_ptr + i + 1) != 0x0A) {
                        fputc('<', out);
                        fputc(' ', out);
                    }
                    // printf("BYTE: %d\n", *delete_ptr + i + 1);
                }

                else {
                    fputc(*(delete_ptr + i), out);
                }
                
            
            }

        }   

         if(delete_ptr - hunk_deletions_buffer == 512) {
            fputc('.', out);
            fputc('.', out);
            fputc('.', out);
            fputc('\n', out);
        } 

        // ---\n section
        fputc('-', out);
        fputc('-', out);
        fputc('-', out);
        fputc('\n', out);
        
        // Append section
                for(int i = 0; i < 511; i++) {

             if(*(append_ptr + i) == 0x0) {
                if(*(append_ptr + i + 1) == 0x0) {
                    break;
                }
            }


            if (*(append_ptr + i) >= 0x01 && *(append_ptr + i) <= 0xFF && *(append_ptr + i + 1) == 0x00  && *(append_ptr + i) != 0x0A) {
                continue;
            }

            else {
                char t = *(append_ptr + i);
                if(i == 2) {
                    fputc('>', out);
                    fputc(' ', out);
                }

                if(*(append_ptr + i) == '\n') {
                    fputc('\n', out);

                     if (*(append_ptr + i + 1) >= 0x01 && *(append_ptr + i + 1) <= 0xFF && *(append_ptr + i + 1) != 0x0A) {
                        fputc('>', out);
                        fputc(' ', out);
                    }
                }

                else {
                    fputc(*(append_ptr + i), out);
                }
            
            }
        }

         if(append_ptr - hunk_additions_buffer == 512) {
            fputc('.', out);
            fputc('.', out);
            fputc('.', out);
            fputc('\n', out);
        } 

        
    }

}

}

/**
 * @brief  Patch a file as specified by a diff.
 * @details  This function reads a diff file from an input stream
 * and uses the information in it to transform a source file, read on
 * another input stream into a target file, which is written to an
 * output stream.  The transformation is performed "on-the-fly"
 * as the input is read, without storing either it or the diff file
 * in memory, and errors are reported as soon as they are detected.
 * This mode of operation implies that in general when an error is
 * detected, some amount of output might already have been produced.
 * In case of a fatal error, processing may terminate prematurely,
 * having produced only a truncated version of the result.
 * In case the diff file is empty, then the output should be an
 * unchanged copy of the input.
 *
 * This function checks for the following kinds of errors: ill-formed
 * diff file, failure of lines being deleted from the input to match
 * the corresponding deletion lines in the diff file, failure of the
 * line numbers specified in each "hunk" of the diff to match the line
 * numbers in the old and new versions of the file, and input/output
 * errors while reading the input or writing the output.  When any
 * error is detected, a report of the error is printed to stderr.
 * The error message will consist of a single line of text that describes
 * what went wrong, possibly followed by a representation of the current
 * hunk from the diff file, if the error pertains to that hunk or its
 * application to the input file.  If the "quiet mode" program option
 * has been specified, then the printing of error messages will be
 * suppressed.  This function returns immediately after issuing an
 * error report.
 *
 * The meaning of the old and new line numbers in a diff file is slightly
 * confusing.  The starting line number in the "old" file is the number
 * of the first affected line in case of a deletion or change hunk,
 * but it is the number of the line *preceding* the addition in case of
 * an addition hunk.  The starting line number in the "new" file is
 * the number of the first affected line in case of an addition or change
 * hunk, but it is the number of the line *preceding* the deletion in
 * case of a deletion hunk.
 *
 * @param in  Input stream from which the file to be patched is read.
 * @param out Output stream to which the patched file is to be written.
 * @param diff  Input stream from which the diff file is to be read.
 * @return 0 in case processing completes without any errors, and -1
 * if there were errors.  If no error is reported, then it is guaranteed
 * that the output is complete and correct.  If an error is reported,
 * then the output may be incomplete or incorrect.
 */

int patch(FILE *in, FILE *out, FILE *diff) {
    /*  read in a file, and change that file based on a diff file with hunks, and write the changes into an output file */

    // /* when an error is
    // * detected, some amount of output might already have been produced.
    // * In case of a fatal error, processing may terminate prematurely,
    // * having produced only a truncated version of the result.
    // * In case the diff file is empty, then the output should be an
    // * unchanged copy of the input.
    // * *
    // * 
    // * 
    // * When any
    // * error is detected, a report of the error is printed to stderr.
    // * The error message will consist of a single line of text that describes
    // * what went wrong, possibly followed by a representation of the current
    // * hunk from the diff file, if the error pertains to that hunk or its
    // * application to the input file.  If the "quiet mode" program option
    // * has been specified, then the printing of error messages will be
    // * suppressed.  This function returns immediately after issuing an
    // * error report.
    // * 
    // * /

    HUNK hunk;
    HUNK *hunk_ptr = &hunk;

    char c;
    int holder;
    int in_line_count = 1;
    int out_line_count = 0;

    while((holder = hunk_next(hunk_ptr, diff)) != ERR) {
        
        if(holder == EOF) {
            break;
        }

        int end = hunk.old_start;
        if(hunk.type == 1) {
            end = hunk.old_start + 1;
        }

        while(in_line_count < end) {
            c = inFile_c(in);

            if(global_options != NO_PATCH_OPTION) {
                fputc(c, out);
            }

            if(c == '\n') {
                in_line_count++;
            }
        }

        // Delete
        if(hunk.type == 2) {
            while(in_line_count <= hunk.old_end) {
                c = inFile_c(in);

                 if(c == '\n') {
                    in_line_count++;
                }
            }        
        }


        // Append
        if(hunk.type == 1) {
            // XaX
            // XaX,X

            // Move to old end
            while(in_line_count <= hunk.old_end) {
                c = inFile_c(in);

                if(global_options != NO_PATCH_OPTION) {
                    fputc(c, out);
                }

                if(c == '\n') {
                    in_line_count++;
                }
            }   

            // Append files in hunk 
            while((c = hunk_getc(hunk_ptr, diff)) != EOS && c != ERR) {
                if(global_options != NO_PATCH_OPTION) {
                    fputc(c, out);
                }
            }

            if(c == ERR) {
                printf("HERE\n");
                if(global_options != QUIET_OPTION) {
                    fprintf(stderr, "ERROR: INCORRECT FORMAT DETECTED");
                    hunk_show(hunk_ptr, stderr);
                }
                return -1;
            }

        }

        // Change hunk
        if(hunk.type == 3) {
            // Move to old end
            while(in_line_count <= hunk.old_end) {
                c = inFile_c(in);

                 if(c == '\n') {
                    in_line_count++;
                }
            }

            while((c = hunk_getc(hunk_ptr, diff)) != EOS && c != ERR) {}

            // Print lines before new start
            while(in_line_count < hunk.new_start) {
                c = inFile_c(in);

                if(global_options != NO_PATCH_OPTION) {
                    fputc(c, out);
                }

                if(c == '\n') {
                    in_line_count++;
                }
            }

            // Append section
            while((c = hunk_getc(hunk_ptr, diff)) != EOS && c != ERR) {
                if(global_options != NO_PATCH_OPTION) {
                    fputc(c, out);
                }
            }


            if(c == ERR) {
                if(global_options != QUIET_OPTION) {
                     fprintf(stderr, "ERROR: INCORRECT FORMAT DETECTED");
                    hunk_show(hunk_ptr, stderr);
                }
                return -1;
            }

        }


    }

    if(holder == ERR) {
        if(global_options != QUIET_OPTION) {
             fprintf(stderr, "ERROR: INCORRECT FORMAT DETECTED");
                hunk_show(hunk_ptr, stderr);
            }
        return -1;
    }

    while((c = inFile_c(in)) != ERR) {
         if(global_options != NO_PATCH_OPTION) {
                    fputc(c, out);
                }
    }

    return 0;
}


