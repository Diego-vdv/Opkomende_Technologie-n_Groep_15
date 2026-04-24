const int SOIL_PIN = A0;

void setup() {
  Serial.begin(115200);
  pinMode(SOIL_PIN, INPUT);

  Serial.println("Bodemvochtsensor test gestart");
  Serial.println("Droog = meestal hogere waarde");
  Serial.println("Nat = meestal lagere waarde");
  Serial.println();
}

void loop() {
  int soilValue = analogRead(SOIL_PIN);

  Serial.print("Bodemwaarde: ");
  Serial.println(soilValue);

  delay(1000);
}
// standaard waardes = 540
// met vinger test = 277

