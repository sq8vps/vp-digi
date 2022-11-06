# VP-Digi (PL)
VP-Digi to funkcjonalny, tani, łatwy w montażu i konfiguracji sterownik APRS digipeater oparty na STM32 z wbudowanym modemem KISS.

* modem 1200 Bd Bell 202 (standard VHF APRS)
* Generowanie sygnału R2R lub PWM
* Analogowo-cyfrowe wykrywanie zajętego kanału (wykrywanie nośnika danych)
* koder/dekoder AX.25 zaprojektowany dla ramek APRS
* digipeater: 4 ustawialne aliasy n-N, 4 proste aliasy, opóźnienie lepkie (znane z aprx) lub tylko bezpośrednie, lista czarno-biała.
* 8 niezależnych beaconów
* tryb KISS (może być używany jako zwykły modem z UI-View, APRSIS32, XASTIR itp.)
* USB i 2 UART-y: niezależne, działające w trybie KISS, monitora lub konfiguracji

## Opis, schemat, instrukcja
Jeśli nie interesuje Cię kod źródłowy, to repozytorium nie jest dla Ciebie. Możesz znaleźć pełny opis projektu, schematy, skompilowany firmware i instrukcje [na mojej stronie](https://sq8l.pzk.pl/index.php/vp-digi-cheap-and-functional-aprs-digipeater-controller-with-kiss-modem/).

## Wykorzystanie kodu źródłowego
Firmware został napisany przy użyciu System Workbench for STM32 (SW4STM32) i powinieneś być w stanie zaimportować to repozytorium bezpośrednio do IDE. Kod źródłowy jest publicznie dostępny od wersji 1.3.0.

## Opis techniczny
Projekt został zaprojektowany do uruchomienia na płytce Blue Pill (STM32F103C8T6) z kryształem 8MHz. Firmware został napisany z wykorzystaniem jedynie operacji na rejestrach (z nagłówkami ST) oraz bibliotek CMSIS. HAL jest tam tylko dla USB. Kod jest (dość) obszernie komentowany tam gdzie trzeba, więc nie powinien być bardzo trudny do zrozumienia.

### Demodulator
Są dwa demodulatory (i dekodery) pracujące równolegle, aby zapewnić lepszą wydajność. Sygnał jest próbkowany z częstotliwością 38400Hz (32 próbki na symbol) przez DMA. Przerwanie jest generowane po otrzymaniu 4 próbek, a próbki są decymowane. Następnie przechodzą one przez filtr preemfazy/deemfazy (jeśli jest włączony), który wyrównuje amplitudy tonów. Przefiltrowane próbki są mnożone przez lokalnie generowane tony znakowe i przestrzenne (ich części I i Q - cosinus i sinus). Daje to współczynnik korelacji pomiędzy sygnałem wejściowym a każdym tonem. W międzyczasie wytwarzany jest niefiltrowany symbol, który służy do wysterowania PLL detekcji nośnej danych. Różnica współczynników korelacji stanowi nową próbkę i jest przepuszczana przez filtr dolnoprzepustowy w celu wyeliminowania szumu. Filtrowane próbki są porównywane z zerem i demodulowany symbol jest wysyłany do mechanizmu odzyskiwania bitów.
### Odzyskiwanie bitu (i dekoder NRZI)
Mechanizm odzyskiwania bitów jest oparty na cyfrowym PLL. PLL pracuje nominalnie z częstotliwością 1200Hz (=1200 Baud). Zmiana symbolu powinna nastąpić w pobliżu zera licznika PLL, tak aby licznik przepełnił się w połowie symbolu i wartość symbolu została spróbkowana. Przy każdej zmianie symbolu licznik jest mnożony przez współczynnik <1, aby zbliżyć go do zera, utrzymując PLL i sygnał przychodzący w fazie. Sygnał DCD jest multipleksowany z obu modemów. Wyjaśnione więcej w [modem.c](Src/drivers/modem.c). Próbkowany symbol jest dekodowany przez dekoder NRZI i wysyłany do warstwy AX.25.
### Dekodowanie AX.25
Dekoder AX.25 jest dość standardowy. CRC, PID i Control Byte są sprawdzane i obie ścieżki modemu są multipleksowane, aby wyprodukować tylko jedną ramkę wyjściową.
### Wykrywanie nośnika danych
DCD wykorzystuje podejście analogowo-cyfrowe: oparte na PLL, ale pracujące na cyfrowym, niefiltrowanym strumieniu symboli. PLL działa w taki sam sposób jak PLL odzyskiwania bitów, z wyjątkiem tego, że jest zaimplementowany specjalny mechanizm liczenia impulsów DCD, wyjaśniony w [modem.c](Src/drivers/modem.c). Takie podejście wydaje się być znacznie bardziej efektywne niż typowa, cyfrowa, oparta na AX.25 detekcja.
### Kodowanie AX.25
Koder AX.25 jest również dość prosty. Może obsługiwać wiele ramek w rzędzie (ograniczone długością bufora). Każda transmisja rozpoczyna się od flag preambuły, następnie flag nagłówka, danych rzeczywistych, CRC, flag stopki/rozdzielających, danych rzeczywistych, CRC, flag stopki/rozdzielających... i flag ogona. Bity surowe są żądane przez modulator.
### Modulator (i koder NRZI)
Koder NRZI pracuje dokładnie z częstotliwością 1200Hz (=1200 Baud) i żąda bitów od kodera AX.25. Bity są kodowane do symboli, a interwał timera próbkowania DAC jest ustawiony w zależności od wartości symbolu. Z tego powodu używana jest tylko jedna tabela sinusów. Dla tonu 1200 Hz interwał timera jest większy niż dla tonu 2200 Hz - częstotliwość próbkowania jest zmieniana, aby zmienić częstotliwość sygnału wyjściowego. Indeks tablicy jest zawsze zachowany, aby faza sygnału wyjściowego była ciągła.
## Używanie kodu VP-Digi w twoim projekcie
Chciałbym usłyszeć o projektach, które implementują kod źródłowy VP-Digi. Jeśli tworzysz taki, daj mi znać na sq8vps(at)gmail.com.
## Referencje
Ukończenie projektu zajęło sporo czasu, ale obecnie jest to prawdopodobnie najbardziej efektywny, publicznie dostępny modem oparty na STM32 i najbardziej konfigurowalny, oparty na mikrokontrolerze digipeater APRS. Chciałbym wspomnieć o kilku źródłach, które uznałem za naprawdę przydatne lub inspirujące:
* [multimon-ng](https://github.com/EliasOenal/multimon-ng) - ogólny pomysł na demodulator (korelacja)
* [BeRTOS](https://github.com/develersrl/bertos) - dekoder AX.25
* [Forum thread by SP8EBC](http://forum.aprs.pl/index.php?topic=2086.0) - inspiracja
* [DireWolf](https://github.com/wb2osz/direwolf) - pomysł na odzyskanie bitów PLL
* [A High-Performance Sound-Card AX.25 Modem](https://www.tau.ac.il/~stoledo/Bib/Pubs/QEX-JulAug-2012.pdf) - pomysł na preemfazę i filtrowanie dolnoprzepustowe
* [UZ7HO Soundmodem](http://uz7.ho.ua/packetradio.htm) - pomysł DCD opartego na PLL
## Przyczynianie się
Wszystkie wkłady są doceniane, ale proszę o utrzymanie kodu w miarę czystego. Proszę również upewnić się, że firmware działa dobrze przed stworzeniem pull request.

## Licencja
Projekt jest objęty licencją GNU GPL v3 (zobacz [LICENSE](LICENSE)).