# ESP32 macropad 4x4

## Opis Projektu

W pełni funkcjonalny, bezprzewodowy makropad (klawiatura 4x4 + enkoder obrotowy) oparty na mikrokontrolerze ESP32. Urządzenie działa jako klawiatura Bluetooth (BLE HID) i jest konfigurowane przez innowacyjny interfejs webowy hostowany na zewnętrznym serwerze (Docker/Dockge), komunikujący się z urządzeniem za pomocą **Web Serial API** po kablu USB.

---

## 1. Architektura Sprzętowa (Hardware)

### Komponenty

- **Mikrokontroler:** ESP32-WROOM-32 (DevKit)
- **Matryca:** 16 przełączników mechanicznych (układ 4x4)
- **Diody:** Zabezpieczające przed ghostingiem (kierunek: katoda/pasek w stronę wierszy)
- **Enkoder:** EC11 (obrót nieskończony + przycisk)

### Mapowanie Pinów (Pinout)

| Komponent     | Funkcja           | Pin ESP32 | Tryb (pinMode) | Uwagi                                |
| :------------ | :---------------- | :-------- | :------------- | :----------------------------------- |
| **Matryca**   | Wiersz 0          | GPIO 16   | `OUTPUT`       | Domyślnie HIGH, stan aktywny: LOW    |
| **Matryca**   | Wiersz 1          | GPIO 17   | `OUTPUT`       | j.w.                                 |
| **Matryca**   | Wiersz 2          | GPIO 18   | `OUTPUT`       | j.w.                                 |
| **Matryca**   | Wiersz 3          | GPIO 19   | `OUTPUT`       | j.w.                                 |
| **Matryca**   | Kolumna 0         | GPIO 32   | `INPUT_PULLUP` | Czyta LOW przy wciśnięciu            |
| **Matryca**   | Kolumna 1         | GPIO 33   | `INPUT_PULLUP` | j.w.                                 |
| **Matryca**   | Kolumna 2         | GPIO 25   | `INPUT_PULLUP` | j.w.                                 |
| **Matryca**   | Kolumna 3         | GPIO 26   | `INPUT_PULLUP` | j.w.                                 |
| **Enkoder**   | Kanał A (CLK)     | GPIO 27   | `INPUT_PULLUP` | Wymaga obsługi przerwań (Interrupts) |
| **Enkoder**   | Kanał B (DT)      | GPIO 14   | `INPUT_PULLUP` | Wymaga obsługi przerwań (Interrupts) |
| **Enkoder**   | Przycisk (SW)     | GPIO 12   | `INPUT_PULLUP` | Pin D12 na płytce                    |
| **Zasilanie** | Masa matrycy/enk. | GND       | -              | Wspólna masa                         |

### Zasady Projektowe (Hardware)

1. **Kierunek matrycy:** Ze względu na kierunek diod (katoda do wiersza), skanowanie musi polegać na ściąganiu wierszy do stanu `LOW` i odczytywaniu kolumn.
2. **Piny Enkodera:** Użyto pinów cyfrowych wspierających wbudowane rezystory podciągające (`INPUT_PULLUP`), ponieważ komponent EC11 ich nie posiada.

---

## 2. Architektura Oprogramowania (Firmware ESP32)

Firmware został maksymalnie odchudzony z obsługi sieci (brak serwera WWW i Wi-Fi) na rzecz stabilności BLE i szybkiej komunikacji Serial.

### Główne moduły

1. **Skaner Matrycy:** Odpowiada za debouncing (10-20ms) i odczytywanie stanu klawiszy.
2. **Obsługa Enkodera:** Wykorzystuje przerwania sprzętowe (Hardware Interrupts) dla płynnego odczytu obrotu.
3. **Moduł BLE:** `BleKeyboard` - wysyła kody klawiszy i akcje multimedialne do hosta.
4. **Menadżer Pamięci (LittleFS):** Zapisuje i odczytuje `config.json` z pamięci Flash.
5. **Komunikacja Web Serial:** Czeka na komendy JSON z komputera i wysyła odpowiedzi (115200 baud).
6. **Przełączanie warstw (Hardware Combo):** Jednoczesne przytrzymanie górnego-lewego (r0,c0) i górnego-prawego (r0,c3) klawisza matrycy powoduje przejście do następnej warstwy (cyklicznie 0..4).
7. **System Makr:** Asynchroniczne wykonywanie sekwencji klawiszy (non-blocking, `millis()` w `loop()`). Wspiera do 32 makr po 64 kroków każde.

