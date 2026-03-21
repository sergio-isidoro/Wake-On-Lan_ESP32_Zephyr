#include "button.h"
#include "wifi.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>

/* Get the button configuration from the Devicetree alias 'sw0' */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
/* Structure to store the GPIO callback information */
static struct gpio_callback button_cb_data;

/**
 * @brief Callback function triggered when the button is pressed.
 */
static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    printf("\nBOOT Button Pressed!\n");
    /* Trigger the Wake-on-LAN Magic Packet transmission defined in wifi.c */
    trigger_wol();
}

/**
 * @brief Initializes the BOOT button hardware and interrupt settings.
 */
void button_init(void) {
    /* Check if the GPIO device is ready for use */
    if (!gpio_is_ready_dt(&button)) {
        printf("Error: BOOT button GPIO is not ready.\n");
        return;
    }

    /* Configure the pin as input (pull-up/active-low is handled via Devicetree overlay) */
    gpio_pin_configure_dt(&button, GPIO_INPUT);
    
    /* Configure the interrupt to trigger on the falling edge (button press) */
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    
    /* Initialize the callback structure with the handler function and pin bitmask */
    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    
    /* Add the callback to the GPIO driver */
    gpio_add_callback(button.port, &button_cb_data);
}