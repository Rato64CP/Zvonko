// kazaljke_sata.h
#pragma once

#include <RTClib.h>

void inicijalizirajKazaljke();
void upravljajKazaljkama();
void upravljajKorekcijomKazaljki();
void postaviTrenutniPolozajKazaljki(int trenutnaMinuta);
bool suKazaljkeUSinkronu();
int dohvatiMemoriraneKazaljkeMinuta();
void obavijestiKazaljkeDSTPromjena(int pomakMinuta);

// Ručna pozicija i brza korekcija kazaljki
void postaviRucnuPozicijuKazaljki(int satKazaljke, int minutaKazaljke);
void pokreniBudnoKorekciju();
void zatraziPoravnanjeTaktaKazaljki();
void postaviRucnuBlokaduKazaljki(bool blokirano);
bool jeRucnaBlokadaKazaljkiAktivna();
bool mozeSeRucnoNamjestatiKazaljke();
