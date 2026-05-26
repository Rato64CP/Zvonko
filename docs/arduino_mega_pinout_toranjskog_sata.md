# ZVONKO v. 1.0 - Arduino Mega pinout toranjskog sata

Ovaj dokument je citljiv pregled svih aktivnih pinova i konekcija za `Arduino Mega 2560` u sustavu `ZVONKO v. 1.0`. Glavni izvor istine i dalje ostaje [podesavanja_piny.h](../main/podesavanja_piny.h), a ova datoteka sluzi kao pomoc pri spajanju, servisiranju i provjeri instalacije.

## Brzi pregled svih aktivnih konekcija

Ova tablica daje servisni pregled svih trenutno aktivnih konekcija na `Arduino Mega 2560`, ne samo izlaza.

| Pin | Funkcija | Smjer | Tip / podsustav | Aktivno stanje | Napomena |
|---:|---|---|---|---|---|
| `2` | `RTC SQW` | Ulaz | Sinkronizacija vremena | impuls `1 Hz` | `DS3231 SQW` referentni takt |
| `3` | `433 MHz DATA` | Ulaz | Daljinski prijemnik | impulsni signal | `SRX882` data izlaz |
| `7` | Tipka `GORE` | Ulaz | Lokalni izbornik | `LOW` | `INPUT_PULLUP` |
| `8` | Tipka `DOLJE` | Ulaz | Lokalni izbornik | `LOW` | `INPUT_PULLUP` |
| `9` | Tipka `LIJEVO` | Ulaz | Lokalni izbornik | `LOW` | `INPUT_PULLUP` |
| `10` | Tipka `DESNO` | Ulaz | Lokalni izbornik | `LOW` | `INPUT_PULLUP` |
| `11` | Tipka `DA` | Ulaz | Lokalni izbornik | `LOW` | `INPUT_PULLUP` |
| `12` | Tipka `NE` | Ulaz | Lokalni izbornik | `LOW` | `INPUT_PULLUP` |
| `14` | `TX3` | Dvosmjerno | `ESP32` serijski most | UART | `Serial3` prema `ESP32` |
| `15` | `RX3` | Dvosmjerno | `ESP32` serijski most | UART | `Serial3` prema `ESP32` |
| `18` | `TX1` | Dvosmjerno | slobodno | UART | Rezervirano za buduce prosirenje |
| `19` | `RX1` | Dvosmjerno | slobodno | UART | Rezervirano za buduce prosirenje |
| `20` | `SDA` | Dvosmjerno | `I2C` | sabirnica | `LCD`, `DS3231`, EEPROM/FRAM |
| `21` | `SCL` | Dvosmjerno | `I2C` | sabirnica | `LCD`, `DS3231`, EEPROM/FRAM |
| `22` | Relej parne kazaljke | Izlaz | Relej | `HIGH` | Impuls kazaljki, parni korak |
| `23` | Relej neparne kazaljke | Izlaz | Relej | `HIGH` | Impuls kazaljki, neparni korak |
| `24` | Relej parne ploce | Izlaz | Relej | `HIGH` | Prva faza pomaka okretne ploce |
| `25` | Relej neparne ploce | Izlaz | Relej | `HIGH` | Druga faza pomaka okretne ploce |
| `26` | Zvono 1 | Izlaz | Relej | `HIGH` | Zvono 1, automatika ili rucna sklopka |
| `27` | Zvono 2 | Izlaz | Relej | `HIGH` | Zvono 2, automatika ili rucna sklopka |
| `28` | Cekic 1 - muski | Izlaz | Relej/cekic | `HIGH` | Puni sat, slavljenje i mrtvacko |
| `29` | Cekic 2 - zenski | Izlaz | Relej/cekic | `HIGH` | Pola sata, kvartalno i posebni nacini |
| `30` | Cavao 1 | Ulaz | Okretna ploca | prema kontaktu | Ulaz ploce |
| `31` | Cavao 2 | Ulaz | Okretna ploca | prema kontaktu | Ulaz ploce |
| `32` | Cavao 3 | Ulaz | Okretna ploca | prema kontaktu | Ulaz ploce |
| `33` | Cavao 4 | Ulaz | Okretna ploca | prema kontaktu | Ulaz ploce |
| `34` | Cavao 5 | Ulaz | Okretna ploca | prema kontaktu | Ulaz ploce |
| `35` | slobodno | - | - | - | Pin vise nema dodijeljenu funkciju |
| `36` | Lampica Zvono 1 | Izlaz | LED | `HIGH` | Aktivnost ili signalizirani zavrsetak |
| `37` | Lampica Zvono 2 | Izlaz | LED | `HIGH` | Aktivnost ili signalizirani zavrsetak |
| `38` | Lampica Slavljenje | Izlaz | LED | `HIGH` / treptanje | Slavljenje ili termalna pauza |
| `39` | Lampica Mrtvacko | Izlaz | LED | `HIGH` / treptanje | Mrtvacko ili cekanje zavrsetka |
| `40` | Nadzor mreze | Ulaz | `UPS` | `LOW=mreza` | `HIGH` znaci rad samo s `UPS-a` |
| `41` | Kip-prekidac tisine | Ulaz | Tihi rezim | `LOW=ON` | `INPUT_PULLUP` |
| `42` | Tipka mrtvackog | Ulaz | Posebni nacin | `LOW` | Trenutni `toggle` ulaz |
| `43` | Kip-prekidac slavljenja | Ulaz | Posebni nacin | `LOW=ON` | `INPUT_PULLUP` |
| `44` | Rucna sklopka zvona 1 | Ulaz | Rucni override | `LOW=ON` | `INPUT_PULLUP` |
| `45` | Rucna sklopka zvona 2 | Ulaz | Rucni override | `LOW=ON` | `INPUT_PULLUP` |
| `46` | Lampica tihog rezima | Izlaz | LED | `HIGH` | Konacni tihi rezim aktivan |
| `47` | Relej nocne rasvjete | Izlaz | Relej | `HIGH` | Ukljucen nocu prema suncevoj automatici |
| `A0` | Thumbwheel desetice `1` | Ulaz | Mrtvacko timer | `LOW` prema `GND` | `BCD 1` |
| `A2` | Thumbwheel desetice `2` | Ulaz | Mrtvacko timer | `LOW` prema `GND` | `BCD 2` |
| `A3` | Thumbwheel desetice `4` | Ulaz | Mrtvacko timer | `LOW` prema `GND` | `BCD 4` |
| `A4` | Thumbwheel desetice `8` | Ulaz | Mrtvacko timer | `LOW` prema `GND` | `BCD 8` |
| `A5` | Thumbwheel jedinice `8` | Ulaz | Mrtvacko timer | `LOW` prema `GND` | `BCD 8` |
| `A6` | Thumbwheel jedinice `4` | Ulaz | Mrtvacko timer | `LOW` prema `GND` | `BCD 4` |
| `A7` | Thumbwheel jedinice `2` | Ulaz | Mrtvacko timer | `LOW` prema `GND` | `BCD 2` |
| `A8` | Thumbwheel jedinice `1` | Ulaz | Mrtvacko timer | `LOW` prema `GND` | `BCD 1` |
| `A9` | Tipka sunce vece | Ulaz | Sunceva automatika | `LOW` | Trenutni servisni unos |
| `A10` | Lampica sunce vece | Izlaz | LED | `HIGH` | Stanje vecernje automatike |
| `A11` | Tipka sunce jutro | Ulaz | Sunceva automatika | `LOW` | Trenutni servisni unos |
| `A12` | Lampica sunce jutro | Izlaz | LED | `HIGH` | Stanje jutarnje automatike |
| `A13` | Tipka sunce podne | Ulaz | Sunceva automatika | `LOW` | Trenutni servisni unos |
| `A14` | Lampica sunce podne | Izlaz | LED | `HIGH` | Stanje podnevne automatike |

