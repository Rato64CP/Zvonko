// daljinski_433.cpp - 433 MHz prijemnik SRX882 za toranjski sat

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "daljinski_433.h"

#include "pc_serial.h"
#include "podesavanja_piny.h"
#include "slavljenje_mrtvacko.h"
#include "zvonjenje.h"

namespace {

const unsigned long DALJINSKI_433_MIN_TRAJANJE_IMPULSA_US = 80UL;
const unsigned long DALJINSKI_433_GAP_OKVIRA_US = 5000UL;
const unsigned long DALJINSKI_433_MAKS_RAZMAK_PONAVLJANJA_SLAVLJENJE_MS = 350UL;
const unsigned long DALJINSKI_433_MAKS_RAZMAK_PONAVLJANJA_ZVONO_I_MRTVACKO_MS = 500UL;
const uint8_t DALJINSKI_433_EV1527_BROJ_BITOVA = 24;
const uint8_t DALJINSKI_433_MAKS_IMPULSA = 80;
const uint8_t DALJINSKI_433_MIN_IMPULSA_ZA_OKVIR = 20;
const uint32_t DALJINSKI_433_KOD_TIPKE_A = 0xBE5338UL;
const uint32_t DALJINSKI_433_KOD_TIPKE_B = 0xBE5334UL;
const uint32_t DALJINSKI_433_KOD_TIPKE_C = 0xBE5332UL;
const uint32_t DALJINSKI_433_KOD_TIPKE_D = 0xBE5331UL;

volatile uint16_t siroviImpulsi[DALJINSKI_433_MAKS_IMPULSA];
volatile uint8_t brojSirovihImpulsa = 0;
volatile unsigned long zadnjiRubUs = 0UL;
volatile bool okvirSpremanZaObradu = false;
bool daljinski433Inicijaliziran = false;
uint32_t zadnjiPrihvaceniKod = 0UL;
unsigned long zadnjePrihvacanjeKodaMs = 0UL;

void prekidDaljinskog433() {
  const unsigned long sadaUs = micros();
  const unsigned long razmakUs = sadaUs - zadnjiRubUs;
  zadnjiRubUs = sadaUs;

  if (okvirSpremanZaObradu) {
    return;
  }

  if (razmakUs < DALJINSKI_433_MIN_TRAJANJE_IMPULSA_US) {
    return;
  }

  if (razmakUs > DALJINSKI_433_GAP_OKVIRA_US) {
    if (brojSirovihImpulsa >= DALJINSKI_433_MIN_IMPULSA_ZA_OKVIR) {
      okvirSpremanZaObradu = true;
    } else {
      brojSirovihImpulsa = 0;
    }
    return;
  }

  if (brojSirovihImpulsa < DALJINSKI_433_MAKS_IMPULSA) {
    siroviImpulsi[brojSirovihImpulsa++] = static_cast<uint16_t>(razmakUs);
  } else {
    brojSirovihImpulsa = 0;
  }
}

bool jeUnutarTolerancije(unsigned long vrijednost, unsigned long cilj, unsigned long tolerancijaPosto) {
  const unsigned long tolerancija = (cilj * tolerancijaPosto) / 100UL;
  const unsigned long donjaGranica = (tolerancija > cilj) ? 0UL : (cilj - tolerancija);
  const unsigned long gornjaGranica = cilj + tolerancija;
  return vrijednost >= donjaGranica && vrijednost <= gornjaGranica;
}

unsigned long dohvatiAntiRepeatProzorZaKod(uint32_t kod) {
  if (kod == DALJINSKI_433_KOD_TIPKE_C) {
    return DALJINSKI_433_MAKS_RAZMAK_PONAVLJANJA_SLAVLJENJE_MS;
  }
  if (kod == DALJINSKI_433_KOD_TIPKE_A ||
      kod == DALJINSKI_433_KOD_TIPKE_B ||
      kod == DALJINSKI_433_KOD_TIPKE_D) {
    return DALJINSKI_433_MAKS_RAZMAK_PONAVLJANJA_ZVONO_I_MRTVACKO_MS;
  }
  return DALJINSKI_433_MAKS_RAZMAK_PONAVLJANJA_ZVONO_I_MRTVACKO_MS;
}

bool dekodirajEv1527PodatkeOdIndeksa(const uint16_t* impulsi,
                                     uint8_t brojImpulsa,
                                     uint8_t indeksPodataka,
                                     uint16_t bazniImpulsUs,
                                     uint32_t* kod) {
  if (impulsi == nullptr || kod == nullptr) {
    return false;
  }

  const uint8_t potrebniImpulsi = DALJINSKI_433_EV1527_BROJ_BITOVA * 2;
  if (indeksPodataka > brojImpulsa || (indeksPodataka + potrebniImpulsi) > brojImpulsa) {
    return false;
  }

  uint32_t procitaniKod = 0UL;
  for (uint8_t bit = 0; bit < DALJINSKI_433_EV1527_BROJ_BITOVA; ++bit) {
    const uint16_t prvi = impulsi[indeksPodataka + (bit * 2)];
    const uint16_t drugi = impulsi[indeksPodataka + (bit * 2) + 1];
    const bool prviKratki = jeUnutarTolerancije(prvi, bazniImpulsUs, 70UL);
    const bool prviDugi = jeUnutarTolerancije(prvi, bazniImpulsUs * 3UL, 45UL);
    const bool drugiKratki = jeUnutarTolerancije(drugi, bazniImpulsUs, 70UL);
    const bool drugiDugi = jeUnutarTolerancije(drugi, bazniImpulsUs * 3UL, 45UL);

    procitaniKod <<= 1;
    if (prviKratki && drugiDugi) {
      // bit 0
    } else if (prviDugi && drugiKratki) {
      procitaniKod |= 1UL;
    } else {
      return false;
    }
  }

  *kod = procitaniKod;
  return true;
}

bool dekodirajEv1527Okvir(const uint16_t* impulsi,
                          uint8_t brojImpulsa,
                          uint32_t* kod) {
  if (impulsi == nullptr || kod == nullptr || brojImpulsa < 48) {
    return false;
  }

  uint16_t bazniImpulsUs = 0xFFFFU;
  for (uint8_t i = 0; i < brojImpulsa; ++i) {
    const uint16_t impuls = impulsi[i];
    if (impuls >= 150U && impuls < bazniImpulsUs) {
      bazniImpulsUs = impuls;
    }
  }

  if (bazniImpulsUs == 0xFFFFU) {
    return false;
  }

  int indeksSinkroPocetka = -1;
  for (uint8_t i = 0; i + 1 < brojImpulsa; ++i) {
    const uint16_t prvi = impulsi[i];
    const uint16_t drugi = impulsi[i + 1];
    const bool prviKratki = jeUnutarTolerancije(prvi, bazniImpulsUs, 70UL);
    const bool drugiDugi = drugi >= static_cast<uint16_t>(bazniImpulsUs * 8UL);
    if (prviKratki && drugiDugi) {
      indeksSinkroPocetka = i;
      break;
    }
  }

  if (indeksSinkroPocetka < 0) {
    // Neki daljinski za toranjski sat daju 48 ili 49 impulsa jer se dugi
    // sinkro-razmak odreze kao granica okvira. U tom slucaju probamo
    // dekodirati cistih 24 bita bez punog sinkro-para.
    if (brojImpulsa == (DALJINSKI_433_EV1527_BROJ_BITOVA * 2)) {
      return dekodirajEv1527PodatkeOdIndeksa(impulsi, brojImpulsa, 0, bazniImpulsUs, kod);
    }

    if (brojImpulsa == (DALJINSKI_433_EV1527_BROJ_BITOVA * 2 + 1)) {
      if (dekodirajEv1527PodatkeOdIndeksa(impulsi, brojImpulsa, 0, bazniImpulsUs, kod)) {
        return true;
      }
      return dekodirajEv1527PodatkeOdIndeksa(impulsi, brojImpulsa, 1, bazniImpulsUs, kod);
    }

    return false;
  }

  const int indeksPodataka = indeksSinkroPocetka + 2;
  return dekodirajEv1527PodatkeOdIndeksa(
      impulsi, brojImpulsa, static_cast<uint8_t>(indeksPodataka), bazniImpulsUs, kod);
}

void obradiKodDaljinskog(uint32_t kod) {
  const unsigned long sadaMs = millis();
  const unsigned long antiRepeatProzorMs = dohvatiAntiRepeatProzorZaKod(kod);
  if (kod == zadnjiPrihvaceniKod &&
      (sadaMs - zadnjePrihvacanjeKodaMs) < antiRepeatProzorMs) {
    return;
  }

  zadnjiPrihvaceniKod = kod;
  zadnjePrihvacanjeKodaMs = sadaMs;

  if (kod == DALJINSKI_433_KOD_TIPKE_A) {
    if (jeZvonoAktivno(1)) {
      zahtijevajOperatorovoGasenjeZvona(1);
      posaljiPCLog(F("433 MHz: tipka A -> gasim MUSKO zvono toranjskog sata"));
    } else {
      ukljuciZvono(1);
      posaljiPCLog(F("433 MHz: tipka A -> palim MUSKO zvono toranjskog sata"));
    }
    return;
  }

  if (kod == DALJINSKI_433_KOD_TIPKE_B) {
    if (jeZvonoAktivno(2)) {
      zahtijevajOperatorovoGasenjeZvona(2);
      posaljiPCLog(F("433 MHz: tipka B -> gasim ZENSKO zvono toranjskog sata"));
    } else {
      ukljuciZvono(2);
      posaljiPCLog(F("433 MHz: tipka B -> palim ZENSKO zvono toranjskog sata"));
    }
    return;
  }

  if (kod == DALJINSKI_433_KOD_TIPKE_C) {
    preklopiSlavljenjeDaljinskimUpravljacem();
    return;
  }

  if (kod == DALJINSKI_433_KOD_TIPKE_D) {
    if (jeMrtvackoUTijeku()) {
      zaustaviMrtvacko();
      posaljiPCLog(F("433 MHz: tipka D -> gasim MRTVACKO zvono toranjskog sata"));
    } else {
      zapocniMrtvacko();
      posaljiPCLog(F("433 MHz: tipka D -> palim MRTVACKO zvono toranjskog sata"));
    }
    return;
  }

  char log[88];
  snprintf_P(log,
             sizeof(log),
             PSTR("433 MHz: ignoriram nepoznat kod 0x%06lX"),
             kod);
  posaljiPCLog(log);
}

}  // namespace

