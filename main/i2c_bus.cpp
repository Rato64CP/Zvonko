#include "i2c_bus.h"

#include <Arduino.h>
#include <Wire.h>

#include "podesavanja_piny.h"

namespace {

void oslobodiI2CSabirnicuAkoJeZaglavila() {
  pinMode(PIN_SDA, INPUT_PULLUP);
  pinMode(PIN_SCL, INPUT_PULLUP);
  delayMicroseconds(5);

  if (digitalRead(PIN_SDA) == HIGH) {
    return;
  }

  pinMode(PIN_SCL, OUTPUT);
  digitalWrite(PIN_SCL, HIGH);
  delayMicroseconds(5);

  for (uint8_t i = 0; i < 9; ++i) {
    if (digitalRead(PIN_SDA) == HIGH) {
      break;
    }

    digitalWrite(PIN_SCL, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN_SCL, HIGH);
    delayMicroseconds(5);
  }

  pinMode(PIN_SDA, OUTPUT);
  digitalWrite(PIN_SDA, LOW);
  delayMicroseconds(5);
  digitalWrite(PIN_SCL, HIGH);
  delayMicroseconds(5);
  digitalWrite(PIN_SDA, HIGH);
  delayMicroseconds(5);

  pinMode(PIN_SDA, INPUT_PULLUP);
  pinMode(PIN_SCL, INPUT_PULLUP);
}

}  // namespace

void pripremiI2CSabirnicuSigurno() {
  oslobodiI2CSabirnicuAkoJeZaglavila();
  Wire.begin();
  #if defined(WIRE_HAS_TIMEOUT) || defined(TWBR)
  Wire.setWireTimeout(25000, true);
  #endif
}
