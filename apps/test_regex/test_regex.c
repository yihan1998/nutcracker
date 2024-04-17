#include <stdio.h>
#include <stdlib.h>
#include <regex.h>

int test_regex_init() {
    regex_t regex;
    int result;
    char *pattern = "^[a-zA-Z]+[0-9]*$";  // Regex pattern: one or more letters followed by zero or more numbers
    char *test_string = "Hello123";

    // Compile the regular expression
    result = regcomp(&regex, pattern, REG_EXTENDED);
    if (result) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    // Execute regular expression
    result = regexec(&regex, test_string, 0, NULL, 0);
    if (!result) {
        printf("'%s' matches the pattern '%s'\n", test_string, pattern);
    } else if (result == REG_NOMATCH) {
        printf("'%s' does not match the pattern '%s'\n", test_string, pattern);
    } else {
        char error_message[100];
        regerror(result, &regex, error_message, sizeof(error_message));
        fprintf(stderr, "Regex match failed: %s\n", error_message);
        exit(1);
    }

    // Free memory allocated to the pattern buffer by regcomp
    regfree(&regex);

    return 0;
}