// chester_funcs.c: Service functions for chester primarily operating
// upon suite_t structs.

#include "chester.h"

////////////////////////////////////////////////////////////////////////////////
// PROBLEM 1 Functions
////////////////////////////////////////////////////////////////////////////////

int suite_create_testdir(suite_t *suite)
{
    // Declare a struct to hold the file status information
    struct stat fileStats;

    // Check if the suite or the test directory path is NULL
    if (suite == NULL || suite->testdir == NULL)
    {
        return -1; // Return error if suite or directory path is NULL
    }

    // Check if the directory already exists
    int exist_dir = stat(suite->testdir, &fileStats);

    if (exist_dir == 0) // Directory exists
    {
        if (S_ISDIR(fileStats.st_mode)) // Check if it is a directory
        {
            return 0; // Return 0 if it is a valid directory
        }
        else
        {
            // Print an error message if a non-directory file with the same name exists
            printf("ERROR: Could not create test directory '%s'\n       Non-directory file with that name already exists\n", suite->testdir);
            return -1; // Return error
        }
    }

    // Create the directory
    mkdir(suite->testdir, 0755);
    return 1; // Return 1 to indicate the directory was successfully created
}

// PROBLEM 1: Creates the testing results directory according to the
// name in the suite `testdir` field. If testdir does not exist, it is
// created as directory with permisions of User=read/write/execute
// then returns 1. If testdir already exists and is a directory, does
// nothing and returns 0. If a non-directory file named testdir
// already exists, print an error message and return -1 to indicate
// testing cannot proceed. The error message is:
//
// ERROR: Could not create test directory 'XXX'
//        Non-directory file with that name already exists
//
// with XXX substituted with the value of testdir
//
// CONSTRAINT: This function must be implemented using low-level
// system calls. Use of high-level calls like system("cmd") will be
// reduced to 0 credit. Review system calls like stat() and mkdir()
// for use here. The access() system call may be used but keep in mind
// it does not distinguish between regular files and directories.

int suite_test_set_outfile_name(suite_t *suite, int testnum)
{
    // Declare a character array to hold the output file name
    char out_name[100];

    // Check if the test number is less than 10 to determine formatting
    if (testnum < 10)
    {
        // If the test number is less than 10, format the output file name with a leading zero
        sprintf(out_name, "%s/%s-output-0%d.txt", suite->testdir, suite->prefix, testnum);
    }
    else
    {
        // If the test number is 10 or greater, format the output file name without the leading zero
        sprintf(out_name, "%s/%s-output-%d.txt", suite->testdir, suite->prefix, testnum);
    }

    // Get the test object for the given test number and assign the output file name
    test_t *test = &suite->tests[testnum];
    test->outfile_name = strdup(out_name); // Allocate memory and copy the string to the test's outfile_name

    return 0;
}
// PROBLEM 1: Sets the field `outfile_name` for the numbered
// tests. The filename is constructed according to the pattern
//
// TESTDIR/PREFIX-output-05.txt
//
// with TESTDIR and PREFIX replaced by the testdir and prefix fields
// in the suite and the 05 replaced by the test number. The test
// number is formatted as indicated: printed in a width of 2 with 0
// padding for single-digit test numbers. The sprintf() or snprintf()
// functions are useful to create the string. The string is then
// duplicated into the heap via strdup() and a pointer to it saved in
// `outfile_name`. The file is not created but the name will be used
// when starting a test as output will be redirected into
// outfile_name. This function should always return 0.

