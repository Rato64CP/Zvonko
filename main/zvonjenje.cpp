#include <Arduino.h>
#include <avr/pgmspace.h>
#include <RTClib.h>
#include "zvonjenje.h"
#include "slavljenje_mrtvacko.h"
#include "podesavanja_piny.h"
#include "time_glob.h"
#include "lcd_display.h"
#include "postavke.h"
#include "pc_serial.h"
#include "debouncing.h"

// ==================== CONSTANTS ====================

static const uint8_t BROJ_ZVONA_MAX = 2;
static const uint8_t BROJ_RUCNIH_SKLOPKI_ZVONA = 2;
static const unsigned long INTERVAL_TREPTANJA_LAMPICE_INERCIJE_MS = 500UL;
static const unsigned long MAKS_TRAJANJE_RUCNE_SKLOPKE_ZVONA_MS = 30UL * 60UL * 1000UL;
static const unsigned long MIN_RAD_ZA_PUNU_INERCIJU_BEZ_KOCNICE_MS = 20000UL;
static const unsigned long PROZOR_CEKANJA_DRUGOG_GASENJA_BEZ_KOCNICE_MS = 2000UL;

static const uint8_t PINOVI_ZVONA[BROJ_ZVONA_MAX] = {
  PIN_ZVONO_1,
  PIN_ZVONO_2
};

static const uint8_t PINOVI_LAMPICA_ZVONA[BROJ_ZVONA_MAX] = {
  PIN_LAMPICA_ZVONO_1,
  PIN_LAMPICA_ZVONO_2
};

static const uint8_t PINOVI_RUCNIH_SKLOPKI[BROJ_RUCNIH_SKLOPKI_ZVONA] = {
  PIN_BELL1_SWITCH,
  PIN_BELL2_SWITCH
};

// ==================== STATE TRACKING ====================

static struct {
  bool aktivan[BROJ_ZVONA_MAX];
  unsigned long start_ms[BROJ_ZVONA_MAX];
  unsigned long duration_ms[BROJ_ZVONA_MAX];
} zvona = {};

// Inercija nakon aktivacije zvona.
static struct {
  bool aktivna[BROJ_ZVONA_MAX];
  unsigned long vrijeme_pocetka[BROJ_ZVONA_MAX];
  unsigned long trajanje_ms[BROJ_ZVONA_MAX];
} inercija = {};

// Rucno upravljanje fizickim sklopkama ima prioritet nad automatikom.
static struct {
  bool override_aktivan[BROJ_RUCNIH_SKLOPKI_ZVONA];
  bool prisilno_iskljucen[BROJ_RUCNIH_SKLOPKI_ZVONA];
  unsigned long vrijeme_ukljucenja_ms[BROJ_RUCNIH_SKLOPKI_ZVONA];
} manualnoUpravljanje = {};

static struct {
  bool aktivno;
  uint8_t prvo_zvono;
  unsigned long vrijeme_prvog_zahtjeva_ms;
} sinkroniziranoGasenjeNaCekanju = {};


static bool dozvoliPaljenjeZvonaIzRucneSklopke = false;
static bool globalnaBlokadaZvona = false;
static bool blokadaZvonaTihiRezim = false;
static bool blokadaZvonaUPS = false;
static bool produzeniZavrsetakBezKocniceAktivan = false;
static uint8_t produzeniZavrsetakBezKocniceZvono = 0;

// ==================== RELAY CONTROL ====================

static bool jeValjanIndeksZvona(int indeks) {
  return indeks >= 0 && indeks < BROJ_ZVONA_MAX;
}

static bool jeZvonoOmogucenoPoPostavkama(int zvono) {
  return zvono >= 1 && zvono <= dohvatiBrojZvona();
}

static bool jeGlobalnaBlokadaZvonaAktivna() {
  return globalnaBlokadaZvona || blokadaZvonaTihiRezim || blokadaZvonaUPS;
}

static unsigned long dohvatiTrajanjeInercijeZvonaMs(int indeks) {
  if (indeks == 0) {
    return static_cast<unsigned long>(dohvatiInercijuZvona1Sekunde()) * 1000UL;
  }
  if (indeks == 1) {
    return static_cast<unsigned long>(dohvatiInercijuZvona2Sekunde()) * 1000UL;
  }
  return 0UL;
}

static unsigned long dohvatiDosadasnjeTrajanjeRadaZvonaMs(int indeks, unsigned long sadaMs) {
  if (!jeValjanIndeksZvona(indeks) || !zvona.aktivan[indeks]) {
    return 0UL;
  }

  return sadaMs - zvona.start_ms[indeks];
}

