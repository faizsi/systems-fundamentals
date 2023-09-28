# CSE320-HW1
Homework 1 - CSE 320 - Spring 2023

Professor Eugene Stark

Due Date: Friday 02/17/2023 @ 11:59pm

Read the entire doc before you start

Introduction
In this assignment, you will implement a command-line program
(called fliki) whose purpose is to apply patches, given in a "diff" file,
to a source file, producing an edited, target version.
The goal of this homework is to familiarize yourself with C programming,
with a focus on input/output, bitwise manipulations, and the use of pointers.
For all assignments in this course, you MUST NOT put any of the functions
that you write into the main.c file.  The file main.c MUST ONLY contain
#includes, local #defines and the main function (you may of course modify
the main function body).  The reason for this restriction has to do with our
use of the Criterion library to unit test your code.
Beyond this, you may have as many or as few additional .c files in the src
directory as you wish.  Also, you may declare as many or as few headers as you wish.
Note, however, that header and .c files distributed with the assignment base code
often contain a comment at the beginning which states that they are not to be
modified.  PLEASE take note of these comments and do not modify any such files,
as they will be replaced by the original versions during grading.

😱 Array indexing ('A[]') is not allowed in this assignment.
You MUST USE pointer arithmetic instead.
No array brackets ('A[]') are allowed.
This means you cannot declare your own arrays.
All necessary arrays are declared in the global.h header file.
You MUST USE these arrays. DO NOT create your own arrays.
We WILL check for this.


🤓 Reference for pointers: https://beej.us/guide/bgc/html/#pointers.


Getting Started
Fetch base code for hw1 as described in hw0. You can find it at this link:
https://gitlab02.cs.stonybrook.edu/cse320/hw1.
IMPORTANT: 'FETCH AND MERGE', DO NOT 'CLONE'.
Both repos will probably have a file named .gitlab-ci.yml with different contents.
Simply merging these files will cause a merge conflict. To avoid this, we will
merge the repos using a flag so that the .gitlab-ci.yml found in the hw1
repo will replace the hw0 version.  To merge, use this command:

git merge -m "Merging HW1_CODE" HW1_CODE/master --strategy-option=theirs



😱 Based on past experience, many students will either ignore the above command or forget
to use it.  The result will be a merge conflict, which will be reported by git.
Once a merge conflict has been reported, it is essential to correct it before committing
(or to abort the merge without committing -- use git merge --abort and go back and try again),
because git will have inserted markers into the files involved indicating the locations of the
conflicts, and if you ignore this and commit anyway, you will end up with corrupted files.
You should consider it important to read up at an early stage on merge conflicts with git and
how to resolve them properly.

Here is the structure of the base code:
.
├── .gitlab-ci.yml
└── hw1
    ├── .gitignore
    ├── hw1.sublime-project
    ├── include
    │   ├── debug.h
    │   ├── fliki.h
    │   └── global.h
    ├── Makefile
    ├── rsrc
    │   ├── empty
    │   ├── empty_file1.diff
    │   ├── file1
    │   ├── file1_empty.diff
    │   ├── file1_file2.diff
    │   └── file2
    ├── src
    │   ├── fliki.c
    │   ├── main.c
    │   └── validargs.c
    ├── test_output
    │   └── .git-keep
    └── tests
        └── basecode_tests.c


The .gitlab-ci.yml file is a file that specifies "continuous integration" testing
to be performed by the GitLab server each time you push a commit.  Usually it will
be configured to check that your code builds and runs, and that any provided unit tests
are passed.  You are free to change this file if you like.


