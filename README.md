# SoilMate – Arduino Pro Mini port

Original project: https://makerworld.com/en/models/1342417-soilmate-the-cutest-reminder-to-water-your-plant
(designer: ChromeCraft; license: Standard Digital File License, personal use). The original code is in `code.txt` on the MakerWorld page. It has been reviewed and ported to the Arduino Pro Mini in `src/main.cpp`.

## 🌱 Smart Soil Moisture Sensor with Emoticons (Arduino Pro Mini + LED Matrix)

This fun and functional project uses an Arduino Pro Mini, an 8x8 LED matrix, and a capacitive soil moisture sensor to show your plant's "mood" using simple emoticons. The original version also served a small web page over Wi-Fi; this port has no Wi-Fi, so that part was removed.

### 😄 What it does

- Measures soil moisture every second and averages the last 10 readings.
- Displays a matching face on the LED matrix:
  - 😊 Happy face if moisture is ideal, with an eye-blink face for 5 s every minute and a tongue-out face for 4 s every hour.
  - 😢 Sad face if it's too dry **or** too wet, with an angry face for 5 s every 2 minutes.
  - ❤️ A heart alternating with the happy face for 10 s when the soil stops being dry (you watered it).
- Prints the averaged reading on the serial port every second (used for calibration).

## Hardware (decided)
| Component | Details |
|---|---|
| Microcontroller | Arduino Pro Mini (ATmega328P, **8 MHz** crystal, confirmed from the serial output: a 16 MHz build printed at half the baud rate), the board previously used for the calculator project. An Arduino Micro was the earlier plan. |
| Display | 8x8 matrix with MAX7219 (blue module, IN side on the left, OUT side on the right) |
| Sensor | Capacitive soil moisture sensor (the original one) |
| Clock (v2 only) | DS1302 RTC module (no alarms and no interrupt pin) |
| Power | USB power bank (18650 battery and boost converter were ruled out). The Pro Mini has no USB port, so the 5 V goes to its VCC pin (not RAW). |
| Printing | Bambu Lab A1 mini, PLA |

## Version decision
- **v1 (now):** no DS1302 and no watchdog. Goal: get the sensor and the display working first.
- **v2 (later):** add the DS1302 with a watchdog loop so the display only shows from 11:00 to 12:00, with a single sensor reading at 11:00.

## Wiring

D10, D11, D12 and D13 are reserved for the ISP programmer and carry nothing else, so the programmer can stay connected all the time.

| Signal | Arduino Pro Mini |
|---|---|
| Matrix VCC | VCC (5V) |
| Matrix GND | GND |
| Matrix DIN | D4 |
| Matrix CS | D5 |
| Matrix CLK | D6 |
| Sensor VCC | VCC (5V) |
| Sensor GND | GND |
| Sensor AOUT | A0 |

### Display (IN side, the 5 bare holes without pins)
The module ships with pins soldered on the OUT side (CLK, CS, DOUT, GND, VCC), but **the IN side is the one to wire**. No pin needs to be connected twice.

- The matrix is driven with software (bit-banged) SPI, like the original sketch, so it can sit on any three digital pins: change `DATA_PIN`, `CS_PIN` and `CLK_PIN`. D4, D5 and D6 follow the DIN, CS, CLK order of the module's header.
- Library: MD_MAX72XX (majicdesigns), software SPI constructor: `MD_MAX72XX(hardwareType, 4, 6, 5, 1)` (data, clock, CS, number of modules).
- The hardware type is `FC16_HW`, same as the original. If the face appears mirrored or rotated, try `GENERIC_HW` or `PAROLA_HW`.

### Sensor
- The capacitive sensor runs on 5V and its analog output goes to A0 (change `SENSOR_PIN`).
- Power-saving idea, not done in v1: power it from a digital pin and switch it on only while reading.
- **Thresholds are not the original numbers.** The original sketch reads 0–4095 over 3.3 V and the Pro Mini reads 0–1023 over 5 V, so the original 3000 (dry) and 1500 (wet) were converted by voltage: `raw × 3.3/4095 × 1023/5`, giving **495** and **247**. Scaling only the range (733 and 366) would be wrong: the sensor tops out around 2.5 V, about 510 on the Pro Mini, so it would never read as dry. These are starting points until the sensor is calibrated.