static unsigned long apsolutnaRazlikaULong(unsigned long prvi, unsigned long drugi) {
  return (prvi >= drugi) ? (prvi - drugi) : (drugi - prvi);
}

static bool jePunaInercijaDostupnaZaObaZvona(unsigned long trajanjeZvona1Ms,
                                             unsigned long trajanjeZvona2Ms) {
  return trajanjeZvona1Ms >= MIN_RAD_ZA_PUNU_INERCIJU_BEZ_KOCNICE_MS &&
         trajanjeZvona2Ms >= MIN_RAD_ZA_PUNU_INERCIJU_BEZ_KOCNICE_MS;
}

static unsigned long skratiTrajanjeUzMinimalniRad(unsigned long baznoTrajanjeMs,
                                                  unsigned long skracenjeMs) {
  if (baznoTrajanjeMs <= MIN_RAD_ZA_PUNU_INERCIJU_BEZ_KOCNICE_MS || skracenjeMs == 0UL) {
    return baznoTrajanjeMs;
  }

  if (baznoTrajanjeMs <= (MIN_RAD_ZA_PUNU_INERCIJU_BEZ_KOCNICE_MS + skracenjeMs)) {
    return MIN_RAD_ZA_PUNU_INERCIJU_BEZ_KOCNICE_MS;
  }

  return baznoTrajanjeMs - skracenjeMs;
}

static void ponistiCekanjeSinkroniziranogGasenja() {
  sinkroniziranoGasenjeNaCekanju.aktivno = false;
  sinkroniziranoGasenjeNaCekanju.prvo_zvono = 0;
  sinkroniziranoGasenjeNaCekanju.vrijeme_prvog_zahtjeva_ms = 0UL;
}

static void ponistiProduzeniZavrsetakBezKocnice() {
  produzeniZavrsetakBezKocniceAktivan = false;
  produzeniZavrsetakBezKocniceZvono = 0;
}

static void oznaciProduzeniZavrsetakBezKocnice(int zvono) {
  if (zvono < 1 || zvono > BROJ_ZVONA_MAX) {
    return;
  }

  produzeniZavrsetakBezKocniceAktivan = true;
  produzeniZavrsetakBezKocniceZvono = static_cast<uint8_t>(zvono);
}

static bool treperiLampicaZbogCekanjaDrugogGasenja(int indeks) {
  return jeValjanIndeksZvona(indeks) &&
         sinkroniziranoGasenjeNaCekanju.aktivno &&
         sinkroniziranoGasenjeNaCekanju.prvo_zvono == static_cast<uint8_t>(indeks + 1);
}

static bool treperiLampicaZbogProduzenogZavrsetkaBezKocnice(int indeks) {
  return jeValjanIndeksZvona(indeks) &&
         produzeniZavrsetakBezKocniceAktivan &&
         produzeniZavrsetakBezKocniceZvono == static_cast<uint8_t>(indeks + 1);
}

static void osvjeziLampicuZvona(int indeks, unsigned long sadaMs) {
  if (!jeValjanIndeksZvona(indeks)) {
    return;
  }

  bool lampicaUkljucena = false;
  if (treperiLampicaZbogCekanjaDrugogGasenja(indeks) ||
      treperiLampicaZbogProduzenogZavrsetkaBezKocnice(indeks) ||
      inercija.aktivna[indeks]) {
    lampicaUkljucena =
        ((sadaMs / INTERVAL_TREPTANJA_LAMPICE_INERCIJE_MS) % 2UL) == 0UL;
  } else if (zvona.aktivan[indeks]) {
    lampicaUkljucena = true;
  }

  digitalWrite(PINOVI_LAMPICA_ZVONA[indeks], lampicaUkljucena ? HIGH : LOW);
}

static void osvjeziLampiceZvona(unsigned long sadaMs) {
  for (uint8_t i = 0; i < BROJ_ZVONA_MAX; ++i) {
    osvjeziLampicuZvona(i, sadaMs);
  }
}

static void pokreniInercijuZvona(int indeks) {
  if (!jeValjanIndeksZvona(indeks)) {
    return;
  }

  inercija.aktivna[indeks] = true;
  inercija.vrijeme_pocetka[indeks] = millis();
  inercija.trajanje_ms[indeks] = dohvatiTrajanjeInercijeZvonaMs(indeks);
}

