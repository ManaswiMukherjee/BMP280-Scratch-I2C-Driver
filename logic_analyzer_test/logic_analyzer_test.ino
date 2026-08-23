// Logic analyzer test with pico
#include <SPI.h>

// Pin definitions
#define PIN_UART_TX   0   // GP0: Serial1 TX
#define PIN_SQ_1KHZ   2   // GP2: 1 kHz Square wave
#define PIN_PWM_25    3   // GP3: 25% Duty cycle PWM
#define PIN_BURST     4   // GP4: Short trigger pulse
#define PIN_SPI_CS   17   // GP17: SPI0 Chip Select
#define PIN_SPI_SCK  18   // GP18: SPI0 Clock
#define PIN_SPI_MOSI 19   // GP19: SPI0 Data Out

uint32_t lastPulseTime = 0;
uint32_t lastCommTime = 0;
uint32_t lastSquareToggle = 0;
uint8_t packetCounter = 0;

void setup() {
  // 1. Digital Output Setup
  pinMode(PIN_SQ_1KHZ, OUTPUT);
  pinMode(PIN_BURST, OUTPUT);
  pinMode(PIN_SPI_CS, OUTPUT);
  digitalWrite(PIN_SPI_CS, HIGH);

  // 2. Hardware PWM Setup (25% of 255 = ~64)
  pinMode(PIN_PWM_25, OUTPUT);
  analogWrite(PIN_PWM_25, 64);

  // 3. Hardware UART Setup (GP0 TX)
  Serial1.setTX(PIN_UART_TX);
  Serial1.begin(115200);

  // 4. Hardware SPI Setup (GP17 CS, GP18 SCK, GP19 MOSI)
  SPI.setSCK(PIN_SPI_SCK);
  SPI.setTX(PIN_SPI_MOSI);
  SPI.begin();
}

void loop() {
  uint32_t currentMicros = micros();
  uint32_t currentMillis = millis();

  // 1. Continuous 1 kHz Square Wave (Toggles every 500 µs)
  if (currentMicros - lastSquareToggle >= 500) {
    lastSquareToggle = currentMicros;
    gpio_put(PIN_SQ_1KHZ, !gpio_get(PIN_SQ_1KHZ));
  }

  // 2. Narrow 10 µs Trigger Pulse every 50 ms
  if (currentMillis - lastPulseTime >= 50) {
    lastPulseTime = currentMillis;
    digitalWrite(PIN_BURST, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_BURST, LOW);
  }

  // 3. Periodic UART & SPI burst transmissions every 100 ms
  if (currentMillis - lastCommTime >= 100) {
    lastCommTime = currentMillis;

    // Send ASCII text over UART
    Serial1.print("HELLO RP2040\r\n");

    // Send a 2-byte SPI packet (0xAA delimiter + incrementing counter)
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_SPI_CS, LOW);
    SPI.transfer(0xAA);
    SPI.transfer(packetCounter++);
    digitalWrite(PIN_SPI_CS, HIGH);
    SPI.endTransaction();
  }
}