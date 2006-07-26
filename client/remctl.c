/*  $Id$
**
**  The client for a "K5 sysctl" - a service for remote execution of 
**  predefined commands.  Access is authenticated via GSSAPI Kerberos 5, 
**  authorized via ACL files.
**
**  Originally written by Anton Ushakov <antonu@stanford.edu>
**  Extensive modifications by Russ Allbery <rra@stanford.edu>
**  Copyright 2002, 2003, 2004, 2005, 2006
**      Board of Trustees, Leland Stanford Jr. University
**
**  See README for licensing terms.
*/

#include <config.h>
#include <system.h>

#include <ctype.h>
#include <netdb.h>

#include <client/remctl.h>
#include <util/util.h>

/* Usage message. */
static const char usage_message[] = "\
Usage: remctl <options> <host> <type> <service> <parameters>\n\
\n\
Options:\n\
    -d            Debugging level of output\n\
    -h            Display this help\n\
    -p <port>     remctld port (default: 4444)\n\
    -s <service>  remctld service principal (default: host/<host>)\n\
    -v            Display the version of remctl\n";


/*
**  Lowercase a string in place.
*/
static void 
lowercase(char *string)
{
    char *p;

    for (p = string; *p != '\0'; p++)
        *p = tolower(*p);
}


/*
**  Display the usage message for remctl.
*/
static void
usage(int status)
{
    fprintf((status == 0) ? stdout : stderr, "%s", usage_message);
    exit(status);
}


/*
**  Get the responses back from the server, taking appropriate action on each
**  one depending on its type.  Sets the errorcode parameter to the exit
**  status of the remote command, or to 1 if the remote command failed with an
**  error.  Returns true on success, false if some protocol-level error
**  occurred when reading the responses.
*/
static int
process_response(struct remctl *r, int *errorcode)
{
    struct remctl_output *out;

    *errorcode = 0;
    out = remctl_output(r);
    while (out != NULL && out->type != REMCTL_OUT_DONE) {
        switch (out->type) {
        case REMCTL_OUT_OUTPUT:
            if (out->stream == 1)
                fwrite(out->data, out->length, 1, stdout);
            else if (out->stream == 2)
                fwrite(out->data, out->length, 1, stderr);
            else {
                warn("unknown output stream %d", out->stream);
                fwrite(out->data, out->length, 1, stderr);
            }
            break;
        case REMCTL_OUT_ERROR:
            *errorcode = 1;
            fwrite(out->data, out->length, 1, stderr);
            return 1;
        case REMCTL_OUT_STATUS:
            *errorcode = out->status;
            return 1;
        case REMCTL_OUT_DONE:
            break;
        }
        out = remctl_output(r);
    }
    if (out == NULL) {
        die("error reading from server: %s", remctl_error(r));
        return 0;
    } else
        return 1;
}


/*
**  Main routine.  Parse the arguments, open the remctl connection, send the
**  command, and then call process_response.
*/
int
main(int argc, char *argv[])
{
    int option;
    char *server_host;
    struct hostent *hostinfo;
    char *service_name = NULL;
    unsigned short port = 4444;
    struct remctl *r;
    int errorcode = 0;

    /* Set up logging and identity. */
    message_program_name = "remctl";

    /* Parse options.  The + tells GNU getopt to stop option parsing at the
       first non-argument rather than proceeding on to find options anywhere.
       Without this, it's hard to call remote programs that take options.
       Non-GNU getopt will treat the + as a supported option, which is handled
       below. */
    while ((option = getopt(argc, argv, "+dhp:s:v")) != EOF) {
        switch (option) {
        case 'd':
            message_handlers_debug(1, message_log_stderr);
            break;
        case 'h':
            usage(0);
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 's':
            service_name = optarg;
            break;
        case 'v':
            printf("%s\n", PACKAGE_STRING);
            exit(0);
            break;
        case '+':
            fprintf(stderr, "%s: invalid option -- +\n", argv[0]);
        default:
            usage(1);
            break;
        }
    }
    argc -= optind;
    argv += optind;
    if (argc < 3)
        usage(1);
    server_host = *argv++;
    argc--;

    if (service_name == NULL) {
        hostinfo = gethostbyname(server_host);
        if (hostinfo == NULL)
            die("cannot resolve host %s", server_host);
        service_name = xmalloc(strlen("host/") + strlen(hostinfo->h_name) + 1);
        strcpy(service_name, "host/");
        strcat(service_name, hostinfo->h_name);
        lowercase(service_name);
    }

    /* Open connection. */
    r = remctl_new();
    if (r == NULL)
        sysdie("cannot initialize remctl connection");
    if (!remctl_open(r, server_host, port, service_name))
        die("%s", remctl_error(r));

    /* Do the work. */
    if (!remctl_command(r, (const char **) argv, 1))
        die("%s", remctl_error(r));
    if (!process_response(r, &errorcode))
        die("%s", remctl_error(r));

    /* Shut down cleanly. */
    remctl_close(r);
    return errorcode;
}


/*
**  Local variables:
**  mode: c
**  c-basic-offset: 4
**  indent-tabs-mode: nil
**  end:
*/