int suite_test_create_infile(suite_t *suite, int testnum)
{
    // Get the test struct for the given test number
    test_t *test = &suite->tests[testnum];

    // Check if the test has no input data (i.e., test->input is NULL)
    if (test->input == NULL)
    {
        // If there is no input data, return 0 indicating no file creation needed
        return 0;
    }

    // Declare a character array to hold the input file name
    char in_name[100];

    // Check if the test number is less than 10 to decide the filename format
    if (testnum < 10)
    {
        // Format the input file name with a leading zero if testnum is less than 10
        sprintf(in_name, "%s/%s-input-0%d.txt", suite->testdir, suite->prefix, testnum);
    }
    else
    {
        // Format the input file name without the leading zero for test numbers >= 10
        sprintf(in_name, "%s/%s-input-%d.txt", suite->testdir, suite->prefix, testnum);
    }

    // Assign the generated file name to the test's infile_name property
    test->infile_name = strdup(in_name);

    // Open the input file for writing (create or truncate if it already exists)
    int fd1 = open(test->infile_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd1 == -1)
    {
        // If opening the file fails, print an error message and return -1
        perror("Could not create input file");
        return -1;
    }

    // Write the input data (test->input) to the file
    write(fd1, test->input, strlen(test->input));

    // Close the file descriptor after writing
    close(fd1);

    // Return 0 to indicate success
    return 0;
}
// PROBLEM 1: Creates a file that is used as input for the numbered
// test. The file will contain the contents of the `input` field. If
// that field is NULL, this function immediately returns. Otherwise, a
// file named like
//
//   TESTDIR/PREFIX-input-05.txt
//
// is created with TESTDIR and PREFIX replaced by the `testdir` field
// and `prefix` fields of the suite and the 05 replaced by the test
// number. A copy of this filename is duplicated and retained in the
// `infile_name` field for the test. After opening this file, the
// contents of the `input` field are then written to this file before
// closing the file and returning
// 0. The testing directory is assumed to exist by this function. The
// options associated with the file are to be the following:
// - Open write only
// - Create the file if it does not exist
// - Truncate the file if it does exist
// - Created files have the User Read/Write permission set
// If the function cannot create the input file due to open() failing,
// an error message is printed and -1 is returned; the error message is
// printed using perror() and will appear as:
//
//   Could not create input file : CAUSE
//
// with the portion to the right being added by perror() to show the
// system cause

int suite_test_read_output_actual(suite_t *suite, int testnum)
{
    // Access outfile_name
    char *file_name = suite->tests[testnum].outfile_name;

    // Declare a variable to hold file statistics
    struct stat file_stats;

    // Use the stat system call to retrieve the status of the specified file
    // It fills in file_stats with details about the file (like size, permissions, etc.)
    if (stat(file_name, &file_stats) == -1)
    {
        // If stat fails (file doesn't exist or can't be accessed), print an error message
        perror("Couldn't open file");
        return -1; // Return an error code
    }

    // Retrieve the size of the file from the file stats structure
    ssize_t file_size = file_stats.st_size;

    // Allocate memory to store the contents of the file
    // add 1 extra byte for the null terminator
    char *buffer = malloc(file_size + 1);
    if (buffer == NULL)
    {
        // If memory allocation fails, print an error message and return
        perror("Failed to allocate memory for file contents");
        return -1;
    }

    // Open the file for reading only
    int fd = open(file_name, O_RDONLY);
    if (fd == -1)
    {
        // If the file can't be opened, print an error and return
        perror("Couldn't open file");
        free(buffer); // Free the allocated memory before returning
        return -1;
    }

    // Read the contents of the file into the buffer
    // `bytes_read` stores how many bytes were actually read
    ssize_t bytes_read = read(fd, buffer, file_size);
    if (bytes_read == -1)
    {
        // If the read operation fails, print an error and return
        perror("Couldn't read file");
        close(fd);    // Close the file before returning
        free(buffer); // Free the buffer before returning
        return -1;
    }

    // Null-terminate the buffer to treat it as a string
    buffer[bytes_read] = '\0';

    // Store the buffer into the output_actual field of the test
    suite->tests[testnum].output_actual = buffer;

    // Close the file descriptor
    close(fd);

    // Return the number of bytes read from the file
    return bytes_read;
}
// PROBLEM 1: Reads the contents of the file named in field
// `outfile_name` for the given testnum into heap-allocated space and
// assigns the output_actual field to that space. Uses a combination
// of stat() and read() to efficiently read in the entire contents of
// a file into a malloc()'d block of memory, null terminates it (\0)
// so that the contents may treatd as a valid C string. Returns the
// total number of bytes read from the file on on success (this is
// also the length of the `output_actual` string). If the file could
// not be opened or read, the `output_actual` field is not changed and
// -1 is returned.
//
// CONSTRAINT: This function should perform at most 1 heap allocation;
// use of the realloc() function is barred. System calls like stat()
// MUST be used to determine the amount of memory needed before
// allocation, Failure to do so will lead to loss of credit.

