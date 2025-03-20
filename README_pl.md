# VP-Digi

**VP-Digi** jest funkcjonalnym, tanim, łatwym w budowie i konfiguracji kontrolerem digipeatera APRS opartym na procesorze STM32 z wbudowanym TNC KISS. VP-Digi może również działać na [AIOC](https://github.com/skuep/AIOC)!

* Wiele modemów:
  * 1200 Bd AFSK Bell 202 (standard VHF)
  * 300 Bd AFSK Bell 103 (standard HF)
  * 9600 Bd GFSK G3RUH (standard UHF)
  * 1200 Bd AFSK V.23
* Generowanie sygnału z użyciem DAC/PWM
* Analogowo-cyfrowe wykrywanie zajętości kanału (DCD)
* Obsługa AX.25
* Obsługa FX.25 (AX.25 z korekcją błędów), w pełni kompatybilna z [Direwolf](https://github.com/wb2osz/direwolf) i [UZ7HO Soundmodem](http://uz7.ho.ua/packetradio.htm)
* Digipeater: 4 ustawialne aliasy n-N, 4 proste aliasy, *viscous delay* (znane z aprx) lub tryb bezpośredni, lista czarna i biała
* 8 niezależnych beaconów
* Tryb KISS (użycie jako zwykły modem Packet Radio, Winlink, APRS itp.)
* USB i 2 interfejsy UART: niezależne, działające w trybie KISS, monitora lub konfiguracji

## Pobieranie i konfiguracja
Najnowsze skompilowane oprogramowanie można znaleźć [tutaj](https://github.com/sq8vps/vp-digi/releases).\
Pełną dokumentację można znaleźć [tutaj](doc/manual_pl.md).

## Aktualizacja oprogramowania do wersji 2.0.0+ na starszym sprzęcie 

W wersji 2.0.0 wartości komponentów zostały zmienione, aby umożliwić obsługę szybszych modulacji (9600 Bd). W przypadku potrzeby użycia tych modulacji niektóre komponenty muszą zostać wymienione. Więcej informacji dostępnych jest w [instrukcji obsługi](doc/manual_pl.md).

## Opis, schemat, instrukcje

Instrukcja użytkownika i opis techniczny są dostępne [tutaj](doc/manual_pl.md).

## Kod źródłowy

Firmware został napisany w środowisku STM32CubeIDE, gdzie można bezpośrednio zaimportować projekt. Kod źródłowy można pobrać za pomocą:

```bash
git clone https://github.com/sq8vps/vp-digi.git
```
Począwszy od wersji 2.0.0 konieczne jest także pobranie odpowiedniego modułu ([LwFEC](https://github.com/sq8vps/lwfec) dla obsługi kodowania Reeda-Solomona):
```bash
git submodule init
git submodule update
```

Począwszy od wersji 2.2.0, VP-Digi może także działać na AIOC [AIOC](https://github.com/skuep/AIOC). Kod źródłowy jest wspólny dla płytek "Blue Pill" i AIOC.
Do wyboru są dwa pliki konfiguracyjne do STM32CubeMX: `vp-digi.ioc` i `vp-digi_aioc.ioc`. W celu poprawnej kompilacji i uruchomienia projektu należy:
1. Otworzyć `vp-digi.ioc` albo `vp-digi_aioc.ioc` w STM32CubeMX, w zależności od docelowej platformy.
2. Wygenerować pliki dla wybranego IDE/toolchainu. Należy zaznaczyć *Generate Under Root*. W zakładce *Code Generator* należy zaznaczyć *Generate peripheral initialization as a pair of '.c/.h' files per peripheral*.
3. Zaimportować projekt do wybranego IDE.
4. W celu obsługi FX.25 należy dołączyć katalog `lwfec` do kompilacji. W STM32CubeIDE można to zrobić w zakładkach *Project->Properties->C/C++ General->Paths and Symbols* in *Includes* i *Source Locations*.
5. Jeśli projekt kompilowany jest na płytkę "Blue Pill" z mikrokontrolerem STM32F103Cx, to konieczne jest ustawienie poziomu optymalizacji na `Optimize For Size (-Os)` dla kompilacji release lub `Optimize for Debug (-Og)` dla kompilacji debug. 
W STM32CubeIDE można to zrobić w zakładce *Project->Properties->C/C++ Build->Settings->MCU GCC Compiler->Optimization*. W przeciwnym wypadku program nie zmieści się w pamięci.

Począwszy od wersji 2.0.0 istnieje również możliwość kompilowania oprogramowania z obsługą lub bez obsługi protokołu FX.25. 
Symbol `ENABLE_FX25` musi zostać zdefiniowany, aby włączyć obsługę FX.25. W STM32CubeIDE można to zrobić w menu *Project->Properties->C/C++ Build->Settings->MCU GCC Compiler->Preprocessor->Defined symbols*.

W przypadku ponownego budowania projektu dla innej platformy, należy przejść przez kroki opisane powyżej. Niestety pewne pliki mogą pozostać lub nie być ponownie wygenerowane, co poskutkuje błędem kompilacji.
Należy usunąć katalogi `Drivers`, `Middlewares`, `USB_DEVICE` z wyłączeniem pliku `USB_DEVICE/App/usbd_cdc_if.c`,
`Core/Startup`, pliki `stm32f*_hal_msp.c`, `syscalls.c`, `sysmem.c`, `gpio.c`, `system_stm32f*.c` z katalogu `Core/Src`, `stm32f*_it.h`, `stm32f*_hal_conf.h`, `gpio.h` z katalogu `Core/Inc` oraz `.project`, `.cproject` i `.mxproject` z głównego katalogu.

## Wkład
Każdy wkład jest mile widziany.

## Licencja
Projekt jest objęty licencją GNU GPL v3 (zobacz [LICENSE](LICENSE)).