static void zaustaviInercijuZvona(int indeks) {
  if (!jeValjanIndeksZvona(indeks)) {
    return;
  }

  inercija.aktivna[indeks] = false;
  inercija.vrijeme_pocetka[indeks] = 0UL;
  inercija.trajanje_ms[indeks] = 0UL;
}

static void prekiniPosebneNacineZbogZvona(int indeks) {
  bool prekinutoSlavljenje = false;
  bool prekinutoMrtvacko = false;

  if (jeSlavljenjeUTijeku()) {
    zaustaviSlavljenje();
    prekinutoSlavljenje = true;
  }

  if (jeMrtvackoUTijeku()) {
    zaustaviMrtvacko();
    prekinutoMrtvacko = true;
  }

  if (!prekinutoSlavljenje && !prekinutoMrtvacko) {
    return;
  }

  char log[88];
  snprintf_P(log, sizeof(log), PSTR("Zvono%d: ima prioritet i prekida %s"),
             indeks + 1,
             (prekinutoSlavljenje && prekinutoMrtvacko) ? "slavljenje i mrtvacko"
             : (prekinutoSlavljenje ? "slavljenje" : "mrtvacko"));
  posaljiPCLog(log);
}

static void aktivirajBell_Relej(int indeks) {
  if (!jeValjanIndeksZvona(indeks)) {
    return;
  }

  // Zvona imaju prioritet nad posebnim nacinima cekica toranjskog sata.
  prekiniPosebneNacineZbogZvona(indeks);
  zaustaviInercijuZvona(indeks);
  digitalWrite(PINOVI_ZVONA[indeks], HIGH);
  osvjeziLampicuZvona(indeks, millis());

  char log[40];
  snprintf_P(log,
             sizeof(log),
             PSTR("Zvono%d: aktivirana"),
             indeks + 1);
  posaljiPCLog(log);
  signalizirajZvono_Ringing(indeks + 1);
}

static void deaktivirajBell_Relej(int indeks) {
  if (!jeValjanIndeksZvona(indeks)) {
    return;
  }

  digitalWrite(PINOVI_ZVONA[indeks], LOW);
  osvjeziLampicuZvona(indeks, millis());
  char log[32];
  snprintf_P(log, sizeof(log), PSTR("Zvono%d: deaktivirana"), indeks + 1);
  posaljiPCLog(log);
}

static void ukljuciZvonoIzRucneSklopke(int zvono) {
  dozvoliPaljenjeZvonaIzRucneSklopke = true;
  ukljuciZvono(zvono);
  dozvoliPaljenjeZvonaIzRucneSklopke = false;
}

static void zakaziProduzeniRadZvona(int zvono, unsigned long dodatnoTrajanjeMs) {
  const int indeks = zvono - 1;
  if (!jeValjanIndeksZvona(indeks) || !zvona.aktivan[indeks] || dodatnoTrajanjeMs == 0UL) {
    return;
  }

  zvona.start_ms[indeks] = millis();
  zvona.duration_ms[indeks] = dodatnoTrajanjeMs;
  oznaciProduzeniZavrsetakBezKocnice(zvono);
  osvjeziLampicuZvona(indeks, zvona.start_ms[indeks]);
}

static bool suObaZvonaDovoljnoDugoRadilaZaSinkroniziranoGasenje() {
  const unsigned long sadaMs = millis();
  return jePunaInercijaDostupnaZaObaZvona(
      dohvatiDosadasnjeTrajanjeRadaZvonaMs(0, sadaMs),
      dohvatiDosadasnjeTrajanjeRadaZvonaMs(1, sadaMs));
}

static bool mozePokrenutiCekanjeSinkroniziranogGasenja(int zvono) {
  const int indeks = zvono - 1;
  const int drugiIndeks = (indeks == 0) ? 1 : 0;
  return !jeKocnicaZvonaOmogucena() &&
         jeValjanIndeksZvona(indeks) &&
         jeValjanIndeksZvona(drugiIndeks) &&
         zvona.aktivan[indeks] &&
         zvona.aktivan[drugiIndeks] &&
         suObaZvonaDovoljnoDugoRadilaZaSinkroniziranoGasenje();
}

static void pokreniCekanjeSinkroniziranogGasenja(int zvono) {
  sinkroniziranoGasenjeNaCekanju.aktivno = true;
  sinkroniziranoGasenjeNaCekanju.prvo_zvono = static_cast<uint8_t>(zvono);
  sinkroniziranoGasenjeNaCekanju.vrijeme_prvog_zahtjeva_ms = millis();

  char log[128];
  snprintf_P(log,
             sizeof(log),
             PSTR("Zvona: prvo rucno/daljinsko gasenje ZVONO%d, cekam drugo do %lus radi sinkronog zavrsetka bez kocnice"),
             zvono,
             static_cast<unsigned long>(PROZOR_CEKANJA_DRUGOG_GASENJA_BEZ_KOCNICE_MS / 1000UL));
  posaljiPCLog(log);
}