////////////////////////////////////////////////////////////////////////////////
// PROBLEM 2 Functions
////////////////////////////////////////////////////////////////////////////////

int suite_test_start(suite_t *suite, int testnum)
{
    // Set the output file name
    int ret = suite_test_set_outfile_name(suite, testnum);
    if (ret != 0)
    {
        fprintf(stderr, "Error setting outfile_name\n");
        return -1; // Return -1 to indicate error
    }

    // Get the test case for the given testnum
    test_t *test = &suite->tests[testnum];
    test->state = TEST_RUNNING;

    // Check if outfile_name is set
    if (test->outfile_name == NULL)
    {
        fprintf(stderr, "outfile_name is NULL for test %d\n", testnum);
        return -1; // Return error if outfile_name is still NULL
    }

    // Create the input file
    if (test->input != NULL)
    {
        ret = suite_test_create_infile(suite, testnum);
        if (ret != 0)
        {
            fprintf(stderr, "Error creating input file for test %d\n", testnum);
            return -1; // Return error if the input file creation failed
        }
    }

    // Create the child process using fork()
    pid_t child_pid = fork();

    if (child_pid < 0)
    {
        // Fork failed
        perror("fork failed");
        return -1;
    }

    if (child_pid == 0)
    {
        // Child process
        // Set up output redirection (stdout and stderr)
        int out_fd = open(test->outfile_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd == -1)
        {
            exit(TESTFAIL_OUTPUT); // If open fails, exit
        }

        // Redirect stdout and stderr to the output file using dup2
        if (dup2(out_fd, STDOUT_FILENO) == -1 || dup2(out_fd, STDERR_FILENO) == -1)
        {
            fprintf(stderr, "Output redirection failed\n");
            close(out_fd);         // Close the file descriptor before exiting
            exit(TESTFAIL_OUTPUT); // Exit with a specific code for redirection failure
        }

        close(out_fd); // Close the output file descriptor

        // Handle input redirection if infile_name is set
        if (test->infile_name != NULL)
        {
            int in_fd = open(test->infile_name, O_RDONLY); // Open infile
            if (in_fd == -1)
            {
                fprintf(stderr, "Input file open failed: %s\n", test->infile_name);
                exit(TESTFAIL_INPUT); // Exit with a specific code for input failure
            }

            // Redirect stdin to the input file using dup2
            if (dup2(in_fd, STDIN_FILENO) == -1)
            {
                fprintf(stderr, "Input redirection failed\n");
                close(in_fd);         // Close the file descriptor before exiting
                exit(TESTFAIL_INPUT); // Exit with a specific code for redirection failure
            }
            close(in_fd); // Close the input file descriptor (no longer needed)
        }

        // Prepare the argument vector for exec
        char *argv[100];
        int argc = 0;
        int split_ret = split_into_argv(test->program, argv, &argc);
        if (split_ret != 0)
        {
            fprintf(stderr, "Failed to split command into arguments\n");
            exit(TESTFAIL_EXEC); // Exit with a specific code for exec failure
        }

        // Execute the program using execvp
        execvp(argv[0], argv);

        // If execvp fails
        perror("ERROR: test program failed to exec");
        exit(TESTFAIL_EXEC); // Exit with a specific code for exec failure
    }
    else
    {
        // Parent process
        // Set child_pid and update the test structure
        test->child_pid = child_pid;
        return 0; // Return success
    }
}
// PROBLEM 2: Start a child process that will run program in the
// indicated test number. The parent process first sets the
// outfile_name and creates infile_name with the program input. It
// then creates a child process, sets the test field `child_pid` to
// the child process ID and returns 0.
//
// The child sets up output redirection so that the standard out AND
// standard error streams for the child process is channeled into the
// file named in field `outfile_name`. Note that standard out and
// standard error are "merged" so that they both go to the same
// `outfile_name`. This file should have the same options used when
// opening it as described in suite_test_create_infile(). If
// infile_name is non-NULL, input redirection is also set up with
// input coming from the file named in field `infile_name`. Uses the
// split_into_argv() function to create an argv[] array which is
// passed to an exec()-family system call.
//
// Any errors in the child during input redirection setup, output
// redirection setup, or exec()'ing print error messages and cause an
// immediate exit() with an associated error code. These are as
// follows:
//
// | CONDITION            | EXIT WITH CODE         |
// |----------------------+------------------------|
// | Input redirect fail  | exit(TESTFAIL_INPUT);  |
// | Output redirect fail | exit(TESTFAIL_OUTPUT); |
// | Exec failure         | exit(TESTFAIL_EXEC);   |
//
// Since output redirection is being set up, printing error messages
// in the child process becomes unreliable. Instead, the exit_code for
// the child process should be checked for one of the above values to
// determine what happened.
//
// NOTE: When correctly implemented, this function should never return
// in the child process though the compiler may require a `return ??`
// at the end to match the int return type. NOT returning from this
// function in the child is important as if a child manages to return,
// there will now be two instances of chester running with the child
// starting its own series of tests which will not end well...

