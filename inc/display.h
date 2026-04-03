#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

/* Set backlight brightness 0-100% via LEDC PWM (GPIO15). */
void set_backlight_brightness(uint8_t percent);

#endif /* DISPLAY_H */