static void primijeniSinkroniziranoGasenjeIzCekanja() {
  const unsigned long inercijaZvona1Ms = dohvatiTrajanjeInercijeZvonaMs(0);
  const unsigned long inercijaZvona2Ms = dohvatiTrajanjeInercijeZvonaMs(1);
  const unsigned long razlikaInercijeMs =
      apsolutnaRazlikaULong(inercijaZvona1Ms, inercijaZvona2Ms);

  if (razlikaInercijeMs == 0UL) {
    iskljuciZvono(1);
    iskljuciZvono(2);
    posaljiPCLog(F("Zvona: oba OFF zahtjeva spojena u zajednicko gasenje, inercije su jednake"));
    ponistiCekanjeSinkroniziranogGasenja();
    return;
  }

  const int zvonoSDuljomInercijom = (inercijaZvona1Ms > inercijaZvona2Ms) ? 1 : 2;
  const int zvonoSKracomInercijom = (zvonoSDuljomInercijom == 1) ? 2 : 1;
  iskljuciZvono(zvonoSDuljomInercijom);
  zakaziProduzeniRadZvona(zvonoSKracomInercijom, razlikaInercijeMs);

  char log[144];
  snprintf_P(log,
             sizeof(log),
             PSTR("Zvona: oba OFF zahtjeva spojena u sinkroni zavrsetak bez kocnice, prvo gasim ZVONO%d pa ostavljam ZVONO%d jos %lus"),
             zvonoSDuljomInercijom,
             zvonoSKracomInercijom,
             static_cast<unsigned long>(razlikaInercijeMs / 1000UL));
  posaljiPCLog(log);
  ponistiCekanjeSinkroniziranogGasenja();
}

static void obradiCekanjeSinkroniziranogGasenja(unsigned long sadaMs) {
  if (!sinkroniziranoGasenjeNaCekanju.aktivno) {
    return;
  }

  const int prvoZvono = sinkroniziranoGasenjeNaCekanju.prvo_zvono;
  const int drugoZvono = (prvoZvono == 1) ? 2 : 1;

  if (!jeZvonoAktivno(prvoZvono)) {
    ponistiCekanjeSinkroniziranogGasenja();
    return;
  }

  if (!jeZvonoAktivno(drugoZvono)) {
    ponistiCekanjeSinkroniziranogGasenja();
    iskljuciZvono(prvoZvono);
    return;
  }

  if ((sadaMs - sinkroniziranoGasenjeNaCekanju.vrijeme_prvog_zahtjeva_ms) <
      PROZOR_CEKANJA_DRUGOG_GASENJA_BEZ_KOCNICE_MS) {
    return;
  }

  char log[112];
  snprintf_P(log,
             sizeof(log),
             PSTR("Zvona: drugo gasenje nije stiglo unutar %lus, ZVONO%d gasim pojedinacno"),
             static_cast<unsigned long>(PROZOR_CEKANJA_DRUGOG_GASENJA_BEZ_KOCNICE_MS / 1000UL),
             prvoZvono);
  posaljiPCLog(log);
  ponistiCekanjeSinkroniziranogGasenja();
  iskljuciZvono(prvoZvono);
}

static void obradiSigurnosniTimeoutRucnihSklopkiZvona(unsigned long sadaMs) {
  for (uint8_t i = 0; i < BROJ_RUCNIH_SKLOPKI_ZVONA; ++i) {
    if (!manualnoUpravljanje.override_aktivan[i] ||
        manualnoUpravljanje.prisilno_iskljucen[i] ||
        manualnoUpravljanje.vrijeme_ukljucenja_ms[i] == 0UL) {
      continue;
    }

    if ((sadaMs - manualnoUpravljanje.vrijeme_ukljucenja_ms[i]) <
        MAKS_TRAJANJE_RUCNE_SKLOPKE_ZVONA_MS) {
      continue;
    }

    manualnoUpravljanje.override_aktivan[i] = false;
    manualnoUpravljanje.prisilno_iskljucen[i] = true;
    iskljuciZvono(i + 1);

    char log[96];
    snprintf_P(log,
               sizeof(log),
               PSTR("Rucno ZVONO%d: sigurnosno gasenje nakon 30 min, cekam fizicki povrat prekidaca na OFF"),
               i + 1);
    posaljiPCLog(log);
  }
}