int suite_test_finish(suite_t *suite, int testnum, int status)
{
    test_t *test = &suite->tests[testnum]; // get corresponding test from parameter

    // 1. Set the actual exit code based on status
    if (WIFEXITED(status))
    {
        // The child process exited normally
        test->exit_code_actual = WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status))
    {
        // The child process was terminated by a signal
        test->exit_code_actual = -WTERMSIG(status); // Store negative signal number
    }
    else
    {
        // Some unexpected status, return an error
        fprintf(stderr, "Unexpected child process status\n");
        return -1;
    }

    //  Read the output from the file if the output file name is set
    if (test->outfile_name != NULL)
    {
        // Open the output file for reading
        int fd = open(test->outfile_name, O_RDONLY);
        if (fd == -1)
        {
            perror("Couldn't open file"); // Print errot message if file could not be opened;
            test->state = TEST_FAILED;    // Set test to FAIL and return
            return -1;
        }

        // Dynamically allocate memory for output_actual
        char buf[1024]; // Temporary buffer for reading output
        ssize_t bytes_read;
        size_t total_bytes_read = 0;

        // Read the file descriptor in chunks until EOF
        while ((bytes_read = read(fd, buf, sizeof(buf))) > 0)
        {
            // Reallocate memory to store the output as we read it
            test->output_actual = realloc(test->output_actual, total_bytes_read + bytes_read + 1); // +1 for null terminator
            if (test->output_actual == NULL)
            {
                perror("Memory allocation failed for output_actual");
                close(fd);
                return -1;
            }

            // Copy the bytes read into output_actual
            memcpy(test->output_actual + total_bytes_read, buf, bytes_read);
            total_bytes_read += bytes_read;
        }

        // Null-terminate the output string
        if (test->output_actual != NULL)
        {
            test->output_actual[total_bytes_read] = '\0';
        }

        // If no bytes were read and output_actual is still NULL, set it to an empty string
        if (total_bytes_read == 0)
        {
            test->output_actual = strdup("");
        }

        // Close the file
        close(fd);
    }

    // If output_expect is not NULL, compare expected and actual outputs
    if (test->output_expect != NULL)
    {
        if (strcmp(test->output_expect, test->output_actual) != 0)
        {
            // Output mismatch: test failed
            test->state = TEST_FAILED;
            return 0;
        }
    }

    // Compare the exit codes (exit_code_expect vs exit_code_actual)
    if (test->exit_code_expect != test->exit_code_actual)
    {
        // Exit code mismatch: test failed
        test->state = TEST_FAILED;
        return 0;
    }

    // If all checks pass, set the state to TEST_PASSED
    test->state = TEST_PASSED;

    // Increment tests_passed for the suite if this test passed
    suite->tests_passed++;

    return 0; // Successfully processed the test
}
// PROBLEM 2
//
// Processes a tests after its child process has completed and
// determines whether the tests passes / fails.
//
// The `status` parameter comes from a wait()-style call and is use to
// set the `exit_code_actual` of the test. `exit_code_actual` is one
// of the following two possibilities:
// - 0 or positive integer: test program exited normally and the exit
//   code/status is stored.
// - Negative integer: The tested program exited abnormally due to
//   being signaled and the negative of the signal number is
//   stored. Ex: child received SIGSEGV=11 so exit_code_actual is -11.
// If `status` indicated neither a normal nor abnormal exit, this
// function prints an error and returns (this case is not tested).
//
// Output produced by the test is read into the `output_actual` field
// using previously written functions.
//
// The test's `state` field is set to one of TEST_PASSED or
// TEST_FAILED. Comparisons are done between the fields:
// - output_expect vs output_actual (strings)
// - exit_code_expect vs exit_code_actual (int)
//
// If there is a mismatch with these, the test has failed and its
// `state` is set to TEST_FAILED. If both sets of fields match, the
// state of the test becomes `TEST_PASSED` and the suite's
// `tests_passed` field is incremented.
//
// Special Case: if output_expect is NULL, there is no expected output
// and comparison to output_actual should be skipped. This covers
// testing cases where a program is being run to examine only whether
// it returns the correct exit code or avoids segfaults.

