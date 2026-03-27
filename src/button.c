#include "button.h"
#include "wifi.h"

/** @brief GPIO specification for the BOOT button */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback button_cb_data;

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    printk("\nBOOT Button Pressed!\n");
    trigger_wol();
}

void button_init(void) {
    if (!gpio_is_ready_dt(&button)) {
        printf("Error: BOOT button GPIO is not ready.\n");
        return;
    }

    gpio_pin_configure_dt(&button, GPIO_INPUT);                                 /* Configure the pin as input (pull-up/active-low is handled via Devicetree overlay) */
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);          /* Configure the interrupt to trigger on the falling edge (button press) */
    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));       /* Initialize the callback structure with the handler function and pin bitmask */
    gpio_add_callback(button.port, &button_cb_data);                            /* Add the callback to the GPIO driver */

}

bool button_is_pressed(void) {
    return (gpio_pin_get_dt(&button) == 1);                                     /* Active Low: returns 1 when pressed */
}