// ==================== PUBLIC API ====================

void izracunajTrajanjaDvajuZvonaZaSinkroniZavrsetak(unsigned long baznoTrajanjeMs,
                                                    unsigned long& trajanjeZvona1Ms,
                                                    unsigned long& trajanjeZvona2Ms) {
  trajanjeZvona1Ms = baznoTrajanjeMs;
  trajanjeZvona2Ms = baznoTrajanjeMs;

  if (jeKocnicaZvonaOmogucena() ||
      baznoTrajanjeMs < MIN_RAD_ZA_PUNU_INERCIJU_BEZ_KOCNICE_MS) {
    return;
  }

  const unsigned long inercijaZvona1Ms = dohvatiTrajanjeInercijeZvonaMs(0);
  const unsigned long inercijaZvona2Ms = dohvatiTrajanjeInercijeZvonaMs(1);
  const unsigned long razlikaInercijeMs =
      apsolutnaRazlikaULong(inercijaZvona1Ms, inercijaZvona2Ms);

  if (razlikaInercijeMs == 0UL) {
    return;
  }

  if (inercijaZvona1Ms > inercijaZvona2Ms) {
    trajanjeZvona1Ms = skratiTrajanjeUzMinimalniRad(baznoTrajanjeMs, razlikaInercijeMs);
    return;
  }

  trajanjeZvona2Ms = skratiTrajanjeUzMinimalniRad(baznoTrajanjeMs, razlikaInercijeMs);
}

void ukljuciZvono(int zvono) {
  const int indeks = zvono - 1;
  if (!jeValjanIndeksZvona(indeks) || !jeZvonoOmogucenoPoPostavkama(zvono)) {
    return;
  }

  if (jeGlobalnaBlokadaZvonaAktivna()) {
    return;
  }

  // Fizicke sklopke zvona moraju ostati upotrebljive i kad RTC/NTP jos nije
  // potvrdio vrijeme. Time lokalni zvonar i dalje moze rucno ukljuciti ili
  // iskljuciti zvono, dok automatski i daljinski izvori cekaju potvrdu vremena.
  if (!jeVrijemePotvrdjenoZaAutomatiku() && !dozvoliPaljenjeZvonaIzRucneSklopke) {
    posaljiPCLog(F("Zvona: automatsko ili daljinsko paljenje blokirano dok vrijeme nije potvrdeno"));
    return;
  }

  if (sinkroniziranoGasenjeNaCekanju.aktivno &&
      sinkroniziranoGasenjeNaCekanju.prvo_zvono == static_cast<uint8_t>(zvono)) {
    ponistiCekanjeSinkroniziranogGasenja();
    osvjeziLampiceZvona(millis());

    char log[96];
    snprintf_P(log,
               sizeof(log),
               PSTR("Zvona: ponovni ON za ZVONO%d otkazuje cekanje drugog gasenja"),
               zvono);
    posaljiPCLog(log);
  }

  if (produzeniZavrsetakBezKocniceAktivan &&
      produzeniZavrsetakBezKocniceZvono == static_cast<uint8_t>(zvono)) {
    ponistiProduzeniZavrsetakBezKocnice();
  }

  if (!zvona.aktivan[indeks]) {
    aktivirajBell_Relej(indeks);
    zvona.aktivan[indeks] = true;
    // Rucno i daljinsko paljenje preko weba/API-ja traje dok ne stigne
    // eksplicitno gasenje. Samo putanja `aktivirajZvonjenjeNaTrajanje()`
    // postavlja vremenski ogranicen rad.
    zvona.start_ms[indeks] = millis();
    zvona.duration_ms[indeks] = 0;
    osvjeziLampicuZvona(indeks, zvona.start_ms[indeks]);
  }
}

void iskljuciZvono(int zvono) {
  const int indeks = zvono - 1;
  if (!jeValjanIndeksZvona(indeks)) {
    return;
  }

  if (produzeniZavrsetakBezKocniceAktivan &&
      produzeniZavrsetakBezKocniceZvono == static_cast<uint8_t>(zvono)) {
    ponistiProduzeniZavrsetakBezKocnice();
  }

  if (zvona.aktivan[indeks]) {
    deaktivirajBell_Relej(indeks);
    zvona.aktivan[indeks] = false;
    zvona.start_ms[indeks] = 0;
    zvona.duration_ms[indeks] = 0;
    pokreniInercijuZvona(indeks);
    osvjeziLampicuZvona(indeks, millis());

    char log[56];
    snprintf_P(log,
               sizeof(log),
               PSTR("Zvono%d: inercija (%us) poceta"),
               indeks + 1,
               static_cast<unsigned>((inercija.trajanje_ms[indeks] + 500UL) / 1000UL));
    posaljiPCLog(log);
  }
}

