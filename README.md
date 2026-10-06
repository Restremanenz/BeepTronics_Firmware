# BeepTronics Firmware

Firmware für das eigene Variometer-PCB mit **ESP32-S3-WROOM-1-N16**,
16 MB Quad-SPI-Flash, ohne PSRAM. PlatformIO + Arduino, C++11.

## Entscheidung: gezielt weiterentwickeln

Die bisherige Firmware wird modular weiterentwickelt. Bewahrt bleiben der eigene
MS5611-Treiber mit seiner Sensorkompensation, die Tonkurven, die Start-/Endmelodien
und das LK8EX1-Protokoll über den BLE-UART-Service. Neu getrennt sind Hardware,
Messverarbeitung, Audio, Akkumessung und BLE. Die Rechenlogik lässt sich am PC testen.

Der ursprüngliche Stand ist im ersten Git-Commit sowie im benachbarten Ordner
`BeepTronics_aktuelle_Software` erhalten. Künftige Entwicklung findet hier statt.

## Struktur

```text
BeepTronics_Firmware/
├── platformio.ini             Build-Varianten und feste Bibliotheksversionen
├── boards/
│   └── beeptronics_s3_n16.json Eigenes ESP32-S3-N16-Boardprofil
├── partitions/
│   └── 16mb.csv               Explizite Flash-Aufteilung
├── include/
│   ├── BoardPins.h            Tatsächliche GPIO-Zuordnung des PCBs
│   └── Config.h               Einstellwerte mit Einheiten
├── src/
│   ├── main.cpp               Nur Arduino-Einstiegspunkte
│   ├── app/Firmware.*         Initialisierung, Ablauf, Tasten, Demo
│   ├── core/                  Hardwareunabhängige Rechenlogik
│   │   ├── Measurement.h      Gemeinsame Messdaten und Einheiten
│   │   ├── VarioFilter.*      Druckhöhe, Filter und Steigrate
│   │   ├── ClimbTone.*        Hysterese und Ton-/Pausenzustand
│   │   └── Lk8ex1.*           Satzformat und Prüfsumme
│   ├── drivers/MS5611.*       SPI, Konvertierungen, Kalibrierung
│   └── services/
│       ├── AudioOutput.*      PWM und nicht blockierende Melodien
│       ├── BatteryMonitor.*   Akkumessung in kleinen Arbeitsschritten
│       └── BleTelemetry.*     BLE-Verbindung und Datenversand
├── test/host/                 C++-Regressionstests mit SPI-/Zeit-Simulation
├── scripts/test-host.ps1      Tests unter Windows mit g++
└── docs/HARDWARE.md           Pinbelegung und bekannte Besonderheiten
```

Neue gerätespezifische Treiber gehören nach `src/drivers`, neue reine Berechnungen
nach `src/core`. Zugehörige `.h` und `.cpp` bleiben nebeneinander. `include` enthält
nur projektweite Konfiguration. `lib/` ist derzeit ungenutzt; Drittbibliotheken
werden über PlatformIO verwaltet. `.pio`, `.test-build` und alte `src/build`-Reste
sind keine Quellen und werden nicht versioniert; `src/build` ist vom Build ausgeschlossen.

## Bauen

Im Projektordner mit PlatformIO CLI bzw. über die PlatformIO-Erweiterung in VS Code:

```powershell
pio run -e beeptronics
pio run -e beeptronics_simulation
```

`beeptronics` ist der Standard und verwendet echte Barometerdaten.
`beeptronics_simulation` erzeugt eine wiederholte Steigratenrampe von 0 bis 5 m/s,
benötigt keinen Drucksensor und heißt über BLE **BeepTronics SIMULATION**.
Die lila LED markiert die Demo. Audio und BLE verwenden denselben Messdatensatz.
Ein Taster kann die normale Firmware nicht versehentlich in die Simulation umschalten.

Toolchain-Basis: `espressif32 6.3.2` / Arduino-ESP32 `2.0.9`, FastLED `3.6.0`,
Bounce2 `2.72`, NimBLE-Arduino `1.4.1`. Diese Versionen sind bewusst festgehalten:
ein Wechsel auf neue Arduino-/NimBLE-Hauptversionen ist eine eigene Änderung.
Die Pins werden explizit gesetzt; die generischen Arduino-Variant-Pins sind nicht
die Pinbelegung dieses PCBs.

## Flash und USB

Eigenes Boardprofil statt Adafruit Feather: 240 MHz CPU, QIO-Flash mit 80 MHz,
16 MB Flashgröße, kein `BOARD_HAS_PSRAM`, kein TinyUF2-Abbild. USB verwendet die
native USB-Serial/JTAG-Schnittstelle des S3, CDC beim Booten aktiviert. Die Firmware
wartet nicht auf eine USB-Verbindung. Der serielle Starttext meldet erkannte
Flash- und PSRAM-Größe, sobald ein Monitor die Ausgabe empfangen kann.

| Bereich | Offset | Größe |
|---|---:|---:|
| NVS | 0x9000 | 20 KiB |
| OTA-Verwaltung | 0xE000 | 8 KiB |
| Firmware A | 0x10000 | 3 MiB |
| Firmware B | 0x310000 | 3 MiB |
| Reservierter Datenbereich | 0x610000 | 9,875 MiB |
| Coredump | 0xFF0000 | 64 KiB |

