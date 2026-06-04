# Plan Rozwoju (Upgrade Roadmap) — Makropad ESP32

Dokument opisuje plan rozszerzenia funkcjonalności firmware makropada w kierunku:
- Dynamicznych warstw z nazwami
- Zarządzania warstwami (dodawanie/usuwanie/edycja)
- Złożonych makr
- Wykonania skryptów i programów na PC przez Companion App
- Komunikacji z procesami zewnętrznymi

---

## Faza 0 — Stabilizacja (ukończona)

Naprawa wszystkich krytycznych i wysokopriorytetowych błędów:

| # | Problem | Plik | Status |
|---|---------|------|--------|
| 1 | `set_macros` z `data:null` kasował makra | `serial_protocol.cpp` | ✅ |
| 2 | `millis()` overflow po 50dniach rujnował makro | `macro_handler.cpp` | ✅ |
| 3 | `set_layer` z złym layer milczał | `serial_protocol.cpp` | ✅ |
| 4 | Szybkie podwójne kliknięcie enkodera gubiło drugie | `encoder.cpp` | ✅ |
| 5 | `sendKeycode(type=2)` cicho ignorował zły indeks | `keyboard_sender.cpp` | ✅ |
| 6 | `processConfigJson` nie sprawdzał czy `layers` istnieje | `config_manager.cpp` | ✅ |
| 7 | `getConfigJson` — trywialne, obsłużone | `config_manager.cpp` | ✅ |
| 8 | Stare stringi w RAM po reloadzie | `macro_handler.cpp` | ✅ |
| 9 | Brak ochrony przed długimi liniami Serial | `serial_protocol.cpp` | ✅ |

---

## Faza 1 — Nowy format konfiguracji + nazwy warstw

### Cel
Wprowadzenie wersjonowanego, rozszerzalnego formatu `config.json` z nazwami warstw.

### Zmiany w config.json (zrealizowane)

**Przed:**
```json
{
  "layers": [[...], ...],
  "encoder": [{...}, ...],
  "macros": [...],
  "strings": [...]
}
```

**Po (faza 0 — per-key stringi):**
```json
{
  "layers": [
    [
      [{"t": 1, "k": 95}, {"t": 5, "s": "Hello"}, ...],
      ...
    ]
  ],
  "encoder": [
    {"left": {"t": 2, "k": 1}, ...},
    ...
  ],
  "macros": [
    {"name": "...", "steps": [{"keycode": ..., "delay": ...}, {"text": "...", "delay": ...}]}
  ]
}
```

### Docelowo (Faza 1 — nazwy warstw):
```json
{
  "version": 2,
  "firmware": "2.0.0",
  "layerCount": 5,
  "layers": [
    {
      "id": 0,
      "name": "Numpad",
      "keys": [[{"t":1,"k":95}, ...], ...],
      "encoder": { "left": {"t":2,"k":1}, "right": {"t":2,"k":2}, "button": {"t":2,"k":4} }
    }
  ],
  "macros": [...],
  "commands": []
}
```

### Zmiany w kodzie

| Plik | Zmiana |
|------|--------|
| `include/config_manager.h` | `#define NUM_LAYERS` → `#define MAX_LAYERS 32`, dodaj `uint8_t layerCount`, dodaj strukturę `Layer` |
| `src/config_manager.cpp` | Migracja z v1 do v2 przy starcie, zapis/odczyt nowego formatu, obsługa nazw |
| `include/serial_protocol.h` | Nowe komendy: `get_layers`, `rename_layer` |
| `src/serial_protocol.cpp` | Obsługa nowych komend |

### Nowy typ danych

```cpp
#define MAX_LAYERS 32
#define MAX_LAYER_NAME 24

struct Layer {
    char name[MAX_LAYER_NAME];
    KeyBinding keys[NUM_ROWS][NUM_COLS];
    KeyBinding encoderLeft;
    KeyBinding encoderRight;
    KeyBinding encoderButton;
};

extern Layer layers[MAX_LAYERS];
extern uint8_t layerCount;
```

### Nowe komendy Serial

| Komenda | Payload | Odpowiedź | Opis |
|---------|---------|-----------|------|
| `get_layers` | `{}` | `layers_list` | Pobiera listę warstw (id + nazwa) |
| `rename_layer` | `{"index":N,"name":"..."}` | `status` | Zmienia nazwę warstwy |
| `add_layer` | `{"afterIndex":N,"name":"...","template":N}` | `status` + `layers_list` | Dodaje warstwę |
| `remove_layer` | `{"index":N}` | `status` + `layers_list` | Usuwa warstwę (min 1) |
| `move_layer` | `{"fromIndex":N,"toIndex":M}` | `status` + `layers_list` | Zmienia kolejność |

