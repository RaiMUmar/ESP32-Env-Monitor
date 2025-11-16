#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ------------------ Pin configuration ------------------
static const int DHT_PIN       = 15;
static const int DHT_TYPE      = DHT22;
static const int LDR_PIN       = 34;    // ADC
static const int BUTTON_PIN    = 19;
static const int BUZZER_PIN    = 18;
static const int OLED_WIDTH    = 128;
static const int OLED_HEIGHT   = 64;
static const int OLED_ADDR     = 0x3C;
static const int I2C_SDA_PIN   = 21;
static const int I2C_SCL_PIN   = 22;

// Thresholds
static const float TEMP_HIGH_THRESHOLD = 30.0;  // °C
static const float HUMID_HIGH_THRESHOLD = 70.0; // %
static const int   LIGHT_LOW_THRESHOLD  = 1000; // raw ADC (0-4095)

// ------------------ Global objects ------------------
DHT dht(DHT_PIN, DHT_TYPE);
TwoWire I2Cbus = TwoWire(0);
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &I2Cbus, -1);

// Simple struct to hold readings
struct EnvData {
  float temperature;
  float humidity;
  int lightRaw;
};

// Modes for the display
enum class DisplayMode {
  SUMMARY,
  DETAIL
};

// Class: Sensor manager
class EnvSensor {
public:
  EnvSensor(int dhtPin, int ldrPin)
    : dhtSensor(dhtPin, DHT_TYPE), ldrPin(ldrPin) {}

  void begin() {
    dhtSensor.begin();
  }

  EnvData read() {
    EnvData data;
    data.temperature = dhtSensor.readTemperature();
    data.humidity    = dhtSensor.readHumidity();
    data.lightRaw    = analogRead(ldrPin);
    return data;
  }

private:
  DHT dhtSensor;
  int ldrPin;
};

// Class: Display manager
class DisplayManager {
public:
  DisplayManager(Adafruit_SSD1306& disp)
    : display(disp) {}

  bool begin() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
      return false;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();
    return true;
  }

  void showSummary(const EnvData& data, bool alarmOn) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("ENV MONITOR - SUMMARY"));
    display.println();

    display.print(F("Temp: "));
    if (isnan(data.temperature)) display.println(F("ERR"));
    else {
      display.print(data.temperature, 1);
      display.println(F(" C"));
    }

    display.print(F("Hum : "));
    if (isnan(data.humidity)) display.println(F("ERR"));
    else {
      display.print(data.humidity, 1);
      display.println(F(" %"));
    }

    display.print(F("Light: "));
    display.print(data.lightRaw);
    display.println(F(" /4095"));

    display.println();
    display.print(F("Alarm: "));
    display.println(alarmOn ? F("ON") : F("OFF"));

    display.display();
  }

  void showDetail(const EnvData& data) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("ENV MONITOR - DETAIL"));
    display.println();

    display.println(F("Raw readings:"));
    display.print(F("T: "));
    display.println(isnan(data.temperature) ? NAN : data.temperature, 2);
    display.print(F("H: "));
    display.println(isnan(data.humidity) ? NAN : data.humidity, 2);
    display.print(F("L: "));
    display.println(data.lightRaw);

    display.println();
    display.println(F("Press button"));
    display.println(F("to switch mode."));

    display.display();
  }

private:
  Adafruit_SSD1306& display;
};

// ------------------ Globals for logic ------------------
EnvSensor sensor(DHT_PIN, LDR_PIN);
DisplayManager displayMgr(display);

DisplayMode currentMode = DisplayMode::SUMMARY;
bool alarmOn = false;

unsigned long lastReadMillis   = 0;
unsigned long lastDisplayMillis = 0;
const unsigned long READ_INTERVAL_MS    = 2000;
const unsigned long DISPLAY_INTERVAL_MS = 500;

EnvData latestData;

// Debounce for button
unsigned long lastButtonChange = 0;
const unsigned long BUTTON_DEBOUNCE_MS = 200;
int lastButtonState = LOW;

// ------------------ Helper functions ------------------
void updateAlarm(const EnvData& data) {
  bool tempHigh = !isnan(data.temperature) && data.temperature >= TEMP_HIGH_THRESHOLD;
  bool humHigh  = !isnan(data.humidity) && data.humidity >= HUMID_HIGH_THRESHOLD;
  bool lightLow = data.lightRaw <= LIGHT_LOW_THRESHOLD;

  alarmOn = tempHigh || humHigh || lightLow;
  digitalWrite(BUZZER_PIN, alarmOn ? HIGH : LOW);
}

void handleButton() {
  int state = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  if (state != lastButtonState && (now - lastButtonChange) > BUTTON_DEBOUNCE_MS) {
    lastButtonChange = now;
    lastButtonState = state;

    // Button pressed (rising edge)
    if (state == HIGH) {
      if (currentMode == DisplayMode::SUMMARY) {
        currentMode = DisplayMode::DETAIL;
      } else {
        currentMode = DisplayMode::SUMMARY;
      }
    }
  }
}

// ------------------ Arduino setup/loop ------------------
void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT); // External pull-down to GND
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  sensor.begin();

  I2Cbus.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  bool ok = displayMgr.begin();
  if (!ok) {
    Serial.println("OLED init failed!");
  } else {
    Serial.println("System init OK.");
  }
}

void loop() {
  unsigned long now = millis();

  // Periodic sensor read
  if (now - lastReadMillis >= READ_INTERVAL_MS) {
    lastReadMillis = now;
    latestData = sensor.read();
    updateAlarm(latestData);

    Serial.print("T=");
    Serial.print(latestData.temperature);
    Serial.print("C, H=");
    Serial.print(latestData.humidity);
    Serial.print("%, L=");
    Serial.println(latestData.lightRaw);
  }

  // Periodic display update
  if (now - lastDisplayMillis >= DISPLAY_INTERVAL_MS) {
    lastDisplayMillis = now;
    if (currentMode == DisplayMode::SUMMARY) {
      displayMgr.showSummary(latestData, alarmOn);
    } else {
      displayMgr.showDetail(latestData);
    }
  }

  // Button handling (non-blocking)
  handleButton();
}
