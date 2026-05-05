# Smart Doorbell - ESP-IDF Development Guide

This file contains the distilled workflows and commands for developing the Smart Doorbell firmware using the ESP-IDF VS Code Extension. It serves as permanent context for the AI agent.

## Local Development Workflow

This project uses a hybrid workflow. The user manages long-running development servers in dedicated `tmux` panes, while the AI agent handles implementation, code generation, and terminal-based verification.

### Active Server Topology (Do Not Start/Stop These)

- **Gateway (Spring Boot):** Port `8080`
- **Dashboard (Next.js 14):** Port `3000`
- **Message Broker (Mosquitto MQTT):** Port `1883`
- **Media Storage (MinIO):** Port `9000`

### AI Agent Rules of Engagement

- **Server Management:** NEVER attempt to run `./mvnw spring-boot:run`, `npm run dev`, or `docker-compose up`. Assume these are persistently running in the background.
- **Maven Wrapper:** Always strictly use `./mvnw` (not `mvn`) for any backend commands to ensure Java version consistency.
- **Safe Verification:** To verify code changes without disrupting the user's running development servers:
  - **Backend:** Run `./mvnw compile` or `./mvnw test`. You are authorized to use `curl` to test active REST endpoints on `localhost:8080`.
  - **Frontend:** Run `npx tsc --noEmit` (to check types) or `npm run lint`. NEVER run `npm run build` unless explicitly requested, as it interferes with the active dev cache.
  - **Firmware:** Use standard ESP-IDF build commands to verify C/C++ compilation.
- **Port Conflicts:** If a verification command fails with `EADDRINUSE`, assume the user already has the service running properly.
- **Restarts:** Rely on Next.js Hot Module Replacement and Spring Boot DevTools. Only instruct the user to manually restart their server panes if you modify `pom.xml`, `tailwind.config.ts`, `next.config.mjs`, or firmware `sdkconfig`..

## Hardware Target

- **MCU:** ESP32-S3-DevKitC-1
- **Peripherals:** OV5640 Camera, INMP441 Mic (I2S RX), MAX98357A Speaker (I2S TX)

### Hardware Constraints & Pinout Rules

- **Power Options:** Recommended power supply is via the USB-to-UART Port or the ESP32-S3 USB Port (both 5V). 5V and 3V3 headers are also available.
- **Reserved Pins:** If the board uses Octal SPI flash/PSRAM (e.g. WROOM-1/1U with N8R8 or WROOM-2), **GPIO35, GPIO36, and GPIO37** are strictly reserved for internal communication and are NOT available for external use.
- **RGB LED:** Driven by **GPIO48** (on initial release) or **GPIO38** (on v1.1). Check board revision if the LED does not respond.
- **Strapping Pins:** GPIO0 (BOOT) is used to enter download mode. Do not pull this low during normal operation/boot.
- **JTAG Pins:** GPIO39 (MTCK), GPIO40 (MTDO), GPIO41 (MTDI), GPIO42 (MTMS).

## VS Code ESP-IDF Extension Workflows

Always prefer using the official VS Code Extension commands (accessible via Command Palette `Ctrl+Shift+P`) over raw `idf.py` terminal commands when operating within the editor.

### 1. Configuration & Setup

- **Set Target:** `ESP-IDF: Set Espressif Device Target` -> `esp32s3`
- **SDK Configuration (menuconfig):** `ESP-IDF: SDK Configuration Editor`. Used to enable PSRAM, configure FreeRTOS tick rate, etc.
  - Project specific defaults are saved in `sdkconfig.defaults`.
- **Partition Table:** `ESP-IDF: Partition Table Editor` (for `partitions.csv`) or `ESP-IDF: Open NVS Partition Editor` (for NVS data). We use a custom `partitions.csv` to allocate a 3MB `app` partition to fit the camera/audio logic.
- **CMakeLists Editor:** Use `ESP-IDF: CMakeLists.txt Editor` to manage components and source files instead of manual edits.
- **Component Manager:** `ESP-IDF: Show ESP Component Registry` (e.g., used to install `espressif/esp32-camera`).

### 2. Build, Flash, and Monitor

