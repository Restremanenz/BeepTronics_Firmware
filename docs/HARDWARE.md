# Hardwarezuordnung

Grundlage: `Schmatic.pdf` (Vario V2, Januar 2023) und
`Hoelzl_Franz_Variometer_BeepTronics.pdf` im übergeordneten Vario-Ordner.
Diese Zuordnung muss bei abweichender Bestückung/PCB-Revision angepasst werden.
Maßgebliche Softwaredefinition: `include/BoardPins.h`.

| Funktion | GPIO | Bemerkung |
|---|---:|---|
| Power Latch | 45 | HIGH hält die Hauptversorgung eingeschaltet |
| GPS-/IMU-Versorgung | 7 | HIGH schaltet den gemeinsamen LDO ein |
| Schallgeber | 3 | PWM über Transistor |
| WS2812B | 4 | Status-LED |
| I2C SDA / SCL | 6 / 5 | GPS und MPU-9250, aktuell nicht gestartet |
| ON / DOWN / UP / OK | 17 / 40 / 41 / 42 | Aktiv HIGH, externe Pulldowns |
| Akku-ADC / Messfreigabe | 1 / 2 | Teiler 250 kΩ + 250 kΩ, kapazitive Gate-Ansteuerung |
| MS5611 CS / SCK | 14 / 11 | SPI Mode 0, 1 MHz |
| MS5611 MOSI / MISO | 12 / 13 | Richtungen vom ESP32 aus gesehen |
| Display CS / Reset / Busy | 10 / 8 / 18 | Display momentan nicht initialisiert |
| Display Daten / D/C | 12 / 13 | Sonderfall, siehe unten |
| GPS Reset / EXT_INT | 47 / 21 | EXT_INT ist ein Eingang am GPS |
| IMU Interrupt | 48 | Der generische Arduino-LED-Pin 48 darf hier nicht benutzt werden |
| USB D- / D+ | 19 / 20 | Native USB-Schnittstelle |

## Besonderheiten, die bei Erweiterungen erhalten bleiben müssen

Die alten Namen `MISO=12` und `MOSI=13` waren aus Controllersicht vertauscht.
Der alte Treiber hat das intern kompensiert. Jetzt sind sowohl Namen als auch
Konstruktorargumente eindeutig: `MS5611(cs, miso, mosi, clock)`.
Die physische Belegung bleibt unverändert.

GPIO13 ist zugleich der Barometer-Datenausgang und die D/C-Leitung des Displays.
Eine spätere Displayintegration benötigt kontrolliertes Umschalten der Pinrichtung
und der Busnutzung; zwei gewöhnliche SPI-Treiber parallel zu starten genügt nicht.
Display-CS wird bereits beim Start auf HIGH gelegt.

`sensorPower` steuert GPS **und** IMU. LOW ist im aktuellen Stand beabsichtigt,
da beide noch nicht verwendet werden. Der I2C-Bus wird deshalb nicht initialisiert.
Die GPS-Backupversorgung ist davon getrennt.

Die Akkumessung übernimmt den bisherigen Faktor 2 und die grobe lineare Zuordnung
3,50–3,95 V zu 0–100 %. Sie ist keine kalibrierte Ladezustandsmessung. Durch den
Kondensator am MOSFET-Gate steht nur ein begrenztes Messfenster zur Verfügung:
2 ms Einschwingen, 400 Messungen in Blöcken à 16, Abbruch nach 100 ms.
Am PCB prüfen, ob Spannung und Zeitfenster unter Last passen.

Das Programm verwendet für die Höhe weiterhin 1013,25 hPa als Referenz.
Die vom MS5611 gemessene Temperatur dient der Sensorkompensation und ist keine
zuverlässige Messung der Außenlufttemperatur. Ein QNH-/Höhenabgleich fehlt noch.

Das dokumentierte Modul N16 besitzt 16 MB Flash und kein PSRAM. Der tatsächliche
Modultyp ist vor dem Flashen mit Aufdruck bzw. Geräteabfrage abzugleichen.
Die Modulbezeichnung allein bestätigt weder die aktuelle Bestückung noch
die Funktionsfähigkeit der optionalen Sensoren.
