#include <DHT.h>

const int DHT_PIN = A3;
//moet aangesloten worden op 5V werkt lik niet op 3.3V
const int DHT_TYPE = DHT11;

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.println("DHT test gestart");
  Serial.println("Temperatuur en luchtvochtigheid:");
  Serial.println();
}

void loop() {
  float humidity = dht.readHumidity();
  float temperatureC = dht.readTemperature();

  if (isnan(humidity) || isnan(temperatureC)) {
    Serial.println("Fout: geen geldige DHT meting");
    delay(2000);
    return;
  }

  Serial.print("Temperatuur: ");
  Serial.print(temperatureC, 1);
  Serial.println(" C");

  Serial.print("Luchtvochtigheid: ");
  Serial.print(humidity, 1);
  Serial.println(" %");

  Serial.println();
  delay(2000);
}
