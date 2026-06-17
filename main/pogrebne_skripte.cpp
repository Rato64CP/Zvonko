// pogrebne_skripte.cpp - Sekvence POKOJNIK / POKOJNICA za toranjski sat

#include <Arduino.h>
#include <avr/pgmspace.h>

#include "pogrebne_skripte.h"

#include "pc_serial.h"
#include "slavljenje_mrtvacko.h"
#include "time_glob.h"
#include "ups_nadzor.h"
#include "zvonjenje.h"
#include "otkucavanje.h"
#include "otkucavanje_interno.h"
#include "prekidac_tisine.h"
#include "postavke.h"

namespace {

constexpr unsigned long TRAJANJE_POCETNOG_ZVONA_MS = 2UL * 60UL * 1000UL;
constexpr uint8_t TRAJANJE_MRTVACKOG_KLASICNO_MIN = 10U;
constexpr uint8_t TRAJANJE_MRTVACKOG_PRODULJENO_MIN = 40U;
constexpr uint8_t MUSKO_ZVONO = 1;
constexpr uint8_t ZENSKO_ZVONO = 2;
constexpr uint8_t VRSTA_POKOJNICKOG_KLASICNA = 1;
constexpr uint8_t VRSTA_POKOJNICKOG_PRODULJENA = 2;

enum FazaPogrebneSkripte : uint8_t {
  POGREBNA_SKRIPTA_NEAKTIVNA = 0,
  POGREBNA_SKRIPTA_ZVONO = 1,
  POGREBNA_SKRIPTA_CEKA_ZAVRSETAK = 2,
  POGREBNA_SKRIPTA_MRTVACKO = 3
};

struct StanjePogrebneSkripte {
  bool aktivna;
  uint8_t tip;
  uint8_t zvono;
  bool obaZvona;
  uint8_t trajanjeMrtvackogMin;
  FazaPogrebneSkripte faza;
};

static StanjePogrebneSkripte stanje = {
    false,
    MUSKO_ZVONO,
    MUSKO_ZVONO,
    false,
    TRAJANJE_MRTVACKOG_KLASICNO_MIN,
    POGREBNA_SKRIPTA_NEAKTIVNA};

static const __FlashStringHelper* nazivSkripte(uint8_t tip) {
  return (tip == MUSKO_ZVONO) ? F("POKOJNIK") : F("POKOJNICA");
}

static bool jeSekvencaBlokirana() {
  return jeRtcIzlazniFailSafeAktivan() ||
         !jeVrijemePotvrdjenoZaAutomatiku() ||
         jePrekidacTisineAktivan() ||
         jeUPSModAktivan();
}

static bool jeToranjZauzetZaPogrebnuSekvencu() {
  return jeZvonoUTijeku() ||
         jeLiInerciaAktivna() ||
         jeOtkucavanjeUTijeku() ||
         jeSlavljenjeUTijeku() ||
         jeMrtvackoUTijeku();
}

static void prekiniOtkucavanjeAkoSamoOnoBlokiraPogrebnuSekvencu() {
  if (jeOtkucavanjeUTijeku() &&
      !jeZvonoUTijeku() &&
      !jeLiInerciaAktivna() &&
      !jeSlavljenjeUTijeku() &&
      !jeMrtvackoUTijeku()) {
    prekiniAktivnoOtkucavanjeZbogPosebnogNacina(
        F("pogrebna skripta preuzima prioritet nad otkucavanjem"));
  }
}

static void logirajStanjeSkripte(const __FlashStringHelper* poruka) {
  char log[128];
  char naziv[16];
  char tekstPoruke[72];
  strncpy_P(naziv, reinterpret_cast<PGM_P>(nazivSkripte(stanje.tip)), sizeof(naziv) - 1);
  naziv[sizeof(naziv) - 1] = '\0';
  strncpy_P(tekstPoruke, reinterpret_cast<PGM_P>(poruka), sizeof(tekstPoruke) - 1);
  tekstPoruke[sizeof(tekstPoruke) - 1] = '\0';
  snprintf_P(log,
             sizeof(log),
             PSTR("%s: %s"),
             naziv,
             tekstPoruke);
  posaljiPCLog(log);
}

static void zaustaviAktivnuSekvencuBezLoga() {
  if (stanje.aktivna) {
    if (stanje.obaZvona) {
      if (jeZvonoAktivno(MUSKO_ZVONO)) {
        iskljuciZvono(MUSKO_ZVONO);
      }
      if (jeZvonoAktivno(ZENSKO_ZVONO)) {
        iskljuciZvono(ZENSKO_ZVONO);
      }
    } else if (jeZvonoAktivno(stanje.zvono)) {
      iskljuciZvono(stanje.zvono);
    }
    if (jeMrtvackoUTijeku()) {
      zaustaviMrtvacko();
    }
  }
  stanje.aktivna = false;
  stanje.faza = POGREBNA_SKRIPTA_NEAKTIVNA;
  stanje.obaZvona = false;
  stanje.trajanjeMrtvackogMin = TRAJANJE_MRTVACKOG_KLASICNO_MIN;
}

static bool jePocetnoZvonoZavrsilo() {
  if (stanje.obaZvona) {
    return !jeZvonoAktivno(MUSKO_ZVONO) && !jeZvonoAktivno(ZENSKO_ZVONO);
  }
  return !jeZvonoAktivno(stanje.zvono);
}

static bool pokreniPocetnoZvono(uint8_t zvono, bool obaZvona) {
  if (obaZvona) {
    aktivirajZvonjenjeNaTrajanje(MUSKO_ZVONO, TRAJANJE_POCETNOG_ZVONA_MS);
    aktivirajZvonjenjeNaTrajanje(ZENSKO_ZVONO, TRAJANJE_POCETNOG_ZVONA_MS);
    if (jeZvonoAktivno(MUSKO_ZVONO) && jeZvonoAktivno(ZENSKO_ZVONO)) {
      return true;
    }
    if (jeZvonoAktivno(MUSKO_ZVONO)) {
      iskljuciZvono(MUSKO_ZVONO);
    }
    if (jeZvonoAktivno(ZENSKO_ZVONO)) {
      iskljuciZvono(ZENSKO_ZVONO);
    }
    return false;
  }

  aktivirajZvonjenjeNaTrajanje(zvono, TRAJANJE_POCETNOG_ZVONA_MS);
  return jeZvonoAktivno(zvono);
}

static bool pokreniPogrebnuSekvencu(uint8_t tip) {
  if (stanje.aktivna || (tip != MUSKO_ZVONO && tip != ZENSKO_ZVONO)) {
    return false;
  }

  prekiniOtkucavanjeAkoSamoOnoBlokiraPogrebnuSekvencu();
  if (jeSekvencaBlokirana() || jeToranjZauzetZaPogrebnuSekvencu()) {
    return false;
  }

  const uint8_t vrstaPokojnickogZvona = dohvatiVrstuPokojnickogZvona();
  const bool produljenaVarijanta = vrstaPokojnickogZvona == VRSTA_POKOJNICKOG_PRODULJENA;
  const bool obaZvona = produljenaVarijanta && tip == MUSKO_ZVONO;
  const uint8_t pocetnoZvono = (tip == MUSKO_ZVONO) ? MUSKO_ZVONO : ZENSKO_ZVONO;
  const uint8_t trajanjeMrtvackog =
      produljenaVarijanta ? TRAJANJE_MRTVACKOG_PRODULJENO_MIN : TRAJANJE_MRTVACKOG_KLASICNO_MIN;

  if (!pokreniPocetnoZvono(pocetnoZvono, obaZvona)) {
    return false;
  }

  stanje.aktivna = true;
  stanje.tip = tip;
  stanje.zvono = pocetnoZvono;
  stanje.obaZvona = obaZvona;
  stanje.trajanjeMrtvackogMin = trajanjeMrtvackog;
  stanje.faza = POGREBNA_SKRIPTA_ZVONO;
  if (produljenaVarijanta && obaZvona) {
    logirajStanjeSkripte(F("pokrecem oba zvona 2 minute, zatim cekam inerciju i mrtvacko 40 minuta"));
  } else if (produljenaVarijanta) {
    logirajStanjeSkripte(F("pokrecem zensko zvono 2 minute, zatim cekam inerciju i mrtvacko 40 minuta"));
  } else {
    logirajStanjeSkripte(F("pokrecem zvono 2 minute, zatim cekam inerciju i mrtvacko 10 minuta"));
  }
  return true;
}

static bool prebaciPogrebnuSekvencu(uint8_t tip) {
  if (tip != MUSKO_ZVONO && tip != ZENSKO_ZVONO) {
    return false;
  }

  if (!stanje.aktivna) {
    return pokreniPogrebnuSekvencu(tip);
  }

  if (stanje.tip != tip) {
    logirajStanjeSkripte(F("odbijeno jer je druga pogrebna sekvenca vec aktivna"));
    return false;
  }

  logirajStanjeSkripte(F("rucno zaustavljeno ponovnim pritiskom tipke"));
  zaustaviAktivnuSekvencuBezLoga();
  return true;
}

}  // namespace

