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
| Board | ESP32 DevKitC |
| Display | 1.54" TFT ST7789V, 240×240, SPI |
| Button | BOOT button (GPIO0) |
| LED | Blue LED (GPIO2) |

### Display wiring (SPI3 / VSPI)

| Display pin | ESP32 GPIO |
|---|---|
| MOSI | GPIO 23 |
| SCLK | GPIO 18 |
| CS | GPIO 5 (via SPI3 hw CS) |
| DC | GPIO 21 |
| RST | GPIO 17 |
| BL / VCC | 3.3 V |

---

## Display layout

```
        X           ← spinner (blue, 32 px) — animates while running
    Connecting      ← while connecting to Wi-Fi
    ...             ← dot-pulse animation
  192.168.1.x       ← device IP once connected (white, 24 px)
  [ON]  192.168.x   ← target PC state + IP (grey, 14 px)
  >> WoL sent! <<   ← green flash for 1.5 s after magic packet
```

**AP / portal mode:**
```
       WOL          ← title
   Access Point
  192.168.4.1       ← portal IP
  Configure WiFi
```

---

## First-time setup

1. Flash the firmware (see Build section below)
2. On first boot the device starts in **AP mode** — connect to the Wi-Fi network `WOL_ESP`
3. Open a browser and navigate to `192.168.4.1`
4. Fill in:
   - **SSID** — your Wi-Fi network name
   - **Password** — your Wi-Fi password
   - **Target MAC** — MAC address of the PC to wake (`AA:BB:CC:DD:EE:FF`)
   - **Target IP** — IP address of the PC (used for ping monitoring)
5. Submit — the device saves the config to flash and reboots into station mode

To **factory reset**, erase the flash before flashing:
```bash
west flash --runner esptool -- --erase-all
```

---

## Usage

- **Wake PC** — press the BOOT button; the LED blinks twice and `>> WoL sent! <<` appears on the display
- **Check PC state** — the display shows `[ON]` or `[OFF]` next to the target IP; ping runs every 60 s automatically

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

---

## Project structure

```
Wake_On_Lan/
├── CMakeLists.txt
├── prj.conf
├── boards/
│   ├── esp32_devkitc_procpu.overlay   ← ST7789V SPI3 + pinctrl
│   └── esp32c3_supermini.overlay      ← SSD1306 I2C variant
├── inc/
│   ├── display.h
│   ├── wifi.h
│   ├── portal.h
│   ├── portal_html.h
│   ├── storage.h
│   ├── button.h
│   └── notify.h
└── src/
    ├── main.c       ← init, WDT, flow control
    ├── display.c    ← LVGL thread (priority 7, 8 KB stack)
    ├── wifi.c       ← Wi-Fi connect, WoL, ping, reconnect
    ├── portal.c     ← AP mode, DNS redirector, HTTP config server
    ├── storage.c    ← NVS read/write
    ├── button.c     ← GPIO interrupt → trigger_wol()
    └── notify.c     ← LED blink thread
```

### Thread map

| Thread | Priority | Stack | Role |
|---|---|---|---|
| `main` | 0 | 4 KB | Init, then sleeps forever |
| `display_tid` | 7 | 8 KB | LVGL render loop, 40 FPS cap |
| `blink_tid` | 10 | 512 B | LED blink patterns |
| `wdt_tid` | 14 | 512 B | WDT feed every 2.5 s |
| System workqueue | — | — | WoL send, ping, reconnect |

---

## Zephyr version

Tested with **Zephyr 4.3.0** and **zephyr-sdk-0.17.3**.