### Nowe eventy

| Event | Format | Opis |
|-------|--------|------|
| `layers_list` | `{"cmd":"layers_list","layers":[{"id":0,"name":"Numpad",...}],"active":0}` | Stan warstw |
| `layer_added` | `{"cmd":"layer_added","index":N,"name":"...","count":6}` | Potwierdzenie |
| `layer_removed` | `{"cmd":"layer_removed","index":N,"count":4}` | Potwierdzenie |
| `layer_renamed` | `{"cmd":"layer_renamed","index":N,"name":"Nowa nazwa"}` | Potwierdzenie |

---

## Faza 2 — Event Bus + routing kanałów

### Cel
Centralny system routingu eventów, przygotowujący na dwa kanały komunikacji (Kanał A: WebUI, Kanał B: Companion App).

### Nowy moduł

**Pliki:** `include/event_bus.h`, `src/event_bus.cpp`

```cpp
enum EventChannel : uint8_t {
    CHANNEL_A = 1,   // USB Serial — WebUI
    CHANNEL_B = 2,   // UART1/BLE — Companion App
    CHANNEL_ALL = 3
};

void emitEvent(const char* json, EventChannel target);
void emitEventf(EventChannel target, const char* fmt, ...);
```

### Routing

| Event | Kanał | Uzasadnienie |
|-------|-------|-------------|
| `key_state` | A | Tylko WebUI |
| `layer_change` | A + B | Oba konsumują |
| `macro_progress` | A + B | Oba konsumują |
| `config` | A | Tylko WebUI |
| `macros` | A | Tylko WebUI |
| `status` | A | Tylko WebUI |
| `command_started` | B | Tylko Companion |
| `command_output` | B | Tylko Companion |
| `command_complete` | B | Tylko Companion |
| `log` | A + B | Oba (debugowanie) |

### Zmiany w istniejącym kodzie

| Plik | Zmiana |
|------|--------|
| `src/serial_protocol.cpp` | `sendKeyState`, `sendLayerChange`, `sendMacroProgress` → przez `emitEvent` |
| `src/encoder.cpp` | Logi przez `emitEvent(CHANNEL_ALL, ...)` |
| `src/main.cpp` | `sendLayerChange` → przez `emitEvent` |

---

## Faza 3 — System komend + nowe typy makr

### Cel
Przechowywanie definicji komend (skryptów, executables) w konfiguracji, nowe typy kroków makr (`COMMAND`, `WAIT_CMD`).

### Nowy moduł

**Pliki:** `include/command_handler.h`, `src/command_handler.cpp`

```cpp
#define MAX_COMMANDS 32
#define MAX_CMD_NAME 32
#define MAX_CMD_PATH 256
#define MAX_CMD_ARGS 8
#define MAX_CMD_SCRIPT 512

enum CommandType : uint8_t {
    CMD_EXECUTABLE = 0,
    CMD_SCRIPT = 1,
    CMD_SHELL = 2
};

struct Command {
    char name[MAX_CMD_NAME];
    CommandType type;
    char path[MAX_CMD_PATH];
    char args[MAX_CMD_ARGS][64];
    uint8_t argCount;
    char script[MAX_CMD_SCRIPT];
    char interpreter[16];
    bool runInTerminal;
};
```

### Nowe typy kroków makra

| type | Nazwa | key= | Opis |
|------|-------|------|------|
| 5 | `COMMAND` | indeks komendy | Wykonaj komendę przez Companion App |
| 6 | `WAIT_CMD` | ignorowane | Czekaj na zakończenie bieżącej komendy |

### Nowe komendy Serial (Kanał A)

| Komenda | Payload | Opis |
|---------|---------|------|
| `get_commands` | `{}` | Pobiera listę komend |
| `set_commands` | `{"data":[...]}` | Zapisuje komendy do flash |

### Przykład złożonego makra

Makro może łączyć różne typy kroków: keycode, text, command:

