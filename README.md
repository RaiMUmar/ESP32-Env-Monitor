# ESP32 Environment Monitor Node

Embedded environment monitoring node built on ESP32 in C++ using the Wokwi online hardware simulator.

The firmware reads temperature, humidity, and light level, displays the data on an I2C OLED, and raises an audible alarm when thresholds are exceeded. The project is structured with C++ classes to keep sensor, display, and alarm logic modular and testable.

## Hardware (simulated in Wokwi)

- ESP32 DevKit
- DHT22 temperature & humidity sensor (GPIO 15)
- Analog light sensor module (AO → GPIO 34)
- 0.96" SSD1306 I2C OLED display (SDA → GPIO 21, SCL → GPIO 22)
- Push button (3V3 → button → GPIO 19, 10kΩ pull-down to GND)
- Buzzer (GPIO 18 → buzzer → GND)

The wiring is described in `diagram.json` and can be viewed directly in Wokwi.

## Features

- Periodic sensor sampling (DHT22 + analog light sensor) without `delay()`
- Threshold-based alarm:
  - Temperature ≥ 30 °C
  - Humidity ≥ 70 %
  - Light level below configurable threshold
- OLED UI with two modes:
  - **Summary**: high-level values + alarm status
  - **Detail**: raw readings and debug info
- Debounced push button toggles between display modes
- Buzzer alarm controlled from alarm logic
- Serial logging of readings for debugging / integration

## Code structure

The firmware is written in Arduino style C++:

- `EnvSensor` class — abstracts the DHT22 + light sensor
- `DisplayManager` class — handles SSD1306 rendering
- `EnvData` struct — groups sensor readings
- `DisplayMode` enum — tracks UI state
- Non blocking main loop using `millis()` with separate timing for:
  - sensor reads
  - display refresh
  - button handling

## Running the project

1. Open the project in Wokwi:

   👉 https://wokwi.com/projects/447745577193043969

2. Click **Run**.
3. Use the DHT22 and light sliders in the simulator to change temperature, humidity, and light.
4. Watch the OLED and serial output update.
5. Press the button to switch between **Summary** and **Detail** screens.

## Future improvements

- JSON/CSV telemetry over UART for integration with logging tools
- Configurable thresholds via serial commands
- Port to pure ESP-IDF with FreeRTOS tasks
