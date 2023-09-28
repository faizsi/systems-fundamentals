#include <criterion/criterion.h>
#include <criterion/logging.h>

#include "fliki.h"
#include "global.h"

static char *progname = "bin/fliki";

Test(basecode_suite, validargs_help_test) {
    char *argv[] = {progname, "-h", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int flag = HELP_OPTION;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit (0x%x) not set for -h. Got: %x",
		 flag, opt);
}

Test(basecode_suite, validargs_no_patch_test) {
    char *argv[] = {progname, "-n", "mydiffs", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int exp_opt = NO_PATCH_OPTION;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(opt, exp_opt, "Invalid options settings.  Got: 0x%x | Expected: 0x%x",
		 opt, exp_opt);
}

Test(basecode_suite, validargs_no_flags_test) {
    char *argv[] = {progname, "mydiffs", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int exp_opt = 0;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(opt, exp_opt, "Invalid options settings.  Got: 0x%x | Expected: 0x%x",
		 opt, exp_opt);
    cr_assert_eq(diff_filename, argv[1], "Variable 'diff_filename' was not correctly set");
}

Test(basecode_suite, validargs_error_test) {
    char *argv[] = {progname, "-q", "-n", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int exp_ret = -1;
    int ret = validargs(argc, argv);
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
}

Test(basecode_suite, help_system_test) {
    char *cmd = "bin/fliki -h > /dev/null 2>&1";

    // system is a syscall defined in stdlib.h
    // it takes a shell command as a string and runs it
    // we use WEXITSTATUS to get the return code from the run
    // use 'man 3 system' to find out more
    int return_code = WEXITSTATUS(system(cmd));

    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with 0x%x instead of EXIT_SUCCESS",
		 return_code);
}

Test(basecode_suite, fliki_basic_test) {
    char *cmd = "bin/fliki rsrc/file1_file2.diff < rsrc/file1 > test_output/basic_test.out";
    char *cmp = "cmp test_output/basic_test.out rsrc/file2";

    int return_code = WEXITSTATUS(system(cmd));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with 0x%x instead of EXIT_SUCCESS",
		 return_code);
    return_code = WEXITSTATUS(system(cmp));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program output did not match reference output.");
}


///////////////////////////////////////  BLACKBOX TEST CASES  ///////////////////////////////////////
Test(blackbox_suite, ignore_after_help_test, .timeout = 5) {
    char *cmd = LIMITS"bin/fliki -h -bogus_opt > /dev/null 2>&1";
    int return_code = WEXITSTATUS(system(cmd));
    assert_func_expected_status(return_code, EXIT_SUCCESS, "ignore_after_help_test");
}

/**
 * @brief Check if Errors are printed to stderr and not stdout
 * @details Compare stderr to standard help message, Compare stdout to empty file
 */
Test(blackbox_suite, ensure_errors_to_stderr, .timeout = 5) {
    char *cmd = LIMITS"bin/fliki -n -h > test_output/err_2_stderr.stdout 2>test_output/err_2_stderr.stderr";
    char *cmp_stdout = "cmp rsrc/empty test_output/err_2_stderr.stdout";
    char *cmp_stderr = "cmp rsrc/help_msg test_output/err_2_stderr.stderr";

    int return_code = WEXITSTATUS(system(cmd));
    assert_func_expected_status(return_code, EXIT_FAILURE, "ensure_errors_to_stderr");

    return_code = WEXITSTATUS(system(cmp_stdout));
    if (return_code)
        cr_log_error("STDOUT not empty");
    assert_func_expected_status(return_code, EXIT_SUCCESS, "ensure_errors_to_stderr");

    return_code = WEXITSTATUS(system(cmp_stderr));
    if (return_code)
        cr_log_error("Help message not printed to stderr");
    assert_func_expected_status(return_code, EXIT_SUCCESS, "ensure_errors_to_stderr");
}

/**
 * @brief Diff large JSON
 * @details Tests if diff hunks extracted by randomly modifying a large JSON are correctly applied
 *          test1.json - Original Large JSON
 *          test2.json - Randomly modified (only deletion) test1.json
 *
 */
Test(blackbox_suite, large_json_test, .timeout = 5) {
    char *cmd = LIMITS"bin/fliki rsrc/test2_test1.diff < rsrc/test2.json > test_output/large_test.json";
    char *cmp = "cmp test_output/large_test.json rsrc/test1.json";

    int return_code = WEXITSTATUS(system(cmd));
    assert_func_expected_status(return_code, EXIT_SUCCESS, "large_json_test");

    return_code = WEXITSTATUS(system(cmp));
    if (return_code)
        cr_log_error("Diff applied incorrectly");
    assert_func_expected_status(return_code, EXIT_SUCCESS, "large_json_test");
}

/**
 * @brief Invalid Diff File - Missing '---' separator (Using file1, file2)
 *
 */
Test(blackbox_suite, invalid_diff_separator, .timeout = 5) {
    char *cmd = LIMITS"bin/fliki rsrc/file1_file2_no_sep.diff < rsrc/file1 > test_output/no_sep_test.out";
    char *cmp = "cmp test_output/no_sep_test.out rsrc/empty";

    int return_code = WEXITSTATUS(system(cmd));
    assert_func_expected_status(return_code, EXIT_FAILURE, "invalid_diff_separator");

    return_code = WEXITSTATUS(system(cmp));
    if (return_code)
        cr_log_error("Unexpected output to stdout");
    assert_func_expected_status(return_code, EXIT_SUCCESS, "invalid_diff_separator");
}

/**
 * @brief Invalid Diff File - Missing '<' from beginning (Using file1, file2)
 *
 */
Test(blackbox_suite, invalid_dir_diff, .timeout = 5) {
    char *cmd = LIMITS"bin/fliki rsrc/file1_file2_baddir.diff < rsrc/file1 > test_output/baddir_test.out";
    char *cmp = "cmp test_output/baddir_test.out rsrc/empty";

    int return_code = WEXITSTATUS(system(cmd));
    assert_func_expected_status(return_code, EXIT_FAILURE, "invalid_dir_diff");

    return_code = WEXITSTATUS(system(cmp));
    if (return_code)
        cr_log_error("Unexpected output to stdout");
    assert_func_expected_status(return_code, EXIT_SUCCESS, "invalid_dir_diff");
}

/**
 * @brief Test quiet mode
 * @details Stderr should be empty and exit status should be failure
 */
Test(blackbox_suite, test_quiet_mode, .timeout = 5) {
    char *cmd = LIMITS"bin/fliki -q rsrc/file1_file2_baddir.diff < rsrc/file1 > test_output/quiet_mode.stdout 2>test_output/quiet_mode.stderr";
    char *cmp_stdout = "cmp rsrc/empty test_output/quiet_mode.stdout";
    char *cmp_stderr = "cmp rsrc/empty test_output/quiet_mode.stderr";

    int return_code = WEXITSTATUS(system(cmd));
    assert_func_expected_status(return_code, EXIT_FAILURE, "test_quiet_mode");

    return_code = WEXITSTATUS(system(cmp_stdout));
    if (return_code)
        cr_log_error("STDOUT not empty");
    assert_func_expected_status(return_code, EXIT_SUCCESS, "test_quite_mode");

    return_code = WEXITSTATUS(system(cmp_stderr));
    if (return_code)
        cr_log_error("Help message not printed to stderr");
    assert_func_expected_status(return_code, EXIT_SUCCESS, "test_quiet_mode");
}

/**
 * @brief Test no-output mode
 * @details Stdout should be empty and exit status should be success
 */
Test(blackbox_suite, test_no_out_mode, .timeout = 5) {
    char *cmd = LIMITS"bin/fliki -n rsrc/file1_file2.diff < rsrc/file1 > test_output/no_out_mode.stdout";
    char *cmp_stdout = "cmp rsrc/empty test_output/no_out_mode.stdout";

    int return_code = WEXITSTATUS(system(cmd));
    assert_func_expected_status(return_code, EXIT_SUCCESS, "test_no_out_mode");

    return_code = WEXITSTATUS(system(cmp_stdout));
    if (return_code)
        cr_log_error("STDOUT not empty");
    assert_func_expected_status(return_code, EXIT_SUCCESS, "test_no_out_mode");
}


///////////////////////////////////////////  FLIKI TESTS //////////////////////////////////////
Test(fliki_hunk_show_test, basic_test, .timeout = 5) {
    char *TEST_FILE = "rsrc/file1_file2.diff";

    /**
     * FIRST TEST
     */

    // Creating new hunks to test on.
    HUNK lib_newHunk;
    FILE *lib_diffFile = fopen(TEST_FILE, "r");

    HUNK newHunk;
    FILE *diffFile = fopen(TEST_FILE, "r");

    // Advance program states
    int hunkLen;
    hunkLen = advance_lib_hunk_state(&lib_newHunk, lib_diffFile);
    advance_student_hunk_state(&newHunk, diffFile, hunkLen);

    char *lib_buf = capture_lib_hunk_show_output(&lib_newHunk);
    char *buf = capture_student_hunk_show_output(&newHunk);

    assert_fliki_expected_string(buf, lib_buf, "basic_test_first_hunk");

    /**
     * SECOND TEST
     */

    hunkLen = advance_lib_hunk_state(&lib_newHunk, lib_diffFile);
    advance_student_hunk_state(&newHunk, diffFile, hunkLen);

    lib_buf = capture_lib_hunk_show_output(&lib_newHunk);
    buf = capture_student_hunk_show_output(&newHunk);

    assert_fliki_expected_string(buf, lib_buf, "basic_test_second_hunk");

    /**
     * THIRD TEST
     */

    hunkLen = advance_lib_hunk_state(&lib_newHunk, lib_diffFile);
    advance_student_hunk_state(&newHunk, diffFile, hunkLen);

    lib_buf = capture_lib_hunk_show_output(&lib_newHunk);
    buf = capture_student_hunk_show_output(&newHunk);

    assert_fliki_expected_string(buf, lib_buf, "basic_test_third_hunk");
}

Test(fliki_hunk_show_test, print_middle_test, .timeout = 5) {
    char *TEST_FILE = "rsrc/file1_file2.diff";

    /**
     * FIRST TEST
     */
    HUNK lib_newHunk;
    FILE *lib_diffFile = fopen(TEST_FILE, "r");

    HUNK newHunk;
    FILE *diffFile = fopen(TEST_FILE, "r");

    // Advance program state to end of first hunk
    int hunkLen;
    hunkLen = advance_lib_hunk_state(&lib_newHunk, lib_diffFile);
    advance_student_hunk_state(&newHunk, diffFile, hunkLen);

    // Puts the file descriptor to somewhere random in the second hunk.
    hunk_next(&newHunk, diffFile);
    lib_hunk_next(&lib_newHunk, lib_diffFile);
    for (int i = 0; i < 6 + 1; i++) {
        int newChar = hunk_getc(&newHunk, diffFile);
        int libChar = lib_hunk_getc(&lib_newHunk, lib_diffFile);
    }

    // Capture output
    char *lib_buf = capture_lib_hunk_show_output(&lib_newHunk);
    char *buf = capture_student_hunk_show_output(&newHunk);

    assert_fliki_expected_string(buf, lib_buf, "print_middle_test_in_middle");

    /**
     * READING REST OF HUNK
     */

    // Second HUNK test.
    for (int i = 0; i < 29 + 1; i++) {
        int newChar = hunk_getc(&newHunk, diffFile);
        int libChar = lib_hunk_getc(&lib_newHunk, lib_diffFile);
    }

    lib_buf = capture_lib_hunk_show_output(&lib_newHunk);
    buf = capture_student_hunk_show_output(&newHunk);

    assert_fliki_expected_string(buf, lib_buf, "print_middle_test_normal");
}

Test(fliki_hunk_show_test, elipsis_test, .timeout = 5) {
    char *TEST_FILE = "rsrc/long_text_file.diff";

    // Creating new hunks to test on.
    HUNK lib_newHunk;
    FILE *lib_diffFile = fopen(TEST_FILE, "r");

    HUNK newHunk;
    FILE *diffFile = fopen(TEST_FILE, "r");

    // Advance program states
    int hunkLen = advance_lib_hunk_state(&lib_newHunk, lib_diffFile);
    advance_student_hunk_state(&newHunk, diffFile, hunkLen);

    char *lib_buf = capture_lib_hunk_show_output(&lib_newHunk);
    char *buf = capture_student_hunk_show_output(&newHunk);

    assert_fliki_expected_string(buf, lib_buf, "elipsis_test");
}

//Tests for int patch(FILE *in, FILE *out, FILE *diff)
/**
 * @brief generate output with file1 and file1_file2.diff
 * @details cmp output file2 should be empty
 * @return 0
 *
 *
 */
Test(patch_test_suite, basic_test, .timeout = 5){

    char *IN_FILE = "rsrc/file1";
    char *OUT_FILE = "test_output/patch_basic.out";
    char *DIFF_FILE = "rsrc/file1_file2.diff";
    char *ANSWER_FILE = "rsrc/file2";

    // open in, out, diff files
    FILE *in = fopen(IN_FILE, "r");
    FILE *out = fopen(OUT_FILE, "w");
    FILE *diff = fopen(DIFF_FILE, "r");

    int return_code = patch(in, out, diff);
    fclose(in);
    fclose(out);
    fclose(diff);
    //patch should return 0
    assert_func_expected_status(return_code, 0, "patch returns with error");
    //diff output with answer
    check_patch_output(ANSWER_FILE, OUT_FILE, "basic_test");
}

/**
 * @brief generate output with empty diff file
 * @details return: 0
 *          output file should be the same as input file
 */

Test(patch_test_suite, empty_diff_test, .timeout = 5){

    char *IN_FILE = "rsrc/file1";
    char *OUT_FILE = "test_output/patch.out";
    char *DIFF_FILE = "rsrc/empty";
    char *ANSWER_FILE = "rsrc/file1";

    // open in, out, diff files
    FILE *in = fopen(IN_FILE, "r");
    FILE *out = fopen(OUT_FILE, "w");
    FILE *diff = fopen(DIFF_FILE, "r");

    int return_code = patch(in, out, diff);
    fclose(in);
    fclose(out);
    fclose(diff);

    //patch should return 0 UPDATE- since the doc did not define error status for empty diff file, dont check the return code
    //assert_func_expected_status(return_code, 0, "patch returns with error");


    //diff output with answer
    check_patch_output(ANSWER_FILE, OUT_FILE, "empty_diff_test");

}

/**
 * @brief Invalid Diff File - Missing '<' from beginning (Using file1, file2)
 * @return -1.
 *
 */
Test(patch_test_suite, invalid_diff_test, .timeout = 5){

    char *IN_FILE = "rsrc/file1";
    char *OUT_FILE = "test_output/patch_invalid_diff.out";
    char *DIFF_FILE = "rsrc/file1_file2_baddir.diff";
    char *ANSWER_FILE = "rsrc/empty";

    // open in, out, diff files
    FILE *in = fopen(IN_FILE, "r");
    FILE *out = fopen(OUT_FILE, "w");
    FILE *diff = fopen(DIFF_FILE, "r");

    //execute patch function
    int return_code = patch(in, out, diff);
    fclose(in);
    fclose(out);
    fclose(diff);

    //patch should return -1
    assert_func_expected_status(return_code, -1, "patch should return with error");

    //diff output with answer -  should be empty
    check_patch_output(ANSWER_FILE, OUT_FILE, "invalid_diff_test");

}

/**
 * @brief Invalid Diff File - Invalid Diff File - Missing '---' separator (Using file1, file2)
 * @return -1
 *
 */
Test(patch_test_suite, no_sep_test, .timeout = 5){


    char *IN_FILE = "rsrc/file1";
    char *OUT_FILE = "test_output/patch_no_sep.out";
    char *DIFF_FILE = "rsrc/file1_file2_no_sep.diff";
    char *ANSWER_FILE = "rsrc/empty";

    // open in, out, diff files
    FILE *in = fopen(IN_FILE, "r");
    FILE *out = fopen(OUT_FILE, "w");
    FILE *diff = fopen(DIFF_FILE, "r");



    //execute patch function
    int return_code = patch(in, out, diff);
    fclose(in);
    fclose(out);
    fclose(diff);

    //patch should return -1
    assert_func_expected_status(return_code, -1, "patch should return with error");

    //diff output with answer -  should be empty
    check_patch_output(ANSWER_FILE, OUT_FILE, "no_sep_test");

}

/**
 * @brief invalid diff file - input line number mismatch in change opearation
 * @return -1
 * @param out output should be empty
 *
 *
*/
    
Test(patch_test_suite, wrong_line_number_test, .timeout = 5){


    char *IN_FILE = "rsrc/file1";
    char *OUT_FILE = "test_output/patch_wrong_line_number.out";
    char *DIFF_FILE = "rsrc/file1_file2_wrong_line_number.diff";
    char *ANSWER_FILE = "rsrc/patch_wrong_line_number_sol.out";

    // open in, out, diff files
    FILE *in = fopen(IN_FILE, "r");
    FILE *out = fopen(OUT_FILE, "w");
    FILE *diff = fopen(DIFF_FILE, "r");


    //execute patch function
    int return_code = patch(in, out, diff);
    fclose(in);
    fclose(out);
    fclose(diff);

    //patch should return -1
    assert_func_expected_status(return_code, -1, "patch should return with error");

    //not checking the output here since it is considered a truncated hunk test
    //check_patch_output(ANSWER_FILE, OUT_FILE, "wrong_line_number_test");

}

/**
 * @brief invalid diff file - line range in changing hunk does not match the output lines
 * @return -1
 * @param out output should be empty
 *
 *
*/

Test(patch_test_suite, wrong_line_range_test, .timeout = 5){

    char *IN_FILE = "rsrc/file1";
    char *OUT_FILE = "test_output/patch_wrong_line_range.out";
    char *DIFF_FILE = "rsrc/file1_file2_wrong_line_range.diff";
    char *ANSWER_FILE = "rsrc/empty";

    // open in, out, diff files
    FILE *in = fopen(IN_FILE, "r");
    FILE *out = fopen(OUT_FILE, "w");
    FILE *diff = fopen(DIFF_FILE, "r");

    //execute patch function
    int return_code = patch(in, out, diff);
    fclose(in);
    fclose(out);
    fclose(diff);

    //patch should return -1
    assert_func_expected_status(return_code, -1, "patch should return with error");

    //not checking the output here since it is considered a truncated hunk test
    // check_patch_output(ANSWER_FILE, OUT_FILE, "wrong_line_range_test");

}

/**
 * @brief invalid diff file - deleted line number exceeds input file lines leads to truncated results
 * @return -1
 * @param out output should discard deletion on the third hunk, but apply changes to the first two hunks. Should match rsrc/patch_wrong_delete_line_sol.out
 *
 *
*/

Test(patch_test_suite, wrong_delete_line_test, .timeout = 5){


    char *IN_FILE = "rsrc/file1";
    char *OUT_FILE = "test_output/patch_wrong_delete_line.out";
    char *DIFF_FILE = "rsrc/file1_file2_wrong_delete_line.diff";
    char *ANSWER_FILE = "rsrc/patch_wrong_delete_line_sol.out";

    // open in, out, diff files
    FILE *in = fopen(IN_FILE, "r");
    FILE *out = fopen(OUT_FILE, "w");
    FILE *diff = fopen(DIFF_FILE, "r");

    //execute patch function
    int return_code = patch(in, out, diff);
    fclose(in);
    fclose(out);
    fclose(diff);


    //patch should return -1
    assert_func_expected_status(return_code, -1, "patch should return with error");

    //not checking the output here since it is considered a truncated hunk test
    // check_patch_output(ANSWER_FILE, OUT_FILE, "wrong_delete_line_test - output file should be truncated");

}


/**
 * @brief Valid Diff File - Check if it returns the correct hunks from it.
 *
 */
Test(hunk_next_suite, basic_pass_test, .timeout = 5) {
    HUNK hunk;
    HUNK lib_hunk;
    char *test_file = "rsrc/hunk_test1.diff";
    FILE *diff = fopen(test_file, "r");
    FILE *diff_solution = fopen(test_file, "r");
    for (int i = 0; i < 4; i++){
        int exp_ret = lib_hunk_next(&lib_hunk, diff_solution);
        int ret = hunk_next(&hunk, diff);
        int result = 0;
        if (exp_ret == 0)
            result = compare_hunks(&hunk, &lib_hunk);
        else if (exp_ret == EOF)
            assert_func_expected_status(ret, exp_ret, "not returning EOF. end-of-file or error reading from the input stream not detected");
        else if (exp_ret == ERR)
            assert_func_expected_status(ret, exp_ret, "not returning ERR. The data in the input stream should have not been interpreted as a hunk");
        assert_func_expected_status(result, 0, "hunk was parsed incorrectly");
    }
    fclose(diff_solution);
    fclose(diff);
}

/**
 * @brief Corrupted Diff File - truncated in middle of seconde hunk's header
 *
 */
Test(hunk_next_suite, truncated_diff_test, .timeout = 5) {
    HUNK hunk;
    FILE *diff = fopen("rsrc/hunk_test3.diff", "r");
    hunk_next(&hunk, diff);
    int ret = hunk_next(&hunk, diff);
    assert_func_expected_status(ret, ERR, "not returning ERR. Diff file had malformed hunk header!");

    fclose(diff);
}


/**
 * @brief Invalid Diff File - with invalid command character
 *
 */
Test(hunk_next_suite, invalid_diff_command_test, .timeout = 5) {
    HUNK hunk;
    HUNK lib_hunk;
    char *test_file = "rsrc/hunk_test2.diff";
    FILE *diff = fopen(test_file, "r");
    FILE *diff_solution = fopen(test_file, "r");
    int exp_ret = lib_hunk_next(&lib_hunk, diff_solution);
    int ret = hunk_next(&hunk, diff);
    int result = 0;
    if (exp_ret == 0)
        result = compare_hunks(&hunk, &lib_hunk);
    else if (exp_ret == EOF)
        assert_func_expected_status(ret, exp_ret, "not returning EOF. end-of-file or error reading from the input stream not detected");
    else if (exp_ret == ERR)
        assert_func_expected_status(ret, exp_ret, "not returning ERR. The data in the input stream should have not been interpreted as a hunk");
    assert_func_expected_status(result, 0, "hunk was parsed incorrectly");

    fclose(diff_solution);
    fclose(diff);
}

/**
 * @brief Check if hunk_next() returns the next hunk successfully without finishing parsing of the previous hunk
 *
 */
Test(hunk_next_suite, state_agnostic_hunk_test, .timeout = 5) {
    HUNK hunk;
    HUNK lib_hunk;
    char *test_file = "rsrc/hunk_test1.diff";
    FILE *diff = fopen("rsrc/hunk_test1.diff", "r");
    FILE *diff_solution = fopen(test_file, "r");
    lib_hunk_next(&lib_hunk, diff_solution);
    hunk_next(&hunk, diff);
    lib_hunk_getc(&lib_hunk, diff_solution);
    hunk_getc(&hunk, diff);
    int exp_ret = lib_hunk_next(&lib_hunk, diff_solution);
    int ret = hunk_next(&hunk, diff);
    int result = 0;
    if (exp_ret == 0)
        result = compare_hunks(&hunk, &lib_hunk);
    else if (exp_ret == EOF)
        assert_func_expected_status(ret, exp_ret, "not returning EOF. end-of-file or error reading from the input stream not detected");
    else if (exp_ret == ERR)
        assert_func_expected_status(ret, exp_ret, "not returning ERR. The data in the input stream should have not been interpreted as a hunk");
    assert_func_expected_status(result, 0, "hunk was parsed incorrectly");

    fclose(diff_solution);
    fclose(diff);
}

/**
 * @brief Check if hunk_next() properly interpretes 0a1 hunk
 *
 */
Test(hunk_next_suite, leading_zero_pass_test1, .timeout = 5) {
    HUNK hunk;
    HUNK lib_hunk;
    char *test_file = "rsrc/hunk_test4.diff";
    FILE *diff = fopen(test_file, "r");
    FILE *diff_solution = fopen(test_file, "r");
    int exp_ret = lib_hunk_next(&lib_hunk, diff_solution);
    int ret = hunk_next(&hunk, diff);
    int result = 0;
    if (exp_ret == 0)
        result = compare_hunks(&hunk, &lib_hunk);
    else if (exp_ret == EOF)
        assert_func_expected_status(ret, exp_ret, "not returning EOF. end-of-file or error reading from the input stream not detected");
    else if (exp_ret == ERR)
        assert_func_expected_status(ret, exp_ret, "not returning ERR. The data in the input stream should have not been interpreted as a hunk");
    assert_func_expected_status(result, 0, "hunk was parsed incorrectly");

    fclose(diff_solution);
    fclose(diff);
}

/**
 * @brief Check if hunk_next() properly interpretes 1d0 hunk
 *
 */
Test(hunk_next_suite, leading_zero_pass_test2, .timeout = 5) {
    HUNK hunk;
    HUNK lib_hunk;
    char *test_file = "rsrc/hunk_test5.diff";
    FILE *diff = fopen(test_file, "r");
    FILE *diff_solution = fopen(test_file, "r");
    int exp_ret = lib_hunk_next(&lib_hunk, diff_solution);
    int ret = hunk_next(&hunk, diff);
    int result = 0;
    if (exp_ret == 0)
        result = compare_hunks(&hunk, &lib_hunk);
    else if (exp_ret == EOF)
        assert_func_expected_status(ret, exp_ret, "not returning EOF. end-of-file or error reading from the input stream not detected");
    else if (exp_ret == ERR)
        assert_func_expected_status(ret, exp_ret, "not returning ERR. The data in the input stream should have not been interpreted as a hunk");
    assert_func_expected_status(result, 0, "hunk was parsed incorrectly");

    fclose(diff_solution);
    fclose(diff);
}


/**
 * @brief Check if hunk_next() properly complains about 1c0 hunk
 *        
 * 
 *        This test is commented out, since people can check for this error in
 *        different parts of their program other than hunk_next(), and therefore
 *        this shouldn't be checked in hunk_next_suite
 *
 */
/* Test(hunk_next_suite, leading_zero_fail_test, .timeout = 5) {
    HUNK hunk;
    HUNK lib_hunk;
    char *test_file = "rsrc/hunk_test6.diff";
    FILE *diff = fopen(test_file, "r");
    FILE *diff_solution = fopen(test_file, "r");
    int exp_ret = lib_hunk_next(&lib_hunk, diff_solution);
    int ret = hunk_next(&hunk, diff);
    int result = 0;
    if (exp_ret == 0)
        result = compare_hunks(&hunk, &lib_hunk);
    else if (exp_ret == EOF)
        assert_func_expected_status(ret, exp_ret, "not returning EOF. end-of-file or error reading from the input stream not detected");
    else if (exp_ret == ERR)
        assert_func_expected_status(ret, exp_ret, "not returning ERR. The data in the input stream should have not been interpreted as a hunk");
    assert_func_expected_status(result, 0, "hunk was parsed incorrectly");

    fclose(diff_solution);
    fclose(diff);
}*/


/**
 * @brief Check if hunk_getc() will return a basic line, followed by EOS.
 * @see rsrc/getc.diff, lines 1-2
 */
Test(hunk_getc_suite, basic_line_return_test, .timeout = 5) {
    int hunk_start = 1;  // Line # of hunk header
    char *path = "rsrc/getc.diff";
    HUNK hunk;
    FILE *getc_stream = fopen(path, "r");
    FILE *f_stream = fopen(path, "r");

    skip_lines(getc_stream, hunk_start-1);  // Go to desired hunk
    hunk_next(&hunk, getc_stream);  // Setup hunk struct
    skip_lines(f_stream, hunk_start);  // Go to desired hunk

    // Match entire line.
    int received, expected;
    cr_assert_eq(0, compare_hunk_lines(1, f_stream, getc_stream, &hunk, &expected, &received),
        "Invalid char received for hunk_getc. Got: %i | Expected: %i ", received, expected);

    // Ensure EOS
    received = hunk_getc(&hunk, getc_stream);
    cr_assert_eq(received, EOS,
        "hunk_getc failed to return EOS! Got: %i | Expected: %i (EOS)", received, EOS);

    // Close files.
    fclose(getc_stream);
    fclose(f_stream);
}

/**
 * @brief Check if hunk_getc() will return multiple lines with identifiers inside.
 * @see rsrc/getc.diff, lines 3-6
 */
Test(hunk_getc_suite, multiple_line_return_test, .timeout = 5) {
    int hunk_start = 3;  // Line # of hunk header
    char *path = "rsrc/getc.diff";
    HUNK hunk;
    FILE *getc_stream = fopen(path, "r");
    FILE *f_stream = fopen(path, "r");

    skip_lines(getc_stream, hunk_start-1);  // Go to desired hunk
    hunk_next(&hunk, getc_stream);  // Setup hunk struct
    skip_lines(f_stream, hunk_start);  // Go to desired hunk

    // Match entire hunk.
    int received, expected;
    cr_assert_eq(0, compare_hunk_lines(3, f_stream, getc_stream, &hunk, &expected, &received),
        "Invalid char received for hunk_getc. Got: %i | Expected: %i ", received, expected);

    // Ensure EOS
    received = hunk_getc(&hunk, getc_stream);
    cr_assert_eq(received, EOS,
        "hunk_getc failed to return EOS! Got: %i | Expected: %i (EOS)", received, EOS);

    // Close files.
    fclose(getc_stream);
    fclose(f_stream);
}

/**
 * @brief Check if hunk_getc() will make the appropriate EOS returns in a change-type hunk.
 * @see rsrc/getc.diff, lines 7-10
 */
Test(hunk_getc_suite, basic_change_return_test, .timeout = 5) {
    int hunk_start = 7;
    char *path = "rsrc/getc.diff";
    HUNK hunk;
    FILE *getc_stream = fopen(path, "r");
    FILE *f_stream = fopen(path, "r");

    skip_lines(getc_stream, hunk_start-1);  // Go to desired hunk
    hunk_next(&hunk, getc_stream);  // Setup hunk struct
    skip_lines(f_stream, hunk_start);  // Go to desired hunk

    // Match entire line.
    int received, expected;
    cr_assert_eq(0, compare_hunk_lines(1, f_stream, getc_stream, &hunk, &expected, &received),
        "Invalid char received for hunk_getc. Got: %i | Expected: %i ", received, expected);

    // Ensure EOS
    received = hunk_getc(&hunk, getc_stream);
    cr_assert_eq(received, EOS,
        "hunk_getc failed to return EOS! Got: %i | Expected: %i (EOS)", received, EOS);

    // Move to second section of change hunk.
    skip_lines(f_stream, 1);  // Skip '---' line

    // Match entire line.
    cr_assert_eq(0, compare_hunk_lines(1, f_stream, getc_stream, &hunk, &expected, &received),
        "Invalid char received for hunk_getc. Got: %i | Expected: %i ", received, expected);

    // Ensure EOS
    received = hunk_getc(&hunk, getc_stream);
    cr_assert_eq(received, EOS,
        "hunk_getc failed to return EOS! Got: %i | Expected: %i (EOS)", received, EOS);

    // Close files.
    fclose(getc_stream);
    fclose(f_stream);
}

/**
 * @brief Check if hunk_getc() will correctly return ERR when encountering ">" as opposed to "> "
 * @see rsrc/getc.diff, lines 11-12
 */
Test(hunk_getc_suite, basic_error_test, .timeout = 5) {
    int hunk_start = 11;
    char *path = "rsrc/getc.diff";
    HUNK hunk;
    FILE *getc_stream = fopen(path, "r");

    skip_lines(getc_stream, hunk_start-1);  // Go to desired hunk
    hunk_next(&hunk, getc_stream);  // Setup hunk struct

    // Verify ERR
    int received = hunk_getc(&hunk, getc_stream);
    cr_assert_eq(received, ERR,
        "Invalid return received for hunk_getc. Got: %i | Expected: %i (ERR)", received, ERR);

    // Close files
    fclose(getc_stream);
}

/**
 * @brief Check if hunk_getc() will correctly return ERR when encountering the wrong type of
 * brackets on the second part of a change hunk.
 * @see rsrc/getc.diff, lines 21-24
 */
Test(hunk_getc_suite, mismatch_brackets_test, .timeout = 5) {
    int hunk_start = 21;
    char *path = "rsrc/getc.diff";
    HUNK hunk;
    FILE *f_stream = fopen(path, "r");
    FILE *getc_stream = fopen(path, "r");

    skip_lines(getc_stream, hunk_start-1);  // Go to desired hunk
    hunk_next(&hunk, getc_stream);  // Setup hunk struct
    skip_lines(f_stream, hunk_start);  // Go to desired hunk

    // Match first part of hunk
    int received, expected;
    cr_assert_eq(0, compare_hunk_lines(1, f_stream, getc_stream, &hunk, &expected, &received),
        "Invalid char received for hunk_getc. Got: %i | Expected: %i", received, expected);

    // Ensure EOS
    received = hunk_getc(&hunk, getc_stream);
    cr_assert_eq(received, EOS,
        "hunk_getc failed to return EOS! Got: %i | Expected: %i (EOS)", received, EOS);

    // Verify ERR
    received = hunk_getc(&hunk, getc_stream);
    cr_assert_eq(received, ERR,
        "Invalid return received for hunk_getc. Got: %i | Expected: %i (ERR)", received, ERR);

    // Close files
    fclose(getc_stream);
    fclose(f_stream);
}

/**
 * @brief Check if hunk_getc() will correctly return ERR when encountering EOF in the middle of a hunk.
 * @see rsrc/getc.diff, lines 36-37
 */
Test(hunk_getc_suite, eof_mid_hunk_test, .timeout = 5) {
    int hunk_start = 36;
    char *path = "rsrc/getc.diff";
    HUNK hunk;
    FILE *f_stream = fopen(path, "r");
    FILE *getc_stream = fopen(path, "r");

    skip_lines(getc_stream, hunk_start-1);  // Go to desired hunk
    hunk_next(&hunk, getc_stream);  // Setup hunk struct
    skip_lines(f_stream, hunk_start);  // Go to desired hunk

    // Match first part of hunk
    int received, expected;
    cr_assert_eq(-1, compare_hunk_lines(1, f_stream, getc_stream, &hunk, &expected, &received),
        "Invalid char received for hunk_getc. Got: %i | Expected: %i (ERR)", received, ERR);

    // Close files
    fclose(getc_stream);
    fclose(f_stream);
}


///////////////////////////////////////////////////// FILIKI UTILS ///////////////////////////////////////////////////
#define LIMITS "ulimit -f 100; ulimit -t 10; "

static char *progname = "bin/fliki";
/**
 * @brief Assert return status for any function
 *
 */
static void assert_func_expected_status(int status, int expected, const char *caller)
{
        cr_assert_eq(status, expected,
                        "Invalid return for %s. Got: %d | Expected: %d",
                        caller, status, expected);
}

/**
 * @brief Assert return status for any function - Inequality
 *
 */
static void assert_func_unexpected_status(int status, int expected, const char *caller)
{
        cr_assert_neq(status, expected,
                        "Invalid return for %s. Return should not be %d | Got: %d",
                        caller, expected, status);
}

/**
 * @brief Assert expected value for global opts
 *
 */
static void assert_fliki_expected_options(int option, int expected)
{
    cr_assert_eq(option, expected, "Invalid options settings. Got: 0x%x | Expected: 0x%x",
                        option, expected);
}

/**
 * @brief Assert expected pointer is NULL
 *
 */
static void assert_fliki_expected_null_ptr(void *ptr, const char* caller)
{
    cr_assert_null(ptr, "Test Failed: %s returned a non-NULL pointer, Expected NULL", caller);
}

/**
 * @brief Assert expected pointer is not NULL
 *
 */
static void assert_fliki_expected_notnull_ptr(void *ptr, const char* caller)
{
    cr_assert_not_null(ptr, "Test Failed: %s returned NULL pointer, Expected not-NULL", caller);
}

/**
 * @brief Assert expected value for String
 */
static void assert_fliki_expected_string(char *result, char *expected, const char* caller)
{
    cr_assert_null(strcmp(result, expected), "Test Failed: %s", caller);
}


/**
 * @brief Compare two hunks
 */
static int compare_hunk_types(HUNK_TYPE type, HUNK_TYPE lib_type)
{
    int res = 1;
    switch(type) {
        case HUNK_APPEND_TYPE:
            res = (lib_type == HUNK_APPEND_TYPE) ? 1 : 0;
            break;
        case HUNK_DELETE_TYPE:
            res = (lib_type == HUNK_DELETE_TYPE) ? 1 : 0;
            break;
        case HUNK_CHANGE_TYPE:
            res = (lib_type == HUNK_CHANGE_TYPE) ? 1 : 0;
            break;
        default:
            res = 0;
    }
    return res;
}
static int compare_hunks(HUNK *hp1, HUNK *hp2)
{
    int res = 0;
    res =  (hp1->new_start == hp2->new_start) ? 0 : 1;
    res = res | ((hp1->new_end == hp2->new_end) ? 0 : 1);
    res = res | ((hp1->old_start == hp2->old_start) ? 0 : 1);
    res = res | ((hp1->old_end == hp2->old_end) ? 0 : 1);
    res = res | ((hp1->serial == hp2->serial) ? 0 : 1);
    res = res | (compare_hunk_types(hp1->type,hp2->type) ? 0 : 1);
    return res;
}

/**
 * Advances given file pointer past a certain number of '\n's.
 * ex. if line = 2, it will consume data from the fp until
 * it has read 2 '\n' chars, thus you are now on line 3.
 *
 * @return 0 if successful, -1 if EOF was encountered.
 */
static int skip_lines(FILE *fp, int lines) {
    int n = 0;
    int rv;
    while (n < lines) {
        rv = fgetc(fp);
        if (rv == EOF) return -1;
        if (rv == '\n') ++n;
    }
    return 0;
}

/**
 * Advances a given file pointer past a certain number of characters.
 * ex. if chars = 2, it will consume 2 chars from the stream.
 *
 * @return 0 if successful, -1 if EOF was encountered.
 */
static int skip_chars(FILE *fp, int chars) {
    int i = 0;
    while (i++ < chars) {
        if (fgetc(fp) == EOF) return -1;
    }
    return 0;
}

/**
 * Compares the output of each file stream.
 *
 * @param lines_to_compare The amount of lines to compare.
 * @param f_stream This stream will be passed to fgetc() for output.
 *  This should be initially be pointing at the start of the first line *after* the header.
 *  At the start of each line, the first 2 chars from this stream will be ignored (corresponding to "> ").
 * @param getc_stream This stream will be passed to hunk_getc() for output.
 *  This should initially be pointing at the start of the first line *after* the header.
 * @param hunk The hunk structure associated with the lines.
 * @param expected Storage for expected result. Will contain expected char in case of mismatch.
 * @param received Storage for received result. Will contain received char in case of mismatch.
 * @return 0 if the outputs match, -1 if it reached end-of-line but otherwise matched, 1 if they do not match, 2 if there was an error
 * That is, return values <= 0 indicate a success, and > 0 indicate a fail.
 */
static int compare_hunk_lines(int lines_to_compare, FILE *f_stream, FILE *getc_stream, HUNK *hunk, int *expected, int *received) {
    for (int cur_line = 0; cur_line < lines_to_compare; cur_line++) {
        if (skip_chars(f_stream, 2)) return 2;  // Skip "> "
        do {
            *expected = fgetc(f_stream);
            *received = hunk_getc(hunk, getc_stream);
            if (*received == EOF) return 1;  // Prevents infinite loop; hunk_getc() should never return EOF.
            if (*expected != *received)
                return (*expected == EOF && *received == ERR) ? -1 : 1;
        } while (*expected != '\n');
    }
    return 0;
}

/**
 * @brief Advances file pointer and hunk info count number of times forwards
 */
static void advance_student_hunk_state(HUNK *hp, FILE *fp, unsigned int count) {
    // Reading student code until isLibFunction is reached.
    hunk_next(hp, fp);
    for (int i = 0; i < count; i++) {
        int newChar = hunk_getc(hp, fp);
    }
}

/**
 * @brief Advances file pointer and hunk info until EOS is returned.
 *
 * @return number of calls it took until EOS was obtained.
 */
static int advance_lib_hunk_state(HUNK *hp, FILE *fp) {
    // Using solution code until EOS is reached.
    int i = 0, newChar = 0, firstEOSReached = 0;
    lib_hunk_next(hp, fp);
    do {
        if (newChar == EOS) firstEOSReached = 1;

        newChar = lib_hunk_getc(hp, fp);
        i += 1;
    } while(newChar != EOS || (!firstEOSReached && hp->type == HUNK_CHANGE_TYPE));

    return i;
}

/**
 * @brief Captures output of the lib_hunk_show() function.
 */
static char *capture_lib_hunk_show_output(HUNK *hp) {
    // Creating output chunk from open_memstream and checking solution.
    char *lib_buf;
    size_t lib_len;
    FILE *lib_ptr = open_memstream(&lib_buf, &lib_len);
    lib_hunk_show(hp, lib_ptr);
    fclose(lib_ptr);

    return lib_buf;
}

/**
 * @brief Captures output of the student's hunk_show() function.
 */
static char *capture_student_hunk_show_output(HUNK *hp) {
    // Creating output chunk from open_memstream and checking solution.
    char *buf;
    size_t len;
    FILE *ptr = open_memstream(&buf, &len);
    hunk_show(hp, ptr);
    fclose(ptr);

    return buf;
}

/**
 * @brief comnpare answer with student's outputfile
 */

static void check_patch_output(char *answer, char *out, char* testname){
    //diff output
    char* cmp;
    cmp = malloc(strlen(answer)+strlen(out)+1+5); //allocate 5 space for  "cmp " and " "

    strcpy(cmp, "cmp "); /* copy name into the new var */
    strcat(cmp, answer);
    strcat(cmp, " ");
    strcat(cmp, out);
    //char *cmp = "cmp rsrc/file2 test_output/patch_basic.out";
    //fprintf(stdout, "in defined func : %s", cmp);
    int return_code = WEXITSTATUS(system(cmp));
    //fprintf(stdout, " return code: %d\n", return_code);
    if (return_code)
        cr_log_error("patch applied incorrectly");
    assert_func_expected_status(return_code, EXIT_SUCCESS, testname);

    free(cmp);

}
#endif


///////////////////////////////////////////////////// VALID ARGS ////////////////////////////////////////
Test(validargs_suite, no_diff_file_given, .timeout = 5) {
    char *argv[] = {progname, "-n", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    int opt = global_options;
    int exp_opt = 0;

    assert_func_expected_status(ret, exp_ret, "no_diff_file_given");
    //assert_fliki_expected_options(opt, exp_opt);
    //assert_fliki_expected_null_ptr(diff_filename, "no_diff_file_given");
}

/**
 * @brief No Flags Nor Diff Provided
 *
 */
Test(validargs_suite, no_diff_file_or_flag_given, .timeout = 5) {
    char *argv[] = {progname, NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    int opt = global_options;
    int exp_opt = 0;

    assert_func_expected_status(ret, exp_ret, "no_diff_file_or_flag_given");
    //assert_fliki_expected_options(opt, exp_opt);
    //assert_fliki_expected_null_ptr(diff_filename, "no_diff_file_or_flag_given");
}


/**
 * @brief Too many flags provided
 *
 */
Test(validargs_suite, too_many_flags, .timeout = 5) {
    char *argv[] = {progname, "-n", "-h", "-q", "-p", "-r", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    int opt = global_options;
    int exp_opt = 0;

    assert_func_expected_status(ret, exp_ret, "too_many_flags");
    //assert_fliki_expected_options(opt, exp_opt);
    //assert_fliki_expected_null_ptr(diff_filename, "too_many_flags");
}

/**
 * @brief Incorrect ordering of flags
 *
 */
Test(validargs_suite, incorrect_input_flag_order, .timeout = 5) {
    char *argv[] = {progname, "-q", "-h", "-n", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    int opt = global_options;
    int exp_opt = 0;

    assert_func_expected_status(ret, exp_ret, "incorrect_input_flag_order");
    //assert_fliki_expected_options(opt, exp_opt);
    //assert_fliki_expected_null_ptr(diff_filename, "incorrect_input_flag_order");
}


/**
 * @brief 1st least significant bit is 1 if '-h' is set
 *
 */
Test(validargs_suite, first_least_significant_bit_if_h_set, .timeout = 5) {
    char *argv[] = {progname, "-h",  NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options & (1 << 0);
    int exp_opt = 1;

    assert_func_expected_status(ret, exp_ret, "first_least_significant_bit_if_h_set");
    assert_fliki_expected_options(opt, exp_opt);
    assert_fliki_expected_null_ptr(diff_filename, "first_least_significant_bit_if_h_set");
}

/**
 * @brief Correct setting of diff_filename when no flags
 *
 */
Test(validargs_suite, diff_filename_no_flags, .timeout = 5) {
    char *argv[] = {progname, "foobar", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int exp_opt = 0;

    assert_func_expected_status(ret, exp_ret, "diff_filename_no_flags");
    assert_fliki_expected_options(opt, exp_opt);
    cr_assert_eq(diff_filename, argv[1], "Variable diff_filename was not properly set");
}

/**
 * @brief Improper modification of global_options or diff_filename on error
 *
 */
Test(validargs_suite, options_modified_on_error, .timeout = 5) {
    char *argv[] = {progname, "-n", "-x", "foobar", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    int opt = global_options;
    int exp_opt = 0;

    assert_func_expected_status(ret, exp_ret, "options_modified_on_error");
    assert_fliki_expected_options(opt, exp_opt);
    assert_fliki_expected_null_ptr(diff_filename, "options_modified_on_error");
}