```json
{
  "name": "Deploy Project",
  "steps": [
    {"text": "git pull", "delay": 100},  // wpisz tekst
    {"keycode": 0x0028, "delay": 500},   // Enter
    {"keycode": 0x0500, "delay": 0},     // COMMAND #0 (git pull)
    {"keycode": 0x0600, "delay": 30000}, // WAIT_CMD (timeout 30s)
    {"keycode": 0x0501, "delay": 0},     // COMMAND #1 (build)
    {"keycode": 0x0600, "delay": 120000},// WAIT_CMD (timeout 2min)
    {"keycode": 0x0004, "delay": 100}    // KEY A
  ]
}
```

---

## Faza 4 — Kanał B (UART1) + protokół Companion App

### Cel
Fizyczny kanał komunikacji między ESP32 a Companion App (Python/Node) przez drugi UART.

### Konfiguracja hardware

| Parametr | Wartość |
|----------|---------|
| UART | UART1 |
| TX pin | GPIO 4 |
| RX pin | GPIO 5 |
| Baud | 115200 |
| Buf. size | 512 bajtów |

### Nowy moduł

**Pliki:** `include/channel_b.h`, `src/channel_b.cpp`

```cpp
void initChannelB();
void processChannelB();
bool sendToCompanion(const char* json);
bool receiveFromCompanion(String& json);
```

### Kolejka komend (ring buffer)

```cpp
#define CMD_QUEUE_SIZE 8
struct CommandRequest {
    uint8_t commandId;
    unsigned long queuedAt;
};
```

### Protokół Companion App (Kanał B)

**Parowanie:**
```
ESP → Companion: {"cmd":"companion_hello","fw":"2.0.0","protocol":"cp_v1"}
Companion → ESP: {"cmd":"companion_ack","status":"ready","capabilities":["exec","shell","script"]}
```

**Wykonanie komendy:**
```
ESP → Companion: {"cmd":"exec_command","id":0}
Companion → ESP: {"cmd":"command_started","id":0,"pid":12345}
Companion → ESP: {"cmd":"command_output","id":0,"stream":"stdout","data":"line\n"}
Companion → ESP: {"cmd":"command_complete","id":0,"exitCode":0,"duration":4500}
```

**Keepalive:**
```
ESP → Companion: {"cmd":"ping"}
Companion → ESP: {"cmd":"pong"}
```

**Błędy:**
```
Companion → ESP: {"cmd":"command_failed","id":0,"error":"File not found"}
```

### Komendy Kanału B

| Komenda | Payload | Odpowiedź | Opis |
|---------|---------|-----------|------|
| `exec_command` | `{"id":N}` | `command_started` | Wykonaj zdefiniowaną komendę |
| `exec_raw` | `{"type":"shell","cmd":"...","interpreter":"powershell"}` | `command_started` | Wykonaj dowolne polecenie |
| `get_command_status` | `{}` | `command_status` | Status bieżącej komendy |
| `abort_command` | `{}` | `status` | Przerwij wykonywanie |

---

## Faza 5 — Walidacja + UX

### Cel
Rozszerzona walidacja danych wejściowych, szczegółowe kody błędów, rozszerzone logowanie.

### Kody błędów

| Kod | Znaczenie |
|-----|-----------|
| `ERR_INVALID_PARAM` | Niepoprawny parametr |
| `ERR_OUT_OF_RANGE` | Wartość poza zakresem |
| `ERR_MISSING_FIELD` | Brak wymaganego pola |
| `ERR_JSON_PARSE` | Błąd parsowania JSON |
| `ERR_SAVE_FAILED` | Błąd zapisu do flash |
| `ERR_MACRO_INDEX` | Nieprawidłowy indeks makra |
| `ERR_CMD_INDEX` | Nieprawidłowy indeks komendy |
| `ERR_LAYER_LIMIT` | Osiągnięto max liczbę warstw |
| `ERR_BUSY` | Zajęty (makro w trakcie) |

### Ujednolicony format błędu

```json
{
  "cmd": "status",
  "status": "error",
  "code": "ERR_OUT_OF_RANGE",
  "msg": "Layer index 7 out of range (max 4)",
  "cmd_source": "set_layer"
}
```

### Poziomy logowania

| Poziom | Użycie |
|--------|--------|
| `success` | Udana operacja konfiguracyjna |
| `info` | Informacja o stanie |
| `warn` | Ostrzeżenie (niekrytyczne) |
| `error` | Błąd operacji |
| `debug` | Szczegóły dla debugowania |
| `verbose` | Pełny dump danych (opcjonalne) |

### Funkcja walidacji `validateConfig()`

