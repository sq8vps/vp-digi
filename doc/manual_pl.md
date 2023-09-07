# VP-Digi - dokumentacja
Copyright 2023 Piotr Wilkoń\
Zezwala się na kopiowanie, rozpowszechnianie i/lub modyfikowanie tego dokumentu na warunkach Licencji GNU Wolnej Dokumentacji (GNU Free Documentation License) w wersji 1.3 lub dowolnej nowszej opublikowanej przez Free Software Foundation,
bez żadnych Części nienaruszalnych, bez Tekstów przedniej
lub tylnej strony okładki. Egzemplarz licencji zamieszczono w pliku [LICENSE_FDL](LICENSE_FDL).\
Rejestr zmian dostępny jest na końcu tego dokumentu.
## Spis treści
- [VP-Digi - dokumentacja](#vp-digi---dokumentacja)
  - [Spis treści](#spis-treści)
  - [1. Opis funkcjonalny](#1-opis-funkcjonalny)
  - [2. Podręcznik użytkownika](#2-podręcznik-użytkownika)
    - [2.1. Tryb konfiguracji](#21-tryb-konfiguracji)
      - [2.1.1. Polecenia](#211-polecenia)
      - [2.1.2. Przykładowe ustawienia](#212-przykładowe-ustawienia)
    - [2.2. Tryb monitora](#22-tryb-monitora)
      - [2.2.1. Polecenia](#221-polecenia)
      - [2.2.2. Widok pakietów odbieranych](#222-widok-pakietów-odbieranych)
    - [2.3. Tryb KISS](#23-tryb-kiss)
    - [2.4. Kalibracja poziomów sygnału](#24-kalibracja-poziomów-sygnału)
    - [2.5. Programowanie](#25-programowanie)
      - [2.5.1. Programowanie za pomocą programatora ST-Link](#251-programowanie-za-pomocą-programatora-st-link)
      - [2.5.2. Programowanie za pomocą przejściówki USB-UART](#252-programowanie-za-pomocą-przejściówki-usb-uart)
    - [2.6. Uruchomienie](#26-uruchomienie)
  - [3. Opis techniczny](#3-opis-techniczny)
    - [3.1. Sprzęt](#31-sprzęt)
      - [3.1.1. Odbiór](#311-odbiór)
      - [3.1.2. Nadawanie](#312-nadawanie)
    - [3.2. Oprogramowanie](#32-oprogramowanie)
      - [3.2.1. Modem](#321-modem)
        - [3.2.1.1. Demodulacja](#3211-demodulacja)
        - [3.2.1.2. Modulacja](#3212-modulacja)
        - [3.2.1.3. Odzyskiwanie bitów](#3213-odzyskiwanie-bitów)
      - [3.2.2. Protokoły](#322-protokoły)
        - [3.2.2.1. Odbiór](#3221-odbiór)
        - [3.2.2.2. Nadawanie](#3222-nadawanie)
      - [3.2.3. Digipeater](#323-digipeater)
  - [4. Rejestr zmian dokumentacji](#4-rejestr-zmian-dokumentacji)
    - [2023/09/06](#20230906)

## 1. Opis funkcjonalny
VP-Digi jest samodzielnym sterownikiem digipeatera AX.25 (np. APRS) oraz kontrolerem TNC w standardzie KISS.\
Oprogramowanie wyposażone jest w modemy:
- Bell 202 AFSK 1200 Bd 1200/2200 Hz (standard VHF)
- G3RUH GFSK 9600 Bd (standard UHF)
- Bell 103 AFSK 300 Bd 1600/1800 Hz (standard HF)
- V.23 AFSK 1200 Bd 1300/2100 Hz (alternatywny standard VHF)

Każdy modem wykorzystuje mechanizm wykrywania nośnej w oparciu o detekcję prawidłowego sygnału modulującego, a nie zdekodowanych danych, dzięki czemu zwiększona jest niezawodność, a ilość kolizji - zmniejszona.

Ponadto oprogramowanie obsługuje protokoły:
- AX.25 (standard Packet Radio/APRS)
- FX.25 - AX.25 z korekcją błędów, w pełni kompatybilny z AX.25

VP-Digi umożliwia konfigurację:
- Własnego znaku, SSID i adresu przeznaczenia
- Parametrów modemu (*TXDelay*, *TXTail*, czasu ciszy, typu DAC, typu odbiornika)
- Prędkości pracy portów szeregowych
- 4 aliasów digipeatera typu *New-N* (np. *WIDEn-N*)
- 4 aliasów prostych digipeatera (np. *MIASTO*)
- Progów ilości skoków dla aliasów typu *New-N*
- Włączenia trasowania każdego aliasu
- Trybu *viscous delay* i tylko bezpośredniego dla każdego aliasu
- Listy filtrującej (wykluczanie lub wyłączność)
- Włączenia odbioru pakietów niebędących pakietami APRS
- Włączenia monitorowania własnych pakietów przez port KISS

Urządzenie daje do dyspozycji:
- 2 porty szeregowe
- port USB

z których każdy pracuje niezależnie i może działać w trybie:
- TNC KISS
- monitora ramek
- terminala konfiguracyjnego

## 2. Podręcznik użytkownika
### 2.1. Tryb konfiguracji
Konfiguracja urządzenia może być dokonana przez dowolny port (USB, UART1, UART2). Przejście do trybu konfiguracji następuje po wydaniu polecenia `config`.
> Uwaga! Jeśli port pracuje w trybie KISS (domyślnym po uruchomieniu), to wpisywane znaki nie będą widoczne.

> Uwaga! Po zakończeniu konfiguracji należy zapisać ją do pamięci poleceniem `save`

#### 2.1.1. Polecenia
W trybie konfiguracji dostępne są następujące polecenia:
- `call ZNAK-SSID` - ustawia znak wywoławczy wraz z SSID. Znak wywoławczy musi składać się z maksymalnie 6 znaków A-Z i 0-9. SSID jest opcjonalne. SSID musi zawierać się w zakresie 0-15.
- `dest ADRES` – ustawia adres przeznaczenia. Adres musi składać się z maksymalnie 6 znaków A-Z i 0-9, bez SSID. *APNV01* jest oficjalnym i zalecanym adresem dla VP-Digi.
- `modem <1200/300/9600/1200_V23>` - ustawia typ modemu: *1200* dla modemu 1200 Bd, *300* dla modemu 300 Bd, *9600* dla modemu 9600 Bd lub *1200_V23* dla alternatywnego modemu 1200 Bd. 
- `txdelay CZAS` – ustawia długość preambuły do nadania przed ramką. Wartość w milisekundach z zakresu od 30 do 2550
- `txtail CZAS` – ustawia ogona nadawanego po ramce. Wartość w milisekundach, z zakresu od 10 do 2550. Jeśli nie zachodzi potrzeba, należy ustawić wartość minimalną.
- `quiet CZAS` – ustawia czas, który musi upłynąć pomiędzy zwolnieniem się kanału a włączeniem nadawania. Wartość w milisekundach z zakresu od 100 do 2550.
- `uart NUMER baud PREDKOSC` - ustawia prędkość (1200-115200 Bd) pracy wybranego portu szeregowego.
- `uart NUMBER mode <kiss/monitor/config` - ustawia domyślny tryb pracy wybranego portu szeregowego (0 dla USB).
- `pwm <on/off>` – ustawia typ DAC. *on*, gdy zainstalowany jest filtr PWM, *off* gdy zainstalowana jest drabinka R2R. Od wersji 2.0.0 zalecane jest użycie wyłącznie PWM.
- `flat <on/off>` – konfiguruje modem do użycia z radiem z wyjściem *flat audio*. *on* gdy sygnał podawany jest ze złącza *flat audio*, *off* gdy sygnał podawany jest ze złącza słuchawkowego. Opcja ma wpływ jedynie na modemy 1200 Bd.
- `beacon NUMER <on/off>` – *on* włącza, *off* wyłącza beacon o podanym numerze z zakresu od 0 do 7.
- `beacon NUMER iv CZAS` – ustawia interwał nadawania (w minutach) beaconu o numerze z zakresu od 0 do 7.
- `beacon NUMER dl CZAS` – ustawia opóźnienie/przesunięcie nadawania (w minutach) beaconu o numerze z zakresu od 0 do 7.
- `beacon NUMER path SCIEZKAn-N[,SCIEZKAn-N]/none` – ustawia ścieżkę beaconu o numerze z zakresu od 0 do 7. Polecenie przyjmuje jeden (np. *WIDE2-2*) lub dwa (np. *WIDE2-2,SP3-3*) elementy ścieżki albo opcję *none* dla braku ścieżki.
- `beacon NUMER data TRESC` – ustawia treść beaconu o numerze z zakresu od 0 do 7.
- `digi <on/off>` – *on* włącza, *off* wyłącza digipeater
- `digi NUMER <on/off>` – *on* włącza, *off* wyłącza obsługę aliasu o numerze z zakresu od 0 do 7.
- `digi NUMER alias ALIAS` – ustawia alias o numerze z zakresu od 0 do 7. W przypadku slotów 0-3 (typ *n-N*) przyjmuje do 5 znaków bez SSID, w przypadku slotów 4-7 (aliasy proste) przyjmuje aliasy w formie jak znak wywoławczy wraz z SSID lub bez.
- `digi NUMER max N` – ustawia maksymalne *n* (z zakresu od 1 do 7) dla normalnego powtarzania aliasów typu *n-N* (zakres od 0 do 3).
- `digi NUMER rep N` – ustawia minimalne *n* (z zakresu od 0 do 7), od którego aliasy typu *n-N* (zakres od 0 do 3) będą przetwarzane jak aliasy proste. *N* równe 0 wyłącza tę funkcję.
- `digi NUMER trac <on/off>` – ustawia wybrany alias (z zakresu od 0 do 7) jako trasowalny (*on*) lub nietrasowalny (*off*)
- `digi NUMER viscous <on/off>` – *on* włącza, *off* wyłącza funkcję *viscous delay* dla aliasu z zakresu od 0 do 7.
- `digi NUMER direct <on/off>` – *on* włącza, *off* wyłącza funkcję powtarzania tylko ramek odebranych bezpośrednio dla aliasu z zakresu od 0 do 7.
> Zasadę działania digipeatera opisano w [sekcji 3.2.3](#323-digipeater).
- `digi NUMER filter <on/off>` – *on* włącza, *off* wyłącza filtrowanie ramek dla aliasu z zakresu od 0 do 7.
- `digi filter <black/white>` – ustawia typ listy filtrującej ramki: *black* (wykluczenie - ramki od znaków z listy nie będą powtarzane) lub *white* (wyłączność – tylko ramki od znaków z listy będą powtarzane).
- `digi dupe CZAS` – ustawia czas bufora filtrującego duplikaty, który zapobiega wielokrotnemu powtarzaniu już powtórzonego pakietu. Czas w sekundach z zakresu od 5 do 255.
- `digi list POZYCJA set ZNAK-SSID` – wpisuje znak na wybraną pozycję (z zakresu od 0 do 19) listy filtrującej. Można używać znaku \* do zamaskowania wszystkich liter do końca znaku. *?* maskuje pojedynczą literę w znaku. Do zamaskowania SSID można użyć \* lub *?*.
- `digi list POZYCJA remove` – usuwa wybraną pozycję (z zakresu od 0 do 19) z listy filtrującej
- `monkiss <on/off>` – *on* włącza, *off* wyłącza wysyłanie własnych i powtórzonych ramek na porty KISS
- `nonaprs <on/off>` – *on* włącza, *off* wyłącza odbiór pakietów niebędących pakietami APRS (np. dla Packet Radio)
- `fx25 <on/off>` - *on* włącza, *off* wyłącza obsługę protokołu FX.25. Po włączeniu jednocześnie będą odbierane pakiety AX.25 i FX.25.
- `fx25tx <on/off>` - *on* włącza, *off* wyłącza nadawanie z użyciem protokołu FX.25. Jeśli obsługa FX.25 jest wyłączona całkowicie (polecenie *fx25 off*), to pakiety zawsze będą nadawane z użyciem AX.25.

Ponadto dostępne są polecenia kontrolne:
- `print` – pokazuje aktualne ustawienia.
- `list` – pokazuje zawartość listy filtrującej.
- `save` – zapisuje ustawienia do pamięci i restartuje urządzenie. Należy zawsze użyć tej komendy po zakończeniu konfiguracji. W przeciwnym wypadku niezapisana konfiguracja zostanie porzucona.
- `eraseall` – czyści całą konfigurację i restartuje urządzenie.

Dostępne są także polecenia wspólne:
- `help` – pokazuje stronę pomocy
- `reboot` – restartuje urządzenie
- `version` – pokazuje informacje o wersji oprogramowania
- `monitor` – przełącza port do trybu monitora
- `kiss` – przełącza port do trybu KISS

#### 2.1.2. Przykładowe ustawienia

- Beacon\
    Przykład dla beaconu o numerze 0. Dostępne są beacony od 0 do 7.
    1. *beacon 0 data !5002.63N/02157.91E#Digi Rzeszow* – ustawienie danych beaconu w standardzie APRS
    2. *beacon 0 path WIDE2-2,SP2-2* – ustawienie ścieżki
    3. *beacon 0 iv 15* – nadawanie co 15 minut
    4. *beacon 0 dl 5* – beacon będzie nadany po raz pierwszy po 5 minutach od uruchomienia urządzenia
    5. *beacon 0 on* – uruchomienie beaconu
- Digipeater WIDE1-1 (pomocniczy)\
    Przykład dla aliasu o numerze 0. Dostępne są aliasy 0-3.
    1. *digi 0 alias WIDE* – powtarzanie aliasów typu WIDEn-N. Liczby n i N NIE są ustawiane tutaj.
    2. *digi 0 max 1* – powtarzanie ścieżek maksymalnie WIDE1-1
    3. *digi 0 rep 0* – wyłączenie limitowania i upraszczania ścieżek
    4. *digi 0 trac on* – włączenie trasowania ścieżki (ścieżki typu WIDEn-N są trasowane)
    5. *digi 0 on* – włączenie obsługi danego aliasu
    6. *digi on* – włączenie digipeatera

- Digipeater pomocniczy *viscous delay* (zalecana konfiguracja)\
    Przykład dla aliasu o numerze 0. Dostępne są aliasy 0-3.
    1. *digi 0 alias WIDE* – powtarzanie aliasów typu WIDEn-N. Liczby n i N NIE są ustawiane tutaj.
    2. *digi 0 max 2* – powtarzanie ścieżek maksymalnie WIDE2-2
    3. *digi 0 rep 3* – ścieżki WIDE3-3 i dłuższe będą upraszczane przy powtarzaniu
    4. *digi 0 trac on* – włączenie trasowania ścieżki (ścieżki typu WIDEn-N są trasowane)
    5. *digi 0 viscous on* – włącznie trybu *viscous delay*
    6. *digi 0 on* – włączenie obsługi danego aliasu
    7. *digi on* – włączenie digipeatera

- Digi regionalne WIDEn-N + SPn-N (zalecana konfiguracja)\
    Przykład dla aliasów 0 (dla WIDE) i 1 (dla SP).
    1. *digi 0 alias WIDE* – powtarzanie aliasów typu WIDEn-N. Liczby n i N NIE są ustawiane tutaj.
    2. *digi 0 max 2* – powtarzanie ścieżek maksymalnie WIDE2-2
    3. *digi 0 rep 3* – ścieżki WIDE3-3 i dłuższe będą upraszczane przy powtarzaniu
    4. *digi 0 trac on* – włączenie trasowania ścieżki (ścieżki typu WIDEn-N są trasowane)
    5. *digi 0 on* – włączenie obsługi danego aliasu
    6. *digi 1 alias SP* – powtarzanie aliasów typu SPn-N. Liczby n i N NIE są ustawiane tutaj.
    7. *digi 1 max 7* – powtarzanie maksymalnie ścieżek SP7-7
    8. *digi 1 rep 0* – brak limitu dla ścieżki
    9. *digi 1 trac off* – wyłączenie trasowania ścieżki (ścieżki regionalne nie są trasowane)
    10. *digi 1 on* – włączenie obsługi danego aliasu
    11. *digi on* – włączenie digipeatera

### 2.2. Tryb monitora
Praca urządzenia może być obserwowana przez dowolny port (USB, UART1, UART2). Przejście do trybu monitora następuje po wydaniu polecenia `monitor`.
> Uwaga! Jeśli port pracuje w trybie KISS (domyślnym po uruchomieniu), to wpisywane znaki nie będą widoczne.

W trybie monitora wyświetlane są pakiety odbierane i nadawane, a ponadto możliwe jest dokonanie kalibracji poziomów sygnału.

#### 2.2.1. Polecenia
Dostępne są następujące polecenia:
- `beacon NUMER` - nadaje beacon z zakresu 0 do 7, o ile ten beacon jest włączony.
- `cal <low/high/alt/stop>` - rozpoczyna lub kończy tryb kalibracji: *low* nadaje niski ton, *high* nadaje wysoki ton, *alt* nadaje bajty zerowe/zmieniające się tony, a *stop* zatrzymuje transmisję. Dla modemu 9600 Bd zawsze nadawane są bajty zerowe. 

Dostępne są także polecenia wspólne:
- `help` – pokazuje stronę pomocy
- `reboot` – restartuje urządzenie
- `version` – pokazuje informacje o wersji oprogramowania
- `config` – przełącza port do trybu konfiguracji
- `kiss` – przełącza port do trybu KISS
  
#### 2.2.2. Widok pakietów odbieranych

Dla każdego odebranego pakietu AX.25 wyświetlany jest nagłówek w następującym formacie:
> Frame received [...], signal level XX% (HH%/LL%)
> 
natomiast dla każdego odebranego pakietu FX.25:
> Frame received [...], N bytes fixed, signal level XX% (HH%/LL%)

Gdzie kolejno:
- *...* określa, które modemy odebrały pakiet i jakiego rodzaju filtr został użyty. Możliwe są następujące statusy:
  - *P* - filtr z preemfazą wysokiego tonu
  - *D* - filtr z deemfazą wysokiego tonu
  - *F* - filtr płaski
  - *N* - brak filtra
  - *_* - modem nie odebrał ramki\
Przykładowo status *[_P]* oznacza, że pierwszy modem nie odebrał ramki, a drugi odebrał ramkę i używa filtru z preemfazą. Inny przykładowy status *[N]* oznacza, że dostępny jest tylko jeden modem bez filtra i to on odebrał ramkę.
- *N* określa, ile bajtów zostało naprawionych przez protokół FX.25. Dla pakietów AX.25 pole to nie jest wyświetlane.
- *XX%* określa, jaki jest poziom sygnału, tj. jego amplituda.
- *HH%* określa, jaki jest poziom górnego piku sygnału.
- *LL%* określa, jaki jest poziom dolnego piku sygnału.
> Uwaga! Poziom górnego piku powinien być dodatni, a dolnego ujemny. Ponadto poziomy te powinny być symetryczne względem zera. Duża dysproporcja świadczy o nieprawidłowej polaryzacji wejścia przetwornika ADC.

### 2.3. Tryb KISS
Tryb KISS służy do pracy jako standardowe TNC KISS, współpracujące z wieloma programami do Packet Radio, APRS i podobnych. W trybie tym nie jest dostępne tzw. echo (odsyłanie wpisywanych znaków). Dostępne są jedynie polecenia przełaczenia trybu:
- `monitor` – przełącza port do trybu monitora
- `config` – przełącza port do trybu konfiguracji

### 2.4. Kalibracja poziomów sygnału
Po uruchomieniu urządzenia należy przejść do trybu monitora (polecenie `monitor`) i czekać na pojawienie się pakietów. Należy wyregulować poziom sygnału tak, aby większość pakietów miała poziom sygnału ok. 50% (jak opisano w [sekcji 2.2.2](#222-widok-pakietów-odbieranych)) Poziom sygnału odbieranego należy utrzymywać w zakresie 10-90%.\
Aby zapewnić najlepszą wydajność sieci, poziom sygnału nadawanego powinien być prawidłowo ustawiony. Jest to szczególnie istotne w przypadku modemu 1200 Bd, gdzie sygnał nadawany jest przez standardowe złącze mikrofonowe radiotelefonu FM. Zbyt duży poziom sygnału prowadzi do występowania zniekształceń i dużych dysproporcji amplitud tonów.\
Do kalibracji potrzebny jest odbiornik FM zestrojony na tę samą częstotliwość, co nadajnik. Należy przejść do trybu monitora (polecenie `monitor`) i właczyć nadawanie tonu wysokiego (polecenie `cal high`). Należy ustawić potencjometr do pozycji minimalnego poziomu wysterowania, a następnie powoli zwiększać poziom, jednocześnie uważnie nasłuchując siły sygnału w odbiorniku, który powinien rosnąć. W pewnym momencie poziom sygnału przestanie się zwiększać. Wówczas należy delikatnie cofnąć potencjometr i wyłączyć tryb kalibracji (polecenie `cal stop`). Po tej operacji nadajnik powinien być poprawnie wysterowany.
> Uwaga! Jeśli nie uda się osiągnąć punktu, w którym poziom sygnału przestanie się zwiększać, to prawdopodobnie wartość rezystancji w torze nadawczym jest zbyt duża. Jeśli poziom sygnału jest wyraźnie zbyt niski, należy zmniejszyć wartość tej rezystancji. W przeciwnym wypadku nie jest konieczne podejmowanie kroków.

### 2.5. Programowanie
Programowanie (wgranie oprogramowania) można przeprowadzić na dwa sposoby: za pomocą programatora *ST-Link* lub poprzez port szeregowy UART1 (np. za pomocą przejściówki USB-UART).\
W obydwu sposobach konieczne jest wcześniejsze pobranie oprogramowania VP-Digi (plik HEX).
#### 2.5.1. Programowanie za pomocą programatora ST-Link
1. Pobrać i zainstalować program *ST-Link Utility* wraz z odpowiednimi sterownikami.
2. Podłączyć VP-Digi do programatora.
3. Podłączyć programator do złącza USB.
4. W *ST-Link Utility* przejść do *File->Open file* i wybrać pobrany plik HEX z oprogramowaniem.
5. Przejść do *Target->Program & Verify* i kliknąć *Start*. Po chwili urządzenie się zrestartuje i uruchomi się właściwe oprogramowanie.

#### 2.5.2. Programowanie za pomocą przejściówki USB-UART
1. Upewnić się, że przejściówka pracuje z poziomami logicznymi 0/3.3V. 
2. Jeśli to konieczne, zainstalować sterowniki do przejściówki. 
3. Pobrać i zainstalować program *Flasher-STM32*. 
4. Na płytce ustawiamy zworkę bliższą przycisku reset na położenie 0, a dalszą na 1. 
5. Podłączyć pin *TX* przejściówki do pinu *PA10*, a *RX* do *PA9*. Podłączyć zasilanie (np. z przejściówki). 
6. Ustalić pod którym portem COM dostępna jest przejściówka.
7. Uruchomić *Flasher-STM32*, wybrać odpowiedni port COM i kliknąć *Next*. 
    > Jeżeli ukazał się błąd, należy zresetować mikrokontroler i sprawdzić połączenia oraz ustawienie zworek. 
8. W następnym ekranie nacisnąć *Next*.
9. Na kolejnym ekranie wybrać opcję *Download to device* i wybrać pobrany plik HEX. 
10. Zaznaczyć opcje *Erase neccesary pages* oraz *Verify after download* i kliknąć *Next*. Po chwili układ powinien być zaprogramowany. Przestawić obydwie zworki z powrotem na położenie 0 i zresetować mikrokontroler, a przejściówkę odłączyć. Powinno uruchomić się właściwe oprogramowanie.
### 2.6. Uruchomienie
Urządzenie należy skonstruować wg. schematu przedstawionego w [sekcji 3.1](#31-sprzęt) i zaprogramować (opis w [sekcji 2.5](#25-programowanie)).\
Po uruchomieniu wszystkie porty (USB, UART1, UART2) pracują w trybie KISS, a prędkość pracy portów UART1 i UART2 to 9600 Bd (prędkość portu USB nie ma znaczenia).\
W celu konfiguracji VP-Digi lub monitorowania ramek koniecznie jest połączenie z komputerem przez port USB lub UART i zainstalowanie programu terminalowego (*TeraTerm*, *RealTerm* itp.). Jak wspominano w sekcjach poprzedzających, przejście do tryb konfiguracji i monitora następuje po wprowadzeniu poleceń, odpowiednio, `config` i `monitor` i zatwierdzeniu enterem. 
> W trybie KISS odsyłanie wprowadzanych znaków (*echo*) jest wyłączone, zatem nie będzie widać wprowadzanych poleceń.

Konfigurację urządzenia należy przeprowadzić za pomocą poleceń opisanych w [sekcji 2.1](#21-tryb-konfiguracji).

## 3. Opis techniczny
### 3.1. Sprzęt
VP-Digi natywnie pracuje na mikrokontrolerze STM32F103C8T6 z częstotliwością rdzenia równą 72 MHz, co wymaga użycia zewnętrznego rezonatora 8 MHz. Do budowy urządzenia zalecane jest użycie płytki znanej jako *STM32 Blue Pill*.

Budowę VP-Digi w oparciu o płytkę *STM32 Blue Pill* przedstawiono na schemacie:
![VP-Digi schematic](schematic.png)
#### 3.1.1. Odbiór
Sygnał odbierany podawany jest na pin *PA0* z użyciem układu kondensatorów odsprzęgających (*C4*, *C6*) oraz rezystorów polaryzujących (*R6*, *R9*) i ograniczających (*R7*, *R11*). W celu zapewnienia poprawnego odbioru modulacji FSK muszą być zastosowane kondensatory o stosunkowo dużych pojemnościach. Dla zapewnienia poprawnego odbioru napięcie stałe na pinie *PA0* musi być w okolicach połowy napięcia zasilania, tj. 1.65 V. Nieprawidłowa polaryzacja objawia się w postaci asymetrii poziomu sygnału (patrz [sekcja 2.2.2](#222-widok-pakietów-odbieranych)) lub zanikiem odbioru.
W torze odbiorczym zastosowano szeregowy rezystor *R7* mający na celu ograniczenie maksymalnego prądu pinu, aby wykorzystując wbudowane w mikrokontroler diody wykonać ogranicznik napięcia chroniący przetwornik przed uszkodzeniem spowodowanym zbyt dużym poziomem sygnału.\
Jeśli radiotelefon ma regulowany poziom sygnału wyjściowego, to instalacja elementów *RV2*, *R11* i *C6* nie jest konieczna.
#### 3.1.2. Nadawanie
Sygnał nadawany może zostać generowany z użyciem przetwornika PWM lub R2R (niezalecany od wersji 2.0.0). Przetwornik PWM wykorzystuje sprzętową modulację szerokości impulsów oraz filtr RC drugiego rzędu (*R5*, *C3*, *R3*, *C2*) do odfiltrowania składowych niepożądanych. Filtr zbudowany jest z dwóch filtrów RC pierwszego rzędu o częstotliwości odcięcia ok. 7200 Hz w celu zapewnienia odpowiedniego pasma przenoszenia dla modemu 9600 Bd. Ponadto dla poprawnego nadawania modulacji FSK musi być zastosowany kondensator odsprzęgający *C1* o stosunkowo dużej pojemności.
Rezystor *R2* służy do znacznego zmniejszenia amplitudy sygnału gdy używane jest standardowe wejście mikrofonowe o dużym wzmocnieniu. Wartość tego rezystora należy dobrać indywidualnie.
Radiotelefon przełączany jest w stan nadawania poprzez podanie stanu niskiego przez tranzystor NPN *Q1*. Jeśli radiotelefon ma wejście PTT, to należy połączyć je bezpośrednio do kolektora tranzystora i nie montować rezystora *R1*. Dla standardowych radiotelefonów z wejściem mikrofonowym należy zainstalować rezystor *R1*.

### 3.2. Oprogramowanie
Oprogramowanie VP-Digi napisane jest w języku C z użyciem standardowych bibliotek CMSIS i nagłówków ST dla mikrokontrolera z użyciem operacji na rejestrach. Biblioteka HAL wykorzystana wyłącznie do obsługi USB oraz konfiguracji sygnałów zegarowych.
#### 3.2.1. Modem
##### 3.2.1.1. Demodulacja
Odbiór ramek rozpoczyna się od próbkowania sygnału wejściowego. Sygnał wejściowy jest nadpróbkowany czterokrotnie, co daje odpowiednio próbkowanie 153600 Hz dla modemu 9600 Bd i 38400 Hz dla pozostałych modemów. Po odebraniu czterech próbek są one decymowane i wysyłane do dalszego przetwarzania (z wynikową częstotliwością, odpowiednio, 38400 Hz i 9600 Hz). Dla modemu 1200 Bd wykorzystywane jest 8 próbek na symbol, dla modemu 300 Bd - 32 próbki na symbol, a dla modemu 9600 Bd - 4 próbki na symbol. Z użyciem mechanizmu podobnego do AGC śledzona jest amplituda sygnału. Jeśli modem posiada filtr wstępny, to próbki są filtrowane. Dla modemów AFSK próbka obecna i poprzednie mnożone są przez sygnały liczbowego generatora o częstotliwościach odpowiadających częstoliwości tzw. *mark* i *space* (liczona jest korelacja, która tutaj odpowiada dyskretnej demodulacji częstotliwości). Wynik mnożenia w każdej ścieżce jest sumowany, a następnie wyniki są od siebie odejmowane, co daje "miękki" symbol. W przypadku modemu (G)FSK omówiony krok nie występuje, gdyż funkcję demodulatora FM (FSK) pełni radiotelefon. Na tym etapie prowadzone jest wykrywanie nośnej, które oparte jest o prostą cyfrową PLL. Pętla ta nominalnie działa z częstotliwością równą prędkości symbolowej sygnału (np. 1200 Hz = 1200 Bd). W momencie zmiany "miękkiego" symbolu odbieranego sprawdzana jest odległość od przejścia licznika PLL przez zero. Jeśli jest ona niewielka, tzn. sygnały te są w fazie, to sygnał wejściowy jest prawdopodobnie prawidłowy. Wówczas zwiększana jest wartość dodatkowego licznika. Jeśli przekroczy ona ustalony próg, to ostatecznie sygnał jest uznawany za prawidłowy. Algorytm działa również w drugą stronę - jeśli sygnały nie są w fazie, to wartość licznika jest zmniejszana.
Sygnał zdemodulowany ("miękki symbol") jest filtrowany przez filtr dolnoprzepustowy (odpowiedni do prędkości symbolowej) w celu usunięcia szumu, a następnie wartość symbolu jest określana i przesyłana do mechanizmu odzyskiwania bitów.
##### 3.2.1.2. Modulacja
Modulator AFSK oparty jest o wystawianie próbek sygnału sinusoidalnego, wygenerowanego w momencie startu urządzenia, na przetwornik cyfrowo-analogowy. W zależności od częstotliwości symbolu zmieniana jest częstotliwość wystawiania próbek, tzn. dla tablicy o długości *N* i częstotliwości sygnału *f* kolejne próbki wystawiane są z częstotliwością *f/N*. W trakcie generowania sygnału zachowywana jest ciągłość indeksów tablicy, co skutkuje zachowaniem fazy sygnału.\
W przypadku modemu GFSK symbole z warstwy wyższej (sygnał prostokątny) są filtrowane przez filtr dolnoprzepustowy (wspólny dla demodulatora i modulatora) i wystawiane ze stałą częstotliwością 38400 Hz na przetwornik.
##### 3.2.1.3. Odzyskiwanie bitów
Odzyskiwanie bitów prowadzone jest przez modem. Zastosowana jest cyfrowa PLL (podobna jak w przypadku wykrywania nośnej), działająca z częstotliwością równą częstotliwości symbolowej. W momencie każdego pełnego okresu pętli określana jest ostateczna wartość symbolu na podstawie kilku poprzednio odebranych symboli. W tym samym czasie faza pętli dostrajana jest do fazy sygnału, tzn. do momentów zmiany symbolu. W przypadku modemu wykorzystującego mieszanie bitów (*scrambling*, np. modem G3RUH) wykonywany jest *descrambling*. Ostatecznie wykonywane jest dekodowanie NRZI i bity są przekazywane do wyższej warstwy.
#### 3.2.2. Protokoły
##### 3.2.2.1. Odbiór
Protokoły HDLC, AX.25 i w dużej mierze FX.25 obsługiwane są przez jeden moduł będący dość rozbudowaną maszyną stanów.
Odebrane bity są na bieżąco zapisywane w rejestrze przesuwnym. Rejestr ten monitorowany jest pod kątem wystąpenia flagi HDLC w celu wykrycia początku i końca ramki AX.25, ale również synchronizacji bitowej z nadajnikiem (tzn. wyrównania do pełnego bajtu). Gdy włączony jest odbiór FX.25, to równoczeście monitorowane jest wystąpienie któregoś z tagów korelacyjnych, który również pełni funkcję synchronizacyjną i początku ramki, ale tym razem FX.25. Odbierane bity są zapisywane do bufora, a suma kontrolna jest na bieżąco liczona. Istotnym momentem jest odbiór pierwszych ośmiu bajtów danych, podczas których nie wiadomo, czy jest to ramka FX.25, czy nie, więc wówczas dekodery obydwu protokołów pracują równocześnie. Jeśli tag korelacyjny nie pokrywa się z żadnym znanym, to ramka traktowana jest jako pakiet AX.25. Wówczas bity zapisywane są aż do momentu wystąpienia kolejnej flagi. Następnie, jeśli dozwolony jest wyłącznie odbiór pakietów APRS, sprawdzane są pola Control i PID. Ostatecznie sprawdzana jest suma kontrolna. Jeśli jest prawidłowa, to dokonywana jest multipleksacja modemów (w wypadku gdy więcej niż jeden modem odbierze ten sam pakiet). W przypadku, gdy tag korelacyjny jest prawidłowy, to na jego podstawie określana jest oczekiwana długość pakietu i zapisywane są wszystkie bajty aż do osiągnięcia tej długości. Następnie sprawdzana jest poprawność danych i ewentualna naprawa z użyciem algorytmu Reeda-Solomona. Niezależnie od wyniku operacji surowa ramka jest dekodowana jak pakiet AX.25 (usuwane są dodatkowe bity, flagi) i sprawdzana jest suma kontrolna. Jeśli jest prawidłowa, to podobnie dokonywana jest multipleksacja modemów.
##### 3.2.2.2. Nadawanie
Podobnie jak w przypadku odbioru moduł generujący bity do nadania jest maszyną stanów. Początkowo nadawana jest preambuła o zadanej długości. Gdy używany jest protokół AX.25, to nadawana jest określona ilość flag i nadawane są bity informacyjne. Na bieżąco realizowane jest nadziewanie bitami (*bit stuffing*) i liczenie sumy kontrolnej, która jest dołączana po nadaniu całej ramki. Następuje nadanie określonej liczby flag, i jeżeli są kolejne pakiety do nadania, to od razu następuje przejście do nadania właściwych danych. Ostatecznie nadawany jest ogon o zadanej długości i transmisja kończy się.\
W przypadku FX.25 pakiet wejściowy jest wcześniej kodowany jak pakiet AX.25, tzn. zostają dodane dodatkowe bity, flagi, CRC i pakiet jest umieszczany w oddzielnym buforze. Dzięki temu odbiorniki nieobsługujące FX.25 nadal będą mogły odebrać ten pakiet. Pozostała część bufora zostaje wypełniona odpowiednimi bajtami. Następnie wykonywane jest kodowanie Reeda-Solomona, które wprowadza do bufora bajty parzystości. Gdy rozpoczyna się transmisja, nadana zostaja preambuła, ale po niej nadawany jest odpowiednio dobrany tag korelacyjny. Wówczas następuje nadanie wcześniej przygotowanej ramki. Jeśli są do nadania kolejne pakiety, to proces się powtarza. Ostatecznie nadawany jest ogon i transmisja kończy się.
#### 3.2.3. Digipeater
Po odebraniu pakietu liczony jest jego hasz (algorytm CRC32). Następnie sprawdzane jest wystąpienie takiego samego haszu w buforze *viscous delay*. Jeśli hasze są takie same, to pakiet jest usuwany z bufora i nie są podejmowane żadne dalsze działania. Podobnie sprawdzane jest wystąpienie takiego samego haszu w buforze filtra duplikatów. Jeśli hasze są takie same, to pakiet jest od razu odrzucany. Następnie w ścieżce wyszukiwany jest *H-bit*, informujący o ostatnim elemencie ścieżki przetworzonym przez inne digipeatery. Jeśli nie ma kolejnego elementu gotowego do przetworzenia, to pakiet jest odrzucany. Jeśli występuje element gotowy do przetworzenia, to podejmowane są kroki:
- element porównywany jest z własnym znakiem (tj. czy własny znak występuje *explicite* w ścieżce). Jeśli porównanie jest pomyślne, to do elementu dodawany jest tylko *H-bit*, np. *SR8XXX* przechodzi w *SR8XXX\**.
- element jest w identyczny sposób porównywany z wszystkimi aliasami prostymi wpisanymi do konfiguracji (o ile są one włączone). Jeśli porównanie jest pomyślne, to:
  - jeśli alias jest trasowalny, to zastąpiony zostaje własnym znakiem i dodany zostaje *H-bit* (np. *RZ* zostanie zmienione w *SR8XXX\**)
  - jeśli alias nie jest trasowalny, to dodany zostaje tylko *H-bit* (np. *RZ* zostanie zmienione w *RZ\**)

Pierwsza cześć elementu jest porówywana z wszystkimi aliasami *New-N* (o ile są one włączone). Jeśli porównanie jest pomyślne, to z elementu wyciągana jest liczba zwana *n*, a także zapisywane jest SSID zwane *N* (np. w ścieżce *WIDE2-1* liczba *n*=2, a *N*=1). Początkowo sprawdzane są warunki poprawności ścieżki, tzn. czy 0<*n*<8, czy 0<=*N*<8 oraz czy *N*<=*n*. Jeśli któryś z warunków nie jest spełniony, to pakiet jest odrzucany. Przykłady ścieżek niepoprawnych to:
- *WIDE8-1*
- *WIDE1-2*
- *WIDE0-3*
- *WIDE2-8*
- i wiele innych 

Jednocześnie jeśli w elemencie *N*=0, to ścieżka jest wyczerpana i pakiet jest odrzucany. Potem *n* porównywana jest z zapisaną wartością *max* oraz *rep*. Jeśli *max*<*n*<*rep* (liczba nie zawiera się w żadnym z ustawionych zakresów), to pakiet jest odrzucany. W przypadku, gdy ścieżka jest trasowana, to jeśli:
- *n* <= *max* i *N* > 1, np. *WIDE2-2*, to przed elementem dodawany jest znak digipeatera z *H-bitem*, a *N* jest pomniejszane o 1, np. *SR8XXX\*,WIDE2-1*
- *n* <= *max* i *N*=1, np. *WIDE2-1*, to przed elementem dodawany jest znak digipeatera z *H-bitem*, *N* jest pomniejszane o 1 (czyli staje się zerem) i dodawany jest *H-bit*, np. *SR8XXX\*,WIDE2\**
- *n* >= *rep* (o ile *rep*>0), np. *WIDE7-4*, to element zostaje zastąpiony znakiem digipeatera z *H-bitem*, np. *SR8XXX\** (następuje ograniczenie ścieżki). Wartość *N* nie jest brana pod uwagę.

W przypadku, gdy ścieżka nie jest trasowana, to jeśli:
- *n* <= *max* i 1 < *N* < *n*, np. *SP3-2*, to *N* jest pomniejszane o 1, np. *SP3-1*
- *n* <= *max* i *N* = *n* i nie jest to pierwszy element całej ścieżki, np. *...,SP3-3*, to *N* jest pomniejszane o 1, np. *...,SP3-2* (zachowanie identyczne jak wcześniej)
- *n* <= *max* i *N* = *n* i jest to pierwszy element całej ścieżki (jest to pierwszy skok pakietu), np. *SP3-3*, to ścieżka traktowana jest jak trasowalna, tzn. przed elementem dodawany jest znak digipeatera z *H-bitem*, a *N* jest pomniejszane o 1, np. *SR8XXX\*,SP3-2*
- *n* <= *max* i *N* = 1, np. *SP3-1*, to *N* jest pomniejszane o 1 (staje się zerem) i dodawany jest *H-bit*, np. *SP3\**
- *n* >= *rep* (o ile *rep* > 0), np. *SP7-4*, to element zostaje zastąpiony znakiem digipeatera z *H-bitem*, np. *SR8XXX\** (następuje ograniczenie ścieżki). Wartość *N* nie jest brana pod uwagę.

> Uwaga! Jeśli wartości *max* i *rep* tworzą zachodzące na siebie przedziały (*max* >= *rep*), to wartość *max* jest traktowana priorytetowo.

> Uwaga! Jeśli *rep*=0, to funkcjonalność ograniczania ścieżki jest wyłączona.

Jeśli dla dopasowanego aliasu włączona jest funkcja *direct only* (powtarzania wyłącznie pakietów odebranych bezpośrednio), a element nie jest pierwszym elementem w całej ścieżce lub *N* < *n*, to pakiet jest odrzucany.
Jeśli dla dopasowanego aliasu włączona jest funkcja *viscous delay*, to gotowy pakiet zapisywany jest w buforze, a jego nadanie jest odkładane. Jeśli ten sam pakiet zostanie powtórzony przez inny digipeater w określonym czasie, to zbuforowany pakiet zostanie usunięty (patrz *początek tej sekcji*). Jeśli żadna z tych funkcji nie jest włączona, to do bufora filtra duplikatów zapisywany jest hasz pakietu i pakiet jest wysyłany.\
Ponadto regularnie odświeżany jest bufor funkcji *viscous delay*. Jeśli minął odpowiedni czas i pakiet nie został usunięty z bufora (patrz *początek tej sekcji*), to jego hasz jest zapisywany do bufora filtra duplikatów, pakiet jest wysyłany i usuwany z bufora *viscous delay*.

## 4. Rejestr zmian dokumentacji
### 2023/09/06
- Pierwsza wersja - Piotr Wilkoń