Napomena:
- `A1` trenutno nije dio aktivnog rasporeda toranjskog sata
- pinovi `5`, `6`, `16` i `17` trenutačno nisu zauzeti aktivnim firmwareom

## Releji kazaljki

| Funkcija | Pin | Napomena |
|---|---:|---|
| Relej parne kazaljke | `22` | Prva faza impulsa za kazaljke |
| Relej neparne kazaljke | `23` | Druga faza impulsa za kazaljke |

## Releji okretne ploce

| Funkcija | Pin | Napomena |
|---|---:|---|
| Relej parne ploce | `24` | Prva faza pomaka okretne ploce |
| Relej neparne ploce | `25` | Druga faza pomaka okretne ploce |

## Zvona i cekici

| Funkcija | Pin | Napomena |
|---|---:|---|
| Zvono 1 | `26` | Relej zvona 1 |
| Zvono 2 | `27` | Relej zvona 2 |
| Cekic 1 - muski | `28` | Satno otkucavanje i posebni nacini |
| Cekic 2 - zenski | `29` | Polusatno, kvartalno i posebni nacini |

## Nocna rasvjeta

| Funkcija | Pin | Napomena |
|---|---:|---|
| Relej nocne rasvjete | `47` | `HIGH = nocna rasvjeta ukljucena` |