OTA und Dateisystem/Fluglogger sind noch nicht implementiert. Die Partitionen
reservieren dafür Platz. PlatformIO vergleicht die Programmgröße mit **3 MiB pro
Firmware-Slot**, obwohl das Modul insgesamt 16 MB besitzt. Die neue Partitionstabelle
ist nicht mit der alten Feather/TinyUF2-Aufteilung identisch. Vor einem späteren
Flashen eventuell vorhandene Geräteeinstellungen/Daten sichern; hier wurde nichts geflasht.

## Bedienung

Die Platine besitzt laut Schaltplan aktiv-HIGH-Taster mit externen Pulldowns.
Aktionen erfolgen wie zuvor beim Loslassen, jetzt ohne interne Pullups:

| Taster | Funktion |
|---|---|
| ON | Einschalten über Hardware; im Betrieb Akkumessung anfordern; in der Demo zusätzlich Simulation zurücksetzen |
| DOWN | Lautstärke um 5 reduzieren, Bestätigungston |
| UP | Lautstärke um 5 erhöhen, Bestätigungston |
| OK | Abschaltmelodie und Power Latch ausschalten |

Orange LED: Einlaufphase von 10 Sekunden. Danach bleibt sie im normalen Betrieb
aus. Rot: Sensorinitialisierung fehlgeschlagen oder nach der Einlaufphase keine
gültigen/frischen Messwerte. Lila: Simulationsfirmware.
Einstellungen werden noch nicht dauerhaft gespeichert. Die Anfangslautstärke ist 20.

## Verbesserungen dieser Basis

- Steigrate verwendet die tatsächliche Zeitdifferenz zwischen Messungen.
- IIR-Gewicht wird an diese Zeitdifferenz angepasst; der Mittelwert über 35 Werte bleibt.
- Filter wird bei ungültigen Messungen oder einer Lücke über 250 ms neu initialisiert.
- Keine BLE-Ausgabe vor gültigen Messwerten oder bei veralteten Daten.
- Druck konsequent in hPa intern, Pa im LK8EX1-Satz; Steigen in m/s intern, cm/s im Satz.
- Ton-/Pausenwechsel funktioniert auch innerhalb der Hysterese; Verstummen erfolgt sofort
  bei Unterschreiten der Ausschaltschwelle. Frequenz und Dauer sind begrenzt.
- Melodien und Tastentöne enthalten keine blockierenden `delay()`-Aufrufe.
- 400 ADC-Abfragen werden auf mehrere Schleifendurchläufe aufgeteilt, unabhängig vom Audio.
- MS5611: explizit initialisierter Zustand, eigene statt statisch geteilter Rohdaten,
  PROM-CRC-Prüfung, Ablehnung leerer/gesättigter ADC-Werte, kein dynamisch angelegtes SPI-Objekt.
- BLE nutzt den Verbindungszustand der Bibliothek; Sätze werden bei kleiner MTU in
  passende Fragmente zerlegt. Die empfangende App muss den UART-Datenstrom zusammensetzen.
- Bei Sensorfehlern bleibt die Bedienung verfügbar; die Firmware ruft kein `abort()` auf.

## Tests und nächste Schritte

Mit installiertem `g++`:

```powershell
./scripts/test-host.ps1
```

Die Tests prüfen konstantes Steigen bei mehreren Abtastraten, variable Messabstände,
Sinken, ungültige Daten, Wiederanlauf, Timerüberläufe, Ton-Hysterese, Protokolleinheiten,
Prüfsummen und den Sensortreiber mit einem simulierten SPI-Sensor sowie Datenblattwerten.
Das ist ein eigener Host-Testlauf, kein `pio test` und kein Ersatz für einen Gerätetest.

Prüfstand am 06.10.2026: beide PlatformIO-Varianten erfolgreich gebaut, 43 Host-
Prüfungen bestanden, Binär-Partitionstabelle sowie 16-MB-Kennung in Bootloader und
Firmware geprüft. Normaler Build: 533.805 Byte Programm und 31.284 Byte statischer
RAM-Bedarf; dynamische BLE-/RTOS-Speichernutzung kommt zur Laufzeit hinzu.
Die FastLED-Meldung „No hardware SPI pins defined“ bezieht sich auf FastLEDs
SPI-LED-Treiber; das MS5611 verwendet weiterhin die separate Hardware-SPI-Klasse.

Vor der Verwendung am Gerät sind insbesondere diese Änderungen praktisch zu prüfen:
USB/Boot, erkannte 16 MB, tatsächliche Tasterpegel, SPI-Messung, Tonverhalten,
Akkumesszeitfenster und BLE-Empfang einschließlich Wiederverbindung/MTU.
Die neue Filter-Zeitbasis braucht einen Vergleich mit aufgezeichneten echten Messwerten.

GPS, IMU, E-Paper, Sinkton, dauerhafte Einstellungen, Flugaufzeichnung und OTA
sind die nächsten getrennten Ausbauschritte. Die bisher nur definierten Sinkschwellen
wurden entfernt, damit die Konfiguration keine noch nicht vorhandene Funktion suggeriert.

Referenzen: [PlatformIO: eigene Boards](https://docs.platformio.org/en/stable/platforms/creating_board.html),
[Espressif: WROOM-1-Datenblatt](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf).
