# Opkomende Technologieën: Groep 15


#  Plant Health Monitor
Slimme monitoring van plantgezondheid met sensoren en real-time feedback.

##  Over het Project

Dit project werd ontwikkeld voor het vak *Opkomende Technologieën*.  
We bouwen een embedded systeem dat de gezondheid van een plant (Monstera) monitort met behulp van meerdere sensoren.

De gemeten data wordt:
- verwerkt via logica (thresholds)
- geïnterpreteerd (OK / te laag / te hoog)
- visueel weergegeven op een rond TFT-scherm

Het systeem geeft **direct advies** zoals:
- “Water geven”
- “Te koud”
- “Meer licht nodig”


##  Features

- Temperatuurmeting (DHT11)
- Luchtvochtigheid
- Bodemvochtigheid (% berekend uit analoge waarde)
- Lichtintensiteit (BH1750 lux sensor)
- Visuele feedback (happy/sad plant gezicht)
- Slim advies op basis van plantregels
- Real-time display op rond TFT-scherm

## Logica

De plant wordt geëvalueerd op basis van ideale waarden voor een **Monstera**:

| Parameter        | Ideaal bereik |
|----------------|--------------|
| Temperatuur     | 18 – 27 °C   |
| Luchtvochtigheid| 50 – 70 %    |
| Licht           | 2000 – 12000 lux |
| Bodemvocht      | 35 – 70 %    |

Het systeem bepaalt:
- OK → plant is gezond
- LOW → te weinig
- HIGH → te veel
- IGNORED → bv. nacht (licht < 150 lux)


## BOM

| Component        | Beschrijving                       | Link|
| ---------------- | ---------------------------------- |---|
| Arduino Nano | Microcontroller                    |[Arduino](https://store.arduino.cc/products/arduino-nano)|
| Bodemvochtsensor | Meet waterniveau in de grond       |[Amazon](https://www.amazon.nl/-/en/APKLVSR-Moisture-Capacitive-Hygrometer-Corrosion/dp/B0CQNF7S7L/ref=sr_1_4?s=garden&sr=1-4)|
| DHT11    | Temperatuur- en vochtigheidssensor |[Amazon](https://www.amazon.nl/AZDelivery-DHT11-Temperature-Compatible-Raspberry/dp/B07TXR5NQ6/ref=sxin_15_pa_sp_search_thematic_sspa?cv_ct_cx=feuchtigkeitssensor%2Barduino&s=garden&sbo=RZvfv%2F%2FHxDF%2BO5021pAnSA%3D%3D&sr=1-3-0bff13d2-7188-4a82-bb3a-ccd1e70f0167-spons&aref=gG6522PUsq&sp_csd=d2lkZ2V0TmFtZT1zcF9zZWFyY2hfdGhlbWF0aWM)|
| BH1750           | Digitale lichtsensor (lux)                       |[Amazon](https://www.amazon.nl/-/en/AZDelivery-Brightness-Compatible-Arduino-Raspberry/dp/B07TKWNGZ4/ref=sr_1_9?sr=8-9)|
| GC9A01 TFT         | Rond kleurenscherm                 |[Amazon](https://www.amazon.nl/-/en/Resolution-Interface-Real-time-Monitoring-Instrument/dp/B0DB5MR27T/ref=sr_1_11?sr=8-11)|


## Software

- Arduino IDE  
- Libraries:
  - Adafruit_GFX
  - Adafruit_GC9A01A
  - DHT

## Architectuur

Het project is opgesplitst in 3 delen:

### 1. Sensors
- Leest alle hardware uit
- Bevat:
  - DHT uitlezing (met caching)
  - Bodemmeting (gemiddelde filtering)
  - BH1750 via software I2C

### 2. Logic
- Verwerkt data
- Bepaalt status (OK / LOW / HIGH)
- Genereert advies

### 3. Display
- Visualisatie op TFT
- Toont:
  - Waarden
  - Advies
  - Smiley (happy/sad)
 

<p align=center>  
  <img src="fotos/Opkomende Technologieen - Aeon en Diego - Frame 12.jpg") alt=testlocatie width=75% />
</p> 

## Werking

1. Sensoren meten waarden
2. Data wordt gefilterd en omgerekend
3. Logica bepaalt plantstatus
4. Display toont:
   - cijfers
   - advies
   - emotionele feedback (face)
5. Serial monitor geeft debug info

## Prototype

*(Voeg hier foto's toe van setup)*

## Toekomstige uitbreidingen

* Mobile app (Bluetooth/WiFi)
* Slimme aanbevelingen
* Automatisch water geven

## Team

* Aeon Bonjé
* Diego Vande Vyvere