void inicijalizirajDaljinski433() {
  pinMode(PIN_DALJINSKI_433_DATA, INPUT);
  brojSirovihImpulsa = 0;
  zadnjiRubUs = micros();
  okvirSpremanZaObradu = false;
  zadnjiPrihvaceniKod = 0UL;
  zadnjePrihvacanjeKodaMs = 0UL;

  attachInterrupt(digitalPinToInterrupt(PIN_DALJINSKI_433_DATA), prekidDaljinskog433, CHANGE);

  daljinski433Inicijaliziran = true;
  posaljiPCLog(F("433 MHz: inicijaliziran SRX882 na D3, daljinski A/B/C/D spreman za toranjski sat"));
}

void obradiDaljinski433() {
  if (!daljinski433Inicijaliziran) {
    return;
  }

  const unsigned long sadaUs = micros();
  noInterrupts();
  if (!okvirSpremanZaObradu &&
      brojSirovihImpulsa >= DALJINSKI_433_MIN_IMPULSA_ZA_OKVIR &&
      (sadaUs - zadnjiRubUs) > DALJINSKI_433_GAP_OKVIRA_US) {
    okvirSpremanZaObradu = true;
  }

  if (!okvirSpremanZaObradu) {
    interrupts();
    return;
  }

  uint16_t lokalniImpulsi[DALJINSKI_433_MAKS_IMPULSA];
  const uint8_t lokalniBrojImpulsa = brojSirovihImpulsa;
  for (uint8_t i = 0; i < lokalniBrojImpulsa; ++i) {
    lokalniImpulsi[i] = siroviImpulsi[i];
  }
  brojSirovihImpulsa = 0;
  okvirSpremanZaObradu = false;
  interrupts();

  uint32_t kod = 0UL;
  if (dekodirajEv1527Okvir(lokalniImpulsi, lokalniBrojImpulsa, &kod)) {
    obradiKodDaljinskog(kod);
    return;
  }

  char log[72];
  snprintf_P(log,
             sizeof(log),
             PSTR("433 MHz: ne mogu dekodirati okvir, broj impulsa=%u"),
             static_cast<unsigned>(lokalniBrojImpulsa));
  posaljiPCLog(log);
}