😱  The CI testing is for your own information; it does not directly have
anything to do with assignment grading or whether your commit has been properly
pushed to the server.  If some part of the testing fails, you will see the somewhat
misleading message "commit failed" on the GitLab web interface.
This does not mean that "your attempt to commit has failed" or that "your commit
didn't get pushed to the server"; the very fact that the testing was triggered at
all means that you successfully pushed a commit.  Rather, it means that "the CI tests
performed on a commit that you pushed did not succeed".  The purpose of the tests are
to alert you to possible problems with your code; if you see that testing has failed
it is worth investigating why that has happened.  However, the tests can sometimes
fail for reasons that are not your fault; for example, the entire CI "runner" system
may fail if someone submits code that fills up the system disk.  You should definitely
try to understand why the tests have failed if they do, but it is not necessary to be
overly obsessive about them.



The hw1.sublime-project file is a "project file" for use by the Sublime Text editor.
It is included to try to help Sublime understand the organization of the project so that
it can properly identify errors as you edit your code.


The Makefile is a configuration file for the make build utility, which is what
you should use to compile your code.  In brief, make or make all will compile
anything that needs to be, make debug does the same except that it compiles the code
with options suitable for debugging, and make clean removes files that resulted from
a previous compilation.  These "targets" can be combined; for example, you would use
make clean debug to ensure a complete clean and rebuild of everything for debugging.


The include directory contains C header files (with extension .h) that are used
by the code.  Note that these files often contain DO NOT MODIFY instructions at the beginning.
You should observe these notices carefully where they appear.


The src directory contains C source files (with extension .c).


The tests directory contains C source code (and sometimes headers and other files)
that are used by the Criterion tests.


The rsrc directory contains some samples of data files that you can use for
testing purposes.


The test_output directory is a scratch directory where the Criterion tests can
put output files.  You should not commit any files in this directory to your
git repository.



A Note about Program Output
What a program does and does not print is VERY important.
In the UNIX world stringing together programs with piping and scripting is
commonplace. Although combining programs in this way is extremely powerful, it
means that each program must not print extraneous output. For example, you would
expect ls to output a list of files in a directory and nothing else.
Similarly, your program must follow the specifications for normal operation.
One part of our grading of this assignment will be to check whether your program
produces EXACTLY the specified output.  If your program produces output that deviates
from the specifications, even in a minor way, or if it produces extraneous output
that was not part of the specifications, it will adversely impact your grade
in a significant way, so pay close attention.

😱 Use the debug macro debug (described in the 320 reference document in the
Piazza resources section) for any other program output or messages you many need
while coding (e.g. debugging output).


Part 1: Program Operation and Argument Validation
In this part of the assignment, you will write a function to validate the arguments
passed to your program via the command line. Your program will treat arguments
as follows:


If no flags are provided, you will display the usage and return with an
EXIT_FAILURE return code.


If the -h flag is provided, you will display the usage for the program and
exit with an EXIT_SUCCESS return code.


If the -h flag is not provided, then the last argument on the command line must
be the name of a file that contains "diffs" (more about this below).  The program
will read a source file to be patched from standard input (stdin), it will edit
it in accordance with instructions read from the diff file, and it will write
the edited result to standard output (stdout).  If this patching process is
successful, the program will exit with an EXIT_SUCCESS return code, otherwise
the program will exit with an EXIT_FAILURE return code.
In the latter case, the program will print to standard error (stderr) an error
message or messages describing the error(s) that were discovered.


If the -n (no output) flag is provided, then the program performs a "dry run"
in which it does everything it normally would do, except that it does not produce
any output on stdout.


If the -q (quiet mode) flag is provided, then any error messages that would
otherwise be printed on stderr are suppressed.  The program will still exit
with EXIT_FAILURE if an error is encountered, however.


Note that the program reads a file to be patched from stdin and writes the edited
result to stdout.  Error messages are issued to stderr.  No other output
is produced.  If the program runs without error, then it will exit with the
EXIT_SUCCESS status code; if any error occurs during the execution of the program,
then it will exit with the EXIT_FAILURE status code.

🤓 EXIT_SUCCESS and EXIT_FAILURE are macros defined in <stdlib.h> which
represent success and failure return codes respectively.