- **Build:** `ESP-IDF: Build your Project`.
  - _Tip:_ You can accelerate builds by modifying `idf.ninjaArgs` (e.g., `[-j N]`) in `.vscode/settings.json`.
- **Size Analysis:** `ESP-IDF: Size Analysis of the Binaries`. Enable automatically after build via `idf.enableSizeTaskAfterBuildTask` in `settings.json`. Crucial for ensuring the application fits within the custom 3MB partition.
- **Flash:** `ESP-IDF: Flash your Project`
  - Ensure the correct serial port is selected: `ESP-IDF: Select Port to Use`.
  - Flash methods: `UART` (typical), `JTAG`, or `DFU`.
- **Monitor:** `ESP-IDF: Monitor Device`
  - Essential for viewing `ESP_LOGI`/`ESP_LOGE` output and decoding backtraces on crash.
- **Combined:** `ESP-IDF: Build, Flash and Start a Monitor on your Device`

### 3. Debugging & Analysis

- **Hardware Debugging (JTAG):** OpenOCD is used for JTAG debugging. Configure via `ESP-IDF: Select OpenOCD Board Configuration`. Start via the Run/Debug panel (`F5`).
  - _Note:_ ESP32/ESP32-S3 supports a maximum of **two** hardware breakpoints.
- **Image Viewer:** During a debug session, right-click a raw image buffer (like `camera_fb_t`, `lv_image_dsc_t`, `cv::Mat`, or raw uint8 arrays) and select `View Variable as Image` to preview the camera capture directly in the IDE.
- **Hints Viewer:** Hover over errors in the editor to see helpful resolution hints generated from `hints.yml` during the build process.
- **Application Tracing:** `ESP-IDF: App Trace`. Transfer data/logs to host via JTAG with minimal overhead. (Configure in sdkconfig: `Application Level Tracing`).
- **Heap Tracing:** `ESP-IDF: Heap Trace`. Useful for identifying memory leaks and allocations. Outputs to SystemView.
- **Post-Mortem Debugging:** Set Panic Handler to Core Dump (`UART` or `FLASH`) or GDB Stub in sdkconfig. Use `ESP-IDF: Launch IDF Monitor for Core Dump Mode/GDB Stub Mode`.

### 4. Advanced Workflows

- **Multiple Build Configurations:** Use `ESP-IDF: Open Project Configuration` to define separate profiles (e.g., `dev`, `prod`). This safely overrides `idf.buildPath` and `idf.sdkconfigDefaults` per profile without polluting tracked files.
- **Code Coverage:** Configured via `ESP-IDF: Configure Project SDKConfig for Coverage` (uses GCOV). Build, flash, run the test, and execute `ESP-IDF: Add Editor Coverage` to highlight lines or generate HTML reports.
- **Testing:** Unit tests (using Unity) in a `test/` directory can be discovered via glob patterns (`idf.unitTestFilePattern`).
- **Chat Integration:** ESP-IDF extension supports executing CLI actions via VS Code chat by typing `#espIdfCommands build`, `flash`, `monitor`, etc.
- **Docker / WSL Containers:** `.devcontainer` configuration can be added via `ESP-IDF: Add Docker Container Configuration`. Requires `usbipd-win` on Windows for USB passthrough to WSL/Docker.

## Project Specific Rules

- **Deep Sleep & Interaction Model:** The device relies heavily on deep sleep to maintain battery life. 
  - **Wakeup:** The `app_main` must check `esp_sleep_get_wakeup_cause()`. If woken by `EXT0` (the doorbell button - single press, no hold required), proceed with the event flow. Otherwise, configure wakeup sources and sleep immediately.
  - **Record-and-Send Phase:** Immediately capture a photo, record 5-10 seconds of audio, and HTTP POST them to the Gateway.
  - **Half-Duplex Interaction Phase:** Connect to MQTT and stay awake for a 60-second window listening for the homeowner's "Push to Talk" audio reply. Do not attempt full-duplex AEC.
  - **Sleep:** After the 60-second window expires, power down peripherals and return to deep sleep.
- **Secrets:** Never commit `config.h` (containing WiFi/MQTT credentials). Always use `config.example.h` as the tracked template.
