# Mokomasis maišos generatorius – v0.1

C++ programa skaičiuoja ranka įvesto teksto arba failo turinio **256 bitų maišą**, pateikiamą **64 mažųjų raidžių hex skaitmenimis**. Tai pradinė projekto versija, skirta algoritmo kūrimui ir eksperimentiniam vertinimui.

Šis aprašymas skirtas v0.1 algoritmui, užfiksuotam commit [`ae81752`](https://github.com/ViliusZe/bloku-grandiniu-technologijos/commit/ae8175242969f89b214fc500761ad9c8a7dc8a07). 
## Įgyvendinta

- Teksto įvedimas ranka ir failo turinio skaitymas dvejetainiu režimu.
- Tuščios įvesties palaikymas ir klaidų pranešimai.
- Fiksuota 256 bitų išvestis su pradiniais nuliais.
- UTF-8 konsolės nustatymai „Windows“ aplinkoje.
- 16 MiB priimamos įvesties riba.
- Automatinės formato ir kartotinių kvietimų determinizmo patikros.
- 100 000 porų lavinos eksperimentas ir histograma.
- 12 įvesties dydžių spartos bandymas, po 5 matavimus kiekvienam dydžiui, ir spartos grafikas.

## Kompiliavimas ir paleidimas

Naudota „Visual Studio Community 2026“ su „Desktop development with C++“ komponentais. Projekte nustatytas **C++20** standartas ir **MSVC v145** įrankių rinkinys.

1. Atidaryti [MaisosGeneratorius.slnx](MaisosGeneratorius/MaisosGeneratorius.slnx).
2. Pasirinkti `Release` ir `x64`, sukompiliuoti projektą.
3. Atidaryti `Project → Properties → Configuration Properties → Debugging → Command Arguments`.
4. Nurodyti argumentą pagal lentelę ir paleisti su `Ctrl + F5`.

| Argumentas v0.1 kode | Veikimas |
| --- | --- |
| Laukelis tuščias | Perskaitoma viena ranka įvesta teksto eilutė. |
| `"C:\kelias\failas.txt"` | Skaičiuojama failo turinio maiša. |
| `--test` | Formato ir determinizmo testai. |
| `--avalanche` | Vienos poros `ab` ir `ac` lavinos bandymas. |
| `--avalanche-stats` | 100 000 porų lavinos eksperimentas. |
| `--benchmark "C:\kelias\konstitucija.txt"` | Spartos matavimai su nurodytu failu. |

Pakeitus `Debug` į `Release`, argumentus reikia patikrinti pasirinktoje konfigūracijoje.

**Jei naudojamas vėlesnis kodas, kuriame jau yra abiejų algoritmų pasirinkimas, v0.1 paleidžiama prieš režimą pridedant `--v01`**, pavyzdžiui, `--v01 --test`. Toliau pateikti rezultatai priklauso v0.1 algoritmui.

Baigimo kodas `0` reiškia sėkmę, `1` – aptiktą klaidą.

## Įvesties baitai ir išvestis

Ranka įvestas tekstas skaitomas su `std::getline`. Konsolės įvesties ir išvesties koduotė nustatoma į UTF-8. Įvedimą užbaigiantis Enter sukurtas naujos eilutės simbolis į tekstą neįtraukiamas. Tarpai, raidžių registras ir kiti perskaityti baitai išsaugomi.
Failas atidaromas su `std::ios::binary`. Skaičiuojama **turinio, o ne failo pavadinimo maiša**. Išsaugomi tikslūs baitai, įskaitant tarpus, LF arba CRLF eilučių skirtukus, nulinius baitus ir BOM, jei jis yra. Failo turinys nekonvertuojamas ir nenormalizuojamas.
Todėl tekstas su nauja eilute ir be jos yra skirtingos įvestys. Vienodai atrodantis tekstas taip pat gali turėti skirtingus baitus. Pavyzdžiui, UTF-8 žodis `ąžuolas` turi 7 raides, bet 9 baitus. Lyginant rankinį įvedimą su failu, failas turi turėti tuos pačius UTF-8 baitus, be papildomo BOM ar eilučių skirtuko.
Didžiausia priimama įvestis – **16 MiB, arba 16 777 216 baitų**. Visas turinys laikomas operatyviojoje atmintyje. Failo dydis tikrinamas skaitymo metu; rankinio teksto dydis – jau perskaičius eilutę. Todėl ši riba negarantuoja, kad rankinio teksto nuskaitymas laikinai nenaudos daugiau atminties.

Rezultatas sudarytas iš aštuonių 32 bitų reikšmių. Kiekvienai išvedami 8 hex skaitmenys, įskaitant pradinius nulius. Iš viso gaunami 64 skaitmenys.

## v0.1 algoritmas

Būseną sudaro aštuoni `std::uint32_t` skaičiai. Pradinės reikšmės yra `1, 2, 3, 4, 5, 6, 7, 8`, o daugikliai – `31, 37, 41, 43, 47, 53, 59, 61`.

Kiekvienas įvesties baitas įmaišomas į visas būsenos dalis naudojant daugybą ir XOR. Pabaigoje atliekami aštuoni maišymo roundai tarp būsenos dalių. Sudėtis ir daugyba atliekamos moduliu `2^32`.

```text
state = [1, 2, 3, 4, 5, 6, 7, 8]
multipliers = [31, 37, 41, 43, 47, 53, 59, 61]

kiekvienam įvesties baitui b:
    kiekvienam i nuo 0 iki 7:
        state[i] = (state[i] * multipliers[i]) XOR b

pakartoti 8 raundus:
    previous = state kopija
    kiekvienam i nuo 0 iki 7:
        mixed = previous[i] XOR previous[(i + 1) mod 8]
        mixed = cikliškai pasukti 32 bitus į kairę per 13 pozicijų
        state[i] = mixed * multipliers[i] + previous[(i + 3) mod 8]

rezultatas = sujungti visų state reikšmių 8 skaitmenų hex užrašus
```

Daugikliai leidžia atsižvelgti į baitų tvarką, XOR įmaišo baito reikšmę, o baigiamieji raundai paskirsto pakeitimo įtaką tarp būsenos dalių. Standartinės maišos, tokios kaip MD5 ar SHA-256, šiame algoritme nekviečiamos. Atsitiktinumas naudojamas bandymų įvestims generuoti, ne pačioje maišos funkcijoje.

Žinomas v0.1 rezultatas įvesčiai `ab`, be naujos eilutės pabaigoje:

```text
10f39da4f9a1225dca476d46dba0edbc3d83d7a1045ac93e5d41fa6f3db9c541
```

## Teisingumo patikros

`--test` tikrina 7 įvesčių maišos ilgį ir hex simbolius: tuščią tekstą, `a`, `ab`, `ba`, tekstą su tarpais, 2000 pasikartojančių simbolių ir UTF-8 raidę `ą`.

Determinizmo dalyje tikrinamos 6 įvestys. Po pradinio kvietimo kiekvienos maiša skaičiuojama dar 5 kartus, tarp kvietimų apdorojant kitą tekstą. Tikimasi pranešimų `Formato testai pavyko.` ir `Determinizmo testai pavyko.` bei baigimo kodo `0`.

Pastovumui tarp atskirų programos paleidimų patikrinti reikia išsaugoti ir palyginti visas šešias išvedamas maišas, ne vien sėkmės pranešimą. Šių paleidimų žurnalą dar reikia pridėti.

Papildomai patikrintas bendras įvesties mechanizmas:

| Atvejis | Rezultatas |
| --- | --- |
| Ranka įvestas 'ab' ir failas su tais pačiais baitais | Maišos sutampa. |
| Tuščias failas | Priimamas. |
| Neegzistuojantis failas | Klaidos pranešimas, kodas `1`, maiša neišvedama. |
| Tiksliai 16 777 216 baitų failas | Priimamas, išvedama maiša, kodas `0`. |
| 16 777 217 baitų failas | Dydžio ribos klaida, kodas `1`. |

## Efektyvumo eksperimentas

Naudotas fiksuotas [konstitucija.txt](MaisosGeneratorius/konstitucija.txt) failas: **1351 eilutė, 77 895 baitai**, UTF-8 be BOM, LF eilučių skirtukai. Tai iš oficialaus Konstitucijos PDF paruoštas tekstas. Šaltinis, konvertavimo komanda ir SHA-256 kontrolinės sumos saugomos [konstitucija_source.json](MaisosGeneratorius/konstitucija_source.json).

Imtos pirmų 1, 2, 4, 8 ir toliau dvigubėjančio skaičiaus eilučių ištraukos bei visas failas. Tikslūs eilučių skirtukų baitai išsaugoti.

Matavimo sąlygos:

- `Release x64`, laikrodis `std::chrono::steady_clock`;
- failo skaitymas ir ištraukos sudarymas atliekami prieš matavimą;
- prieš kiekvieno dydžio matavimus atliekami 3 apšilimo kvietimai;
- kvietimų grupė didinama, kol jos trukmė pasiekia bent 20 ms arba pasiekiama 1 048 576 kvietimų riba;
- kiekvienam dydžiui atliekami 5 matavimai, grupės laiką dalijant iš kvietimų skaičiaus;
- rezultatai panaudojami kontrolinei sumai, o kvietimai vykdomi per `volatile` funkcijos rodyklę;
- matuojamas `calculateHash` kvietimas, įskaitant grąžinamo hex užrašo formavimą; failo ir konsolės įvestis bei išvestis neįtraukiamos.

Laikai lentelėje pateikti **mikrosekundėmis vienai maišai**. `1 µs = 0,001 ms`.

| Eilučių | Baitų | Vidurkis, µs | Minimumas, µs | Maksimumas, µs |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 36 | 1,1830 | 1,1402 | 1,3169 |
| 2 | 143 | 1,2364 | 1,2289 | 1,2483 |
| 4 | 204 | 1,2637 | 1,2599 | 1,2672 |
| 8 | 347 | 1,4027 | 1,4002 | 1,4045 |
| 16 | 857 | 1,8259 | 1,7974 | 1,8687 |
| 32 | 1 540 | 2,4159 | 2,3891 | 2,4557 |
| 64 | 2 983 | 3,6410 | 3,6152 | 3,6858 |
| 128 | 5 856 | 6,1529 | 6,0611 | 6,2998 |
| 256 | 13 212 | 12,3062 | 12,3010 | 12,3129 |
| 512 | 28 069 | 25,2822 | 24,9224 | 25,9003 |
| 1024 | 60 389 | 53,2063 | 52,3998 | 54,1914 |
| 1351 | 77 895 | 68,2983 | 67,3754 | 69,1131 |

![v0.1 efektyvumo grafikas](MaisosGeneratorius/images/efektyvumo_grafikas.png)

Pradiniai matavimai: [benchmark_raw.csv](MaisosGeneratorius/benchmark_raw.csv), suvestinė: [benchmark_summary.csv](MaisosGeneratorius/benchmark_summary.csv). Pradiniame faile saugomi kiekvieno matavimo kvietimų skaičius, bendras grupės laikas ir laikas vienai maišai.

Šiame bandyme skaičiavimo laikas didėjo beveik tiesiškai didėjant įvesčiai. Trumpoms įvestims reikšminga pastovioji funkcijos ir hex formavimo kaina. Vieno failo bei vienos aplinkos matavimai neįrodo tokios pačios spartos kitame kompiuteryje.

## Lavinos efekto eksperimentas

Naudota **100 000 porų**: po **25 000** kiekvienam ilgiui – 10, 100, 500 ir 1000 baitų. Generatorius – `std::mt19937`, `seed = 2026`. Abėcėlė:

```text
abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789
```

Kiekvienoje poroje vienas atsitiktinėje pozicijoje pasirinktas ASCII simbolis pakeičiamas kitu tos pačios abėcėlės simboliu. Naujas simbolis tikrinamas, kad skirtųsi nuo senojo. Įvesties ilgis nesikeičia.

```text
Bitų skirtumas (%) = 100 * skirtingų bitų skaičius / 256
Hex skirtumas (%) = 100 * skirtingų hex pozicijų skaičius / 64
```

Bitai lyginami dekodavus po du hex skaitmenis į baitą. Visų rodiklių minimumai, vidurkiai ir maksimumai pateikti procentais:

| Ilgis baitais | Porų | Bitų min. | Bitų vid. | Bitų maks. | Hex min. | Hex vid. | Hex maks. |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 10 | 25 000 | 37,5000 | 50,0308 | 62,8906 | 79,6875 | 93,7588 | 100,0000 |
| 100 | 25 000 | 37,8906 | 50,0087 | 62,5000 | 79,6875 | 93,7634 | 100,0000 |
| 500 | 25 000 | 38,6719 | 49,9963 | 62,5000 | 78,1250 | 93,7309 | 100,0000 |
| 1000 | 25 000 | 38,2812 | 49,9855 | 62,5000 | 79,6875 | 93,7396 | 100,0000 |
| Bendrai | 100 000 | 37,5000 | 50,0053 | 62,8906 | 78,1250 | 93,7482 | 100,0000 |

![v0.1 lavinos efekto histograma](MaisosGeneratorius/images/lavinos_histograma.png)

Pradiniai rezultatai: [avalanche_raw.csv](MaisosGeneratorius/avalanche_raw.csv). Viena eilutė atitinka vieną porą. `position` yra pakeisto simbolio indeksas nuo 0. Įvesčių tekstai CSV nesaugomi; jų atkūrimo būdas yra programoje esantis generatorius su nurodyta abėcėle ir pradine reikšme.

Histogramoje vienas stulpelis atitinka 1 bito skirtumą, arba 0,390625 procentinio punkto. Jo aukštis rodo porų skaičių. Visi matavimai patenka į parodytą 35–65 % ašies intervalą. Grafikui atkurti reikia sugrupuoti `different_bits` reikšmes ir horizontalioje ašyje naudoti `100 * different_bits / 256`.

Bendras bitų skirtumo vidurkis **50,0053 %**, hex skirtumo – **93,7482 %**. Jie artimi orientacinėms 50 % ir 93,75 % reikšmėms. Atskirai porai `ab` ir `ac` pasikeičia 133 iš 256 bitų (51,953125 %).

## Aplinka ir atkuriamumas

| Parametras | Užfiksuota reikšmė |
| --- | --- |
| Operacinė sistema | „Windows“; tiksli versija dar neįrašyta. |
| IDE | „Visual Studio Community 2026“. |
| Įrankių rinkinys ir standartas | MSVC v145, C++20; pilną kompiliatoriaus versijos numerį dar reikia įrašyti. |
| Spartos bandymo konfigūracija | `Release x64`. |
| Užfiksuoto lavinos bandymo konfigūracija | `Debug x64`. |
| Procesorius ir RAM | Dar reikia įrašyti. |
| Atsitiktinių įvesčių generatorius | `std::mt19937`, seed 2026, 62 simbolių ASCII abėcėlė. |

Tie patys bandymai lyginamoms versijoms turi būti vykdomi su tuo pačiu duomenų rinkiniu, įranga ir konfigūracija. Generatorius kiekvieną eksperimentą pradedamas nuo to paties seed. Standartinės bibliotekos `std::uniform_int_distribution` realizacija gali skirtis tarp aplinkų, todėl vien seed visuotiniam atkuriamumui nepakanka.

CSV sukuriami programos darbo aplanke ir pakartotinai paleidus eksperimentą perrašomi. Prieš lyginant kitą versiją, rezultatus reikia išsaugoti atskirais pavadinimais. Vėlesniame kode su versijų pasirinkimu naudojami `v01_` ir `v02_` failų prefiksai.

## Ribotumai ir likę darbai

Geras lavinos efektas ir beveik tiesinis laiko augimas neįrodo kriptografinio saugumo. Ši versija dar neįvertinta pilnu kolizijų ir pirmavaizdžio paieškos eksperimentu. Baigiamasis maišymas negali panaikinti kolizijos, jeigu skirtingos įvestys prieš jį jau sukūrė visiškai vienodą visą būseną.

- Papildyti ir išsaugoti visą 1 eksperimento įvesties failų rinkinį, įskaitant vieno baito pakeitimus pradžioje, viduryje ir gale.
- Pridėti atskirų programos paleidimų determinizmo palyginimo žurnalą.
- Atlikti po 100 000 atsitiktinių porų kolizijų paiešką kiekvienam reikalaujamam ilgiui, viso rinkinio tikrinimą ir struktūruotų įvesčių bandymus.
- Atlikti kandidatų `0000`–`9999` perrinkimą be druskos ir su vieša druska, aptarti slaptą atsitiktinį priedą.
- Papildyti tikslius vykdymo aplinkos duomenis.
- Palyginti v0.1 ir v0.2 tais pačiais testais bei parengti galutines projekto išvadas.

## Šaltiniai

Šaltiniai:

- Kurso 1-ojo darbo „Sukurk savo maišos generatorių“ užduoties formuluotė, 2026 m.
- [Oficialus Konstitucijos PDF, iš kurio paruoštas spartos testų tekstas](https://e-seimas.lrs.lt/rs/actualedition/TAIS.1890/oiUlTMOYlq/format/ISO_PDF/).
- [Microsoft: Console Code Pages](https://learn.microsoft.com/en-us/windows/console/console-code-pages).

