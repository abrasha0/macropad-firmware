# Integration Guide — Makropad ESP32 4×4 + EC11

Kompletny przewodnik dla integratorów tworzących interfejs konfiguracyjny (WebUI / aplikacja desktopowa) dla makropada.

---

## Spis treści

1. [Wprowadzenie](#1-wprowadzenie)
2. [Architektura komunikacji](#2-architektura-komunikacji)
3. [Format bindingu klawiszy (JSON object)](#3-format-bindingu-klawiszy-json-object)
4. [Komendy Host → ESP](#4-komendy-host--esp)
5. [Eventy ESP → Host](#5-eventy-esp--host)
6. [Struktura config.json](#6-struktura-configjson)
7. [System makr](#7-system-makr)
8. [System stringów (per-key)](#8-system-stringów-per-key)
9. [Konfiguracja enkodera](#9-konfiguracja-enkodera)
10. [System warstw (layers)](#10-system-warstw-layers)
11. [Debugowanie i logi](#11-debugowanie-i-logi)
12. [Zachowania specjalne i edge case'y](#12-zachowania-specjalne-i-edge-casey)

---

## 1. Wprowadzenie

Makropad to bezprzewodowe urządzenie HID (BLE Keyboard) oparte na ESP32, wyposażone w:

- 16 klawiszy mechanicznych w matrycy 4×4
- Enkoder obrotowy EC11 (obrót lewo/prawo + przycisk)
- 5 konfigurowalnych warstw (profili)
- System makr (do 16 makr, 32 kroków każde)
- System stringów per-key (string inline w bindingu klawisza, max 64 znaki)

Firmware został celowo odchudzony — **brak Wi-Fi, brak serwera WWW**. Jedyna komunikacja konfiguracyjna odbywa się przez USB Serial (Web Serial API) w formacie JSON.

### Pinout referencyjny

| Komponent   | Funkcja       | Pin ESP32 | Tryb          |
| :---------- | :------------ | :-------- | :------------ |
| Matryca     | Wiersze 0-3   | GPIO 16-19 | OUTPUT        |
| Matryca     | Kolumny 0-3   | GPIO 32,33,25,26 | INPUT_PULLUP |
| Enkoder CLK | Kanał A       | GPIO 14   | INPUT_PULLUP  |
| Enkoder DT  | Kanał B       | GPIO 27   | INPUT_PULLUP  |
| Enkoder SW  | Przycisk      | GPIO 12   | INPUT_PULLUP  |

---

## 2. Architektura komunikacji

- **Medium:** USB Serial (UART przez USB)
- **Baud rate:** 115200
- **Format:** JSON, jedna wiadomość na linię, zakończona `\n`
- **Kodowanie:** UTF-8
- **Max długość linii:** 4096 bajtów (dłuższe są odrzucane z log error)

### Zasady ogólne

1. ESP **nigdy** nie wysyła danych bez przyczyny — odpowiada tylko na komendy lub przy zmianie stanu fizycznego urządzenia.
2. Każda wiadomość to jeden obiekt JSON. Wiadomości nie są grupowane.
3. Wszystkie odpowiedzi i eventy są natychmiast wysyłane — nie ma wewnętrznego kolejkowania.
4. ESP rozpoczyna pracę zaraz po podłączeniu zasilania. Wysyła log `"Setup complete."` gdy jest gotowy.

### Przepływ połączenia (zalecany)

```
1. Użytkownik łączy się przez Web Serial API (navigator.serial.requestPort())
2. Otwiera port z baud rate 115200
3. OPCJONALNIE: wysyła {"cmd":"get_config"} aby pobrać aktualną konfigurację
4. OPCJONALNIE: wysyła {"cmd":"get_macros"} aby pobrać makra
5. UI nasłuchuje eventów: key_state, layer_change, macro_progress, log, status
```

---

## 3. Format bindingu klawiszy (JSON object)

Każda akcja w makropadzie (klawisz matrycy, enkoder) to obiekt JSON o następującej strukturze:

```json
{"t": <type>, "m": <mods>, "k": <key>, "s": <string>}
```

### Pole `t` (type)

| Wartość | Nazwa      | Znaczenie                                 | Dodatkowe pola          |
| :------ | :--------- | :---------------------------------------- | :---------------------- |
| `0`     | `NONE`     | Brak akcji                                | —                       |
| `1`     | `KEY`      | Standardowy klawisz + modyfikatory        | `m`, `k`                |
| `2`     | `MEDIA`    | Klawisz multimedialny                     | `k` (indeks 1-9)        |
| `3`     | `MACRO`    | Wywołanie makra                           | `k` (indeks 0-15)       |
| `4`     | `LAYER`    | Zmiana warstwy                            | `k` (numer 0-4)         |
| `5`     | `STRING`   | Wysłanie stringu przez BLE                | `s` (tekst, max 64 znaki) |

### Pole `m` (mods) — tylko dla `type=1`

| Bit   | Modyfikator         |
| :---- | :------------------ |
| bit 0 | `KEY_LEFT_CTRL`     |
| bit 1 | `KEY_LEFT_SHIFT`    |
| bit 2 | `KEY_LEFT_ALT`      |
| bit 3 | `KEY_LEFT_GUI`      |

Modyfikatory można dowolnie łączyć (OR).

### Pole `k` (key)

Zależne od `type` (patrz tabela wyżej). Dla `type=1` to kod HID klawisza.

### Pole `s` (string) — tylko dla `type=5`

Tekst do wysłania przez BLE. Maksymalnie 64 znaki.

### Przykłady bindingów

```json
{"t": 1, "k": 4}             // klawisz 'A' (HID 0x04)
{"t": 1, "m": 1, "k": 6}    // Ctrl+C (HID 0x06)
{"t": 2, "k": 1}             // VOLUME_UP
{"t": 3, "k": 0}             // pierwsze makro
{"t": 4, "k": 2}             // warstwa 2
{"t": 5, "s": "Hello World"} // string inline
0                             // brak akcji (BIND_NONE)
```

Wartość `0` (liczba) w tablicy layers oznacza brak akcji.

### Kody HID klawiszy (type=1)

Pełna lista kodów HID w standardzie USB: [USB HID Usage Tables](https://www.usb.org/sites/default/files/hut1_5.pdf) (rozdział 10, s. 53–80).

Najczęściej używane:

| Klawisz       | Kod HID (hex) | Uwagi                  |
| :------------ | :------------ | :--------------------- |
| `A`–`Z`       | `0x04`–`0x1D` |                        |
| `0`–`9`       | `0x27`–`0x30` | Górny rząd             |
| `Enter`       | `0x28`        |                        |
| `Esc`         | `0x29`        |                        |
| `Backspace`   | `0x2A`        |                        |
| `Tab`         | `0x2B`        |                        |
| `Space`       | `0x2C`        |                        |
| `-` / `=`     | `0x2D` / `0x2E` |                      |
| `[` / `]`     | `0x2F` / `0x30` |                      |
| `;` / `'`     | `0x33` / `0x34` |                      |
| `,` / `.` / `/` | `0x36` / `0x37` / `0x38` |           |
| Strzałki      | `0x4F`–`0x52` | D, L, R, U            |
| KP_0–KP_9     | `0x59`–`0x62` | Klawiatura numeryczna |
| KP_DIVIDE     | `0x54`        |                        |
| KP_MULTIPLY   | `0x55`        |                        |
| KP_SUBTRACT   | `0x56`        |                        |
| KP_ADD        | `0x57`        |                        |
| KP_ENTER      | `0x58`        |                        |
| KP_DOT        | `0x63`        |                        |
| F1–F12        | `0x3A`–`0x45` |                        |
| `KEY_LEFT_CTRL`  | `0xE0`    | Użyj pola `m`          |
| `KEY_LEFT_SHIFT` | `0xE1`    | Użyj pola `m`          |
| `KEY_LEFT_ALT`   | `0xE2`    | Użyj pola `m`          |
| `KEY_LEFT_GUI`   | `0xE3`    | Użyj pola `m`          |

### Indeksy akcji multimedialnych (type=2)

| key | Akcja                   |
| :-- | :---------------------- |
| 1   | `KEY_MEDIA_VOLUME_UP`   |
| 2   | `KEY_MEDIA_VOLUME_DOWN` |
| 3   | `KEY_MEDIA_MUTE`        |
| 4   | `KEY_MEDIA_PLAY_PAUSE`  |
| 5   | `KEY_MEDIA_NEXT_TRACK`  |
| 6   | `KEY_MEDIA_PREVIOUS_TRACK` |
| 7   | `KEY_MEDIA_STOP`        |
| 8   | `KEY_MEDIA_CALCULATOR`  |
| 9   | `KEY_MEDIA_WWW_HOME`    |

---

## 4. Komendy Host → ESP

### 4.1 `get_config`
Pobiera pełną konfigurację (layers, encoder, macros).

**Request:**
```json
{"cmd":"get_config"}
```

**Response:** `config` event (patrz sekcja 5.1)

---

### 4.2 `set_config`
Zapisuje pełną konfigurację do pamięci flash. Obiekt `data` musi zawierać `layers` i `encoder`. Pole `macros` jest opcjonalne.

**Request:**
```json
{
  "cmd": "set_config",
  "data": {
    "layers": [[...], [...], [...], [...], [...]],
    "encoder": [{"left": ..., "right": ..., "button": ...}, ...],
    "macros": [...]
  }
}
```

**Response:** `status` event (sekcja 5.2)

> **Ważne:** Jeśli w `encoder` pominięto jakieś warstwy (<5), brakujące warstwy zostaną ustawione na domyślne (VolumeUp/VolumeDown/PlayPause). `encoder` powinien zawierać zawsze 5 wpisów.

---

### 4.3 `get_layer`
Pobiera aktualny numer aktywnej warstwy.

**Request:**
```json
{"cmd":"get_layer"}
```

**Response:** `layer_change` event (sekcja 5.4)

---

### 4.4 `set_layer`
Wymusza zmianę warstwy.

**Request:**
```json
{"cmd":"set_layer","layer":2}
```

Gdzie `layer` ∈ {0, 1, 2, 3, 4}.

**Response (success):** `layer_change` event + `{"cmd":"status","status":"success","msg":"Zmieniono warstwę z WebUI."}`

**Response (error):** `{"cmd":"status","status":"error","msg":"Invalid layer index."}` gdy `layer` ∉ {0,1,2,3,4}

---

### 4.5 `get_macros`
Pobiera tablicę makr z pamięci RAM (aktualnie załadowane).

**Request:**
```json
{"cmd":"get_macros"}
```

**Response:** `macros` event (sekcja 5.5)

---

### 4.6 `set_macros`
Zapisuje makra do pamięci flash. Przyjmuje dwie formy:

**Forma A — tablica makr:**
```json
{"cmd":"set_macros","data":[{"name":"...","steps":[...]}]}
```

**Forma B — obiekt z makrami:**
```json
{
  "cmd": "set_macros",
  "data": {
    "macros": [{"name":"...","steps":[...]}]
  }
}
```

**Response:** `status` event (sekcja 5.2)

> **Walidacja:** `data` musi być tablicą makr lub obiektem z polem `macros`. Jeśli `data` ma nieprawidłowy typ lub brakuje `macros` w obiekcie, zwracany jest `status: error` bez modyfikacji konfiguracji.

---

### 4.7 `run_macro`
Testuje wykonanie makra o podanym indeksie. Makro wykonuje się asynchronicznie.

**Request:**
```json
{"cmd":"run_macro","index":0}
```

**Response:** `status` event + serie `macro_progress` eventów w trakcie wykonywania.

---

## 5. Eventy ESP → Host

### 5.1 `config`
Odpowiedź na `get_config`. Zawiera pełną konfigurację.

```json
{
  "cmd": "config",
  "data": {
    "layers": [[...], [...], [...], [...], [...]],
    "encoder": [{"left":...,"right":...,"button":...}, ...],
    "macros": [{"name":"...","steps":[{"keycode":1302,"delay":50}]}]
  }
}
```

### 5.2 `status`
Odpowiedź na komendy modyfikujące (`set_config`, `set_macros`, `set_layer`, `run_macro`).

```json
{"cmd":"status","status":"success","msg":"Config saved."}
{"cmd":"status","status":"error","msg":"Failed to save config."}
```

### 5.3 `key_state`
Natychmiastowa notyfikacja o zmianie stanu klawisza matrycy. Wysyłana dla **każdej** zmiany (press i release), niezależnie czy klawisz ma przypisaną akcję.

```json
{"cmd":"key_state","row":1,"col":2,"isPressed":true}
```

### 5.4 `layer_change`
Zmiana aktywnej warstwy. Wysyłana w odpowiedzi na:
- Sprzętowe combo (r0,c0 + r0,c3)
- Klawisz z `type=4` (LAYER)
- Enkoder/makro z `type=4`
- Komendę `set_layer`

```json
{"cmd":"layer_change","layer":2}
```

### 5.5 `macros`
Odpowiedź na `get_macros`.

```json
{
  "cmd": "macros",
  "data": [
    {"name":"Macro 1","steps":[{"keycode":1302,"delay":50}]}
  ]
}
```

### 5.6 `macro_progress`
Postęp wykonywania makra. Wysyłany:
- Przy starcie makra: `step=0`
- Po każdym wykonanym kroku: `step=N`
- Przy zakończeniu: `step=total`

```json
{"cmd":"macro_progress","index":0,"step":3,"total":10}
```

Gdy `step == total` — makro zakończone.

### 5.7 `log`
Logi diagnostyczne. Poziom: `info`, `warn`, `error`, `debug`.

```json
{"cmd":"log","level":"info","msg":"Setup complete."}
{"cmd":"log","level":"error","msg":"Config init failed! Using defaults."}
{"cmd":"log","level":"warn","msg":"runMacro: preempting running macro"}
```

---

## 6. Struktura `config.json`

Plik konfiguracyjny przechowywany w LittleFS (`/config.json`). Zawiera wszystkie dane konfiguracyjne.

```json
{
  "layers": [
    [
      [
        {"t": 1, "k": 95},
        {"t": 1, "k": 96},
        {"t": 1, "k": 97},
        {"t": 1, "k": 84}
      ],
      [
        {"t": 1, "k": 92},
        {"t": 1, "k": 93},
        {"t": 1, "k": 94},
        {"t": 1, "k": 85}
      ],
      [
        {"t": 1, "k": 89},
        {"t": 1, "k": 90},
        {"t": 1, "k": 91},
        {"t": 1, "k": 86}
      ],
      [
        {"t": 1, "k": 98},
        {"t": 1, "k": 99},
        {"t": 1, "k": 88},
        {"t": 1, "k": 87}
      ]
    ]
  ],
  "encoder": [
    {"left": {"t": 2, "k": 1}, "right": {"t": 2, "k": 2}, "button": {"t": 2, "k": 4}},
    {"left": {"t": 2, "k": 1}, "right": {"t": 2, "k": 2}, "button": {"t": 2, "k": 4}},
    {"left": {"t": 2, "k": 1}, "right": {"t": 2, "k": 2}, "button": {"t": 2, "k": 4}},
    {"left": {"t": 2, "k": 1}, "right": {"t": 2, "k": 2}, "button": {"t": 2, "k": 4}},
    {"left": {"t": 2, "k": 1}, "right": {"t": 2, "k": 2}, "button": {"t": 2, "k": 4}}
  ],
  "macros": [
    {
      "name": "Nazwa makra",
      "steps": [
        {"keycode": 1302, "delay": 50},
        {"text": "Hello", "delay": 100}
      ]
    }
  ]
}
```

### Self-healing

Jeśli plik `config.json` nie istnieje lub jest niepoprawny JSON, firmware automatycznie generuje domyślny plik z 5 warstwami:
- **Layer 0:** Numpad (klawiatura numeryczna)
- **Layer 1:** NFS Underground 2 (sterowanie)
- **Layer 2:** CS:GO / CS2 (skróty)
- **Layer 3:** Custom 1 (pusta)
- **Layer 4:** Custom 2 (pusta)

Encoder domyślnie: LEFT=VolumeUp, RIGHT=VolumeDown, BUTTON=PlayPause.

---

## 7. System makr

### Limity

| Parametr          | Wartość |
| :---------------- | :------ |
| Maksymalna liczba makr | 16 |
| Maksymalna liczba kroków na makro | 32 |
| Maksymalna długość nazwy | 31 znaków |
| Maksymalna długość stringa w kroku | 63 znaki |

### Struktura makra

```json
{
  "name": "Nazwa makra",
  "steps": [
    {"keycode": 1302, "delay": 50},
    {"keycode": 0, "delay": 100},
    {"text": "Hello World", "delay": 0}
  ]
}
```

- `name`: dowolny string (max 31 znaków)
- `steps`: tablica kroków
  - `keycode`: 16-bitowa wartość klawisza. `0` = krok tylko czeka (opóźnienie), nic nie wysyła. Obsługuje typy 0-3 (KEY, MEDIA, MACRO, LAYER).
  - `text`: alternatywnie, string do wysłania przez BLE. Gdy obecne, `keycode` jest ignorowany. Max 63 znaki.
  - `delay`: opóźnienie w **milisekundach** po wykonaniu kroku. Może być `0`.

### Wykonanie

- Makra wykonują się **asynchronicznie** (non-blocking) — skanowanie matrycy i enkoder działają w tle.
- Jedno makro na raz. Jeśli nowe makro zostanie uruchomione podczas trwania innego, bieżące jest **przerywane** (preempted), a klawisze BLE są zwalniane.
- Każdy krok makra może wywołać: klawisz, multimedia, string, zmianę warstwy, a nawet inne makro.

### Wykorzystanie w keymapie

Aby przypisać klawisz matrycy do makra, użyj `type=3`, a w polu `k` umieść indeks makra.

Przykład: `{"t": 3, "k": 0}` → pierwsze makro.

---

## 8. System stringów (per-key)

Stringi są wysyłane przez BLE jako tekst (funkcja `bleKeyboard.print()`). Przydatne do:
- Wklejania haseł
- Wysyłania komend tekstowych
- Wypełniania formularzy

### Przechowywanie

String jest przechowywany **bezpośrednio w bindingu klawisza** — nie ma globalnej listy stringów. Każdy klawisz matrycy lub enkoder może mieć własny, niezależny string.

### Wykorzystanie w keymapie

Aby przypisać klawisz do stringa, użyj `type=5` z polem `s`:

```json
{"t": 5, "s": "Hello World"}
```

### Limit

Maksymalna długość stringa: **64 znaki** (włączając NULL terminator). Strings dłuższe są obcinane przy ładowaniu konfiguracji.

### Stringi w makrach

Krok makra może również wysłać string — użyj pola `text` zamiast `keycode`:

```json
{"text": "Hello World", "delay": 50}
```

---

## 9. Konfiguracja enkodera

Enkoder ma trzy akcje dla każdej warstwy, przechowywane w tablicy 5-elementowej:

```json
{
  "encoder": [
    {"left": {"t": 2, "k": 1}, "right": {"t": 2, "k": 2}, "button": {"t": 2, "k": 4}},
    {"left": {"t": 2, "k": 1}, "right": {"t": 2, "k": 2}, "button": {"t": 2, "k": 4}},
    {"left": {"t": 2, "k": 1}, "right": {"t": 2, "k": 2}, "button": {"t": 2, "k": 4}},
    {"left": {"t": 2, "k": 1}, "right": {"t": 2, "k": 2}, "button": {"t": 2, "k": 4}},
    {"left": {"t": 2, "k": 1}, "right": {"t": 2, "k": 2}, "button": {"t": 2, "k": 4}}
  ]
}
```

| Pole     | Opis                                   |
| :------- | :------------------------------------- |
| `left`   | Akcja przy obrocie w lewo (CCW)        |
| `right`  | Akcja przy obrocie w prawo (CW)        |
| `button` | Akcja przy kliknięciu gałki             |

Każda wartość to obiekt bindingu (taki sam jak w matrycy). Wspiera wszystkie typy: KEY, MEDIA, MACRO, LAYER, STRING.

### Działanie

- Obrót: detekcja przez przerwania sprzętowe (CHANGE na obu pinach), kwadraturowa maszyna stanów, dzielenie przez 4 na detent.
- Przycisk: detekcja zbocza opadającego (press), debounce 20ms blokada (lockout).

### Uwagi

- Jeśli akcja enkodera to `BIND_NONE` (typ 0), nic się nie dzieje (log warning).
- Szybkie kręcenie: system zachowuje resztę z dzielenia przez 4 pomiędzy wywołaniami `processEncoder()`, więc żaden krok nie ginie.

---

## 10. System warstw (layers)

### Podstawy

- 5 warstw (indeksy 0–4)
- Każda warstwa zawiera 16 klawiszy (4×4) + 3 akcje enkodera
- Warstwy są w pełni niezależne

### Zmiana warstwy

1. **Sprzętowe combo:** Jednoczesne przytrzymanie klawiszy (r0,c0) + (r0,c3) przełącza cyklicznie 0→1→2→3→4→0...
2. **Klawisz `type=4`:** Dowolny klawisz matrycy z bindingiem typu `LAYER` zmienia warstwę na wskazaną.
3. **Enkoder/makro `type=4`:** Zmiana warstwy przez enkoder lub krok makra.
4. **Komenda `set_layer`:** Zdalna zmiana warstwy z WebUI.

> **Ważne:** Zmiana warstwy przez combo lub `type=4` wywołuje `bleKeyboard.releaseAll()` — wszystkie przytrzymane klawisze BLE zostają zwolnione, aby uniknąć stuck keys.

### Zachowanie przy zmianie warstwy między press a release

Gdy klawisz zostanie naciśnięty na warstwie X, a zwolniony na warstwie Y (np. przez combo lub klawisz warstwy), system używa bindingu z **momentu naciśnięcia** (przechowywanego w `pressedBinding`). Dzięki temu akcja jest zawsze spójna.

---

## 11. Debugowanie i logi

ESP wysyła logi diagnostyczne w formacie:

```json
{"cmd":"log","level":"info","msg":"..."}
```

Poziomy: `info`, `warn`, `error`, `debug`.

### Kiedy są wysyłane?

- Błędy inicjalizacji
- Błędy parsowania JSON
- Ostrzeżenia (np. preemptowanie makra, pusta akcja enkodera)
- Informacje o stanie (setup complete, zmiana warstwy)
- Debug — stany przycisków enkodera (raw edges)

### Debug enkodera

Enkoder loguje każdy raw edge przycisku (`debug`) oraz każdy wykryty krok obrotu (`info`):

```
{"cmd":"log","level":"debug","msg":"Encoder btn raw edge: PRESS"}
{"cmd":"log","level":"info","msg":"Encoder step: dir=1, type=2"}
{"cmd":"log","level":"info","msg":"Encoder btn PRESS (type=2)"}
```

---

## 12. Zachowania specjalne i edge case'y

### 12.1 Binding `type=1` z `key=0` i `mods=0`

Gdy binding ma `type=1` ale `key=0` i `mods=0`, jest traktowany jako BIND_NONE i pomijany.

### 12.2 Binding `type=1` z `mods != 0` ale `key=0`

Np. `{"t": 1, "m": 1}` (samo Ctrl). Działa — modyfikatory są wysyłane bez klawisza głównego.

### 12.3 Makro z `keycode=0`

Krok makra z `keycode=0` nic nie wysyła, tylko czeka `delay` ms. Przydatne do pauz.

### 12.4 Makro z `text`

Krok makra z polem `text` wysyła string przez BLE. `keycode` jest ignorowany. Jeśli `text` jest pusty, krok jest pomijany.

### 12.5 Rekurencja makr

Makro może wywołać inne makro (krok z `type=3`). Ponieważ system preemptuje bieżące makro, drugie makro zastąpi pierwsze — pierwsze nie zostanie dokończone.

### 12.6 Konflikt combo — klawisze (r0,c0) i (r0,c3)

Gdy combo warstw jest aktywne, klawisze (r0,c0) i (r0,c3) **nie wykonują** swoich przypisanych akcji. Służą wyłącznie do zmiany warstwy w tym momencie. Po zwolnieniu działają normalnie.

### 12.7 ESP rozłączone BLE

Jeśli BLE nie jest połączone, akcje typu `KEY` i `MEDIA` są **cicho pomijane** (brak połączenia = brak wysyłki). Stringi i makra również wymagają połączenia BLE. Zmiana warstwy (`type=4`) działa zawsze, niezależnie od stanu BLE.

### 12.8 Inicjalizacja — kolejność

1. Serial (115200)
2. LittleFS i init config (z self-healing)
3. Init makr
4. Init matrycy
5. Init enkodera
6. BLE Keyboard begin
7. Init Serial Protocol
8. Send initial layer state

Po `"Setup complete."` urządzenie jest gotowe.

### 12.9 Wielkość dokumentu JSON

Konfiguracja dla 5 warstw + makra może być duża. Firmware używa `ArduinoJson 7` z `JsonDocument` (statyczny alokator). Domyślny rozmiar dokumentu jest wystarczający dla pełnej konfiguracji. Nie należy wysyłać payloadów znacznie przekraczających ~8KB.

### 12.10 `set_config` — kompletność danych

`set_config` zawsze nadpisuje całą konfigurację. Jeśli w `data` pominięto tablicę `macros`, ich stan w pamięci RAM jest zerowany. **Zaleca się zawsze wysyłać pełny config** (najpierw `get_config`, modyfikacja, potem `set_config`).

### 12.11 `set_config` — domyślne wartości enkodera

`processConfigJson` używa `parseKeyBinding()` dla pól enkodera. Jeśli w `data.encoder[l]` pominięto pole `left`, zostanie ono ustawione na `BIND_NONE` (a nie zachowane ze starej konfiguracji). **Zawsze wysyłaj pełne obiekty enkodera z wszystkimi trzema polami.**

### 12.12 Overflow `millis()` po ~50 dniach

System makr jest zabezpieczony przed overflowem licznika `millis()` (powrót do 0 po ~50 dniach). W przypadku overflow, aktualne opóźnienie zostaje natychmiast wykonane — może to przyspieszyć ostatnie kroki makra, ale nie powoduje zawieszenia ani utraty kontroli.

### 12.13 Szybkie podwójne kliknięcie enkodera

System debounce enkodera śledzi zbocza release podczas blokady (lockout) po pressie. Jeśli użytkownik szybko zwolni i ponownie naciśnie w czasie 20ms lockoutu, drugie naciśnięcie zostanie poprawnie wykryte po zakończeniu lockoutu.

### 12.14 Walidacja `set_macros`

`set_macros` wymaga poprawnego pola `data`:
- Gdy `data` jest tablicą — zawartość interpretowana jako makra.
- Gdy `data` jest obiektem — musi zawierać pole `macros` (tablica). Brak `macros` skutkuje `status: error` i konfiguracja pozostaje nietknięta.
- Gdy `data` ma inny typ (`null`, `string`, `number`) — `status: error`, brak zmian.

### 12.15 Długi string w bindingu

Jeśli string w `{"t": 5, "s": "..."}` przekracza 64 znaki, zostanie obcięty do 63 znaków + NULL terminator przy ładowaniu konfiguracji.

---

## Podsumowanie API

| Komenda                 | Request                                    | Response                            |
| :---------------------- | :----------------------------------------- | :---------------------------------- |
| `get_config`            | `{"cmd":"get_config"}`                     | `config` event                      |
| `set_config`            | `{"cmd":"set_config","data":{...}}`        | `status` event                      |
| `get_layer`             | `{"cmd":"get_layer"}`                      | `layer_change` event                |
| `set_layer`             | `{"cmd":"set_layer","layer":N}`            | `layer_change` + `status`           |
| `get_macros`            | `{"cmd":"get_macros"}`                     | `macros` event                      |
| `set_macros`            | `{"cmd":"set_macros","data":...}`          | `status` event                      |
| `run_macro`             | `{"cmd":"run_macro","index":N}`            | `status` + `macro_progress` series  |

| Event               | Format                                                                  | Trigger                          |
| :------------------ | :---------------------------------------------------------------------- | :------------------------------- |
| `config`            | `{"cmd":"config","data":{...}}`                                         | `get_config`                     |
| `status`            | `{"cmd":"status","status":"...","msg":"..."}`                           | komendy modyfikujące             |
| `key_state`         | `{"cmd":"key_state","row":R,"col":C,"isPressed":bool}`                  | zmiana stanu klawisza matrycy    |
| `layer_change`      | `{"cmd":"layer_change","layer":N}`                                      | zmiana warstwy (dowolna przyczyna)|
| `macros`            | `{"cmd":"macros","data":[...]}`                                         | `get_macros`                     |
| `macro_progress`    | `{"cmd":"macro_progress","index":N,"step":S,"total":T}`                 | wykonanie makra                  |
| `log`               | `{"cmd":"log","level":"...","msg":"..."}`                               | diagnostyka                      |
