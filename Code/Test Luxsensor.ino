const uint8_t SDA_PIN = A1;
const uint8_t SCL_PIN = A2;
const uint8_t BH1750_ADDR = 0x23;

void i2cDelay() {
  delayMicroseconds(10);
}

void sdaLow() {
  pinMode(SDA_PIN, OUTPUT);
  digitalWrite(SDA_PIN, LOW);
}

void sdaRelease() {
  pinMode(SDA_PIN, INPUT_PULLUP);
}

void sclLow() {
  pinMode(SCL_PIN, OUTPUT);
  digitalWrite(SCL_PIN, LOW);
}

void sclRelease() {
  pinMode(SCL_PIN, INPUT_PULLUP);
}

bool lineHigh(uint8_t pin) {
  return digitalRead(pin) == HIGH;
}

void i2cStart() {
  sdaRelease();
  sclRelease();
  i2cDelay();
  sdaLow();
  i2cDelay();
  sclLow();
}

void i2cStop() {
  sdaLow();
  i2cDelay();
  sclRelease();
  i2cDelay();
  sdaRelease();
  i2cDelay();
}

bool i2cWriteByte(uint8_t value) {
  for (uint8_t bit = 0; bit < 8; bit++) {
    if (value & 0x80) {
      sdaRelease();
    } else {
      sdaLow();
    }

    i2cDelay();
    sclRelease();
    i2cDelay();
    sclLow();
    i2cDelay();
    value <<= 1;
  }

  sdaRelease();
  i2cDelay();
  sclRelease();
  i2cDelay();
  const bool ack = !lineHigh(SDA_PIN);
  sclLow();
  i2cDelay();
  return ack;
}

uint8_t i2cReadByte(bool sendAck) {
  uint8_t value = 0;
  sdaRelease();

  for (uint8_t bit = 0; bit < 8; bit++) {
    value <<= 1;
    sclRelease();
    i2cDelay();
    if (lineHigh(SDA_PIN)) {
      value |= 1;
    }
    sclLow();
    i2cDelay();
  }

  if (sendAck) {
    sdaLow();
  } else {
    sdaRelease();
  }

  i2cDelay();
  sclRelease();
  i2cDelay();
  sclLow();
  i2cDelay();
  sdaRelease();

  return value;
}

void bh1750Start() {
  i2cStart();
  i2cWriteByte((BH1750_ADDR << 1) | 0);
  i2cWriteByte(0x10);
  i2cStop();
}

float bh1750ReadLux() {
  i2cStart();
  i2cWriteByte((BH1750_ADDR << 1) | 1);
  const uint8_t msb = i2cReadByte(true);
  const uint8_t lsb = i2cReadByte(false);
  i2cStop();

  const uint16_t raw = (static_cast<uint16_t>(msb) << 8) | lsb;
  return raw / 1.2f;
}

void setup() {
  Serial.begin(9600);
  sdaRelease();
  sclRelease();
  delay(200);
  bh1750Start();
}

void loop() {
  Serial.println(bh1750ReadLux());
  delay(1000);
}
