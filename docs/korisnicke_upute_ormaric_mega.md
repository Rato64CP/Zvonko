# 🕰️ ZVONKO - korisnicke upute za ormaric i Arduino Megu

Ove upute su za osobu koja upravlja toranjskim satom na samom ormaricu: preko LCD-a, sest tipki, prekidaca, lampica, okretne ploce i lokalnih tipki.

## ⚠️ Sigurnost

- Toranjski sat upravlja zvonima, cekicima, kazaljkama i okretnom plocom. Ne diraj mehaniku dok lampice pokazuju rad ili dok se cuje rad motora.
- Ako je aktivna inercija zvona, zvono se jos smiruje iako relej vise ne mora biti ukljucen.
- Ako LCD prikazuje `SUSTAV ZAKLJUCAN`, `EEPROM GRESKA`, `RTC OGRANICEN RAD` ili slicnu gresku, ne forsirati rad mehanike prije provjere uzroka.
- `UPS mod` znaci da logika moze ostati ziva bez mreznog napona, ali izlazi prema mehanici ostaju blokirani dok nema mreze.

## 🧭 Glavni prikaz LCD-a

- Prvi red prikazuje vrijeme, izvor vremena i temperaturu RTC modula.
- Izvor vremena moze biti `NTP`, `MAN`, `ERR` ili `---`.
- Drugi red prikazuje datum ili trenutno vaznu aktivnost: zvono, otkucavanje, slavljenje, mrtvacko, gresku, UPS stanje ili kratku servisnu poruku.
- Dvotocke u vremenu trepere kad sustav radi neku aktivnost ili kad WiFi nije spojen.

## 🎛️ Tipke izbornika

- `LIJEVO` i `DESNO` - pomicanje kroz stavke ili polja
- `GORE` i `DOLJE` - promjena vrijednosti
- `DA` - potvrda i spremanje
- `NE` - izlaz bez spremanja ili povratak

Drzanje `GORE` ili `DOLJE` ubrzava uredjivanje brojcanih vrijednosti, npr. sati, minuta, impulsa, inercije i polozaja.

## 🧩 Glavni izbornik

Glavni izbornik vodi ove skupine:

- `Maticni sat`
- `Kazaljke`
- `Okretna ploca`
- `Mreza`
- `Otkuc./BAT`
- `Stapici`
- `Sunce`
- `Blagdani`
- `Sustav`

## ⏱️ Maticni sat

- Ovdje se uredjuje lokalno vrijeme i NTP postavka.
- Nakon potvrde `DA`, Mega upisuje vrijeme u RTC i koristi ga za rad toranjskog sata.
- Ako je NTP ukljucen, Mega sama bira siguran trenutak za sinkronizaciju, kad su kazaljke i ploca mirne.

## 🕘 Kazaljke

- Ovdje se rucno uskladjuje memorirani polozaj kazaljki.
- Tijekom uredjivanja automatika kazaljki je blokirana.
- Spremi samo kad si siguran da LCD polozaj odgovara stvarnom polozaju kazaljki.

## ⚙️ Okretna ploca

- Polozaj ploce se uredjuje u valjanim koracima od `15 min`.
- Ploca cita cavle na pocetku minute koju pokazuje.
- Ako u toj minuti traje satno otkucavanje, citanje cavala ceka kraj otkucavanja.
- Ako se toranjski sat nakon nestanka napajanja vrati usred prozora zvonjenja preko cavla, sustav smije jos pokrenuti zvono. Nakon isteka trajanja zvona, zakasnelo zvonjenje se ne pokrece.

## 🔔 Cavli okretne ploce

Raspored cavala je fiksan:

- radni dan i subota:
  - cavao `1` - `ZVONO 1`
  - cavao `2` - `ZVONO 2`
- nedjelja:
  - cavao `3` - nedjeljno `ZVONO 1`
  - cavao `4` - nedjeljno `ZVONO 2`
- cavao `5` - `SLAVLJENJE`

Cavao `5` za slavljenje ceka kraj zvona i inercije. Vadjenje cavla zaustavlja samo slavljenje koje je pokrenula automatika ploce.

## 🌐 Mreza

- Mrezne postavke sluze za ESP WiFi most.
- WiFi se moze ukljuciti ili iskljuciti.
- Osnovni rad toranjskog sata ne ovisi o WiFi vezi.

## 🔕 Otkucavanje i BAT

`BAT od/do` odredjuje kada redovno otkucavanje smije raditi.

Primjer:

- `BAT od 6`
- `BAT do 22`

To znaci da redovno otkucavanje radi od `06:00` do `22:00`. Izvan tog raspona ne radi redovno otkucavanje, ali zvona, sunceva automatika i okretna ploca nastavljaju raditi.