## Build and flash
PlatformIO project with a single source file, `src/main.cpp`. Open the folder in VS Code with the PlatformIO extension and use **Build** (✓) and **Upload** (→) in the status bar, or from a terminal:

```
pio run -t upload      # build and flash
```

### Upload through an Arduino Uno (ISP)
The Pro Mini has no USB port and no bootloader, so every upload goes through an Arduino Uno running the ArduinoISP sketch (built for 115200 baud), on `/dev/cu.usbserial-A5069RR4`; change `upload_port` in `platformio.ini` if that moves.

| Uno (programmer) | Pro Mini |
|---|---|
| D10 | RST |
| D11 | D11 (MOSI) |
| D12 | D12 (MISO) |
| D13 | D13 (SCK) |
| 5V | VCC |
| GND | GND |

- `isp_hold_port.py` stops the Uno from resetting into its own bootloader when the upload starts. A 10 µF capacitor between the Uno's RESET and GND does the same job in hardware.
- A good upload reports `Hardware Version: 2` and `Firmware Version: 1.18` in verbose mode (that is ArduinoISP) and ends with `bytes of flash verified`.
- If it stops with `Device signature = 0x000000`, the Uno's bootloader answered instead of ArduinoISP, or a wire is off. Nothing was written.

### Reading the moisture values
The readings come out of the Pro Mini's TX pin at 115200 baud. The Uno cannot show them, so use a separate USB-serial adapter (currently a CP2102 on `/dev/cu.usbserial-0001`, set as `monitor_port`): adapter RXD → Pro Mini TXO, GND → GND, VCC unconnected. Then `pio device monitor`, or the plug icon in VS Code.

## Printing (A1 mini, PLA)
- Reviewed `SoilMate.stl`: the original plate measures ~346 × 132 × 19 mm, but it is 8 separate parts. The largest is 113 × 56.6 × 15 mm and the tallest is 19 mm. Everything fits on a single A1 mini plate (180 × 180 mm); a simulated layout fit in ~165 × 165 mm.
- In Bambu Studio: choose A1 mini 0.4, the 0.20 mm Standard profile, PLA filament, then press Arrange (A).
- The original design targeted ABS (2 walls, 15% infill). With PLA, use 3–4 walls around the M3 screw areas and, if something fits too tightly, X-Y hole compensation of about +0.1 mm.
- PLA softens at about 55–60 °C: keep it out of direct sunlight.

## Estimated power use (my estimates, measure in practice)
- Pro Mini awake: ~16 mA. Pro Mini asleep: ~3 mA (power LED and regulator). Measure both with a multimeter in series.
- Matrix on: ~12 mA at low brightness (`mx.control(MD_MAX72XX::INTENSITY, 1)`, which is what `BRIGHTNESS` is set to; the original used 3).
- Sensor: ~5 mA while powered.
- **v1 (no clock, no sleep, display always on):** ~33 mA → ~790 mAh/day → ~7.5 days on a 10,000 mAh power bank (~6,000 mAh usable at 5V).
- **v2 (DS1302 + watchdog, display 1 h/day):** ~89 mAh/day → ~65–70 days on the same power bank. The clock and watchdog cost only ~1 mAh/day. This assumes the sensor is switched from a digital pin; left on 5V it adds ~120 mAh/day and cuts this to ~29 days.
- If the v2 display ran from 11:00 to midnight (13 h): ~230 mAh/day → ~26 days.
- **Risk:** many power banks switch off when the load stays below ~50–100 mA. Both v1 and v2 are below that. Look for one with a low-current ("trickle") mode, or use a USB wall charger.

## Notes for v2 (DS1302)
- Module pins: VCC, GND, CLK, DAT, RST (3-wire protocol, not I2C). Suggested library: Rtc by Makuna.
- No alarms or interrupt: the Pro Mini wakes every 8 s with the watchdog, reads the time and goes back to sleep, except at 11:00 (read the sensor, show the face) and at 12:00 (put the MAX7219 in shutdown mode).
- DS1302 drift: about 1 minute or more per month; resync the time every few months.
- A regular CR2032 is fine on DS1302 modules (they usually have no charging circuit).


## License and credits
The design, the face bitmaps and the original sketch are by ChromeCraft and are distributed under the MakerWorld Standard Digital File License (personal use). This repository contains a derived port and grants no rights to the original files, and the original `code.txt`, STL and 3MF files are not included. Check the original license before reusing anything beyond personal use.