🤓 stdin, stdout, and stderr are special I/O "streams", defined
in <stdio.h>, which are automatically opened at the start of execution
for all programs, do not need to be reopened, and (almost always) should not
be closed.


🤓 For accessing an input stream (such as stdin) you should use the fgetc()
function, which reads a single byte of data from a stream.  When parsing
input read from a stream, there are some situations in which it is convenient
to take a byte of data that has already been read and "push it back" into the
input stream.  The ungetc() function should be used for this.  Do not attempt
to use ungetc() to "push back" more than one character.  The fgetc() and
ungetc() functions are the only functions that you should use for reading input.
For writing to an output stream, you may use fputc() and fprintf().
It will also be necessary to use the fopen() function to open the file containing
the "diffs".


😱 Any libraries that help you parse strings are prohibited for this assignment
(string.h, ctype.h, etc).  The use of atoi, scanf, fscanf, sscanf,
and similar functions is likewise prohibited.  This is intentional and
will help you practice parsing strings and manipulating pointers.

The usage scenarios for this program are described by the following message,
which is printed by the program when it is invoked without any arguments:
USAGE: bin/fliki [-h] [-n] [-q] DIFF_FILE
   -h       Help: displays this help menu.
   -n       Dry run: no patched output is produced.
   -q       Quiet mode: no error output is produced.

If -h is specified, then it must be the first option on the command line, and any
other options are ignored.

If -h is not specified, then a filename DIFF_FILE must be the last argument.  In this case,
the program reads the DIFF_FILE, which is assumed to be in the traditional format
produced by 'diff' and it applies the edits specified therein to lines of text read
from stdin.  The patched result is written to stdout and any error reports are written
to stderr.

If the program succeeds in applying all the patches without any errors, then it exits
with exit code EXIT_SUCCESS.  Otherwise, the program exits with EXIT_FAILURE after issuing
an error report on stderr.  In case of an error, the output that is produced on stdout may
be truncated at the point at which the error was detected.

If the -n option is specified, then the program performs a 'dry run' in which only error
reports are produced (on stderr) and no patched output is produced on stdout.

If the -q option is specified, then the program does not produce any error reports on
stderr, although it still exits with EXIT_FAILURE should an error occur.

The square brackets indicate that the enclosed argument is optional.
A valid invocation of the program implies that the following hold about
the command-line arguments:


If the -h flag is provided, it is the first argument after
the program name and any other arguments that follow are ignored.


The optional flags may come in any order.


If -h is not given, then any optional flags must be followed by a filename,
which is the last argument on the command line.


For example, the following are a subset of the possible valid argument combinations:

$ bin/fliki -h ...
$ bin/fliki -n foo
$ bin/fliki -n -q foo


😱 The ... means that all arguments, if any, are to be ignored; e.g.
the usage bin/fliki -h -x -y BLAHBLAHBLAH -z is equivalent to bin/fliki -h.

Some examples of invalid combinations would be:

$ bin/fliki -n
$ bin/fliki foo bar
$ bin/fliki -n -h foo
$ bin/fliki


😱 You may use only "raw" argc and argv for argument parsing and
validation. Using any libraries that parse command line arguments (e.g.
getopt) is prohibited.


😱 You MAY NOT use dynamic memory allocation in this assignment
(i.e. malloc, realloc, calloc, mmap, etc.).  You have been provided
with pre-declared arrays which you should use as storage for the purpose
discussed further below.


🤓 Reference for command line arguments: https://beej.us/guide/bgc/html/#command-line-arguments.

NOTE: The make command compiles the fliki executable into the bin folder.
All commands from here on are assumed to be run from the hw1 directory.

Required Validate Arguments Function
In global.h, you will find the following function prototype (function
declaration) already declared for you. You MUST implement this function
as part of the assignment.

int validargs(int argc, char **argv);


The file validargs.c contains the following specification of the required behavior
of this function:

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



😱 This function must be implemented as specified as it will be tested
and graded independently. It should always return -- the USAGE macro should
never be called from validargs.