////////////////////////////////////////////////////////////////////////////////
// PROBLEM 1 Functions
////////////////////////////////////////////////////////////////////////////////

void print_window(FILE *out, char *str, int center, int lrwidth)
{
    int len = strlen(str);        // Get the length of the string
    int start = center - lrwidth; // Calculate the start index
    int end = center + lrwidth;   // Calculate the end index

    // Clamp start and end to be within valid bounds
    if (start < 0)
    {
        start = 0;
    }
    if (end >= len)
    {
        end = len - 1;
    }

    for (int i = start; i <= end; i++)
    {
        fprintf(out, "%c", str[i]); // Print each character of the substring to the file
    }

    fprintf(out, "\n"); // Print a newline at the end
}
// PROBLEM 3
//
// Print part of the string that contains index center to the given
// out file. Print characters in the string between
// [center-lrwidth,center+lrwidth] with the upper bound being
// inclusive. If either the start or stop point is out of bounds,
// truncate the printing: the minimum starting point is index 0, the
// maximum stopping point is the string length.
//
// EXAMPLES:
// char *s = "ABCDEFGHIJKL";
// //         012345678901
// print_window(stdout, s, 4, 3);
// // BCDEFGH
// // 1234567
// print_window(stdout, s, 2, 5);
// // ABCDEFGH
// // 01234567
// print_window(stdout, s, 8, 4);
// // EFGHIJKL
// // 45678901
//
// NOTE: this function is used when creating test results to show
// where expected and actual output differ

int differing_index(char *strA, char *strB)
{
    int i = 0;

    // Iterate through both strings
    while (strA[i] != '\0' && strB[i] != '\0')
    {
        if (strA[i] != strB[i])
        {
            return i; // Return the index where characters differ
        }
        i++;
    }

    // If no differences are found but the strings have different lengths
    if (strA[i] != strB[i])
    {
        return i; // Return the index where one string ends (the length of the shorter string)
    }

    // If strings are identical
    return -1;
}

// PROBLEM 3
//
// Finds the lowest index where different characters appear in strA and
// strB. If the strings are identical except that one is longer than
// the other, the index returned is the length of the shorter
// string. If the strings are identical, returns -1.
//
// EXAMPLES:
// differing_index("01234567","0123x567") -> 4
// differing_index("012345","01234567")   -> 6
// differing_index("012345","01x34567")   -> 2
// differing_index("012345","012345")     -> -1
//
// NOTE: this function is used when creating test results to show
// where expected and actual output differ

