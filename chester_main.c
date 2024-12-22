#include "chester.h"

int main(int argc, char *argv[])
{
    if (argc < 2) // Check to see if there are the mininum number of arguments typed in by user
    {
        printf("Need test file name");
        return -1;
    }

    // Iniitalize the test file anem and initialize variables if parsing is needed
    char *test_file = argv[1];
    int sep_tests[MAX_TESTS];
    int tests_count = 0;

    // If there are extra arguments, parse the test numbers
    if (argc > 2)
    {
        for (int i = 2; i < argc; i++)
        {
            sep_tests[tests_count] = atoi(argv[i]);
            tests_count++;
        }
    }
    suite_t *suite = malloc(sizeof(suite_t)); // Allocate size of a suite
    suite_init(suite);                        // Function call that makes every variable in the suite_t struct to null

    // Initialize the suite from the file
    int ret = suite_init_from_file_peg(suite, test_file);
    if (ret != 0)
    {
        printf("Error initializing suite");
        return -1;
    }

    if (tests_count == 0) // tests_count wasnt changed because no parsing needed
    {
        for (int i = 0; i < suite->tests_count; i++) // Initialize suite->tests_torun_count to all tests
        {
            suite->tests_torun[i] = i;
        }
        suite->tests_torun_count = suite->tests_count;
    }
    else
    {
        for (int i = 0; i < tests_count; i++) // Initialize suite->tests_torun_count to only the specified tests
        {
            suite->tests_torun[i] = sep_tests[i];
        }
        suite->tests_torun_count = tests_count;
    }

    printf("%s : running %d / %d tests\n", argv[1], suite->tests_torun_count, suite->tests_count);

    int result = suite_run_tests_singleproc(suite); // Call function that runs the tests
    if (result != 0)                                // If there is any error corresponding to the suite_run_tests_singleproc function.
    {
        printf("ERROR: problems encountered during test run\n");
        suite_dealloc(suite); // Free variables of suite
        free(suite);          // Free suite
        return -1;            // Exit function due to error
    }

    suite_print_results_table(suite); // Call function that prints the result table

    printf("Overall: %d / %d tests passed\n", suite->tests_passed, suite->tests_torun_count);
    suite_dealloc(suite); // Free variables of suite
    free(suite);          // Free suite

    return 0;
}