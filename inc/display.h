#ifndef DISPLAY_H
#define DISPLAY_H

/*
 * display_start() — creates and starts the LVGL display thread.
 * Call from main() AFTER setting SHARED->ap_mode.
 */
void display_start(void);

#endif /* DISPLAY_H */
