/*
 * USB Power Monitoring System (INA219) (Converted to pure C syntax)
 * ------------------------------------------------------------------
 * Measures real-time bus voltage, shunt voltage, current, and power.
 * Author: Mahi Raghuvanshi
 */

#include <Arduino.h> // Required for basic microcontroller pin mappings
#include <Wire.h>
#include <Adafruit_INA219.h>

// Uncomment if using an SSD1306 OLED for live display
// #define USE_OLED
#ifdef USE_OLED
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #define SCREEN_WIDTH 128
  #define SCREEN_HEIGHT 64
  
  // Allocate static configuration data for the global pointer mapping
  Adafruit_SSD1306 displayInstance(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
  Adafruit_SSD1306 *display = &displayInstance;
#endif

// Static instantiation for the sensor library pointer logic
Adafruit_INA219 ina219Instance;
Adafruit_INA219 *ina219 = &ina219Instance;

// Sampling interval (ms)
const unsigned long SAMPLE_INTERVAL_MS = 500;
unsigned long lastSampleTime = 0;

// Simple moving-average filter to smooth noisy readings
const int FILTER_SAMPLES = 5;
float currentBuffer[FILTER_SAMPLES] = {0};
int filterIndex = 0;

// Pure C helper function for calculation
float smoothCurrent(float newReading) {
    currentBuffer[filterIndex] = newReading;
    filterIndex = (filterIndex + 1) % FILTER_SAMPLES;

    float sum = 0;
    for (int i = 0; i < FILTER_SAMPLES; i++) {
        sum += currentBuffer[i];
    }
    return sum / FILTER_SAMPLES;
}

// ---- Main Arduino Framework Functions ----

void setup(void) {
    Serial.begin(115200);
    while (!Serial) { 
        delay(10); 
    }

    Serial.println("USB Power Monitoring System (INA219)");
    Serial.println("-------------------------------------");

    // Replace object-oriented dot-notation with pointer assignments
    if (!ina219->begin()) {
        Serial.println("ERROR: INA219 not detected. Check wiring.");
        while (1) { 
            delay(10); 
        }
    }

    // Call calibration profile over the sensor structure
    ina219->setCalibration_32V_1A();

#ifdef USE_OLED
    if (display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        display->clearDisplay();
        display->setTextColor(SSD1306_WHITE);
        display->setTextSize(1);
        display->setCursor(0, 0);
        display->println("Power Monitor Ready");
        display->display();
    }
#endif

    Serial.println("Calibration complete. Beginning measurements...\n");
}

void loop(void) {
    unsigned long now = millis();
    if (now - lastSampleTime < SAMPLE_INTERVAL_MS) return;
    lastSampleTime = now;

    // Data collection using explicit C syntax structural pointers
    float shuntVoltage_mV = ina219->getShuntVoltage_mV();
    float busVoltage_V    = ina219->getBusVoltage_V();
    float current_mA_raw  = ina219->getCurrent_mA();
    float current_mA      = smoothCurrent(current_mA_raw);
    float power_mW        = ina219->getPower_mW();
    float loadVoltage_V   = busVoltage_V + (shuntVoltage_mV / 1000.0);

    // ---- Serial output (CSV-friendly for logging) ----
    Serial.print("Bus(V): ");       Serial.print(busVoltage_V, 3);
    Serial.print("  Load(V): ");    Serial.print(loadVoltage_V, 3);
    Serial.print("  Current(mA): "); Serial.print(current_mA, 2);
    Serial.print("  Power(mW): ");   Serial.println(power_mW, 2);

    // Basic over-current guard/alert 
    const float CURRENT_LIMIT_MA = 900.0;
    if (current_mA > CURRENT_LIMIT_MA) {
        Serial.println("WARNING: Current draw exceeds safe threshold!");
    }

#ifdef USE_OLED
    display->clearDisplay();
    display->setCursor(0, 0);
    display->setTextSize(1);
    display->print("V: ");  display->print(loadVoltage_V, 2); display->println(" V");
    display->print("I: ");  display->print(current_mA, 1);   display->println(" mA");
    display->print("P: ");  display->print(power_mW, 1);     display->println(" mW");
    display->display();
#endif
}