void zahtijevajOperatorovoGasenjeZvona(int zvono) {
  const int indeks = zvono - 1;
  if (!jeValjanIndeksZvona(indeks)) {
    return;
  }

  if (!sinkroniziranoGasenjeNaCekanju.aktivno) {
    if (mozePokrenutiCekanjeSinkroniziranogGasenja(zvono)) {
      pokreniCekanjeSinkroniziranogGasenja(zvono);
      return;
    }

    iskljuciZvono(zvono);
    return;
  }

  if (sinkroniziranoGasenjeNaCekanju.prvo_zvono == static_cast<uint8_t>(zvono)) {
    return;
  }

  if (!mozePokrenutiCekanjeSinkroniziranogGasenja(zvono)) {
    ponistiCekanjeSinkroniziranogGasenja();
    iskljuciZvono(zvono);
    return;
  }

  primijeniSinkroniziranoGasenjeIzCekanja();
}

bool jeLiInerciaAktivna() {
  bool baremJednaAktivna = false;
  const unsigned long sadaMs = millis();

  for (uint8_t i = 0; i < BROJ_ZVONA_MAX; ++i) {
    if (!inercija.aktivna[i]) {
      continue;
    }

    const unsigned long proteklo = sadaMs - inercija.vrijeme_pocetka[i];
    if (proteklo >= inercija.trajanje_ms[i]) {
      inercija.aktivna[i] = false;
      osvjeziLampicuZvona(i, sadaMs);
      char log[48];
      snprintf_P(log,
                 sizeof(log),
                 PSTR("Inercija Z%d: zavrsena nakon %us"),
                 i + 1,
                 static_cast<unsigned>((inercija.trajanje_ms[i] + 500UL) / 1000UL));
      posaljiPCLog(log);
      continue;
    }

    baremJednaAktivna = true;
  }

  osvjeziLampiceZvona(sadaMs);
  return baremJednaAktivna;
}

bool jeZvonoUTijeku() {
  for (uint8_t i = 0; i < BROJ_ZVONA_MAX; i++) {
    if (zvona.aktivan[i]) {
      return true;
    }
  }
  return false;
}

bool jeZvonoAktivno(int zvono) {
  const int indeks = zvono - 1;
  if (!jeValjanIndeksZvona(indeks)) {
    return false;
  }
  return zvona.aktivan[indeks];
}

bool jeProduzeniZavrsetakZvonaBezKocniceAktivan() {
  return produzeniZavrsetakBezKocniceAktivan;
}

void aktivirajZvonjenje(int zvono) {
  ukljuciZvono(zvono);
}

void aktivirajZvonjenjeNaTrajanje(int zvono, unsigned long trajanjeMs) {
  const unsigned long sadaMs = millis();
  aktivirajZvonjenje(zvono);
  const int indeks = zvono - 1;
  if (jeValjanIndeksZvona(indeks)) {
    zvona.start_ms[indeks] = sadaMs;
    zvona.duration_ms[indeks] = trajanjeMs;
  }
}

void deaktivirajZvonjenje(int zvono) {
  iskljuciZvono(zvono);
}