int suite_test_make_resultfile(suite_t *suite, int testnum)
{
    if (testnum >= suite->tests_count)
    {
        return -1; // Invalid test number
    }

    test_t *test = &suite->tests[testnum];

    // Construct the result file path
    char resultfile_path[512];
    sprintf(resultfile_path, "%s/%s-result-%02d.md", suite->testdir, suite->prefix, testnum);

    // Store the result file path in the test structure
    test->resultfile_name = strdup(resultfile_path); // Assign the result file name

    // Open the result file for writing
    FILE *out = fopen(resultfile_path, "w");
    if (out == NULL)
    {
        fprintf(stderr, "ERROR: Could not create result file '%s'\n", resultfile_path);
        return -1;
    }

    // Write the test information using write()

    fprintf(out, "# TEST %d: %s (%s)\n", testnum, test->title, (test->state == TEST_PASSED) ? "ok" : "FAIL");

    // DESCRIPTION section
    fprintf(out, "## DESCRIPTION\n%s\n\n", test->description);

    // PROGRAM section
    fprintf(out, "## PROGRAM: %s\n\n", test->program);

    // INPUT section
    if (test->input != NULL)
    {
        fprintf(out, "## INPUT:\n%s\n\n", test->input);
    }
    else
    {
        fprintf(out, "## INPUT: None\n\n");
    }

    // OUTPUT section
    fprintf(out, "## OUTPUT: ");
    if (test->output_expect == NULL)
    {
        fprintf(out, "skipped check\n\n");
    }
    else
    {
        // Check for differences in output
        int diff_index = differing_index(test->output_expect, test->output_actual);
        if (diff_index != -1)
        {
            fprintf(out, "MISMATCH at char position %d\n", diff_index);
            fprintf(out, "### Expect\n");
            print_window(out, test->output_expect, diff_index, TEST_DIFFWIDTH);
            fprintf(out, "### Actual\n");
            print_window(out, test->output_actual, diff_index, TEST_DIFFWIDTH);
        }
        else
        {
            fprintf(out, "ok\n\n");
        }
    }

    // EXIT CODE section
    fprintf(out, "## EXIT CODE: ");
    if (test->exit_code_expect != test->exit_code_actual)
    {
        fprintf(out, "MISMATCH\n- Expect: %d\n- Actual: %d\n", test->exit_code_expect, test->exit_code_actual);
    }
    else
    {
        fprintf(out, "ok\n");
    }
    fprintf(out, "\n");

    // RESULT section
    fprintf(out, "## RESULT: %s\n", (test->state == TEST_PASSED) ? "ok" : "FAIL");

    // Close the file
    fclose(out);

    return 0;
}
// PROBLEM 3
//
// Creates a result file for the given test. The general format is shown in the example below.
//   # TEST 6: wc 1 to 10 (FAIL)              // testnum and test title, print "ok" for passed tests
//   ## DESCRIPTION
//   Checks that wc works with input          // description field of test
//
//   ## PROGRAM: wc                           // program field of test
//
//   ## INPUT:                                // input field of test, "INPUT: None" for NULL input
//   1
//   2
//   3
//   4
//   5
//   6
//   7
//   8
//   9
//   10
//                                            // if output_expect is NULL, print "OUTPUT: skipped check"
//   ## OUTPUT: MISMATCH at char position 3   // results of differing_index() between
//   ### Expect                               // output_expect and output_actual fields
//   10 10 21                                 // output_expect via calls to print_window()
//
//   ### Actual
//   10  9 20                                 // output_actual via calls to print_window()
//                                            // if no MISMATCH in output, prints ## OUTPUT: ok
//
//   ## EXIT CODE: ok                         // MISMATCH if exit_code_expect and actual don't match and
//                                            // prints Expect/Actual values
//   ## RESULT: FAIL                          // "ok" for passed tests
//
// The file to create is named according to the pattern
//
// TESTDIR/PREFIX-result-05.md
//
// with TESTDIR / PREFIX and 05 substituted for the `testdir` and
// `prefix` fields of suite and 05 for the testnum (width 2 and
// 0-padded). Note the use of the .md extension to identify the output
// as Markdown formatted text.
//
// The output file starts with a heading which prints the a heading
// with the testnum and title in it along with ok/FAIL based on the
// `state` of the test. Then 6 sections are printed which are
// 1. DESCRIPTION
// 2. PROGRAM
// 3. INPUT
// 4. OUTPUT (comparing output_expect and output_actual)
// 5. EXIT CODE (comparing exit_code_expect and exit_code_actual)
// 6. RESULT
//
// In the OUTPUT section, if a difference is detected at position N
// via the differing_index() function, then a window around position N
// is printed into the file for both the expected and actual
// output. The window width used is defined in the header via the
// constant TEST_DIFFWIDTH and is passed to print_window() function.
//
// If the output_expect field is NULL, the OUTPUT section header has
// the message "skipped check" printed next to it.
//
// In the EXIT CODE section, if there is a mismatch between the
// expected and actual exit_code, then they are both printed as in:
// ## EXIT CODE: MISMATCH
// - Expect: 0
// - Actual: 1
//
// The final RESULT section prints either ok / FAIL depending on the
// test state.
//
// If the result file cannot be opened/created, this file prints the
// error message
//   ERROR: Could not create result file 'XXX'
// with XXX substituted for the file name and returns -1. Otherwise
// the function returns 0 on successfully creating the resultfile.

