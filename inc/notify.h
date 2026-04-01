#ifndef NOTIFY_H
#define NOTIFY_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Notification types */
typedef enum {
    NOTIFY_WOL_SENT,   /* 2 Blinks (500ms ON) */
    NOTIFY_PING_UPDATE /* 1 Blink  (500ms ON) */
} notify_type_t;

/**
 * @brief Initialize GPIO for the Blue LED and Display synchronization.
 */
void notify_init(void);

/* @brief Notify the user of an event by blinking the blue LED.
 *        The pattern is determined by the type of event:
 *        - NOTIFY_WOL_SENT: 2 blinks (500ms ON, 200ms OFF)
 *        - NOTIFY_PING_UPDATE: 1 blink (500ms ON)
 */
void notify_event(notify_type_t type);

#endif /* NOTIFY_H */