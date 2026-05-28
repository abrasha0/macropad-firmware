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
| **Enkoder**   | Przycisk (SW)     | GPIO 13   | `INPUT_PULLUP` | -                                    |
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

### Protokół Komunikacji Serial (JSON)

Każda komenda to prawidłowy obiekt JSON zakończony znakiem nowej linii `\n`.

- _Pobieranie configu:_ `{"cmd": "get_config"}`
- _Zapis configu:_ `{"cmd": "set_config", "data": { ... }}`
- _Pobranie aktualnej warstwy:_ `{"cmd": "get_layer"}`
- _Wymuszenie warstwy:_ `{"cmd": "set_layer", "layer": 2}`
- _Live Feedback (ESP -> Web):_ `{"cmd": "key_state", "row": 0, "col": 2, "isPressed": true}` — wysyłany przy każdej zmianie stanu (naciśnięcie/zwolnienie)

### Struktura pliku `config.json`

Plik konfiguracyjny zawiera tablicę `layers` (5 warstw po 4×4 klawiszy, każdy jako 16-bitowa wartość) oraz tablicę `encoder` z konfiguracją enkodera dla każdej warstwy:

```json
{
  "layers": [
    [[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0]],
    ...
  ],
  "encoder": [
    {"left": 4097, "right": 4098, "button": 4100},
    ...
  ]
}
```

Format kodowania 16-bitowej wartości klawisza:
- **bity 15-12 (type):** `0` = standardowy klawisz, `1` = multimedialny, `3` = zmiana warstwy
- **bity 11-8 (mods):** bit0=Ctrl, bit1=Shift, bit2=Alt, bit3=GUI
- **bity 7-0 (key):** kod klawisza HID lub media

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

## 4. Profile Domyślne (Struktura Konfiguracji)

Urządzenie posiada 5 profili. W pamięci są one reprezentowane jako wielowymiarowa tablica. W obecnej wersji firmware generuje domyślne wartości zastępcze — docelowe profile (Numpad, NFS Underground 2, CS:GO, Custom 1, Custom 2) zostaną zaimplementowane w przyszłej wersji na podstawie wytycznych w `docs/wytyczne.md`.

1.  **Numpad:** Klasyczna klawiatura numeryczna. (do implementacji)
2.  **NFS Underground 2:** Klawisze sterowania (Strzałki, Shift/Ctrl do biegów, Spacja ręczny, N nitro). (do implementacji)
3.  **CS:GO / CS2:** Sekcja zakupów, wybór granatów (Z, X, C, V), broń (1, 2, 3), drop (G). (do implementacji)
4.  **Custom 1:** Pusta templatka.
5.  **Custom 2:** Pusta templatka.

---
