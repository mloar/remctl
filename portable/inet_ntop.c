/*
 * Replacement for a missing inet_ntop.
 *
 * Provides an implementation of inet_ntop that only supports IPv4 addresses
 * for hosts that are missing it.  If you want IPv6 support, you need to have
 * a real inet_ntop function; this function is only provided so that code can
 * call inet_ntop unconditionally without needing to worry about whether the
 * host supports IPv6.
 *
 * The canonical version of this file is maintained in the rra-c-util package,
 * which can be found at <http://www.eyrie.org/~eagle/software/rra-c-util/>.
 *
 * Written by Russ Allbery <rra@stanford.edu>
 *
 * The authors hereby relinquish any claim to any copyright that they may have
 * in this work, whether granted under contract or by operation of law or
 * international treaty, and hereby commit to the public, at large, that they
 * shall not, at any time in the future, seek to enforce any copyright in this
 * work against any person or entity, or prevent any person or entity from
 * copying, publishing, distributing or creating derivative works of this
 * work.
 */

#include <config.h>
#include <portable/system.h>
#include <portable/socket.h>

#include <errno.h>

/* This may already be defined by the system headers. */
#ifndef INET_ADDRSTRLEN
# define INET_ADDRSTRLEN 16
#endif

/* Systems old enough to not support inet_ntop may not have this either. */
#ifndef EAFNOSUPPORT
# define EAFNOSUPPORT EDOM
#endif

/*
 * If we're running the test suite, rename inet_ntop to avoid conflicts with
 * the system version.
 */
#if TESTING
# define inet_ntop test_inet_ntop
const char *test_inet_ntop(int, const void *, char *, socklen_t);
#endif

const char *
inet_ntop(int af, const void *src, char *dst, socklen_t cnt)
{
    const unsigned char *p;

    if (af != AF_INET) {
        socket_set_errno(EAFNOSUPPORT);
        return NULL;
    }
    if (cnt < INET_ADDRSTRLEN) {
        errno = ENOSPC;
        return NULL;
    }
    p = src;
    snprintf(dst, cnt, "%u.%u.%u.%u",
             (unsigned int) (p[0] & 0xff), (unsigned int) (p[1] & 0xff),
             (unsigned int) (p[2] & 0xff), (unsigned int) (p[3] & 0xff));
    return dst;
}