The validargs function should return -1 if there is any form of failure.
This includes, but is not limited to:


Invalid number of arguments (too few or too many).


Invalid ordering of arguments.


A missing filename where one is required.


The global_options variable of type long is used to record the mode
of operation (i.e. encode/decode) of the program and associated parameters.
This is done as follows:


If the -h flag is specified, the least significant bit (bit 0) is 1.


If the -n flag is specified, the second-least significant bit (bit 1)
is 1.


If the -q flag is specified, the third-least significant bit (bit 2)
is 1.


In addition, unless -h has been specified, the global variable diff_filename
should be assigned a pointer to the last command-line argument, which should
be the name of the file containing the "diffs".
If validargs returns -1 indicating failure, your program must call
USAGE(program_name, return_code) and return EXIT_FAILURE.
Once again, validargs must always return, and therefore it must not
call the USAGE(program_name, return_code) macro itself.
That should be done in main.
If validargs sets the least-significant bit of global_options to 1
(i.e. the -h flag was passed), your program must call USAGE(program_name, return_code)
and return EXIT_SUCCESS.

🤓 The USAGE(program_name, return_code) macro is already defined for you
in global.h.

If validargs returns 0, then your program must read input data from stdin
and (depending on the options supplied) write output data to stdout and tracing/error
information to stderr.
Upon successful completion, your program should exit with exit status EXIT_SUCCESS;
otherwise, in case of an error it should exit with exit status EXIT_FAILURE.
Unless the program has been compiled for debugging (using make debug),
in a successful run that exits with EXIT_SUCCESS no unspecified output may be produced
by the program.  In an unsuccessful run in which the program exits with EXIT_FAILURE
the program should output to stderr an error message or messages that indicate
the reason for the failure.  These messages are to have a format that consists
of a single line that explains what has gone wrong, possibly followed by a
representation of the "hunk" of the diff file that is associated with the error
(see the comments before the stub for the patch() function in fliki.c
for more details).

🤓 Remember EXIT_SUCCESS and EXIT_FAILURE are defined in <stdlib.h>.
Also note, EXIT_SUCCESS is 0 and EXIT_FAILURE is 1.


Example validargs Executions
The following are examples of the setting of global_options and the
other global variables for various command-line inputs.
Each input is a bash command that can be used to invoke the program.


Input: bin/fliki -h.  Setting: global_options=0x1
(help bit is set, other bits clear).


Input: bin/fliki foo.  Setting: global_options=0x0.
In addition, diff_filename should point to "foo".


Input: bin/fliki -q -n foo.  Setting: global_options=0x6
(the bitwise OR of bits 1 and 2).


Input: bin/fliki -n.  Setting: global_options=0x0.
This is an error case because no filename has been given.
In this case validargs returns -1, leaving global_options
and diff_filename unset.



Overview of Program Operation
Once the command-line arguments have been processed by validargs,
the basic operation of the program is to read lines of text from
standard input (stdin), apply patches read from the diff file,
and write the edited result to standard output (stdout).
The format of the diff file and the patching process are discussed in
more detail in the next section.