U istoj skupini se bira:

- mod otkucavanja
- mod slavljenja
- mod mrtvackog
- vrsta pokojnickog zvona `POK`

Vrste pokojnickog zvona:

- `1` - klasicno: `POKOJNIK` koristi musko zvono, `POKOJNICA` zensko zvono, zatim mrtvacko `10 min`
- `2` - produljeno: `POKOJNIK` pali oba zvona `2 min`, `POKOJNICA` zensko zvono `2 min`, zatim mrtvacko `40 min`

## 🔩 Stapici

- `TR` - trajanje radnog zvonjenja preko cavala
- `TN` - trajanje nedjeljnog zvonjenja preko cavala
- `TS` - trajanje slavljenja
- `S` - odgoda slavljenja nakon zvona i inercije

## 🌅 Sunce i Zdravomarija

Sunceva automatika vodi `JUTRO`, `PODNE`, `VECER` i nocnu rasvjetu.

Za `JUTRO` i `VECER` postoje dva nacina vremena:

- `AUTO` - sustav racuna vrijeme prema izlasku ili zalasku sunca i spremljenoj odgodi
- `RUC` - korisnik upisuje tocno vrijeme `HH:MM`

Primjer:

- Ako je `JUTRO` u nacinu `RUC 06:00`, jutarnja Zdravomarija zvoni u `06:00` svaki dan dok se postavka ne promijeni.
- Ako je `JUTRO` u nacinu `AUTO`, vrijeme se svaki dan racuna prema suncu.

Fizicke tipke i LED lampice `JUTRO`, `PODNE` i `VECER` ne mijenjaju `AUTO/RUC`. One samo pale ili gase termin.

Kad se tipka ukljuci, LCD `5 s` prikazuje:

- `ZDRAVOM. JUTRO` i vrijeme `HH:MM`
- `PODNEVNO ZVONO` i vrijeme `HH:MM`
- `ZDRAVOM. VECER` i vrijeme `HH:MM`

Kad se tipka iskljuci, LCD prikazuje `ZDRAVOMARIJA OFF` ili `PODNE OFF`.

LED lampica:

- stalno svijetli kad je termin ukljucen
- ne svijetli kad je termin iskljucen
- treperi dok to zvonjenje traje

## ✝️ Blagdani i mise

- Dnevna misa pokrece musko zvono `30 min` prije upisanog vremena mise.
- Nedjeljna i blagdanska misa pokrecu nedjeljno zvonjenje oba zvona `2 h` i `1 h` prije mise.
- Posebni raspored za Svi sveti i Dusni dan moze pokretati mrtvacko u zadanom prozoru.

## 🛠️ Sustav

U sustavskim postavkama nalaze se:

- LCD svjetlo
- logiranje
- UPS mod
- kocnica zvona
- inercija `INR1` i `INR2`
- impuls cekica
- polaritet izlaznih releja ako je dostupan u postavkama

Kod relejne logike treba biti posebno oprezan. Pogresan polaritet moze ukljuciti releje u krivom stanju.

## 🔇 Tihi rezim

Tihi rezim blokira zvona, cekice, slavljenje i mrtvacko. Ne zaustavlja kazaljke ni okretnu plocu.

Tihi rezim moze doci iz:

- fizickog kip-prekidaca
- uskrsne tisine
- web togglea `TIHI MOD`

## 🚨 Greske i oporavak

Najvaznije poruke:

- `NEMA STRUJE!` - aktivan UPS mod, mehanicki izlazi su blokirani
- `SUSTAV ZAKLJUCAN` - watchdog safe mode, izlazi su blokirani
- `EEPROM GRESKA` - trajna memorija ima problem, potreban pregled
- `RTC OGRANICEN RAD` - RTC ili SQW nije pouzdan, automatika vremena je ogranicena
- `ERR` na prvom redu LCD-a - vrijeme nije pouzdano za punu automatiku

Safe mode se otkljucava drzanjam `DA / SELECT` oko `5 s`, ali tek nakon sto je jasno zasto je sustav usao u safe mode.

## ✅ Brza dnevna provjera

- Vrijeme na LCD-u je tocno.
- Izvor vremena je `NTP` ili `MAN`, a ne `ERR`.
- LED lampice `JUTRO/PODNE/VECER` odgovaraju zeljenom rasporedu.
- Nema poruke `NEMA STRUJE!`, `EEPROM GRESKA`, `RTC OGRANICEN RAD` ili `SUSTAV ZAKLJUCAN`.
- Ako je WiFi dostupan, web sucelje prikazuje stvarno stanje tipki.
