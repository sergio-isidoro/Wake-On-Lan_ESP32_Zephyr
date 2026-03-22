#ifndef BUTTON_H
#define BUTTON_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Initializes the BOOT button GPIO and its interrupts.
 */
void button_init(void);
bool button_is_pressed(void);

#endif /* BUTTON_H */