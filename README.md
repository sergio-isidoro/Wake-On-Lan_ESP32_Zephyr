# Wake On LAN — ESP32 DevKitC (Zephyr RTOS)

A Zephyr RTOS application for the ESP32 DevKitC that sends Wake-on-LAN magic packets to wake a target PC on the local network, with a 1.54" ST7789V TFT display showing live status.

---

## Features

- **Wake-on-LAN** — sends a UDP magic packet (port 9) to wake a target PC via its MAC address
- **Captive Portal** — on first boot (or after factory reset), hosts a Wi-Fi Access Point (`WOL_ESP`) with a web configuration page to enter SSID, password, target MAC, and target IP
- **Persistent config** — credentials stored in NVS flash; survives reboots
- **Ping monitor** — periodically pings the target PC (up to 3 attempts, every 60 s) and updates the display with online/offline status
- **Auto-reconnect** — reconnects to Wi-Fi automatically 3 s after disconnection
- **TFT display** — 240×240 ST7789V via SPI3 (MIPI DBI), driven by LVGL; shows spinner animation, device IP, target PC state, and WoL confirmation
- **Hardware watchdog** — MWDT1 armed at 5 s, fed every 2.5 s from a dedicated thread; resets SoC on hang
- **LED notification** — blue LED blinks on WoL sent (2×) or ping state change (1×)
- **Boot button** — press to send a magic packet immediately

---

## Hardware

| Component | Details |
|---|---|
| Board | ESP32 DevKitC (WROOM-32) |
| Display | 1.54" TFT ST7789V, 240×240, SPI |
| Button | BOOT button (GPIO0) |
| LED | Blue LED (GPIO2) |

### Display wiring (SPI3 / VSPI)

| Display pin | ESP32 GPIO |
|---|---|
| MOSI | GPIO 23 |
| SCLK | GPIO 18 |
| CS | GPIO 5 (SPI3 hw CS) |
| DC | GPIO 21 |
| RST | GPIO 17 |
| BL / VCC | 3.3 V |

---

## Captive Portal

On first boot the device starts in AP mode. Connect to `WOL_ESP` and open `192.168.4.1` in a browser.

### Configuration form

![Portal config form](assets/portal_form.png)

Fill in SSID, password, target PC IP, and MAC address, then press **SAVE**.

### Validation

![Portal validation error](assets/portal_validation.png)

If any field is missing or the IP/MAC format is invalid, an inline error is shown — no page reload.

### Saved

![Portal saved screen](assets/portal_saved.png)

On success the page shows **SAVED! — Rebooting...** and the device restarts in station mode with the saved credentials.

---

## Serial output

Connect a serial monitor at **115200 baud** to observe runtime logs.

### Wi-Fi connecting

![Serial: connecting to Wi-Fi](assets/connecting_wifi.png)

```
[WIFI] Connecting to <SSID>...
[WIFI] Connected!
```

### Wake-on-LAN sent

![Serial: WoL magic packet sent](assets/wol_magic_packet.png)

```
BOOT Button Pressed!
[WIFI] WoL Magic Packet Sent to AA:BB:CC:DD:EE:FF
```

### Factory reset

![Serial: factory reset](assets/factory_reset.png)

```
[WIFI] Connecting to <SSID>...
[SYSTEM] FACTORY RESET ACTIVATED!
```

To trigger a factory reset: hold the BOOT button while the device is attempting to connect to Wi-Fi. The stored credentials are erased and the device reboots into portal mode.

---

## Display layout

```
        X           <- spinner (blue, 32 px) -- animates while running
    Connecting      <- while connecting to Wi-Fi
    ...             <- dot-pulse animation
  192.168.1.x       <- device IP once connected (white, 24 px)
  [ON]  192.168.x   <- target PC state + IP (grey, 14 px)
  >> WoL sent! <<   <- green flash for 1.5 s after magic packet
```

**AP / portal mode:**
```
       WOL
   Access Point
  192.168.4.1
  Configure WiFi
```

---

## Build & flash

```bash
# Install dependencies (first time only)
pip install west
west init -m <this-repo-url> .
west update
west zephyr-export
pip install -r deps/zephyr/scripts/requirements.txt
west blobs fetch hal_espressif

# Build
west build -b esp32_devkitc/esp32/procpu .

# Flash
west flash
```

To erase flash before flashing (factory reset):
```bash
west flash --runner esptool -- --erase-all
```

---

## Project structure

```
Wake_On_Lan/
├── CMakeLists.txt
├── prj.conf
├── boards/
│   ├── esp32_devkitc_procpu.overlay   <- ST7789V SPI3 + pinctrl
│   └── esp32c3_supermini.overlay      <- SSD1306 I2C variant
├── assets/                            <- screenshots for this README
├── inc/
│   ├── display.h
│   ├── wifi.h
│   ├── portal.h
│   ├── portal_html.h
│   ├── storage.h
│   ├── button.h
│   └── notify.h
└── src/
    ├── main.c       <- init, WDT, flow control
    ├── display.c    <- LVGL thread (priority 7, 8 KB stack)
    ├── wifi.c       <- Wi-Fi connect, WoL, ping, reconnect
    ├── portal.c     <- AP mode, DNS redirector, HTTP config server
    ├── storage.c    <- NVS read/write
    ├── button.c     <- GPIO interrupt -> trigger_wol()
    └── notify.c     <- LED blink thread
```

### Thread map

| Thread | Priority | Stack | Role |
|---|---|---|---|
| `main` | 0 | 4 KB | Init, then sleeps forever |
| `display_tid` | 7 | 8 KB | LVGL render loop, 40 FPS cap |
| `blink_tid` | 10 | 512 B | LED blink patterns |
| `wdt_tid` | 14 | 512 B | WDT feed every 2.5 s |
| System workqueue | -- | -- | WoL send, ping, reconnect |

---

## Zephyr version

Tested with **Zephyr 4.3.0** and **zephyr-sdk-0.17.3**.
