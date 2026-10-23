#include <Wire.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp; // Use I2C interface

void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(1000);

  // Set explicit I2C pins for RP2040-Zero (GP0 for SDA, GP1 for SCL)
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();

  // Initialize BMP280 with address 0x76 (change to 0x77 if SDO is pulled high)
  if (!bmp.begin(0x76)) {
    Serial.println(F("Could a valid BMP280 sensor be found, check wiring!"));
    while (1);
  }

  // Default sensor configuration settings
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     
                  Adafruit_BMP280::SAMPLING_X2,     // Temperature oversampling
                  Adafruit_BMP280::SAMPLING_X16,    // Pressure oversampling
                  Adafruit_BMP280::FILTER_X16,      // Filtering
                  Adafruit_BMP280::STANDBY_MS_500); // Standby time
}

void loop() {
  Serial.print(F("Temperature = "));
  Serial.print(bmp.readTemperature());
  Serial.println(F(" °C"));

  Serial.print(F("Pressure = "));
  Serial.print(bmp.readPressure() / 100.0); // Convert Pa to hPa
  Serial.println(F(" hPa"));

  Serial.print(F("Approx. Altitude = "));
  Serial.print(bmp.readAltitude(1013.25)); // Standard sea-level pressure in hPa
  Serial.println(F(" m"));

  Serial.println();
  delay(2000);
}