#include <EEPROM.h>

const int RF_PIN = 2;

#define LED1 13
#define LED2 12
#define LED3 11
#define LED4 10

// EEPROM addresses (4 buttons)
#define ADDR1 0
#define ADDR2 4
#define ADDR3 8
#define ADDR4 12

volatile unsigned long lastTime = 0;
volatile unsigned int pulse = 0;

unsigned long code = 0;
int bitCount = 0;

bool state1 = false;
bool state2 = false;
bool state3 = false;
bool state4 = false;

bool learnMode = false;   // 👉 learning ON/OFF

unsigned long lastSignalTime = 0;

// timing
#define SHORT_MIN 200
#define SHORT_MAX 700

#define LONG_MIN 800
#define LONG_MAX 2000

#define SYNC_MIN 9000
#define SYNC_MAX 14000

// ---------- EEPROM HELPERS ----------
unsigned long readCode(int addr) {
  unsigned long val = 0;
  EEPROM.get(addr, val);
  return val;
}

void writeCode(int addr, unsigned long val) {
  EEPROM.put(addr, val);
}

// ---------- INTERRUPT ----------
void handleInterrupt() {

  unsigned long time = micros();
  pulse = time - lastTime;
  lastTime = time;

  lastSignalTime = millis();

  if (pulse < 150) return;

  if (pulse > 5000) {
    bitCount = 0;
    code = 0;
    return;
  }

  if (pulse > SYNC_MIN && pulse < SYNC_MAX) {
    bitCount = 0;
    code = 0;
    return;
  }

  static int pair = 0;
  static unsigned int first = 0;

  if (pair == 0) {
    first = pulse;
    pair = 1;
    return;
  }

  if (pair == 1) {

    if (first > SHORT_MIN && first < SHORT_MAX &&
        pulse > LONG_MIN && pulse < LONG_MAX) {
      code <<= 1;
      bitCount++;
    }
    else if (first > LONG_MIN && first < LONG_MAX &&
             pulse > SHORT_MIN && pulse < SHORT_MAX) {
      code = (code << 1) | 1;
      bitCount++;
    }

    pair = 0;
  }

  // ---------- FRAME COMPLETE ----------
  if (bitCount >= 24) {

    if (code == 0) {
      bitCount = 0;
      code = 0;
      return;
    }

    Serial.print("RX CODE: ");
    Serial.println(code);

    // ===== LEARN MODE =====
    if (learnMode) {

      if (EEPROM.read(20) == 1) {
        writeCode(ADDR1, code);
        Serial.println("BTN1 SAVED");
      }
      else if (EEPROM.read(20) == 2) {
        writeCode(ADDR2, code);
        Serial.println("BTN2 SAVED");
      }
      else if (EEPROM.read(20) == 3) {
        writeCode(ADDR3, code);
        Serial.println("BTN3 SAVED");
      }
      else if (EEPROM.read(20) == 4) {
        writeCode(ADDR4, code);
        Serial.println("BTN4 SAVED");
      }

      learnMode = false;
      bitCount = 0;
      code = 0;
      return;
    }

    // ===== NORMAL MODE =====
    if (code == readCode(ADDR1)) {
      state1 = !state1;
      digitalWrite(LED1, state1);
      Serial.println("LED1 TOGGLE");
    }

    else if (code == readCode(ADDR2)) {
      state2 = !state2;
      digitalWrite(LED2, state2);
      Serial.println("LED2 TOGGLE");
    }

    else if (code == readCode(ADDR3)) {
      state3 = !state3;
      digitalWrite(LED3, state3);
      Serial.println("LED3 TOGGLE");
    }

    else if (code == readCode(ADDR4)) {
      state4 = !state4;
      digitalWrite(LED4, state4);
      Serial.println("LED4 TOGGLE");
    }

    bitCount = 0;
    code = 0;
  }
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(9600);

  pinMode(RF_PIN, INPUT);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(RF_PIN), handleInterrupt, CHANGE);

  Serial.println("SYSTEM READY");
  Serial.println("Send 1-4 in Serial to learn button");
}

// ---------- LOOP ----------
void loop() {

  // safety reset
  if (millis() - lastSignalTime > 1000) {
    code = 0;
    bitCount = 0;
  }

  // 👉 SERIAL LEARN CONTROL
  if (Serial.available()) {
    char c = Serial.read();

    if (c == '772660') {
      EEPROM.write(20, 1);
      learnMode = true;
      Serial.println("LEARN BTN1");
    }
    if (c == '772664') {
      EEPROM.write(20, 2);
      learnMode = true;
      Serial.println("LEARN BTN2");
    }
    if (c == '772658') {
      EEPROM.write(20, 3);
      learnMode = true;
      Serial.println("LEARN BTN3");
    }
    if (c == '772657') {
      EEPROM.write(20, 4);
      learnMode = true;
      Serial.println("LEARN BTN4");
    }
  }
}
