#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Inter-task state — single-core version
 *
 * Replaces the dual-core shared memory struct.
 * All fields are written by the Wi-Fi/portal task and read by the
 * display thread. Access is safe because LVGL and the display thread
 * only READ, and Zephyr's cooperative scheduler + atomic booleans
 * are sufficient for these field sizes on Xtensa/RISC-V.
 *
 * volatile is kept so the compiler does not cache reads.
 */

typedef struct {
    /* Wi-Fi / IP state */
    volatile char  my_ip[16];
    volatile char  target_ip[16];
    volatile bool  has_ip;
    volatile bool  last_known_state;   /* true = target PC online */

    /* Display mode */
    volatile bool  ap_mode;            /* true = show portal screen */

    /* Events — set by wifi/button task, cleared by display thread */
    volatile bool  wol_sent;           /* flash "WoL sent!" for 2 s  */
    volatile bool  factory_reset;      /* flash "Factory Reset" for 2 s */
} shared_t;

/* Single global instance — defined in main.c */
extern shared_t g_shared;
#define SHARED (&g_shared)

/* Compiler + memory barrier (no inter-core memw needed on single-core) */
static inline void shared_sync(void)
{
    __asm__ volatile ("" ::: "memory");
}

#endif /* SHARED_H */
