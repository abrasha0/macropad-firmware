# Wytyczne Projektowe: Makropad ESP32

Dokument opisuje wyłącznie zbiór wymagań i funkcjonalności dla oprogramowania układowego (firmware) makropada w wersji MVP. Kwestie sprzętowe, pinout i architektura opisane są w głównym pliku `README.md`.

## 1. Wymagania Funkcjonalne

1. **Skanowanie Matrycy Klawiszy (4x4)**
   - Algorytm nieblokujący.
   - Obsługa programowego debouncingu dla fizycznych przełączników (optymalnie 10-20ms).
   - Wsparcie dla jednoczesnego wciśnięcia wielu klawiszy (NKRO lub optymalny rollover dla BLE).

2. **Obsługa Klawiatury Bluetooth (BLE HID)**
   - Rozgłaszanie urządzenia jako standardowa klawiatura Bluetooth.
   - Wysyłanie kodów pojedynczych przycisków, przycisków multimedialnych oraz kombinacji klawiszy z modyfikatorami (np. Ctrl/Shift/Alt).
   - Wysyłanie zdefiniowanych sekwencji (makr).

3. **Obsługa Enkodera Obrotowego (EC11)**
   - Płynny odczyt zdarzeń kręcenia (lewo/prawo) bez gubienia kroków – wymagane użycie sprzętowych przerwań (Hardware Interrupts).
   - Obsługa kliknięcia enkodera (przycisk w osi).

4. **Zarządzanie Pamięcią i Konfiguracją (LittleFS)**
   - Struktura pamięci musi opierać się o odczyt/zapis pliku `config.json` w systemie plików `LittleFS`.
   - **Self-healing:** Przy starcie (`setup`), jeśli plik konfiguracyjny nie istnieje (lub uległ uszkodzeniu), firmware ma automatycznie wygenerować domyślny plik JSON obsługujący 5 warstw (profili) i zapisać go we flashu mikrokontrolera.

5. **Profile Klawiszy (Warstwy)**
   - System musi obsługiwać 5 całkowicie niezależnych, personalizowanych profili makropada.
   - Możliwość natychmiastowego przełączania między profilami.
   - Zmiana profilu wpływa na zaprogramowane akcje dla guzików matrycy i enkodera.

6. **Komunikacja Serial (API Web Serial) i Live Feedback**
   - Dwukierunkowa, bezprzewodowa komunikacja z desktopową lub webową aplikacją konfiguracyjną poprzez port szeregowy po kablu USB (115200 baud).
   - Komunikacja oparta w 100% na formacie JSON.
   - System ma reagować na komendy pobrania konfiguracji (`get_config`) i jej nadpisania w pamięci flash (`set_config`).
   - Tryb **Live Feedback**: Wciśnięcie klawisza na matrycy natychmiastowo wysyła raport po Serial z informacją o rzędzie i kolumnie aktywowanego przycisku na potrzeby wizualizacji w aplikacji Web.

## 2. Ograniczenia i Założenia Niefunkcjonalne (Minimalizm)

- **Brak stosu sieciowego:** Całkowite odcięcie serwera Wi-Fi, AsyncWebServer oraz OTA bezpośrednio na urządzeniu z uwagi na przesiadkę na Web Serial (port USB). Mikrokontroler ma pozostać "chudy" i odciążony z procesów sieciowych.
- **Wydajność:** System asynchroniczny, główna pętla `loop()` pozbawiona blokujących opóźnień (`delay()`). Skaner matrycy ani komunikacja nie mogą się zawieszać.
- **Optymalizacja Pamięci (JSON):** Ze względu na odbieranie i parsowanie dużych zrzutów konfiguracyjnych dla wszystkich 5 warstw, należy rygorystycznie optymalizować zasoby. Odpowiedni dobór rozmiaru sterty (np. użycie `JsonDocument` w ArduinoJson 7 lub `DynamicJsonDocument` we wcześniejszych wersjach) jest wymagany, by uniknąć wyrzucania wyjątków (Heap OOM) ze strony `ArduinoJson`.
