# Opkomende Technologieën: Groep 15


#  Plant Health Monitor
Slimme monitoring van plantgezondheid met sensoren en real-time feedback.

##  Over het Project

Dit project wordt ontwikkeld voor Opkomende Technologieën.
We bouwen een systeem dat de gezondheid van een plant monitort aan de hand van verschillende sensoren en deze informatie duidelijk weergeeft op een scherm.

Het doel is om gebruikers te helpen hun planten beter te verzorgen door direct inzicht te geven in wat de plant nodig heeft.

##  Features

* Bodemvochtigheid meten
* Temperatuur monitoren
* Lichtintensiteit detecteren
* Live data op display (LCD/OLED)
* Waarschuwingen bij slechte condities

## BOM

| Component        | Beschrijving                       | Link|
| ---------------- | ---------------------------------- |---|
| Arduino Nano | Microcontroller                    |[Arduino](https://store.arduino.cc/products/arduino-nano)|
| Bodemvochtsensor | Meet waterniveau in de grond       |[Amazon](https://www.amazon.nl/-/en/APKLVSR-Moisture-Capacitive-Hygrometer-Corrosion/dp/B0CQNF7S7L/ref=sr_1_4?s=garden&sr=1-4)|
| DHT11    | Temperatuur- en vochtigheidssensor |[Amazon](https://www.amazon.nl/AZDelivery-DHT11-Temperature-Compatible-Raspberry/dp/B07TXR5NQ6/ref=sxin_15_pa_sp_search_thematic_sspa?cv_ct_cx=feuchtigkeitssensor%2Barduino&s=garden&sbo=RZvfv%2F%2FHxDF%2BO5021pAnSA%3D%3D&sr=1-3-0bff13d2-7188-4a82-bb3a-ccd1e70f0167-spons&aref=gG6522PUsq&sp_csd=d2lkZ2V0TmFtZT1zcF9zZWFyY2hfdGhlbWF0aWM)|
| LDR              | Lichtsensor                        |[Amazon](https://www.amazon.nl/-/en/AZDelivery-Brightness-Compatible-Arduino-Raspberry/dp/B07TKWNGZ4/ref=sr_1_9?sr=8-9)|
| LCD/OLED         | Display voor output                |[Amazon](https://www.amazon.nl/-/en/Resolution-Interface-Real-time-Monitoring-Instrument/dp/B0DB5MR27T/ref=sr_1_11?sr=8-11)|


## Software

* Arduino IDE

## Architectuur

<p align=center>  
  <img src="fotos/Opkomende Technologieen - Aeon en Diego - Frame 12.jpg") alt=testlocatie width=75% />
</p> 
 

## Prototype

*(Voeg hier foto's toe van setup)*

## Werking

1. Sensoren verzamelen data
2. Arduino leest en verwerkt deze data
3. Resultaten worden weergegeven op het scherm
4. Gebruiker krijgt feedback (bv. “Plant heeft water nodig”)

## Toekomstige uitbreidingen

* Mobile app (Bluetooth/WiFi)
* Slimme aanbevelingen
* Automatisch water geven

## Team

* Aeon Bonjé
* Diego Vande Vyvere





