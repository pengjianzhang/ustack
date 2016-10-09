#ifndef ___XX_DEBUG_H
#define ___XX_DEBUG_H

#include <stdio.h>

//#define __DEBUG

#ifdef __DEBUG
#define DPRINTF(x...) printf(x)
#else
#define DPRINTF(x...)
#endif


#ifdef __DEBUG
#define DPRINT_PACKET(pkt) debug_print_packet(pkt)
#else
#define DPRINT_PACKET(pkt)
#endif


/* This abort is more efficient than abort() because it does not mangle the
 * stack and stops at the exact location we need.
 * */
#define ABORT_NOW() (*(int*)0=0)

/* this one is provided for easy code tracing.
 * Usage: TRACE(sess||0, fmt, args...);
 *        TRACE(sess, "");
 * */
#define TRACE(fmt, args...) do {                            \
        fprintf(stderr,                                           \
                "[%s:%d %s] [sess %p(%x)] " fmt "\n",      \
                __FILE__, __LINE__, __FUNCTION__,                 \
                ##args);                                           \
        } while (0)

#endif
