/*
 * USB Power Monitoring System (INA219)
 * ------------------------------------
 * Measures real-time bus voltage, shunt voltage, current, and power
 * of a USB load using the INA219 high-side current/power monitor,
 * and streams the readings over serial (and optionally an OLED).
 *
 * Hardware:
 *   - Microcontroller: Arduino Uno / Nano (ATmega328P) or ESP32
 *   - Sensor: INA219 breakout (I2C)
 *   - Optional: 0.91"/0.96" SSD1306 OLED display (I2C)
 *
 * Wiring (I2C):
 *   INA219 VCC  -> 3.3V/5V
 *   INA219 GND  -> GND
 *   INA219 SDA  -> A4 (Uno) / GPIO21 (ESP32)
 *   INA219 SCL  -> A5 (Uno) / GPIO22 (ESP32)
 *   INA219 VIN+ -> USB source (+)
 *   INA219 VIN- -> Load (+)
 *
 * Library dependency: Adafruit_INA219 (Adafruit_BusIO as well)
 *   Install via Arduino Library Manager: "Adafruit INA219"
 *
 * Author: Mahi Raghuvanshi
 */

#include <Wire.h>
#include <Adafruit_INA219.h>

// Uncomment if using an SSD1306 OLED for live display
// #define USE_OLED
#ifdef USE_OLED
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #define SCREEN_WIDTH 128
  #define SCREEN_HEIGHT 64
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#endif

Adafruit_INA219 ina219;

// Sampling interval (ms)
const unsigned long SAMPLE_INTERVAL_MS = 500;
unsigned long lastSampleTime = 0;

// Simple moving-average filter to smooth noisy readings
const int FILTER_SAMPLES = 5;
float currentBuffer[FILTER_SAMPLES] = {0};
int filterIndex = 0;

float smoothCurrent(float newReading) {
  currentBuffer[filterIndex] = newReading;
  filterIndex = (filterIndex + 1) % FILTER_SAMPLES;

  float sum = 0;
  for (int i = 0; i < FILTER_SAMPLES; i++) sum += currentBuffer[i];
  return sum / FILTER_SAMPLES;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Serial.println(F("USB Power Monitoring System (INA219)"));
  Serial.println(F("-------------------------------------"));

  if (!ina219.begin()) {
    Serial.println(F("ERROR: INA219 not detected. Check wiring."));
    while (1) { delay(10); }
  }

  // Calibration profile — choose based on expected load current.
  // For typical USB loads (< 1A), the 32V/1A range gives the best resolution.
  ina219.setCalibration_32V_1A();

#ifdef USE_OLED
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("Power Monitor Ready"));
    display.display();
  }
#endif

  Serial.println(F("Calibration complete. Beginning measurements...\n"));
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) return;
  lastSampleTime = now;

  float shuntVoltage_mV = ina219.getShuntVoltage_mV();
  float busVoltage_V    = ina219.getBusVoltage_V();
  float current_mA_raw  = ina219.getCurrent_mA();
  float current_mA      = smoothCurrent(current_mA_raw);
  float power_mW        = ina219.getPower_mW();
  float loadVoltage_V   = busVoltage_V + (shuntVoltage_mV / 1000.0);

  // ---- Serial output (CSV-friendly for logging) ----
  Serial.print(F("Bus(V): "));   Serial.print(busVoltage_V, 3);
  Serial.print(F("  Load(V): ")); Serial.print(loadVoltage_V, 3);
  Serial.print(F("  Current(mA): ")); Serial.print(current_mA, 2);
  Serial.print(F("  Power(mW): ")); Serial.println(power_mW, 2);

  // Basic over-current guard/alert (tune threshold to your design)
  const float CURRENT_LIMIT_MA = 900.0;
  if (current_mA > CURRENT_LIMIT_MA) {
    Serial.println(F("WARNING: Current draw exceeds safe threshold!"));
  }

#ifdef USE_OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print(F("V: "));  display.print(loadVoltage_V, 2); display.println(F(" V"));
  display.print(F("I: "));  display.print(current_mA, 1);   display.println(F(" mA"));
  display.print(F("P: "));  display.print(power_mW, 1);     display.println(F(" mW"));
  display.display();
#endif
}