Part 2: Diffs and Patching
The original diff program was written in the early 1970s for the Unix
system by M. Douglas McIlroy and James Hunt at Bell Telephone Laboratories.
The underlying idea of diff is to compare two versions of a source file
and produce some kind of "minimal" list of differences between them.
The original diff program output these differences in the form of a
script for the line-oriented text editor ed, which could run the script
to transform one version of the source file into the other.
The modern version of diff used on Linux systems is a reimplementation
done as part of Project GNU.
A significant application of the diff utility is to make it possible
to store multiple versions of a file (some source code, say) in a compressed
form by storing just one complete version together with a set of "diffs"
by which that version can be transformed into the others.  This idea
became the basis for revision-control systems such as SCCS, CVS, and, later, git. In 1984, Larry Wall wrote a program called patchwhich could accept diffs in a variety of formats (which were introduced to improve upon the somewhat fragile nature of patching using the originaledscript format) and apply the diffs to one version of a file, transforming it into another. Thepatch` program provided on modern-day Linux systems is a remote descendant
of Larry Wall's original code.
A specification for the format of output files produced by diff can be found
here.
We will be concerned with the "Diff Default Output Format" discussed there,
also known as "traditional" diff format.
As an example, consider the following two text files:

This is file1.
This line does not appear in file2.
This line is in common between file1 and file2.
Here is another line that they have in common.
This line also does not appear in file2.



This is file2.
This line is in common between file1 and file2.
This line does not appear in file1.
Here is another line that they have in common.


The following is the output of the command diff file1 file2:

1,2c1
< This is file1.
< This line does not appear in file2.
---
> This is file2.
3a3
> This line does not appear in file1.
5d4
< This line also does not appear in file2.


The output consists of what the patch documentation refers to as "hunks",
each of which resembles (but is not exactly the same thing as) an ed command.
The sequence of hunks describes a series of changes to be made to file1
to transform it into file2.
There are three types of hunks, examples of which can be seen above.
One type of hunk describes deletions to be made from file1.
For example, the first hunk, which starts 1,2c1, says that the range
of lines in file1 from line 1 to line 2, inclusive, is to be
changed to (i.e. deleted and replaced by) a different line, which will appear as line 1
of file2.  The lines to be deleted are listed explicitly, with the
special two-character sequence "< " prepended.  The line to be inserted
is also listed, identified by the two character sequence "> ".
The deletions and additions are separated by a line that contains
only "---".
The second hunk, which starts 3a3, consists only of additions.
It says that a new line is to be added, at a position that corresponds
to line number 3 in both file1 and file2.
The third hunk, which starts 5d4, consists only of deletions.
It says that line number 5 in file1 is to be deleted, and that
line number 4 in file2 would be the immediately preceding line.
In general, the first line of each hunk has an "old range", an edit
command, and a "new range".  The old range and new range either
take the form of two line numbers separated by a comma, or a single
line number, which abbreviates the case in which the two numbers are
the same.  The single-character command is one of a, d, or c.
Following the first line of the hunk will be a series of lines to
be deleted (in the case of d or c commands) and a series of
lines to be appended (in the case of a or c) commands.
In the case of a c command, both the deletions and additions sections
are present, and they are separated by a line containing ---.

Part 3: Implementation
What you are to do is to write a program with the following behavior:
given a filename specified as a command-line argument, it reads that
file, tries to interpret its contents as diffs, successively applies
these diffs to source file lines read from standard input, and writes
the modified output to standard output.  In case errors are detected,
error reports are written to the standard error output.
The header file global.h lists prototypes (reproduced below) for functions
you are required to implement.  Each function that is listed has a stub (dummy)
implementation in one of the source files in the basecode.
The comment preceding the stub implementation will serve as the detailed
specification of what that function should do.
These specifications are not reproduced in the present document,
because of the difficulty of maintaining the consistency of the version here with
that in the source files when changes are made.

/*
 * Function you are to implement that validates and interprets command-line arguments
 * to the program.  See the stub in validargs.c for specifications.
 */
extern int validargs(int argc, char **argv);

/*
 * Functions you are to implement that perform the main functions of the program.
 * See the assignment handout and the comments in front of the stub for each function
 * in fliki.c for full specifications.
 */
extern int patch(FILE *in, FILE *out, FILE *diff);

extern int hunk_next(HUNK *hp, FILE *in);
extern int hunk_getc(HUNK *hp, FILE *in);
extern void hunk_show(HUNK *hp, FILE *out);


The data types (such as HUNK) that are referred to by these interface
specifications are defined in the additional header file fliki.h.
The global.h file also defines some global variables that your functions
must manipulate, as detailed in the header files and in the comments
that serve as the function specifications.
The functions hunk_next() and hunk_getc() provide the ability to
iterate through the hunks in a diff file.
The function hunk_show() is used to show the contents of the current
hunk when an error occurs.
The function patch() is the function that orchestrates the process of
reading the input, patching it, and producing the output.
Besides implementing the above functions, you will of course also have to make
modifications to the main() function, so that after calling validargs()
it opens the diff file, calls the patch() function to perform patching,
and returns the proper result codes.
Quite likely, you will want to implement additional functions besides those
that have been specified.
Since these functions will be helper functions that will be completely
local to fliki.c, it is good practice to declare such functions as static,
so that their existence is not visible elsewhere in the program.
Such functions do not require that prototypes be put into a header file,
and in fact it is undesirable (from a modularity point of view) to do so.
If a prototype is required for such a function, due to desired or necessary
ordering of function definitions within a module, it is sufficient to simply
put that prototype near the beginning of the module, before the definition
or first use of the function.
For this assignment, you probably will not want to implement an additional
program module with functions that are called from somewhere else.
However, if you were to do so, it would be desirable to place prototypes for such
functions in a header file that is #included both in the place where the
functions are defined and the place where they are used.  Be careful,
though -- the header files handed out with the basecode are marked DO NOT MODIFY,
so if you want to do this you should create your own header file, rather than
modifying the ones in the basecode.
There is one other thing to be said concerning the specifications
that have been given for you, which seems to trip up a lot of people.
Any function for which a specification has been given must be implemented
so as to satisfy that specification exactly as stated.  In particular,
you may not modify the prototype for that function in any way (that much should
be self-evident, since it would require changing header files that are marked
DO NOT MODIFY).  In addition, you should not introduce additional global
or state variables whose values modify the behavior of the specified functions
in such a way that from the point of view of a client (caller)
the behavior would be inconsistent with the given specifications.
If you do that, your functions will likely not pass unit tests
(used in grading) which check whether the functions have been implemented
in a way that satisfies their specifications.  Also, in the real world,
if you are working in a team and you unilaterally make such changes to
specifications you have been given, your resulting code will likely not
inter-operate correctly with code written by other people implementing
other parts of the design.
Having said that, I will say by way of clarification that the specifications
of the "hunk" functions for this assignment presuppose the existence of some
internal state information that persists across calls and keeps track of what
the hunk parser is currently doing.
For example, you will need to keep track of whether you are currently
reading deletion lines or addition lines from the hunk, as well as whether
or not you are currently at the beginning of a line of input.
You may introduce state variables for this purpose; however,
you may not expose these variables to the rest of the program
(e.g. by requiring that initializations be performed on them from main()).
It should be possible for us to make whatever sequence of calls we like
to the hunk_next(), hunk_getc(), and hunk_show() functions,
and the behavior of these calls should conform to the stated specifications,
regardless of how you chose to implement these functions internally
or what particular state variables you chose.

Part 4: "On-the-Fly" Operation
The mode of operation of fliki is to apply patches on-the-fly as
the input is being read in a character-by-character fashion, without
needing to buffer in memory either the contents file being patched or the
contents of hunks from the diff file.  An advantage of implementing it
this way is that it is not necessary to use dynamic storage allocation
or to make any assumptions about the maximum sizes of things like
lines of text or the number of text lines in a hunk.
Disadvantages include less flexibility in the way the program can operate.
You might well find this way of doing things to be foreign to your normal
way of thinking -- in that case, it will be a good exercise for you and
that's why I've specified the assignment in this particular way.
As was already indicated, you may not use any dynamic storage allocation
functions (such as malloc()), and you are not permitted to allocate
storage by declaring your own arrays.  It is entirely possible to do
the actual patching operation without using any such auxiliary storage.
However, for producing useful error messages, it is helpful to be able
to print out some kind of representation of the contents of a failed
hunk other than just the header line.  For this, it is necessary to
be able to buffer some amount of the deletions and additions sections
of at least the current hunk in memory.  I have provided (in global.h)
two char arrays, hunk_deletions_buffer and hunk_additions_buffer,
for this purpose.  These arrays are of a fixed, limited size, and
you should not expect the entirety of the deletions or additions
section of any given hunk to fit within them.  What you are to do
with these arrays is to use them to buffer the deletions and additions
lines from the current hunk as you read it, as much as will fit.
When hunk_show() is called to print out a representation of the
current hunk as part of an error message, it should include such
portion of the deletions and additions sections of the hunk as
has been buffered.  These should be printed out using "> ", "< ",
and ---, as in the traditional diff format.
However, if there was not enough space in the buffer to hold the entire
of one of the sections, then only a truncated version of that section
can be printed.  In that case, you should indicate that truncation has
occurred by printing the sequence "...\n" at the end of the section.
To give you some practice with pointers and the C memory model,
you are required to store the deletions and additions lines in the
provided array according to a particular format, which will now
be described.  You must follow this format exactly, as some of the
grading tests will check for it.
The data for each line should begin with two bytes that represent
the number of characters in the line (including the final '\n'),
given in little endian byte order (i.e. least-significant
byte first).  These two bytes should be followed by the specified
number of characters that make up the line.  Note that a terminating
'\0' character will not be used in this representation.
Each line should have a nonzero length.  The end of the sequence
of lines will be indicated by the presence of two zero bytes where
a nonzero size would normally be expected.
When filling the buffer, the lines are to be packed successively
in the buffer, without any gaps between them (other than the size
bytes).  The buffer should be filled as full as possible without
going beyond the end, subject to the condition that the two zero
bytes that indicate the end of the data must always be present.
If there is too much data to fit in the buffer, then the two
zero bytes will occur in the last two positions of the buffer,
indicating that truncation has occurred
(i.e. that there was additional line data that didn't fit).
In this case, when the lines are printed out (by hunk_show())
an elipsis ...\n should be printed out at the end to indicate
that truncation has occurred.  If the two zero bytes do not occur
in the last two positions of the buffer array, then truncation has
not occurred and no elipsis should be printed.
As a simple example, the following three lines of text:

one
two
three


should be stored in the buffer as the following sequence of bytes:

0x4, 0x0, 'o', 'n', 'e', '\n', 0x4, 0x0, 't', 'w', 'o', '\n', 0x6,
0x0, 't', 'h', 'r', 'e', 'e', '\n', 0x0, 0x0


To reiterate, the buffering described above is performed solely for
the purpose of generating readable error messages and it has nothing
to do with the patching operation itself.

Part 5: Miscellaneous

Input and Output Redirection
The fliki program reads from stdin and writes to stdout.
If you want the program to take input from a file or produce output to
a file, you may run the program using input and output redirection,
which is implemented by the shell using facilities provided by the operating
system kernel.
A simple example of a command that uses such redirection is the following:

$ bin/fliki mydiffs < old > new


This will cause the input to the program to be redirected from the text file
old and the output from the program to be redirected to the file new.
The redirection is accomplished by the shell, which interprets the < symbol
to mean "input redirection from a file" and the > symbol to mean
"output redirection to a file".  It is important to understand that redirection
is handled by the shell and that the bin/fliki program never sees any
of the redirection arguments; in the above example it sees only bin/fliki mydiffs
and it just reads from stdin and writes to stdout.
Alternatively, the output from a command can be piped
to another program, without the use of a disk file.
This could be done, for example, by the following command:

$ bin/fliki mydiffs < old | less


This sends the output to a program called less, which displays the first
screenful of the output and then gives you the ability to scan forward and
backward to see different parts of it.  Type h at the less prompt to get
help information on what you can do with it.  Type q at the prompt to exit less.
The less program is indispensible for viewing lengthy output produced by
a program.
Pipelines are a powerful tool for combining simple component programs into
combinations that perform complex transformations on data.
For example, you could connect several instances of fliki together as
follows:

$ bin/fliki mydiffs1 < old | bin/fliki mydiffs2 | bin/fliki mydiffs3 > new


Here there are three instances of fliki, each running as a separate process
and each using its own diffs.  The first instance of fliki patches
the contents of the file old using mydiffs1 and sends the result along
to the second instance of fliki.  The second instance applies the
patches from mydiffs2 and sends the result along to the third instance,
which applies patches from mydiffs3.  The final result is stored in
file new.
We just mention one other useful command that is often used with pipelines;
namely cat.
The cat command (short for "concatenate and print") is a command that reads
files specified as arguments, concatenates their contents, and prints the result
to stdout.
For example, an alternative way to send the contents of a file old as input
to fliki is the following:

cat old | bin/fliki mydiffs



Unit Testing
Unit testing is a part of the development process in which small testable
sections of a program (units) are tested individually to ensure that they are
all functioning properly. This is a very common practice in industry and is
often a requested skill by companies hiring graduates.

🤓 Some developers consider testing to be so important that they use a
work flow called test driven development. In TDD, requirements are turned into
failing unit tests. The goal is then to write code to make these tests pass.

This semester, we will be using a C unit testing framework called
Criterion, which will give you some
exposure to unit testing. We have provided a basic set of test cases for this
assignment.
The provided tests are in the tests/basecode_tests.c file. These tests do the
following:


validargs_help_test ensures that validargs sets the help bit
correctly when the -h flag is passed in.


validargs_no_patch_test ensures that validargs sets the NO_PATCH_OPTION
bit correctly when the -n flag is passed.


validargs_no_flags_test ensures that validargs correctly handles the
case in which just a filename is passed with no option arguments.


validargs_error_test ensures that validargs returns an error when the -q
and -n flags are supplied with no filename argument.


help_system_test uses the system syscall to execute your program through
Bash and checks to see that your program returns with EXIT_SUCCESS.


fliki_basic_test performs a basic test of the patching function.



Compiling and Running Tests
When you compile your program with make, a fliki_tests executable will be
created in your bin directory alongside the fliki executable. Running this
executable from the hw1 directory with the command bin/fliki_tests will run
the unit tests described above and print the test outputs to stdout. To obtain
more information about each test run, you can use the verbose print option:
bin/fliki_tests --verbose=0.
The tests we have provided are very minimal and are meant as a starting point
for you to learn about Criterion, not to fully test your homework. You may write
your own additional tests in tests/basecode_tests.c, or in additional source
files in the tests directory.  However, this is not required for this assignment.
Criterion documentation for writing your own tests can be
found here.
Note that grades are assigned based on the number of our own test cases
(not given to you in advance) that your program passes.
So you should work on the assignments in such a way that whatever you do submit
will function.  Code that is completely broken (this includes code that does
not compile) will not score any points, regardless of how voluminous it might be
or how long you might have spent on it.

Sample Input Files
In the rsrc directory I have placed the file1 and file2 example files
from above, as well as file file1_file2.diff which contains the output
of diff file1 file2.  You can use these as sample input to get started.
You can obviously also easily make your own input using the diff program
available on Linux.

Hand-in instructions
TEST YOUR PROGRAM VIGOROUSLY BEFORE SUBMISSION!
Make sure that you have implemented all the required functions specifed in global.h.
Make sure that you have adhered to the restrictions (no array brackets, no prohibited
header files, no modifications to files that say "DO NOT MODIFY" at the beginning,
no functions other than main() in main.c) set out in this assignment document.
Make sure your directory tree looks basically like it did when you started
(there could possibly be additional files that you added, but the original organization
should be maintained) and that your homework compiles (you should be sure to try compiling
with both make clean all and make clean debug because there are certain errors that can
occur one way but not the other).
This homework's tag is: hw1
$ git submit hw1

🤓 When writing your program try to comment as much as possible. Try to
stay consistent with your formatting. It is much easier for your TA and the
professor to help you if we can figure out what your code does quickly!