```cpp
bool validateConfig(JsonDocument& doc) {
    // Sprawdza:
    // - layers: istnieje, niepusta, każdy wiersz ma 4 kolumny
    // - encoder: istnieje, każde pole left/right/button poprawnym bindingiem
    // - macros: stepCount ≤ MAX_MACRO_STEPS
    // - commands: każde pole w zakresie
    // - brak cyklicznych referencji makr
    return true; // lub false + log
}
```

---

## Faza 6 — Companion App (osobny projekt)

### Cel
Aplikacja (Python/Node.js) łącząca się z ESP32 przez Kanał B, wykonująca skrypty i programy na PC.

### Architektura

```
┌────────────┐   Serial USB    ┌──────────────────┐
│   WebUI    │◄──────────────►│   ESP32 Firmware  │
│  (Browser) │  Web Serial API │  (Kanał A)        │
└────────────┘                 └────────┬─────────┘
                                        │ UART1 (GPIO4/5)
                                        ▼
                               ┌──────────────────┐
                               │  Companion App    │
                               │  (Python/Node.js) │
                               │                   │
                               │  ┌─────────────┐  │
                               │  │ .exe / .ps1  │  │
                               │  │ .py / .sh    │  │
                               │  │ terminal     │  │
                               │  └─────────────┘  │
                               └──────────────────┘
```

### Wymagania funkcjonalne

1. Połączenie z ESP32 przez UART (USB-UART konwerter) lub wirtualny COM port
2. Parsowanie protokołu `cp_v1`
3. Wykonanie komend:
   - `.exe` — bezpośrednie uruchomienie
   - `.py` — przez `python.exe`
   - `.ps1` — przez `powershell.exe`
   - `.sh` / `.bash` — przez `bash.exe` (WSL/MSYS2)
   - Dowolny terminal: `cmd.exe`, `powershell`, `bash`
4. Przekazywanie outputu (stdout/stderr) w czasie rzeczywistym
5. Zwracanie kodu wyjścia
6. Timeout execution
7. Możliwość przerwania (abort)
8. Keepalive (ping/pong)

### Rekomendowany stack

| Warstwa | Technologia |
|---------|-------------|
| Core | Python 3.10+ (asyncio) lub Node.js |
| Serial | `pyserial` / `serialport` |
| JSON | wbudowane |
| Procesy | `asyncio.subprocess` / `child_process` |
| Packaging | PyInstaller (standalone .exe) / pkg |

---

## Podsumowanie — zależności między fazami

```
Faza 0 (stabilizacja)
    │
    ▼
Faza 1 (config v2 + nazwy warstw) ◄── blokuje ──┐
    │                                              │
    ▼                                              │
Faza 2 (Event Bus)                                 │
    │                                              │
    ├──────────────────────────────────────────────┤
    ▼                                              │
Faza 3 (command_handler + typy 5/6)               │
    │                                              │
    ▼                                              │
Faza 4 (Kanał B + protokół cp_v1)                 │
    │                                              │
    ▼                                              │
Faza 5 (walidacja + UX) ──── zależne od ──────────┘
    │
    ▼
Faza 6 (Companion App — osobny projekt)
```

---

## Recommended workflow

1. **Zacznij od Fazy 1** — nowy config to fundament wszystkiego
2. **Równolegle Faza 2** — event bus, bo routing przyda się od razu
3. **Faza 3** — command handler (można testować bez Kanału B, emulując odpowiedzi przez Serial)
4. **Faza 4** — dopiero gdy command handler działa (hardware + protokół)
5. **Faza 5** — walidacja i UX (ciągłe ulepszanie, robić równolegle z 3-4)
6. **Faza 6** — osobny projekt, startować po Fazie 4

---

## Appendix A: Szacowany nakład pracy

| Faza | Pliki do zmiany | Nowe pliki | Szacowany czas |
|------|-----------------|------------|----------------|
| 0 | 6 | 0 | ✅ Ukończone |
| 1 | 4 | 0 | 2-3 dni |
| 2 | 4 | 2 | 1-2 dni |
| 3 | 3 | 2 | 2-3 dni |
| 4 | 2 | 2 | 2-3 dni |
| 5 | 3 | 0 | 1 dzień |
| 6 | 0 (osobny projekt) | nowy repo | 3-5 dni |

**Razem firmware:** ~8-12 dni developerskich
**Companion App:** ~3-5 dni (pierwsza wersja)
