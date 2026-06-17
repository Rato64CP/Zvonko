# 🌐 ZVONKO - korisnicke upute za web sucelje ESP-a

Ove upute su za upravljanje toranjskim satom preko web sucelja koje vodi ESP modul. ESP je mrezni most: prikazuje stanje, salje zahtjeve Megi i pomaze s NTP-om, ali Mega ostaje glavni autoritet za toranjski sat.

## ⚠️ Vazno pravilo

- Web ne smije zaobilaziti Megu.
- Ako Mega odbije naredbu, web je ne smije smatrati izvrsenom.
- Ako web izgubi vezu, toranjski sat nastavlja raditi preko Mege i RTC-a.

## 🔐 Prijava

- Dashboard i API koriste prijavu.
- Korisnik otvara adresu ESP-a u pregledniku, npr. `http://192.168.8.210/`.
- Preglednik trazi korisnicko ime i lozinku ili PIN, ovisno o verziji web autentikacije u firmwareu.
- Setup stranica u privremenom AP nacinu ne koristi istu prijavu dok se postavlja nova mreza.

## 🏠 Dashboard

Glavna stranica dashboarda prikazuje servisne tipke:

- `MUSKO`
- `ZENSKO`
- `SLAVI`
- `BRECA`
- `POKOJNIK`
- `POKOJNICA`
- `JUTRO`
- `PODNE`
- `VECER`
- `TIHI MOD`

Boja tipki prati stvarno stanje koje Mega salje kroz `STATUS:`.

## 🔔 Tipke zvona

Tipke `MUSKO`, `ZENSKO`, `SLAVI` i `BRECA` koriste se kao servisne komande.

- `MUSKO` pokrece ili zaustavlja musko zvono prema logici Mege.
- `ZENSKO` pokrece ili zaustavlja zensko zvono.
- `SLAVI` pokrece ili zaustavlja slavljenje.
- `BRECA` pokrece posebno brecanje ako je podrzano u konfiguraciji.

Ako je aktivan tihi rezim, RTC greska, UPS blokada ili drugi sigurnosni uvjet, Mega moze odbiti pokretanje.

## 🕯️ Pokojnik i pokojnica

Tipke `POKOJNIK` i `POKOJNICA` pokrecu jednokratne pogrebne skripte.

Vrsta skripte bira se u `BAT` postavkama:

- `POK 1` - klasicna varijanta
  - `POKOJNIK`: musko zvono `2 min`, zatim mrtvacko `10 min`
  - `POKOJNICA`: zensko zvono `2 min`, zatim mrtvacko `10 min`
- `POK 2` - produljena varijanta
  - `POKOJNIK`: oba zvona `2 min`, zatim nakon inercije mrtvacko `40 min`
  - `POKOJNICA`: zensko zvono `2 min`, zatim nakon inercije mrtvacko `40 min`

Ako je u tijeku redovno otkucavanje, skripta ga prekida prema sigurnoj logici Mege.

## 🌅 Jutro, podne i vecer

Web tipke `JUTRO`, `PODNE` i `VECER` pale ili gase odgovarajuci termin sunceve automatike.

- Ako je tipka ukljucena, termin smije zvoniti.
- Ako je tipka iskljucena, termin ne zvoni.
- Web tipka ne mijenja `AUTO/RUC` nacin vremena.

Vrijeme jutarnje i vecernje Zdravomarije uredjuje se na lokalnom LCD izborniku Mege:

- `AUTO` - vrijeme se racuna prema suncu
- `RUC` - koristi se korisnikov upis `HH:MM`

Web protokol cuva ta polja da ih web spremanje ne pregazi.

## 🔇 Tihi mod

`TIHI MOD` na webu ulazi u isti jedinstveni tihi rezim kao lokalni kip-prekidac.

Tihi mod blokira:

- zvona
- cekice
- slavljenje
- mrtvacko

Ne blokira:

- kazaljke
- okretnu plocu

Ako lokalni kip-prekidac promijeni stanje, dashboard se nakon sljedeceg osvjezavanja uskladi sa stvarnim stanjem iz Mege.

## ⚙️ Stranica postavki

Stranica `/settings` uredjuje sigurne postavke koje ne pomicu mehaniku odmah.

Skupine:

- `Sustav`
- `Stapici`
- `BAT`
- `Sunce`

Nakon spremanja web salje cijeli paket Megi. Mega validira vrijednosti, sprema ih i zatim web ponovno cita stvarno stanje.

## 🛠️ Sustav

Skupina `Sustav` sadrzi:

- `LCD svjetlo`
- `Logiranje`
- `UPS mod`
- `Kocnica`
- `INR1`
- `INR2`
- `Impuls cekica`

Znacenje:

- `LCD svjetlo` ukljucuje ili gasi pozadinsko osvjetljenje LCD-a uz pravila nocnog rezima.
- `Logiranje` ukljucuje ili smanjuje PC/serijske logove.
- `UPS mod` govori sustavu da je spojen pomocni napon i da bez mreze treba blokirati mehanicke izlaze.
- `Kocnica` odredjuje radi li sustav s kocnicom zvona ili bez nje.
- `INR1` i `INR2` su inercije zvona nakon gasenja.
- `Impuls cekica` odredjuje trajanje impulsa cekica.

