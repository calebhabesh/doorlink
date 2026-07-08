# Hardware Bring-up Checklist

This document tracks the bring-up process for the first revision of the Smart Doorbell custom PCB. 

**WARNING: Do not connect the battery or USB power until visual inspections are complete.**

## Rev B Camera Errata And Ordering Gate

Rev A camera errata: the OV5640 FPC connector routed camera `DOVDD` to `+3V3`. The AliExpress `DCXYX-LZTKQJ-5M-339-V1` OV5640 module uses pin 11 as `DOVDD`, and that IO rail must be `+2V8` for this design.

Rev B schematic requirements before any PCB layout update or JLCPCB reorder:

- J3 pin 4 `AVDD` = `+2V8`
- J3 pin 10 `DVDD` = `+1V5`
- J3 pin 11 `DOVDD` = `+2V8`
- J3 pin 24 `AFVDD` = `+2V8`
- R13/R14 SCCB pullups = `+2V8`
- ESP32-driven `CAM_XCLK`, `CAM_RST`, and `CAM_PWDN` pass through resistor dividers before reaching the camera-side FPC pins.
- Camera output signals `Y9..Y2`, `PCLK`, `VSYNC`, and `HREF` remain direct camera-to-ESP32 input nets.

Run these from the repository root before moving to PCB layout:

```bash
python3 scripts/verify-camera-interface.py
kicad-cli sch erc --format report -o /tmp/smart-doorbell-erc.rpt pcb/smart-doorbell/smart-doorbell.kicad_sch
```

## Phase 1: Power & Smoke Test
- [ ] **Visual Inspection:** Check for solder bridges on the hand-soldered ESP32-S3 and ICS-43434. Verify polarity of LDOs and the battery charger.
- [ ] **Short Circuit Check:** Use a multimeter in continuity mode to check for shorts between 3.3V, 2.8V, 1.5V, and GND *before* applying power.
- [ ] **Battery/USB Connection:** Plug in USB (without battery). Verify no excessive heat from components.
- [ ] **Rail Checks:** Measure voltage at test points/pins:
  - Main rail: ~3.3V
  - Camera analog rail: ~2.8V
  - Camera digital core rail: ~1.5V

## Phase 2: Core MCU & Flashing
- [ ] **USB Enumeration:** Verify the ESP32-S3 enumerates on the host PC via native USB-Serial-JTAG (`/dev/ttyACM0` or `/dev/ttyUSB0`).
- [ ] **Boot/Reset:** Press the RESET button and observe the boot log over serial. Verify ESP32 enters download mode if BOOT is held during RESET.
- [ ] **Flash Scaffold:** Flash the basic bring-up firmware scaffold (no Wi-Fi/Camera/Audio logic yet). Verify it boots and logs "Hello World".

## Phase 3: Peripherals & GPIO
- [ ] **Status LED:** Verify GPIO47 toggles the status LED.
- [ ] **Button Wakeup:** Press the doorbell button. Verify the hardware pulls GPIO2 high/low correctly and the ESP32 logs the interrupt/wakeup.
- [ ] **Battery ADC:** Measure battery voltage via multimeter and compare to ESP32 ADC reading on GPIO1.

## Phase 4: I2C & Camera (OV5640)
- [ ] **I2C Scanner:** Run an I2C/SCCB scan on pins SDA=GPIO38, SCL=GPIO39. Verify the OV5640 responds at address `0x3C`.
- [ ] **Camera Init:** Initialize the ESP32-Camera driver. Verify it detects the OV5640 without PWDN/RST errors.
- [ ] **Image Capture:** Capture a low-res (QVGA) JPEG and print the buffer size to ensure DVP data lines (D0-D7, PCLK, VSYNC, HREF) are routing correctly.

## Phase 5: Audio (I2S)
- [ ] **Microphone (ICS-43434):** Initialize I2S RX on pins WS=4, SCK=5, SD=6. Record a 2-second buffer and verify non-zero/non-clipping PCM data.
- [ ] **Speaker (MAX98357A):** Drive AMP_EN (GPIO44) HIGH. Initialize I2S TX on WS=4, SCK=5, DIN=7. Play a simple sine wave tone.

## Phase 6: Integration
- [ ] **Wi-Fi Connect:** Connect to local network.
- [ ] **Upload Test:** Execute a `POST` request to the Pi gateway (`192.168.1.10:8080`) with dummy media payload. Verify successful receipt in the dashboard.
- [ ] **Deep Sleep Current:** Put the ESP32 into deep sleep. Use a multimeter in series with the battery to measure the sleep current. Target is < 100µA.
