#include <DHT.h>

// Config
const int DHT_PIN = A3; // pin op arduino 
// best aangesluiten aan 5V, werkt niet goed met 3.3V
const int DHT_TYPE = DHT11; // type sensor
const unsigned long MEET_INTERVAL = 2000; // Tijd tussen metingen (ms)

// Initialiseer DHT sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Setup
void setup() {
  Serial.begin(115200);
  dht.begin();
  
  Serial.println("DHT test gestart");
  Serial.println("Temperatuur en luchtvochtigheid:");
  Serial.println();
}

void loop() {
  // Metingen uitvoeren
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // in graden Celsius
  // Controleer of meting gelukt is
  if (isnan(humidity) || isnan(temperatureC)) {
    Serial.println("Fout: geen geldige DHT meting");
    wachtEnHerhaal();
    return;
  }
  // Resultaten weergeven
  Serial.print("Temperatuur: "); Serial.print(temperatureC, 1); Serial.println(" C");
  Serial.print("Luchtvochtigheid: "); Serial.print(humidity, 1); Serial.println(" %");
  
  Serial.println();
  // Wacht voor volgende meting
  wachtEnHerhaal();
}

// Zorgt voor consistente wachttijd
void wachtEnHerhaal() {
  delay(MEET_INTERVAL);
}