////////////////////////////////////////////////////////////////////////////////
// PROBLEM 4 Functions
////////////////////////////////////////////////////////////////////////////////

int suite_run_tests_singleproc(suite_t *suite)
{
    // Create the test directory for the suite
    if (suite_create_testdir(suite) == -1)
    {
        printf("ERROR: Failed to create test directory\n");
        return -1; // Return early if directory creation fails
    }

    // Initialize variables to track progress
    int ret;
    int status;
    suite->tests_passed = 0; // Reset passed count before running tests

    // Print the progress message for starting tests
    printf("Running with single process: ");

    // Loop through all the tests in tests_torun[]
    for (int i = 0; i < suite->tests_torun_count; i++)
    {
        int test_index = suite->tests_torun[i];   // Get the current test number
        test_t *test = &suite->tests[test_index]; // Get the test

        // Call suite_test_start to set up and start the test
        ret = suite_test_start(suite, test_index);
        if (ret != 0)
        {
            // If suite_test_start failed, print error and move to next test
            printf("Error in starting test %d\n", test_index);
            continue;
        }

        // Step 5: Wait for the child process to complete
        ret = waitpid(test->child_pid, &status, 0);
        if (ret == -1)
        {
            perror("waitpid failed"); // If failure, continute to next test
            continue;
        }

        printf("."); // Print dot for progress

        // Read the actual output of the test
        ret = suite_test_read_output_actual(suite, test_index);
        // Figure out which tests passes/failed
        ret = suite_test_finish(suite, test_index, status);

        // Write the result to the result file
        ret = suite_test_make_resultfile(suite, test_index);
        if (ret != 0)
        {
            printf("Error writing result for test %d\n", test_index);
        }
    }

    // Print final status
    printf(" Done\n");

    return 0;
}

// PROBLEM 4
//
// Runs tests in the suite one at time. Before begining the tests,
// creates the testing directory with a call to
// suite_create_testdir().  If the directory cannot be created, this
// function returns -1 without further action.
//
// The tests with indices in the field `tests_torun[]` are run in the
// order that they appear there. This is done in a loop.
// `suite_test_start(..)` is used to start tests and wait()-style
// system calls are used to suspend execution until the child process
// is finished. Additional functions previously written are then used
// to
// - Assign the exit_code for the child
// - Read the actual output into the test struct
// - Set the pass/fail state
// - Produce a results file for the test
//
// Prints the "Running with single process:" and each test that
// completes prints a "." on the screen to give an indication of
// progress. "Done" is printed when all tests complete so that a full
// line which runs 8 tests looks like
//
//    Running with single process: ........ Done
//
// If errors arise such as with waiting for a child process, failures
// with getting the test output, or other items, error messages should
// be printed but the loop should continue. No specific error messages
// are required and no testing is done; error messages are solely to
// aid with debugging problems.

void suite_print_results_table(suite_t *suite)
{
    for (int i = 0; i < suite->tests_torun_count; i++)
    {

        int test_index = suite->tests_torun[i];   // Get the current test number
        test_t *test = &suite->tests[test_index]; // Get the test

        if (suite->tests_torun[i] >= 10) // If test num >= 10, dont add space before the testnum is printed
        {
            printf("%d) %s             :", suite->tests_torun[i], test->title);
        }
        else
        {
            printf(" %d) %s             :", suite->tests_torun[i], test->title);
        }

        if (test->state == TEST_PASSED) // If test passed, print ok
        {
            printf(" ok\n");
        }
        else
        {
            printf(" FAIL -> see %s\n", test->resultfile_name); // If fail, print FAIL followed up by the resultfile_name
        }
    }
    return;
}
// PROBLEM 4
//
// Prints a table of test results formatted like the following.
//
//  0) echo check           : FAIL -> see chester-test/prob1-result-00.txt
//  1) sleep 2s             : ok
//  2) pwd check            : FAIL -> see chester-test/prob1-result-02.txt
//  3) seq check            : ok
//  4) ls check             : FAIL -> see chester-test/prob1-result-04.txt
//  5) ls not there         : ok
//  6) wc 1 to 10           : FAIL -> see chester-test/prob1-result-06.txt
//  7) date runs            : ok
//
// The test number at the beginning of the line is printed with width
// 2 and space padded. The Test title is printed with a width of 20,
// left-aligned using capabilities of printf().  If the test passes,
// the message "ok" is added while if it fails, a FAIL appears and the
// result file associated with the test is indicated. This function
// honors the `tests_torun[]` array and will only print table results
// for tests with indices in this array.