void iskljuciObaZvonaSinkronizirano() {
  ponistiCekanjeSinkroniziranogGasenja();
  ponistiProduzeniZavrsetakBezKocnice();
  const bool obaZvonaAktivna = jeZvonoAktivno(1) && jeZvonoAktivno(2);
  if (!jeKocnicaZvonaOmogucena() && obaZvonaAktivna) {
    const unsigned long sadaMs = millis();
    const unsigned long trajanjeZvona1Ms = dohvatiDosadasnjeTrajanjeRadaZvonaMs(0, sadaMs);
    const unsigned long trajanjeZvona2Ms = dohvatiDosadasnjeTrajanjeRadaZvonaMs(1, sadaMs);
    if (jePunaInercijaDostupnaZaObaZvona(trajanjeZvona1Ms, trajanjeZvona2Ms)) {
      const unsigned long inercijaZvona1Ms = dohvatiTrajanjeInercijeZvonaMs(0);
      const unsigned long inercijaZvona2Ms = dohvatiTrajanjeInercijeZvonaMs(1);
      const unsigned long razlikaInercijeMs =
          apsolutnaRazlikaULong(inercijaZvona1Ms, inercijaZvona2Ms);

      if (razlikaInercijeMs > 0UL) {
        if (inercijaZvona1Ms > inercijaZvona2Ms) {
          iskljuciZvono(1);
          zakaziProduzeniRadZvona(2, razlikaInercijeMs);
        } else {
          iskljuciZvono(2);
          zakaziProduzeniRadZvona(1, razlikaInercijeMs);
        }

        char log[120];
        snprintf_P(log,
                   sizeof(log),
                   PSTR("Zvona: bez kocnice sinkroniziram kraj po inerciji (Z1=%lus, Z2=%lus, prag=%lus)"),
                   static_cast<unsigned long>(inercijaZvona1Ms / 1000UL),
                   static_cast<unsigned long>(inercijaZvona2Ms / 1000UL),
                   static_cast<unsigned long>(MIN_RAD_ZA_PUNU_INERCIJU_BEZ_KOCNICE_MS / 1000UL));
        posaljiPCLog(log);
        return;
      }
    }
  }

  iskljuciZvono(1);
  iskljuciZvono(2);
}

static void primijeniEfektivnuGlobalnuBlokaduZvona(bool prethodnoAktivna) {
  const bool novaBlokada = jeGlobalnaBlokadaZvonaAktivna();
  if (prethodnoAktivna == novaBlokada) {
    return;
  }

  if (novaBlokada) {
    ponistiCekanjeSinkroniziranogGasenja();
    ponistiProduzeniZavrsetakBezKocnice();
    for (uint8_t i = 0; i < BROJ_ZVONA_MAX; ++i) {
      if (!zvona.aktivan[i]) {
        continue;
      }

      deaktivirajBell_Relej(i);
      zvona.aktivan[i] = false;
      zvona.start_ms[i] = 0UL;
      zvona.duration_ms[i] = 0UL;
    }
    osvjeziLampiceZvona(millis());
    posaljiPCLog(F("Globalna blokada zvona: UKLJUCENA"));
  } else {
    posaljiPCLog(F("Globalna blokada zvona: ISKLJUCENA"));
  }
}

void postaviGlobalnuBlokaduZvona(bool blokiraj) {
  if (globalnaBlokadaZvona == blokiraj) {
    return;
  }

  const bool prethodnoAktivna = jeGlobalnaBlokadaZvonaAktivna();
  globalnaBlokadaZvona = blokiraj;
  primijeniEfektivnuGlobalnuBlokaduZvona(prethodnoAktivna);
}

void postaviBlokaduZvonaTihiRezim(bool blokiraj) {
  if (blokadaZvonaTihiRezim == blokiraj) {
    return;
  }

  const bool prethodnoAktivna = jeGlobalnaBlokadaZvonaAktivna();
  blokadaZvonaTihiRezim = blokiraj;
  primijeniEfektivnuGlobalnuBlokaduZvona(prethodnoAktivna);
}

void postaviBlokaduZvonaUPS(bool blokiraj) {
  if (blokadaZvonaUPS == blokiraj) {
    return;
  }

  const bool prethodnoAktivna = jeGlobalnaBlokadaZvonaAktivna();
  blokadaZvonaUPS = blokiraj;
  primijeniEfektivnuGlobalnuBlokaduZvona(prethodnoAktivna);
}

// ==================== INITIALIZATION ====================

void inicijalizirajZvona() {
  for (uint8_t i = 0; i < BROJ_ZVONA_MAX; i++) {
    pinMode(PINOVI_ZVONA[i], OUTPUT);
    digitalWrite(PINOVI_ZVONA[i], LOW);
    pinMode(PINOVI_LAMPICA_ZVONA[i], OUTPUT);
    digitalWrite(PINOVI_LAMPICA_ZVONA[i], LOW);
    zvona.aktivan[i] = false;
    zvona.start_ms[i] = 0;
    zvona.duration_ms[i] = 0;
  }

  for (uint8_t i = 0; i < BROJ_RUCNIH_SKLOPKI_ZVONA; i++) {
    pinMode(PINOVI_RUCNIH_SKLOPKI[i], INPUT_PULLUP);
    manualnoUpravljanje.override_aktivan[i] = false;
    manualnoUpravljanje.prisilno_iskljucen[i] = false;
    manualnoUpravljanje.vrijeme_ukljucenja_ms[i] = 0UL;
  }

  for (uint8_t i = 0; i < BROJ_ZVONA_MAX; ++i) {
    inercija.aktivna[i] = false;
    inercija.vrijeme_pocetka[i] = 0UL;
    inercija.trajanje_ms[i] = 0UL;
  }
  ponistiCekanjeSinkroniziranogGasenja();
  ponistiProduzeniZavrsetakBezKocnice();
  posaljiPCLog(F("Zvona: inicijalizirana za 2 zvona toranjskog sata"));
}

