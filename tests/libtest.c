/*  $Id$
**
**  Some utility routines for writing tests.
**
**  Herein are a variety of utility routines for writing tests.  All
**  routines of the form ok*() take a test number and some number of
**  appropriate arguments, check to be sure the results match the expected
**  output using the arguments, and print out something appropriate for that
**  test number.  Other utility routines help in constructing more complex
**  tests.
*/

#include <config.h>
#include <system.h>

#include <sys/wait.h>

#include <tests/libtest.h>
#include <util/util.h>

/* A global buffer into which message_log_buffer stores error messages. */
char *errors = NULL;


/*
**  Initialize things.  Turns on line buffering on stdout and then prints out
**  the number of tests in the test suite.
*/
void
test_init(int count)
{
    if (setvbuf(stdout, NULL, _IOLBF, BUFSIZ) != 0)
        syswarn("cannot set stdout to line buffered");
    printf("%d\n", count);
}


/*
**  Takes a boolean success value and assumes the test passes if that value
**  is true and fails if that value is false.
*/
void
ok(int n, int success)
{
    printf("%sok %d\n", success ? "" : "not ", n);
}


/*
**  Takes an expected integer and a seen integer and assumes the test passes
**  if those two numbers match.
*/
void
ok_int(int n, int wanted, int seen)
{
    if (wanted == seen)
        printf("ok %d\n", n);
    else
        printf("not ok %d\n  wanted: %d\n    seen: %d\n", n, wanted, seen);
}


/*
**  Takes a string and what the string should be, and assumes the test
**  passes if those strings match (using strcmp).
*/
void
ok_string(int n, const char *wanted, const char *seen)
{
    if (wanted == NULL)
        wanted = "(null)";
    if (seen == NULL)
        seen = "(null)";
    if (strcmp(wanted, seen) != 0)
        printf("not ok %d\n  wanted: %s\n    seen: %s\n", n, wanted, seen);
    else
        printf("ok %d\n", n);
}


/*
**  Takes an expected integer and a seen integer and assumes the test passes
**  if those two numbers match.
*/
void
ok_double(int n, double wanted, double seen)
{
    if (wanted == seen)
        printf("ok %d\n", n);
    else
        printf("not ok %d\n  wanted: %g\n    seen: %g\n", n, wanted, seen);
}


/*
**  Skip a test.
*/
void
skip(int n, const char *reason)
{
    printf("ok %d # skip", n);
    if (reason != NULL)
        printf(" - %s", reason);
    putchar('\n');
}


/*
**  Report the same status on the next count tests.
*/
void
ok_block(int n, int count, int status)
{
    int i;

    for (i = 0; i < count; i++)
        ok(n++, status);
}


/*
**  Skip the next count tests.
*/
void
skip_block(int n, int count, const char *reason)
{
    int i;

    for (i = 0; i < count; i++)
        skip(n++, reason);
}


/*
**  An error handler that appends all errors to the errors global.  Used by
**  error_capture.
*/
static void
message_log_buffer(int len, const char *fmt, va_list args, int error UNUSED)
{
    char *message;

    message = xmalloc(len + 1);
    vsnprintf(message, len + 1, fmt, args);
    if (errors == NULL) {
        errors = concat(message, "\n", (char *) 0);
    } else {
        char *new_errors;

        new_errors = concat(errors, message, "\n", (char *) 0);
        free(errors);
        errors = new_errors;
    }
    free(message);
}


/*
**  Turn on the capturing of errors.  Errors will be stored in the global
**  errors variable where they can be checked by the test suite.  Capturing is
**  turned off with errors_uncapture.
*/
void
errors_capture(void)
{
    if (errors != NULL) {
        free(errors);
        errors = NULL;
    }
    message_handlers_warn(1, message_log_buffer);
    message_handlers_notice(1, message_log_buffer);
}


/*
**  Turn off the capturing of errors again.
*/
void
errors_uncapture(void)
{
    message_handlers_warn(1, message_log_stderr);
    message_handlers_notice(1, message_log_stdout);
}


/*
**  Set up our Kerberos access and return the principal to use for the remote
**  remctl connection, NULL if we couldn't initialize things.  We read the
**  principal to use for authentication out of a file and fork kinit to obtain
**  a Kerberos ticket cache.
*/
char *
kerberos_setup(void)
{
    static const char format[] = "kinit -k -K data/test.keytab %s";
    FILE *file;
    char principal[256], *command;
    size_t length;
    int status;

    if (access("../data/test.keytab", F_OK) == 0)
        chdir("..");
    else if (access("tests/data/test.keytab", F_OK) == 0)
        chdir("tests");
    file = fopen("data/test.principal", "r");
    if (file == NULL)
        return NULL;
    if (fgets(principal, sizeof(principal), file) == NULL) {
        fclose(file);
        return NULL;
    }
    fclose(file);
    if (principal[strlen(principal) - 1] != '\n')
        return NULL;
    principal[strlen(principal) - 1] = '\0';
    length = strlen(format) + strlen(principal);
    command = xmalloc(length);
    snprintf(command, length, format, principal);
    putenv((char *) "KRB5CCNAME=data/test.cache");
    putenv((char *) "KRB5_KTNAME=data/test.keytab");
    status = system(command);
    free(command);
    if (status == -1 || WEXITSTATUS(status) != 0)
        return NULL;
    return xstrdup(principal);
}
