# el-lobo-server

Serwer czatu el-lobo

[insert-logo-here]

## Kompilacja

Wymagania:
* kompilator GCC
* GNU make
> apt install gcc-9 make

Z włączonym debugowaniem
* UBSAN (wybrane moduły)
* symbole dla gdb
* użycie `;` jako separatora
* logowanie na poziomie `Debug`
> make DEBUG=true

Z optymalizacją
* poziom optymalizacji `-O3`
* separator ósemkowo `\31`
* logowanie na poziomie `Info`
> make

Mieszany tryb
* jak z optymalizacją, ale logowanie na poziomie `Debug`
> make DEBUG=some

## Uruchomienie

> ./lobo [numer_portu]

Bez podania portu, nasłuchuje na porcie 1300.

## Protokół

Składnia

| wersja protokołu | 4 literowy nagłówek | liczba argumentów | argumenty | rozdzielone | separatorem |

gdzie:

* | oznacza separator (domyślnie ósemkowo `\31`)
* Nagłówek, składający się z czterech charów, charakteryzuje polecenie/zwracane dane
* wersja protokołu jest integerem, podawana celem zachowania kompatybilności
* liczba argumentów to integer, opisuje liczbę następujących po sobie argumentów. Może wynieść 0
* opcjonalne argumenty/zwracane dane


## Polecenia
Nazwę polecenia przesyłamy w nagłówku. Wersja protokołu 1.

### Obsługiwane przez serwer
| Nazwa | Wersja |                          Przeznaczenie                          |                   Przykładowa składnia                  |                                      Przykładowa odpowiedź                                     |               Przykładowy błąd               |
|:-----:|:------:|:---------------------------------------------------------------:|:-------------------------------------------------------:|:----------------------------------------------------------------------------------------------:|:--------------------------------------------:|
|  CREA |    1   |                    Utwórz nowego użytkownika                    |           \|1\|CREA\|2\|username\|password\|           |                                     \|1\|RETN\|1\|SUCCESS\|                                    |    \|1\|RETN\|2\|ERROR\|NO_PSWD_PROVIDED\|   |
|  SEND |    1   |                  Wyślij wiadomość użytkownikowi                 | \|1\|SEND\|4\|username\|password\|targetUser\|message\| |                                     \|1\|RETN\|1\|SUCCESS\|                                    |      \|1\|RETN\|2\|ERROR\|INVALID_USER\|     |
|  PULL |    1   |         Pobierz nieprzeczytane wiadomości od użytkownika        |       \|1\|PULL\|3\|username\|password\|fromWho\|       | \|1\|RETN\|3\|timestamp\|nadawca\|wiadomosc_1\| \|1\|RETN\|3\|timestamp\|nadawca\|wiadomosc_2\| ...  \|1\|ENDT\| |  \|1\|RETN\|2\|ERROR\|AUTHENTICATION_FAILED\| |
|  PEND |    1   | Pobierz użytkowników, od których masz nieprzeczytane wiadomości |            \|1\|PEND\|2\|username\|password\|           |                  \|1\|RETN\|1\|username1\|<br/> \|1\|RETN\|1\|username2\|<br/> ... \|1\|ENDT\|                 | \|1\|RETN\|2\|ERROR\|AUTHENTICATION_FAILED\| |
|  SUBS |    1   |                      Subskrybuj                                 |            \|1\|SUBS\|2\|username\|password\|           |                                    \|1\|RETN\|1\|SUBSCRIBED\|                                  | \|1\|RETN\|2\|ERROR\|AUTHENTICATION_FAILED\| |
|  USUB |    1   |           Zakończ subskrypcję na tym gnieździe                  |            \|1\|USUB\|2\|username\|password\|           |                                    \|1\|RETN\|1\|UNSUBSCRIBED\|                                  | \|1\|RETN\|2\|ERROR\|AUTHENTICATION_FAILED\| |
|  PULL |    2   |         Pobierz nieprzeczytane wiadomości od użytkownika        |       \|2\|PULL\|3\|username\|password\|fromWho\|       | \|1\|RETN\|3\|timestamp\|nadawca\|wiadomosc_1\| \|1\|RETN\|3\|timestamp\|nadawca\|wiadomosc_2\| ...  \|1\|ENDT\| |  \|1\|RETN\|2\|ERROR\|AUTHENTICATION_FAILED\| |
|  APLL |    2   | Pobierz wszystkie wiadomości od użytkownika i te które wysłałeś użytkownikowi |       \|2\|APLL\|3\|username\|password\|fromWho\|       | \|1\|RETN\|3\|timestamp\|nadawca\|wiadomosc_1\| \|1\|RETN\|3\|timestamp\|nadawca\|wiadomosc_2\| ...  \|1\|ENDT\| |  \|1\|RETN\|2\|ERROR\|AUTHENTICATION_FAILED\| |


Uwagi.
* Hasło w **CREA** nie może być puste.
* **PULL** po wysłaniu wiadomości usuwa je z serwera (serwer przechowuje tylko nieprzeczytane wiadomości).
* **PULL** pobiera na raz wszystkie wiadomości. Przygotuj trochę miejsca. Jednak usuwa wiadomość z serwera dopiero, kiedy udało się całą wytransmitować, więc nawet po przerwaniu połączenia, zacznie pobierać wiadomości które nie zostały przesłane.
* Mechanizm subskrypcji wysyła informacje o nowej wiadomości do subskrybenta. Po przełączeniu się na nowe gniazdo (ponownym połączeniu), trzeba subskrybować od nowa.
Zwracana wiadomość jest postaci `|1|ALRT|2|SUBSCRIPTION_ALERT|whoHasSentTheMessage-username|`.
* **PULL** w wersji `2` nie usuwa wiadomości, jedynie oznacza je jako przeczytane.
* 2**APLL** pobiera zarówno otrzymane jak i wysłane wiadomości. Klient musi sam uporządkować je we właściwej kolejności.
* Mieszanie 2**PULL**, 2**APLL** i 1**PULL** to zły pomysł. W interesie użytkownika jest stosowanie klienta, który korzysta z jednego rodzaju protokołów. Można skompilować serwer z opcją `DISABLE_1PULL=true`, by wyłączyć "groźny" stary `PULL` który kasuje wiadomości.

### Zwracane przez serwer
| Nazwa | Wersja |                            Przeznaczenie                                |
|:-----:|:------:|:-----------------------------------------------------------------------:|
|  RETN |    1   |                        Zwracana wiadomość                               |
|  ENDT |    1   |           Koniec transmisji wielu danych (np. wielu wiadomości)         |
|  ALRT |    1   | Komunikat serwera na inne zdarzenie (aktualnie tylko subskrypcje)       |
