# Pico W BLE Keyboard (GPIO‑Triggered I/K Keys)

This project turns a Raspberry Pi Pico W into a Bluetooth LE keyboard that sends the keys **I** and
**K** when two physical buttons are pressed.  It is based on the official Raspberry Pi / BTstack
`hid_keyboard_demo` example, with minimal modifications to support GPIO input.

This is ideal for simple game controllers, accessibility devices, or any project where physical
switches need to act as keyboard keys on an iPad or other BLE‑capable device.

---

## ✨ Features

- Appears as a BLE keyboard to iPad, iPhone, macOS, Windows, Linux  
- Sends **I** or **K** when GPIO buttons are pressed  
- One keypress per physical press (debounced via edge detection)  
- Uses the official BTstack HID implementation from the Pico SDK  
- No external libraries required  
- Works over BLE (not classic Bluetooth)

---

## 🛠 Hardware Requirements

- Raspberry Pi Pico W  
- 2 × momentary push buttons  
- Wires / breadboard  
- USB cable for flashing (and power)

### Wiring

Each button is wired using the Pico’s internal pull‑ups:

```
3V3 --- internal pull-up  
GPIO ---- button ---- GND
```

Default pins:

| Button | GPIO Pin |
|--------|----------|
| I key  | GP14     |
| K key  | GP15     |

---

## 📁 Project Structure

This repo contains a modified version of:

pico-sdk/lib/btstack/examples/embedded/hid_keyboard_demo.c

Only minimal changes were made:

- Added GPIO setup  
- Added a timer to poll button state  
- Added edge detection to ensure one keypress per press  
- Added a helper to send a single HID keycode  

Everything else remains as in the original demo.

---

## 🧩 Software Requirements

- macOS, Linux, or Windows  
- Pico SDK installed and configured  
- CMake  
- ARM GCC toolchain  

If you haven’t set up the Pico SDK yet, follow the official guide:

https://github.com/raspberrypi/pico-sdk

---

## 🔧 Build Instructions (macOS)

Clone this repo:

git clone <your-repo-url>  
cd pico_ble_keyboard

Create a build directory:

mkdir build  
cd build

Configure the project:

cmake ..

Build it:

make -j4

If successful, you’ll get:

hid_keyboard_demo.uf2

---

## 📥 Flashing the Pico W

1. Hold the **BOOTSEL** button on the Pico W  
2. Plug it into your Mac via USB  
3. Release BOOTSEL  
4. A drive named **RPI-RP2** will appear  
5. Drag‑and‑drop the `.uf2` file onto it  

The Pico will reboot and start advertising as a BLE keyboard.

---

## 📱 Pairing With an iPad

1. Open **Settings → Bluetooth**  
2. Look for **HID Keyboard Demo**  
3. Tap to connect  
4. Press your buttons — you should see **I** and **K** appear in any text field

---

## 🎮 How It Works

A BTstack timer runs every 10 ms and checks the state of the two GPIO pins.

- When a pin transitions from **released → pressed**, the Pico sends a HID report containing the keycode  
- When the pin is released, nothing is sent  
- This ensures **one keypress per physical press**, even if the button is held down or bounces mechanically

Keycodes used:

| Key | HID Code |
|-----|----------|
| I   | 0x0C     |
| K   | 0x0E     |

---

## 🧪 Troubleshooting

### The Pico appears but won’t connect  
Try forgetting the device on your iPad and reconnecting.

### Keys repeat too fast  
Increase the timer interval in the code:

btstack_run_loop_set_timer(&gpio_timer, 20);

### Want different keys?  
Replace the HID codes in the GPIO handler.

---

## 📄 License

This project includes code from the BlueKitchen BTstack HID demo, which is licensed under the permissive terms included in the source file header.

