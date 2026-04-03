#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Inter-task shared state — single-core Zephyr version.
 * Written by Wi-Fi/portal task, read by display thread.
 * volatile prevents compiler caching; shared_sync() is a compiler barrier.
 */

typedef struct {
    volatile char  my_ip[16];
    volatile char  target_ip[16];
    volatile bool  has_ip;
    volatile bool  last_known_state;
    volatile bool  ap_mode;
    volatile bool  wol_sent;
    volatile bool  factory_reset;
} shared_t;

extern shared_t g_shared;
#define SHARED (&g_shared)

static inline void shared_sync(void)
{
    __asm__ volatile("" ::: "memory");
}

#endif /* SHARED_H */
