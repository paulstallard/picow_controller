# Pico W Controller

This project turns a Raspberry Pi Pico W into a Bluetooth LE keyboard that sends the keys **I** and
**K** when two physical buttons are pressed.  It is based on the official Raspberry Pi / BTstack
`hid_keyboard_demo` example, with minimal modifications to support GPIO input and LED status
feedback.

The controller was built for
[MyShift](https://mywhoosh.com/myshift-mywhooshs-virtual-shifting-explained/) virtual gear shifting
in [MyWhoosh](https://mywhoosh.com/). The two buttons can be mounted on the bike and used to control
gear changes by sending the keyboard shortcuts (I and J) that change the virtual gear. The project
was built to work with the game running on an iPad but should work with any platform that can
connect to a BLE keyboard and uses the same shortcuts.

---

## ✨ Features

- Appears as a BLE keyboard to iPad, iPhone, macOS, Windows, Linux  
- Sends **I** or **K** when GPIO buttons are pressed  
- One keypress per physical press (debounced via edge detection)  
- Onboard LED flashes briefly on each keypress  
- Fast LED blink when **not connected** (pairing mode)  
- Slow LED heartbeat when **connected**  
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

The Pico GPIO pins are configured to use their internal pull-up resistors.
The switches should be wired between the relevant GPIO pin and ground.

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

- Added GPIO setup for two buttons  
- Added a BTstack timer to poll button state  
- Added edge detection to ensure one keypress per press  
- Added a helper to send a single HID keycode  
- Added LED flash on keypress  
- Added LED blink patterns for pairing and connected states  

Everything else remains as in the original demo.

---

## 🧩 Software Requirements

- macOS, Linux, or Windows  
- Pico SDK installed and configured  
- CMake  
- ARM GCC toolchain  

If you haven’t set up the Pico SDK yet, follow the [official guide](https://github.com/raspberrypi/pico-sdk).

---

## 🔧 Build Instructions (macOS)

Clone this repo:

```
git clone git@github.com:paulstallard/picow_controller.git
cd picow_controller
```

Create a build directory, configure and build the project:

```
mkdir build  
cd build
cmake ..
make -j4
```

If successful, you’ll get:

`picow_controller.uf2`

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
2. Look for **PICO Controller**  
3. Tap to connect  
4. Press your buttons — you should see **I** and **K** appear in any text field

---

## 💡 LED Behaviour

The onboard LED provides visual feedback:

### Pairing Mode (Not Connected)
- LED blinks **fast** (200 ms interval)
- Indicates the Pico is advertising and ready to pair

### Connected Mode
- LED blinks **slowly** (1 second interval)
- Indicates an active BLE connection

### Keypress Flash
- LED turns **solid ON** for ~50 ms when a key is sent
- Overrides the blink pattern temporarily
- Blinking resumes automatically

---

## 🎮 How It Works

A BTstack timer runs every 10 ms and checks the state of the two GPIO pins.

- When a pin transitions from **released → pressed**, the Pico sends a HID report containing the keycode  
- When the pin is released, nothing is sent  
- This ensures **one keypress per physical press**, even if the button is held down or bounces mechanically  

A second BTstack timer drives the LED blink pattern based on connection state.

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
Increase the GPIO polling interval in the code.

### Want different keys?  
Replace the HID codes in the GPIO handler.

---

## 📄 License

This project includes code from the BlueKitchen BTstack HID demo, which is licensed under the permissive terms included in the source file header.

