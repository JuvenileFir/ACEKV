#include "memcached.h"
// #define REALTIME_MAXDELTA 60*60*24*30
#define xisspace(c) isspace((unsigned char)c)

// volatile rel_time_t current_time;

// static rel_time_t realtime(const time_t exptime) {
//     /* no. of seconds in 30 days - largest possible delta exptime */

//     if (exptime == 0) return 0; /* 0 means never expire */

//     if (exptime > REALTIME_MAXDELTA) {
//         /* if item expiration is at/before the server started, give it an
//            expiration time of 1 second after the server started.
//            (because 0 means don't expire).  without this, we'd
//            underflow and wrap around to some large value way in the
//            future, effectively making items expiring in the past
//            really expiring never */
//         if (exptime <= process_started)
//             return (rel_time_t)1;
//         return (rel_time_t)(exptime - process_started);
//     } else {
//         return (rel_time_t)(exptime + current_time);
//     }
// }

bool safe_strtol(const char *str, int32_t *out) {
    assert(out != NULL);
    errno = 0;
    *out = 0;
    char *endptr;
    long l = strtol(str, &endptr, 10);
    if ((errno == ERANGE) || (str == endptr)) {
        return false;
    }

    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *out = l;
        return true;
    }
    return false;
}
