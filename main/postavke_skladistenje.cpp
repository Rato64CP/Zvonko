// postavke_skladistenje.cpp - EEPROM helperi za postavke toranjskog sata
#include <Arduino.h>
#include <string.h>

#include "postavke_skladistenje.h"

#include "i2c_eeprom.h"

namespace {

struct SunceviDogadajiSpremnikV5 {
  uint16_t potpis;
  uint8_t verzija;
  uint8_t maskaDogadaja;
  uint8_t nocnaRasvjetaOmogucena;
  uint8_t zvona[3];
  int16_t odgodeMin[3];
  uint16_t checksum;
};

uint16_t izracunajChecksumSuncevihDogadajaV5(const SunceviDogadajiSpremnikV5& ulaz) {
  SunceviDogadajiSpremnikV5 kopija = ulaz;
  kopija.checksum = 0;

  const uint8_t* podaci = reinterpret_cast<const uint8_t*>(&kopija);
  uint16_t suma = 0;
  for (size_t i = 0; i < sizeof(kopija); ++i) {
    suma = static_cast<uint16_t>((suma << 1) | (suma >> 15));
    suma = static_cast<uint16_t>(suma + podaci[i] + 0x27);
  }
  return suma;
}

bool jeValjanEEPROMZapisSuncevihDogadajaV5(const SunceviDogadajiSpremnikV5& spremnik) {
  return spremnik.potpis == EepromLayout::SUNCEVI_DOGADAJI_POTPIS &&
         spremnik.verzija == 5 &&
         spremnik.checksum == izracunajChecksumSuncevihDogadajaV5(spremnik);
}

void migrirajSunceveDogadajeV5(const SunceviDogadajiSpremnikV5& izvor,
                               EepromLayout::SunceviDogadajiSpremnik& cilj) {
  memset(&cilj, 0, sizeof(cilj));
  cilj.potpis = izvor.potpis;
  cilj.verzija = izvor.verzija;
  cilj.maskaDogadaja = izvor.maskaDogadaja;
  cilj.nocnaRasvjetaOmogucena = izvor.nocnaRasvjetaOmogucena;
  memcpy(cilj.zvona, izvor.zvona, sizeof(cilj.zvona));
  memcpy(cilj.odgodeMin, izvor.odgodeMin, sizeof(cilj.odgodeMin));
  cilj.rucnaVremenaMaska = 0;
  cilj.reserved = 0;
  cilj.rucneMinute[0] = 6 * 60;
  cilj.rucneMinute[1] = 12 * 60;
  cilj.rucneMinute[2] = 18 * 60;
  cilj.checksum = 0;
}

}  // namespace

uint16_t izracunajChecksumPostavki(const EepromLayout::PostavkeSpremnik& ulaz) {
  EepromLayout::PostavkeSpremnik kopija = ulaz;
  kopija.checksum = 0;

  const uint8_t* podaci = reinterpret_cast<const uint8_t*>(&kopija);
  uint16_t suma = 0;
  for (size_t i = 0; i < sizeof(kopija); i++) {
    suma = static_cast<uint16_t>((suma << 1) | (suma >> 15));
    suma = static_cast<uint16_t>(suma + podaci[i] + 0x3D);
  }
  return suma;
}

uint16_t izracunajChecksumSuncevihDogadaja(const EepromLayout::SunceviDogadajiSpremnik& ulaz) {
  EepromLayout::SunceviDogadajiSpremnik kopija = ulaz;
  kopija.checksum = 0;

  const uint8_t* podaci = reinterpret_cast<const uint8_t*>(&kopija);
  uint16_t suma = 0;
  for (size_t i = 0; i < sizeof(kopija); ++i) {
    suma = static_cast<uint16_t>((suma << 1) | (suma >> 15));
    suma = static_cast<uint16_t>(suma + podaci[i] + 0x27);
  }
  return suma;
}

uint16_t izracunajChecksumBlagdana(const EepromLayout::BlagdaniSpremnik& ulaz) {
  EepromLayout::BlagdaniSpremnik kopija = ulaz;
  kopija.checksum = 0;

  const uint8_t* podaci = reinterpret_cast<const uint8_t*>(&kopija);
  uint16_t suma = 0;
  for (size_t i = 0; i < sizeof(kopija); ++i) {
    suma = static_cast<uint16_t>((suma << 1) | (suma >> 15));
    suma = static_cast<uint16_t>(suma + podaci[i] + 0x19);
  }
  return suma;
}

uint16_t izracunajChecksumMisa(const EepromLayout::MiseSpremnik& ulaz) {
  EepromLayout::MiseSpremnik kopija = ulaz;
  kopija.checksum = 0;

  const uint8_t* podaci = reinterpret_cast<const uint8_t*>(&kopija);
  uint16_t suma = 0;
  for (size_t i = 0; i < sizeof(kopija); ++i) {
    suma = static_cast<uint16_t>((suma + podaci[i]) & 0xFFFFU);
  }
  return suma;
}

