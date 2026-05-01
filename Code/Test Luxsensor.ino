// PIN DEFINITIES
const uint8_t PIN_SDA = A1;
const uint8_t PIN_SCL = A2;

// BH1750 I2C adres
const uint8_t BH1750_ADDRESS = 0x23;

// I2C TIMING
void i2cDelay() {
  delayMicroseconds(10);
}

// LIJN CONTROLE
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

// I2C BASIS
void i2cStart() {
  sdaRelease(); sclRelease(); i2cDelay();
  sdaLow(); i2cDelay();
  sclLow();
}

void i2cStop() {
  sdaLow(); i2cDelay();
  sclRelease(); i2cDelay();
  sdaRelease(); i2cDelay();
}

// BYTE WRITE 
bool i2cWriteByte(uint8_t value) {
  for (uint8_t bit = 0; bit < 8; bit++) {
     if (data & 0x80) {
      sdaRelease();   // HIGH
    } else {
      sdaLow();       // LOW
    }

    i2cDelay(); sclRelease();     // clock HIGH
    i2cDelay(); sclLow();         // clock LOW
    i2cDelay();

    data <<= 1;
  }
  
  // ACK fase
  sdaRelease(); i2cDelay();
  sclRelease(); i2cDelay();
  bool ack = !readLine(PIN_SDA); // LOW = ACK
  sclLow(); i2cDelay();
  return ack;
}

// BYTE READ 
uint8_t i2cReadByte(bool sendAck) {
  uint8_t value = 0;
  sdaRelease();

  for (uint8_t i = 0; i < 8; i++) {
    data <<= 1;
    sclRelease(); i2cDelay();

    if (readLine(PIN_SDA)) {
      data |= 1;
    }
    sclLow(); i2cDelay();
  }
  
  // ACK/NACK
  if (sendAck) {
    sdaLow();
  } else {
    sdaRelease();
  }

  i2cDelay(); sclRelease();i2cDelay();
  sclLow(); i2cDelay();
  sdaRelease();

  return data;
}

// BH1750
void bh1750Start() {
  i2cStart();
  i2cWriteByte((BH1750_ADDR << 1) | 0);  // write mode
  i2cWriteByte(0x10);  // continuous high-res mode
  i2cStop();
}

float bh1750ReadLux() {
  i2cStart();
  i2cWriteByte((BH1750_ADDR << 1) | 1); // read mode
  const uint8_t msb = i2cReadByte(true);
  const uint8_t lsb = i2cReadByte(false);
  i2cStop();

  const uint16_t raw = (static_cast<uint16_t>(msb) << 8) | lsb;
  return raw / 1.2f;
}

// SETUP & LOOP
void setup() {
  Serial.begin(9600);
  sdaRelease();
  sclRelease();
  delay(200);
  bh1750Start();
}

void loop() {
  float lux = bh1750ReadLux();
  Serial.println(lux);

  delay(1000);
}