### Protokół Komunikacji Serial (JSON) — Kontrakt

Komunikacja odbywa się przez USB Serial (115200 baud). Każda wiadomość to pojedynczy obiekt JSON zakończony znakiem nowej linii `\n`. Nie ma odpowiedzi bez komendy — ESP wysyła eventy wyłącznie jako reakcję na komendę lub zmianę stanu fizycznego urządzenia.

#### Komendy Host (WebUI) → ESP

| Komenda      | Format                              | Opis                                                                                              |
| ------------ | ----------------------------------- | ------------------------------------------------------------------------------------------------- |
| `get_config` | `{"cmd":"get_config"}`              | Pobiera pełną konfigurację (layers, encoder, macros, strings)                                     |
| `set_config` | `{"cmd":"set_config","data":{...}}` | Zapisuje pełną konfigurację. `data` zawiera `layers`, `encoder`, opcjonalnie `macros` i `strings` |
| `get_layer`  | `{"cmd":"get_layer"}`               | Pobiera aktualny numer warstwy                                                                    |
| `set_layer`  | `{"cmd":"set_layer","layer":N}`     | Wymusza warstwę N (0-4)                                                                           |
| `get_macros` | `{"cmd":"get_macros"}`              | Pobiera tablicę makr i stringów                                                                   |
| `set_macros` | `{"cmd":"set_macros","data":{...}}` | Zapisuje makra i stringi. `data` może być tablicą makr lub obiektem z `macros` i `strings`        |
| `run_macro`  | `{"cmd":"run_macro","index":N}`     | Testuje wykonanie makra o indeksie N z WebUI                                                      |

#### Eventy ESP → Host (WebUI)

| Event            | Format                                                       | Opis                                                                                     |
| ---------------- | ------------------------------------------------------------ | ---------------------------------------------------------------------------------------- |
| `config`         | `{"cmd":"config","data":{...}}`                              | Odpowiedź na `get_config` — pełna konfiguracja JSON                                      |
| `status`         | `{"cmd":"status","status":"success/error","msg":"..."}`      | Odpowiedź na `set_config`, `set_macros`, `set_layer`, `run_macro`                        |
| `key_state`      | `{"cmd":"key_state","row":R,"col":C,"isPressed":true/false}` | Żywe powiadomienie o zmianie stanu klawisza (naciśnięcie/zwolnienie)                     |
| `layer_change`   | `{"cmd":"layer_change","layer":N}`                           | Zmiana aktywnej warstwy (przez hardware combo, klawisz warstwy lub WebUI)                |
| `macros`         | `{"cmd":"macros","data":[...],"strings":[...]}`              | Odpowiedź na `get_macros`                                                                |
| `macro_progress` | `{"cmd":"macro_progress","index":N,"step":S,"total":T}`      | Postęp wykonania makra — wysyłany po każdym kroku. Gdy `step == total`, makro zakończone |
| `log`            | `{"cmd":"log","level":"info/warn/error/debug","msg":"..."}`  | Logi diagnostyczne                                                                       |

#### Przykłady

```json
// WebUI → ESP: pobranie konfiguracji
{"cmd":"get_config"}

// ESP → WebUI: odpowiedź
{"cmd":"config","data":{"layers":[[...]],"encoder":[{...}],"macros":[...],"strings":[...]}}

// WebUI → ESP: zapis konfiguracji
{"cmd":"set_config","data":{"layers":[[...]],"encoder":[{...}],"macros":[...],"strings":[...]}}

// ESP → WebUI: potwierdzenie
{"cmd":"status","status":"success","msg":"Config saved."}

// WebUI → ESP: pobranie makr
{"cmd":"get_macros"}

// ESP → WebUI: lista makr
{"cmd":"macros","data":[{"name":"Macro 1","steps":[{"keycode":1302,"delay":50}]}],"strings":["Hello"]}

// WebUI → ESP: test makra
{"cmd":"run_macro","index":0}

// ESP → WebUI: live status makra
{"cmd":"macro_progress","index":0,"step":3,"total":10}

// ESP → WebUI: naciśnięcie klawisza (live feedback)
{"cmd":"key_state","row":1,"col":2,"isPressed":true}
```

### Struktura pliku `config.json`