// ==================== MAIN LOOP MANAGEMENT ====================

void upravljajZvonom() {
  unsigned long sadaMs = millis();
  SwitchState novoStanje = SWITCH_RELEASED;

  obradiCekanjeSinkroniziranogGasenja(sadaMs);
  obradiSigurnosniTimeoutRucnihSklopkiZvona(sadaMs);

  for (uint8_t i = 0; i < BROJ_RUCNIH_SKLOPKI_ZVONA; i++) {
    if (obradiDebouncedInput(PINOVI_RUCNIH_SKLOPKI[i], 30, &novoStanje)) {
      char log[80];
      snprintf_P(log, sizeof(log), PSTR("Rucno ZVONO%d %s"), i + 1,
                 (novoStanje == SWITCH_PRESSED) ? "ON" : "OFF");

      if (novoStanje == SWITCH_PRESSED) {
        if (jeZvonoOmogucenoPoPostavkama(i + 1)) {
          manualnoUpravljanje.override_aktivan[i] = true;
          manualnoUpravljanje.prisilno_iskljucen[i] = false;
          manualnoUpravljanje.vrijeme_ukljucenja_ms[i] = sadaMs;
          if (!jeGlobalnaBlokadaZvonaAktivna()) {
            ukljuciZvonoIzRucneSklopke(i + 1);
          } else {
            snprintf_P(log, sizeof(log), PSTR("Rucno ZVONO%d ON (blokirano globalnom blokadom)"), i + 1);
          }
        } else {
          manualnoUpravljanje.override_aktivan[i] = false;
          manualnoUpravljanje.prisilno_iskljucen[i] = false;
          manualnoUpravljanje.vrijeme_ukljucenja_ms[i] = 0UL;
          snprintf_P(log, sizeof(log), PSTR("Rucno ZVONO%d ON (preskoceno)"), i + 1);
        }
      } else {
        manualnoUpravljanje.override_aktivan[i] = false;
        manualnoUpravljanje.prisilno_iskljucen[i] = false;
        manualnoUpravljanje.vrijeme_ukljucenja_ms[i] = 0UL;
        zahtijevajOperatorovoGasenjeZvona(i + 1);
      }
      posaljiPCLog(log);
    }
  }

  if (jeGlobalnaBlokadaZvonaAktivna()) {
    ponistiCekanjeSinkroniziranogGasenja();
    ponistiProduzeniZavrsetakBezKocnice();
    for (uint8_t i = 0; i < BROJ_ZVONA_MAX; ++i) {
      if (zvona.aktivan[i]) {
        deaktivirajBell_Relej(i);
        zvona.aktivan[i] = false;
        zvona.start_ms[i] = 0UL;
        zvona.duration_ms[i] = 0UL;
      }
    }

    jeLiInerciaAktivna();
    return;
  }

  // Ako je rucni override aktivan, ima prioritet nad web/API automatikom.
  for (uint8_t i = 0; i < BROJ_RUCNIH_SKLOPKI_ZVONA; i++) {
    if (manualnoUpravljanje.override_aktivan[i] &&
        !manualnoUpravljanje.prisilno_iskljucen[i] &&
        !zvona.aktivan[i]) {
      ukljuciZvonoIzRucneSklopke(i + 1);
    }
  }

  jeLiInerciaAktivna();
  osvjeziLampiceZvona(sadaMs);

  for (uint8_t i = 0; i < BROJ_ZVONA_MAX; i++) {
    const bool rucniOverride =
        (i < BROJ_RUCNIH_SKLOPKI_ZVONA) ? manualnoUpravljanje.override_aktivan[i] : false;
    if (zvona.aktivan[i] && !rucniOverride && zvona.duration_ms[i] > 0) {
      const unsigned long proteklo = sadaMs - zvona.start_ms[i];
      if (proteklo >= zvona.duration_ms[i]) {
        iskljuciZvono(i + 1);
        char log[40];
        snprintf_P(log, sizeof(log), PSTR("Zvono%d: trajanje isteklo"), i + 1);
        posaljiPCLog(log);
      }
    }
  }
}