## Ulazi okretne ploce

| Funkcija | Pin | Napomena |
|---|---:|---|
| Cavao 1 | `30` | Ulaz ploce |
| Cavao 2 | `31` | Ulaz ploce |
| Cavao 3 | `32` | Ulaz ploce |
| Cavao 4 | `33` | Ulaz ploce |
| Cavao 5 | `34` | Ulaz ploce |

## Sinkronizacija vremena

| Funkcija | Pin | Napomena |
|---|---:|---|
| RTC SQW 1 Hz | `2` | `DS3231 SQW` takt za precizno okidanje |

## I2C sabirnica

| Funkcija | Pin | Napomena |
|---|---:|---|
| SDA | `20` | `DS3231 RTC`, vanjski EEPROM/FRAM i LCD |
| SCL | `21` | `DS3231 RTC`, vanjski EEPROM/FRAM i LCD |

## Lokalne tipke izbornika

Sve tipke lokalnog izbornika rade kao `INPUT_PULLUP` i aktiviraju se spajanjem na `GND`.

| Funkcija | Pin | Napomena |
|---|---:|---|
| GORE | `7` | Navigacija prema gore |
| DOLJE | `8` | Navigacija prema dolje |
| LIJEVO | `9` | Navigacija prema lijevo |
| DESNO | `10` | Navigacija prema desno |
| DA | `11` | Potvrda / `SELECT` |
| NE | `12` | Povratak / `BACK` |

Napomena:
- stara matricna tipkovnica vise nije dio aktivnog firmware toka toranjskog sata
- `433 MHz` prijemnik `SRX882` koristi `3` kao jedini data ulaz
- pinovi `5`, `16` i `17` ostaju slobodni za buduce prosirenje

## Sunceve servisne tipke i lampice

Lokalne servisne tipke `JUTRO`, `PODNE` i `VECER` rade kao trenutni ulazi i ne koriste kip-logiku.

| Funkcija | Pin | Napomena |
|---|---:|---|
| Tipka sunce vece | `A9` | Trenutni unos za vecernju automatiku |
| Lampica sunce vece | `A10` | Stanje vecernje automatike |
| Tipka sunce jutro | `A11` | Trenutni unos za jutarnju automatiku |
| Lampica sunce jutro | `A12` | Stanje jutarnje automatike |
| Tipka sunce podne | `A13` | Trenutni unos za podnevnu automatiku |
| Lampica sunce podne | `A14` | Stanje podnevne automatike |

## 433 MHz daljinski upravljac

Predviden je sirovi `433 MHz` prijemnik `SRX882` s jednim data izlazom.

| Funkcija | Pin | Napomena |
|---|---:|---|
| 433 `DATA` izlaz prijemnika | `3` | `SRX882` data signal prema prekidnom ulazu `Mega 2560` |

Napomena:
- nauceni kodovi tipki obradjuju se u [main/daljinski_433.cpp](../main/daljinski_433.cpp)
- tipke `A`, `B`, `C` i `D` mogu pokretati zvona i slavljenje prema aktivnoj logici firmwarea

## Posebne tipke i prekidaci

