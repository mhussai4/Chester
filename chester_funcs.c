// chester_funcs.c: Service functions for chester primarily operating
// upon suite_t structs.

#include "chester.h"


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
///////////////////

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

int main(int argc, char *argv[]);

