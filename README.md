# el-lobo-server

Serwer czatu el-lobo

[insert-logo-here]

## Protokół

Składnia

| wersja protokołu | 4 literowy nagłówek | liczba argumentów | argumenty | rozdzielone | separatorem |

gdzie:

* | oznacza separator
* Nagłówek, składający się z czterech charów, charakteryzuje polecenie/zwracane dane
* wersja protokołu jest integerem, podawana celem zachowania kompatybilności
* liczba argumentów to integer, opisuje liczbę następujących po sobie argumentów. Może wynieść 0
* opcjonalne argumenty/zwracane dane


## Polecenia
Nazwę polecenia przesyłamy w nagłówku. Wersja protokołu 1.

### Obsługiwane przez serwer
| Nazwa | Wersja |                          Przeznaczenie                          |                   Przykładowa składnia                  |                                      Przykładowa odpowiedź                                     |               Przykładowy błąd               |
|:-----:|:------:|:---------------------------------------------------------------:|:-------------------------------------------------------:|:----------------------------------------------------------------------------------------------:|:--------------------------------------------:|
|  CREA |    1   |                    Utwórz nowego użytkownika                    |           \|1\|CREAT\|2\|username\|password\|           |                                     \|1\|RETN\|1\|SUCCESS\|                                    |    \|1\|RETN\|2\|ERROR\|NO_PSWD_PROVIDED\|   |
|  SEND |    1   |                  Wyślij wiadomość użytkownikowi                 | \|1\|SEND\|4\|username\|password\|targetUser\|message\| |                                     \|1\|RETN\|1\|SUCCESS\|                                    |      \|1\|RETN\|2\|ERROR\|INVALID_USER\|     |
|  PULL |    1   |         Pobierz nieprzeczytane wiadomości od użytkownika        |       \|1\|PULL\|3\|username\|password\|fromWho\|       | \|1\|RETN\|2\|timestamp\|wiadomosc_1\| \|1\|RETN\|2\|timestamp\|wiadomosc_2\| ...  \|1\|ENDT\| |  \|1\|RETN\|2\|ERROR\|AUTENTICATION_FAILED\| |
|  PEND |    1   | Pobierz użytkowników, od których masz nieprzeczytane wiadomości |            \|1\|PEND\|2\|username\|password\|           |                  \|1\|RETN\|username1\| \|1\|RETN\|username2\| ... \|1\|ENDT\|                 | \|1\|RETN\|2\|ERROR\|AUTHENTICATION_FAILED\| |


Uwagi.
* Hasło w **CREA** nie może być puste.
* **PULL** po wysłaniu wiadomości usuwa je z serwera (serwer przechowuje tylko nieprzeczytane wiadomości).
* **PULL** pobiera na raz wszystkie wiadomości. Przygotuj trochę miejsca. Jednak usuwa wiadomość z serwera dopiero, kiedy udało się całą wytransmitować, więc nawet po przerwaniu połączenia, zacznie pobierać wiadomości które nie zostały przesłane.


### Zwracane przez serwer
| Nazwa | Wersja |                   Przeznaczenie                       |
|:-----:|:------:|:-----------------------------------------------------:|
|  RETN |    1   |               Zwracana wiadomość                      |
|  RETB |    1   | Koniec transmisji wielu danych (np. wielu wiadomości) |