// esp_serial_parser.cpp - Obrada dolaznih redaka s ESP32 mosta
#include "esp_serial_internal.h"

namespace {

const char* const ESP_KONTROLNI_TOKENI[] = {
    "ACK:",
    "ERR:",
    "WIFI:",
    "WIFISTATUS",
    "NTP:",
    "NTPLOG:",
    "SLANJE:",
    "STATUS:",
    "CFGREQ",
    "SETREQ:",
    "SETCFG:",
    "CMD:"};

char* pronadiUgradeniKontrolniToken(char* linija, size_t pocetniPomak) {
  if (linija == nullptr) {
    return nullptr;
  }

  char* najbolji = nullptr;
  for (size_t i = 0; i < (sizeof(ESP_KONTROLNI_TOKENI) / sizeof(ESP_KONTROLNI_TOKENI[0])); ++i) {
    char* pronadeno = strstr(linija + pocetniPomak, ESP_KONTROLNI_TOKENI[i]);
    if (pronadeno != nullptr && (najbolji == nullptr || pronadeno < najbolji)) {
      najbolji = pronadeno;
    }
  }

  return najbolji;
}

void odsjeciUgradeniKontrolniTokenAkoTreba(char* linija, size_t pocetniPomak) {
  char* ugradeniToken = pronadiUgradeniKontrolniToken(linija, pocetniPomak);
  if (ugradeniToken == nullptr) {
    return;
  }

  *ugradeniToken = '\0';
  trimJednolinijskiTekstESP(linija);
}

bool jePrepoznataESPLinija(const char* linija) {
  return strcmp(linija, "WIFI:CONNECTED") == 0 ||
         strcmp(linija, "WIFI:DISCONNECTED") == 0 ||
         strncmp(linija, "WIFI:", 5) == 0 ||
         strncmp(linija, "WIFI RX:", 8) == 0 ||
         strncmp(linija, "ACK:", 4) == 0 ||
         strncmp(linija, "ERR:", 4) == 0 ||
         strcmp(linija, "ESP BOOT") == 0 ||
         strncmp(linija, "FAZA:", 5) == 0 ||
         strncmp(linija, "SLANJE:", 7) == 0 ||
         strcmp(linija, "CFGREQ") == 0 ||
         strncmp(linija, "SETUPWIFI:", 10) == 0 ||
         strcmp(linija, "SETREQ:SUSTAV") == 0 ||
         strcmp(linija, "SETREQ:STAPICI") == 0 ||
         strcmp(linija, "SETREQ:BAT") == 0 ||
         strcmp(linija, "SETREQ:SUNCE") == 0 ||
         strcmp(linija, "SETREQ:MISE") == 0 ||
         strcmp(linija, "SETREQ:BLAGDANI_NEP") == 0 ||
         strcmp(linija, "SETREQ:BLAGDANI_POM") == 0 ||
         strncmp(linija, "SETCFG:SUSTAV|", 14) == 0 ||
         strncmp(linija, "SETCFG:STAPICI|", 15) == 0 ||
         strncmp(linija, "SETCFG:BAT|", 11) == 0 ||
         strncmp(linija, "SETCFG:SUNCE|", 13) == 0 ||
         strncmp(linija, "SETCFG:MISE|", 12) == 0 ||
         strncmp(linija, "SETCFG:BLAGDANI_NEP|", 20) == 0 ||
         strncmp(linija, "SETCFG:BLAGDANI_POM|", 20) == 0 ||
         strncmp(linija, "STATUS:", 7) == 0 ||
         strcmp(linija, "STATUS?") == 0 ||
         strncmp(linija, "NTP:", 4) == 0 ||
         strncmp(linija, "CMD:", 4) == 0 ||
         strncmp(linija, "NTPLOG:", 7) == 0;
}

bool obradiPomocnuESPDijagnostikuLiniju(char* linija) {
  if (linija == nullptr || linija[0] == '\0') {
    return false;
  }

  if (strncmp(linija, "WIFI RX:", 8) == 0) {
    odsjeciUgradeniKontrolniTokenAkoTreba(linija, 8);
    logirajLinijuESP(F("Mrezni most log: "), linija);
    return true;
  }

  if (strncmp(linija, "ACK:", 4) == 0 ||
      strncmp(linija, "ERR:", 4) == 0 ||
      strcmp(linija, "ESP BOOT") == 0 ||
      strncmp(linija, "FAZA:", 5) == 0 ||
      strncmp(linija, "SLANJE:", 7) == 0) {
    logirajLinijuESP(F("Mrezni most log: "), linija);
    return true;
  }

  return false;
}

}  // namespace

void obradiESPRedak() {
  trimBuffer();

  if (ulazniBufferDuljina == 0) {
    resetirajUlazniBuffer();
    return;
  }

  if (!jePrepoznataESPLinija(ulazniBuffer)) {
    logirajLinijuESP(F("Mrezni most linija: "), ulazniBuffer);
  }

  if (obradiPomocnuESPDijagnostikuLiniju(ulazniBuffer) ||
      obradiESPWiFiINtpLiniju(ulazniBuffer) ||
      obradiESPPostavkeLiniju(ulazniBuffer) ||
      obradiESPStatusnuLiniju(ulazniBuffer)) {
    resetirajUlazniBuffer();
    return;
  }

  if (strncmp(ulazniBuffer, "CMD:", 4) == 0) {
    const char* komanda = ulazniBuffer + 4;
    const ESPCmdIshod ishod = obradiESPCmdLiniju(komanda);

    if (ishod == ESP_CMD_NEPOZNATA) {
      espSerijskiPort.println(F("ERR:CMD"));
      logirajLinijuESP(F("Nepoznata CMD naredba: "), komanda);
    } else if (ishod == ESP_CMD_OK) {
      espSerijskiPort.println(F("ACK:CMD_OK"));
      logirajLinijuESP(F("Izvrsena CMD naredba: "), komanda);
    } else {
      espSerijskiPort.println(F("ERR:CMD_BUSY"));
      logirajLinijuESP(F("CMD naredba odbijena jer je toranjski sat zauzet: "), komanda);
    }

    resetirajUlazniBuffer();
    return;
  }

  logirajLinijuESP(F("Mrezni most log: "), ulazniBuffer);
  resetirajUlazniBuffer();
}