## 🔩 Stapici

Skupina `Stapici` sadrzi:

- `TR` - trajanje radnog zvonjenja preko cavala
- `TN` - trajanje nedjeljnog zvonjenja preko cavala
- `TS` - trajanje slavljenja
- `S` - odgoda slavljenja nakon zvona i inercije

Cavli nisu web-postavka. Njih cita okretna ploca, a Mega razlikuje radni dan/subotu i nedjelju.

## 🔕 BAT

Skupina `BAT` sadrzi:

- `od`
- `do`
- mod otkucavanja
- mod slavljenja
- mod mrtvackog
- `POK` vrstu pokojnickog zvona

`BAT od/do` je raspon u kojem je redovno otkucavanje dopusteno.

Primjer:

- `od 6`
- `do 22`

Redovno otkucavanje radi od `06:00` do `22:00`. Izvan toga redovno otkucavanje ne radi, ali zvona, sunceva automatika i okretna ploca nastavljaju raditi.

## 🌅 Sunce

Skupina `Sunce` sadrzi:

- ukljucenje `Jutro`
- zvono za `Jutro`
- odgodu jutra kad je jutro u `AUTO` nacinu
- ukljucenje `Podne`
- zvono za `Podne`
- ukljucenje `Vecer`
- zvono za `Vecer`
- odgodu veceri kad je vecer u `AUTO` nacinu
- `Nocnu rasvjetu`

Rucno vrijeme `JUTRO/VECER` ne uredjuje se na webu, nego na lokalnom LCD izborniku Mege.

## ✝️ Blagdani i mise

Stranica `/blagdani` uredjuje:

- dnevnu misu
- nedjeljnu misu
- nepomicne blagdane
- pomicne blagdane

Pravila:

- Prazno vrijeme znaci da je ta misa ili blagdan iskljucen, bez obzira na kvacicu.
- Dnevna misa pokrece musko zvono `30 min` prije mise.
- Nedjeljna i blagdanska misa pokrecu nedjeljno zvonjenje oba zvona `2 h` i `1 h` prije mise.
- Misna zvonjenja cekaju slobodnu mehaniku ako tada traje otkucavanje, zvono ili inercija.

## 📡 WiFi setup

Ako ESP nije spojen na mrezu, moze se pokrenuti privremena mreza:

- SSID: `ZVONKO_setup`
- lozinka: `zvonko10`
- adresa: `http://192.168.4.1/` ili `http://192.168.4.1/setup`

Setup AP se moze pokrenuti:

- tipkom na ESP-u ako je spojena
- ili dugim istovremenim pritiskom `LIJEVO + DESNO` na Mega tipkovnici, samo s glavnog prikaza sata

Nakon spremanja WiFi podataka ESP ih prosljedjuje Megi i pokusava se spojiti na novu mrezu.

## ⏱️ NTP

ESP odrzava mrezni NTP sloj, ali ne smije sam nametati vrijeme Megi.

Tok rada:

- Mega odabire siguran trenutak za `NTPREQ:SYNC`
- ESP tada salje svjeze NTP vrijeme
- Mega provjerava i upisuje RTC samo kad je mehanika u sigurnom stanju

Ako nema interneta, Mega i dalje radi preko `DS3231 RTC-a`. DST prijelaz moze odraditi offline kad je RTC baterija ispravna.

## ⬆️ OTA nadogradnja ESP-a

Ruta `/update` je skrivena i ne prikazuje se kao obican link na dashboardu.

Postupak:

1. Kompajliraj ESP firmware i pronadji `.bin` datoteku.
2. Otvori `http://<ip-esp>/update`.
3. Prijavi se istim podacima kao za dashboard.
4. Odaberi `.bin` datoteku.
5. Pricekaj kraj uploada i restart ESP-a.

## 🚨 Kad web ne radi

Ako se web stranica ne otvara:

- provjeri je li ESP spojen na WiFi
- provjeri IP adresu na LCD-u ili u routeru
- provjeri napajanje ESP-a
- ako je potrebno, pokreni `ZVONKO_setup`

Ako web radi, ali tipka ne pokrece rad:

- provjeri je li aktivan tihi mod
- provjeri `UPS mod`
- provjeri ima li greske `RTC`, `EEPROM` ili `safe mode`
- provjeri je li Mega potvrdila naredbu

## ✅ Brza web provjera

- Dashboard se otvara i trazi prijavu.
- Boje tipki odgovaraju stvarnom stanju.
- `/settings` nakon otvaranja ucita vrijednosti iz Mege.
- Spremanje postavki vrati potvrdu.
- `/blagdani` ucita i spremi mise/blagdane.
- Ako je WiFi spojen, NTP logovi pokazuju uredne sinkronizacije na zahtjev Mege.
