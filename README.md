# Wake-on-LAN for ESP32-C3 (Zephyr RTOS)

*A Zephyr RTOS port and enhancement of the original [Wake-on-LAN_ESP32](https://github.com/sergio-isidoro/Wake-on-LAN_ESP32)*

This project provides a complete, robust, and asynchronous **Wake-on-LAN (WoL)** solution specifically tailored for the **ESP32-C3 SuperMini** using the native **Zephyr RTOS** network stack and a custom UI for 0.42" OLED displays.

---

## ✨ Key Features

* **Zephyr RTOS Native:** Built on Zephyr v4.3+, utilizing native `net_mgmt` and `net_icmp` APIs for reliable, non-blocking network handling.
* **OLED Display UI (SSD1306):** Optimized for ultra-small **0.42" (72x40)** screens.
    * **Partial Refresh Logic:** Only updates changed pixels to prevent flickering and save I2C bandwidth.
    * **Smart IP Filtering:** Displays only the last two octets (e.g., `1.198`) to maximize readability on tiny screens.
    * **Activity Animation:** Real-time refresh indicator (`>>>`) to show the system is alive.
* **Dual Notification System (notify.c):** * **Visual LED Feedback:** Onboard Blue LED (GPIO 8) provides distinct patterns:
        * **2 Blinks (500ms):** Magic Packet successfully sent.
        * **1 Blink (500ms):** Target PC status changed (Online/Offline).
* **Real-time Network Monitor:** Background **ICMP (Ping) monitor** tracks the target PC. It performs 3 initial attempts to handle ARP delays and then checks status every minute.
* **Asynchronous Workqueues:** WoL packet dispatch and Ping checks are offloaded from the ISR to system workqueues, ensuring top-tier stability.

---

## 🛠️ Hardware Requirements

* **Microcontroller:** ESP32-C3 SuperMini.
* **Display:** 0.42" OLED (SSD1306) connected via I2C (SDA: GPIO 5, SCL: GPIO 6).
* **LED:** Internal Blue LED on **GPIO 8**.
* **Button:** Onboard BOOT button on **GPIO 9**.
* **Target PC:** Must support Wake-on-LAN via Ethernet and allow **ICMP Echo Requests (Ping)** through its firewall.

---

## 📂 Project Structure

The project follows a clean, modular architecture:

* `src/main.c`: Orchestrates the initialization of all subsystems.
* `src/wifi.c`: Manages Wi-Fi connectivity, DHCP, ICMP Ping logic, and Magic Packet construction.
* `src/display.c`: Handles the OLED UI, multithreaded rendering, and "Dirty-Check" refresh optimizations.
* `src/notify.c`: Centralized module for LED patterns and UI synchronization.
* `src/button.c`: GPIO interrupt handling for the physical trigger.
* `app.overlay`: Devicetree definitions for the SSD1306 offset (28px) and GPIO mapping.

---

## 🚀 Quick Start

1.  **Configure Network:** Edit `src/wifi.c` with your SSID, Password, and the Target PC's MAC/IP addresses.
2.  **Build:**
    ```bash
    west build -p -b esp32c3_supermini .
    ```
3.  **Flash:**
    ```bash
    west flash
    ```

---

## 🖥️ Display Layout

| Line | Content | Description |
| :--- | :--- | :--- |
| **Top** | `>>>>>>>` | Dynamic activity/refresh animation. |
| **Middle**| `1.198` | Filtered IP address (last two octets). |
| **Bottom**| `ONLINE` | Target PC status (tracked via Ping). |

## Image
![alt text](img/working.gif)

![alt text](img/memory.png)