bool jeValjanEEPROMZapisPostavki(const EepromLayout::PostavkeSpremnik& spremnik) {
  if (spremnik.potpis != EepromLayout::POSTAVKE_POTPIS) {
    return false;
  }
  if (spremnik.verzija != EepromLayout::POSTAVKE_VERZIJA) {
    return false;
  }
  return spremnik.checksum == izracunajChecksumPostavki(spremnik);
}

bool jeValjanEEPROMZapisSuncevihDogadaja(const EepromLayout::SunceviDogadajiSpremnik& spremnik) {
  return spremnik.potpis == EepromLayout::SUNCEVI_DOGADAJI_POTPIS &&
         spremnik.verzija == EepromLayout::SUNCEVI_DOGADAJI_VERZIJA &&
         spremnik.checksum == izracunajChecksumSuncevihDogadaja(spremnik);
}

bool jeValjanEEPROMZapisBlagdana(const EepromLayout::BlagdaniSpremnik& spremnik) {
  return spremnik.potpis == EepromLayout::BLAGDANI_POTPIS &&
         spremnik.verzija == EepromLayout::BLAGDANI_VERZIJA &&
         spremnik.checksum == izracunajChecksumBlagdana(spremnik);
}

bool jeValjanEEPROMZapisMisa(const EepromLayout::MiseSpremnik& spremnik) {
  return spremnik.potpis == EepromLayout::MISE_POTPIS &&
         spremnik.verzija == EepromLayout::MISE_VERZIJA &&
         spremnik.checksum == izracunajChecksumMisa(spremnik);
}

bool ucitajAktualniSpremnikSkeniranjem(EepromLayout::PostavkeSpremnik& spremnik) {
  EepromLayout::PostavkeSpremnik kandidat{};
  if (!VanjskiEEPROM::procitaj(EepromLayout::BAZA_POSTAVKE, &kandidat, sizeof(kandidat))) {
    return false;
  }
  if (!jeValjanEEPROMZapisPostavki(kandidat)) {
    return false;
  }

  spremnik = kandidat;
  return true;
}

bool ucitajSunceveDogadajeSkeniranjem(EepromLayout::SunceviDogadajiSpremnik& spremnik) {
  EepromLayout::SunceviDogadajiSpremnik kandidat{};
  if (!VanjskiEEPROM::procitaj(EepromLayout::BAZA_SUNCEVI_DOGADAJI, &kandidat, sizeof(kandidat))) {
    return false;
  }
  if (jeValjanEEPROMZapisSuncevihDogadaja(kandidat)) {
    spremnik = kandidat;
    return true;
  }

  SunceviDogadajiSpremnikV5 stariKandidat{};
  if (!VanjskiEEPROM::procitaj(EepromLayout::BAZA_SUNCEVI_DOGADAJI,
                               &stariKandidat,
                               sizeof(stariKandidat))) {
    return false;
  }
  if (!jeValjanEEPROMZapisSuncevihDogadajaV5(stariKandidat)) {
    return false;
  }

  migrirajSunceveDogadajeV5(stariKandidat, spremnik);
  return true;
}

bool ucitajBlagdaneSkeniranjem(EepromLayout::BlagdaniSpremnik& spremnik) {
  EepromLayout::BlagdaniSpremnik kandidat{};
  if (!VanjskiEEPROM::procitaj(EepromLayout::BAZA_BLAGDANI, &kandidat, sizeof(kandidat))) {
    return false;
  }
  if (!jeValjanEEPROMZapisBlagdana(kandidat)) {
    return false;
  }

  spremnik = kandidat;
  return true;
}

bool ucitajMiseSkeniranjem(EepromLayout::MiseSpremnik& spremnik) {
  EepromLayout::MiseSpremnik kandidat{};
  if (!VanjskiEEPROM::procitaj(EepromLayout::BAZA_MISE, &kandidat, sizeof(kandidat))) {
    return false;
  }
  if (!jeValjanEEPROMZapisMisa(kandidat)) {
    return false;
  }

  spremnik = kandidat;
  return true;
}

bool obrisiSegmentEeproma(int baznaAdresa, int duljina) {
  uint8_t prazno[32];
  memset(prazno, 0xFF, sizeof(prazno));

  for (int adresa = baznaAdresa; adresa < (baznaAdresa + duljina); adresa += static_cast<int>(sizeof(prazno))) {
    const size_t blok =
        static_cast<size_t>(min(static_cast<int>(sizeof(prazno)), (baznaAdresa + duljina) - adresa));
    if (!VanjskiEEPROM::zapisi(adresa, prazno, blok)) {
      return false;
    }
  }

  return true;
}