Plik konfiguracyjny zawiera tablicę `layers` (5 warstw po 4×4 klawiszy, każdy jako 16-bitowa wartość), tablicę `encoder` z konfiguracją enkodera dla każdej warstwy, oraz opcjonalne tablice `macros` i `strings`:

```json
{
  "layers": [
    [[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0]],
    ...
  ],
  "encoder": [
    {"left": 4097, "right": 4098, "button": 4100},
    ...
  ],
  "macros": [
    {
      "name": "Nazwa makra",
      "steps": [
        {"keycode": 1302, "delay": 50},
        {"keycode": 0, "delay": 100}
      ]
    }
  ],
  "strings": [
    "Hello World"
  ]
}
```

Format kodowania 16-bitowej wartości klawisza:

- **bity 15-12 (type):** `0` = standardowy klawisz, `1` = multimedialny, `2` = makro (key = indeks makra), `3` = zmiana warstwy, `4` = tekst (key = indeks stringa)
- **bity 11-8 (mods):** bit0=Ctrl, bit1=Shift, bit2=Alt, bit3=GUI
- **bity 7-0 (key):** kod klawisza HID / media index / indeks makra / numer warstwy / indeks stringa

#### Struktura makra

Każde makro to obiekt z polem `name` (ciąg znaków) i tablicą `steps`. Każdy krok to:

- `keycode` — 16-bitowa wartość klawisza do wysłania (typy 0-4). Gdy `0`, krok tylko czeka.
- `delay` — opóźnienie w ms po wykonaniu kroku.

Makra są wykonywane asynchronicznie (non-blocking) — skanowanie matrycy i enkoder działają w tle.

#### Struktura stringów

Tablica ciągów znaków (UTF-8), które mogą być wysłane przez BLE jako tekst. Indeks stringa jest używany w typie 4 (`type=4`).

---

## 3. Architektura Interfejsu (Web UI)

Aplikacja konfiguracyjna hostowana jest na domowym serwerze.

### Stos Technologiczny

- **Frontend:** Lekki framework SPA (np. Vue 3/React + Vite).
- **Stylizacja:** Tailwind CSS (Dark Mode).
- **Komunikacja sprzętowa:** Web Serial API (`navigator.serial`).
- **Wdrożenie:** Kontener Docker serwujący statyczne pliki przez Nginx (zarządzany przez panel Dockge).

### Główne Funkcjonalności

1.  **Wizualizacja:** Interaktywna matryca 4x4 podświetlająca się na żywo w reakcji na fizyczne kliknięcia (Live Feedback).
2.  **Edytor Akcji:** Wybór akcji z predefiniowanych list (Klawisze, Modyfikatory, Multimedia, Makra) bez konieczności wpisywania kodów ASCII.
3.  **Obsługa Enkodera:** Osobna sekcja w UI do przypisywania akcji do kręcenia (lewo/prawo) oraz kliknięcia gałki.
4.  **Zarządzanie Profilami:** 5 dostępnych warstw (Numpad, NFS U2, CS:GO, Custom 1, Custom 2).

---

## 4. Profile Domyślne

Urządzenie posiada 5 predefiniowanych profili (warstw) generowanych przy pierwszym uruchomieniu lub po resecie konfiguracji:

| Warstwa | Profil                | Zawartość                                                                                                                             |
| ------- | --------------------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| 0       | **Numpad**            | Klawiatura numeryczna: 7,8,9,/, 4,5,6,\*, 1,2,3,-, 0,.,Enter,+                                                                        |
| 1       | **NFS Underground 2** | Strzałki (sterowanie), Shift (NOS/gear up), Ctrl (gear down), Spacja (ręczny), Enter (menu), WASD (alternatywne), Esc (pauza)         |
| 2       | **CS:GO / CS2**       | Broń (1-5), Buy (B), Drop (G), Reload (R), granaty (Z-HE, X-flash, C-smoke, V-molotov), Use (E), Switch (Q), Inspect (F), Walk (Ctrl) |
| 3       | **Custom 1**          | Pusta templatka — wszystkie keycode = 0                                                                                               |
| 4       | **Custom 2**          | Pusta templatka — wszystkie keycode = 0                                                                                               |

Encoder dla wszystkich warstw domyślnie: **LEFT=VolumeUp**, **RIGHT=VolumeDown**, **BUTTON=PlayPause**.

Profile można w pełni konfigurować przez WebUI.

---