////////////////////////////////////////////////////////////////////////////////
// PROBLEM 5 Functions
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]);

// PROBLEM 5
//
// Defined in the file chester_main.c. Entry point for the Chester
// application which may be invoked with one of the following command
// line forms along with expected output.
//
// >> ./chester tests.md        # RUNS ALL TESTS
// tests.md : running 8 / 8 tests
// Running with single process: ........ Done
//  0) echo check           : FAIL -> see chester-test/prob1-result-00.txt
//  1) sleep 2s             : ok
//  2) pwd check            : FAIL -> see chester-test/prob1-result-02.txt
//  3) seq check            : ok
//  4) ls check             : FAIL -> see chester-test/prob1-result-04.txt
//  5) ls not there         : ok
//  6) wc 1 to 10           : FAIL -> see chester-test/prob1-result-06.txt
//  7) date runs            : ok
// Overall: 4 / 8 tests passed
//
// >> ./chester tests.md 2 4 6  # RUNS ONLY 3 TESTS NUMBERED 2 4 6
// tests.md : running 3 / 8 tests
// Running with single process: ... Done
//  2) pwd check            : FAIL -> see chester-test/prob1-result-02.txt
//  4) ls check             : FAIL -> see chester-test/prob1-result-04.txt
//  6) wc 1 to 10           : FAIL -> see chester-test/prob1-result-06.txt
// Overall: 0 / 3 tests passed
//
// main() parses the indicated input file to create a test suite
// struct. It then determines if all tests or only specified tests
// will be run by analyzing the command line argument structure. The
// `suite.tests_torun[]` and `suite.test_torun_count` fields are set
// according to which tests will be run: either specified tests only
// or 0..tests_count-1 for all tests.
//
// Before running tests, output lines are printed indicating the test
// file and number of tests to be run versus the total number of tests
// in the file. The tests are then run and an output table is produced
// using appropriate functions. The "Overall" line is printed with the
// count of tests passed and that were actually run.
//
// MAKEUP CREDIT: Support the following additional invocations that
// support concurrent test execution.
//
// >> ./chester -max_procs 4 tests.md         # RUN ALL TESTS WITH
// tests.md : running 8 / 8 tests             # 4 CONCURRENT PROCESSES
// Running with 4 processes: ........ Done
//  0) echo check           : FAIL -> see chester-test/prob1-result-00.txt
//  1) sleep 2s             : ok
//  2) pwd check            : FAIL -> see chester-test/prob1-result-02.txt
//  3) seq check            : ok
//  4) ls check             : FAIL -> see chester-test/prob1-result-04.txt
//  5) ls not there         : ok
//  6) wc 1 to 10           : FAIL -> see chester-test/prob1-result-06.txt
//  7) date runs            : ok
// Overall: 4 / 8 tests passed
//
// >> ./chester -max_procs 3 tests.md 2 4 6   # RUN 3 SELECTED TESTS WITH
// tests.md : running 3 / 8 tests             # 3 CONCURRENT PROCESSES
// Running with 3 processes: ... Done
//  2) pwd check            : FAIL -> see chester-test/prob1-result-02.txt
//  4) ls check             : FAIL -> see chester-test/prob1-result-04.txt
//  6) wc 1 to 10           : FAIL -> see chester-test/prob1-result-06.txt
// Overall: 0 / 3 tests passed
//
// Concurrently running processes are run via the associated
// `suite_run_tests_multiproc()` program.  The `-max_procs` command
// line flag sets the `suite.max_procs` field which is used in
// `suite_run_tests_multiproc()` to launch multiple processes to speed
// up test completion.