void inicijalizirajPogrebneSkripte() {
  stanje.aktivna = false;
  stanje.tip = MUSKO_ZVONO;
  stanje.zvono = MUSKO_ZVONO;
  stanje.obaZvona = false;
  stanje.trajanjeMrtvackogMin = TRAJANJE_MRTVACKOG_KLASICNO_MIN;
  stanje.faza = POGREBNA_SKRIPTA_NEAKTIVNA;
}

void upravljajPogrebnimSkriptama(unsigned long sadaMs) {
  (void)sadaMs;

  if (!stanje.aktivna) {
    return;
  }

  if (jeSekvencaBlokirana()) {
    logirajStanjeSkripte(F("prekinuto zbog tihog rezima, gubitka vremena, RTC fail-safea ili UPS moda"));
    zaustaviAktivnuSekvencuBezLoga();
    return;
  }

  switch (stanje.faza) {
    case POGREBNA_SKRIPTA_ZVONO:
      if (jePocetnoZvonoZavrsilo()) {
        stanje.faza = POGREBNA_SKRIPTA_CEKA_ZAVRSETAK;
        logirajStanjeSkripte(F("zvono je zavrsilo, cekam da se smiri inercija"));
      }
      break;

    case POGREBNA_SKRIPTA_CEKA_ZAVRSETAK:
      if (jeZvonoUTijeku() || jeLiInerciaAktivna()) {
        return;
      }

      if (jeOtkucavanjeUTijeku() || jeSlavljenjeUTijeku() || jeMrtvackoUTijeku()) {
        return;
      }

      if (pokusajZapocetiMrtvackoSaFiksnimTrajanjemBezCekanja(stanje.trajanjeMrtvackogMin)) {
        stanje.faza = POGREBNA_SKRIPTA_MRTVACKO;
        if (stanje.trajanjeMrtvackogMin == TRAJANJE_MRTVACKOG_PRODULJENO_MIN) {
          logirajStanjeSkripte(F("pokrecem mrtvacko 40 minuta"));
        } else {
          logirajStanjeSkripte(F("pokrecem mrtvacko 10 minuta"));
        }
      }
      break;

    case POGREBNA_SKRIPTA_MRTVACKO:
      if (!jeMrtvackoUTijeku()) {
        logirajStanjeSkripte(F("sekvenca zavrsena"));
        stanje.aktivna = false;
        stanje.faza = POGREBNA_SKRIPTA_NEAKTIVNA;
        stanje.obaZvona = false;
        stanje.trajanjeMrtvackogMin = TRAJANJE_MRTVACKOG_KLASICNO_MIN;
      }
      break;

    default:
      stanje.aktivna = false;
      stanje.faza = POGREBNA_SKRIPTA_NEAKTIVNA;
      stanje.obaZvona = false;
      stanje.trajanjeMrtvackogMin = TRAJANJE_MRTVACKOG_KLASICNO_MIN;
      break;
  }
}

void zaustaviPogrebneSkripte() {
  zaustaviAktivnuSekvencuBezLoga();
}

uint8_t dohvatiTipAktivnePogrebneSkripte() {
  if (!stanje.aktivna) {
    return 0;
  }
  return stanje.tip;
}

bool prebaciPokojnika() {
  return prebaciPogrebnuSekvencu(MUSKO_ZVONO);
}

bool prebaciPokojnicu() {
  return prebaciPogrebnuSekvencu(ZENSKO_ZVONO);
}