| Funkcija | Pin | Napomena |
|---|---:|---|
| Kip-prekidac slavljenja | `43` | `LOW = slavljenje ukljuceno` |
| Tipka mrtvackog | `42` | Pritisak radi `toggle` |
| Kip-prekidac tihog rezima | `41` | `LOW = tihi rezim ON` |
| Rucna sklopka zvona 1 | `44` | `LOW = ON` |
| Rucna sklopka zvona 2 | `45` | `LOW = ON` |

## Nadzor mreze za UPS mod

| Funkcija | Pin | Napomena |
|---|---:|---|
| Nadzor mreznog napona | `40` | `INPUT_PULLUP`, `LOW = mreza prisutna`, `HIGH = rad samo s UPS-a` |

Napomena:
- ulaz je predviden za optokapler s open-collector izlazom prema `GND`
- dok je `UPS mod` ukljucen i pin prijavi nestanak mreze, firmware blokira zvona, cekice i kazaljke
- okretna ploca ostaje aktivna kako bi toranjski sat i dalje pratio mehanicki raspored

## Signalne lampice

| Funkcija | Pin | Napomena |
|---|---:|---|
| Lampica Zvono 1 | `36` | `HIGH = upaljeno`, treperi tijekom inercije ili sinkronog zavrsetka |
| Lampica Zvono 2 | `37` | `HIGH = upaljeno`, treperi tijekom inercije ili sinkronog zavrsetka |
| Lampica Slavljenje | `38` | `HIGH = upaljeno`, treperi tijekom termalne pauze |
| Lampica Mrtvacko | `39` | `HIGH = upaljeno`, treperi dok ceka kraj inercije |
| Lampica Tihi rezim | `46` | `HIGH = upaljeno` |

## Thumbwheel za mrtvacko zvono

Firmware koristi dvije `BCD 1-2-4-8` znamenke s `INPUT_PULLUP` logikom.

### Desetice

| BCD bit | Pin | Napomena |
|---|---:|---|
| `1` | `A0` | Desetice bit 0 |
| `2` | `A2` | Desetice bit 1 |
| `4` | `A3` | Desetice bit 2 |
| `8` | `A4` | Desetice bit 3 |

### Jedinice

| BCD bit | Pin | Napomena |
|---|---:|---|
| `1` | `A8` | Jedinice bit 0 |
| `2` | `A7` | Jedinice bit 1 |
| `4` | `A6` | Jedinice bit 2 |
| `8` | `A5` | Jedinice bit 3 |

Napomena:
- thumbwheel zatvara prema `GND`
- firmware svaku znamenku cita kao `BCD 1-2-4-8`
- `A1` trenutno nije dio aktivnog thumbwheel rasporeda

## Serijska komunikacija

| Port | Pinovi | Uloga |
|---|---|---|
| `Serial` | USB | PC log i dijagnostika (`115200`) |
| `Serial1` | `RX1=19`, `TX1=18` | Slobodno za buduce prosirenje |
| `Serial3` | `RX3=15`, `TX3=14` | Vanjski `ESP32` mrezni most (`9600`) |

Aktualna postavka firmwarea:
- `ESP_SERIJSKI_PORT = Serial3`

## Kratki sazetak po rasponima pinova

- `2` -> `RTC SQW`
- `3` -> `433 MHz SRX882 DATA`
- `7-12` -> 6 direktnih tipki lokalnog izbornika
- `14-15` -> `Serial3` prema vanjskom `ESP32`
- `16-17` -> slobodni
- `18-19` -> slobodni `Serial1`
- `20-21` -> `I2C`
- `22-29` -> releji kazaljki, ploce, zvona i cekica
- `30-34` -> ulazi ploce
- `35` -> slobodno
- `36-39` -> signalne lampice zvona i posebnih nacina
- `40-45` -> `UPS`, tihi rezim i fizicke sklopke
- `46-47` -> lampica tihog rezima i nocna rasvjeta
- `A0`, `A2-A8` -> thumbwheel mrtvackog
- `A9-A14` -> servisne tipke i lampice sunceve automatike

## Napomena za razvoj

Ako se raspored pinova ikad mijenja, prvo treba uskladiti [podesavanja_piny.h](../main/podesavanja_piny.h), a tek zatim ovu dokumentaciju i sve povezane servisne upute.
