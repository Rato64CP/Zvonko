// izlazni_releji.cpp - zajednicka polarnost releja D22-D29
#include <Arduino.h>
#include "izlazni_releji.h"
#include "podesavanja_piny.h"
#include "postavke.h"
#include "pc_serial.h"

namespace {

const uint8_t PINOVI_IZLAZNIH_RELEJA[] = {
    PIN_RELEJ_PARNE_KAZALJKE,
    PIN_RELEJ_NEPARNE_KAZALJKE,
    PIN_RELEJ_PARNE_PLOCE,
    PIN_RELEJ_NEPARNE_PLOCE,
    PIN_ZVONO_1,
    PIN_ZVONO_2,
    PIN_CEKIC_MUSKI,
    PIN_CEKIC_ZENSKI
};

uint8_t logickaRazinaReleja(bool ukljuceno) {
  const bool obrnuto = jeObrnutaLogikaIzlaznihReleja();
  return (ukljuceno ^ obrnuto) ? HIGH : LOW;
}

}  // namespace

void postaviIzlazniRelej(uint8_t pin, bool ukljuceno) {
  digitalWrite(pin, logickaRazinaReleja(ukljuceno));
}

void iskljuciSveIzlazneReleje() {
  for (uint8_t i = 0; i < (sizeof(PINOVI_IZLAZNIH_RELEJA) / sizeof(PINOVI_IZLAZNIH_RELEJA[0])); ++i) {
    postaviIzlazniRelej(PINOVI_IZLAZNIH_RELEJA[i], false);
  }
}

void inicijalizirajIzlazneReleje() {
  // Vazno za aktivno-LOW relejne ploce: prije prebacivanja pina u OUTPUT
  // prvo se u port latch upise stvarna OFF razina iz spremljenih postavki.
  iskljuciSveIzlazneReleje();
  for (uint8_t i = 0; i < (sizeof(PINOVI_IZLAZNIH_RELEJA) / sizeof(PINOVI_IZLAZNIH_RELEJA[0])); ++i) {
    pinMode(PINOVI_IZLAZNIH_RELEJA[i], OUTPUT);
  }
  posaljiPCLog(jeObrnutaLogikaIzlaznihReleja()
                   ? F("Izlazni releji D22-D29: inicijalizirani kao aktivno-LOW, pocetno OFF")
                   : F("Izlazni releji D22-D29: inicijalizirani kao aktivno-HIGH, pocetno OFF"));
}

void primijeniPolarnostIzlaznihReleja() {
  iskljuciSveIzlazneReleje();
}
