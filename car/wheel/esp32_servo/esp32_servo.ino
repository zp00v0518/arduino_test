#include <Arduino.h>

const int stepPin = 12;
const int dirPin = 13;
const int alarmPin = 14; 
const int ledPin = 2;

long STEPS_PER_REV = 1600; 
const float TOTAL_TURNS = 4.0;
long maxSteps = (STEPS_PER_REV * TOTAL_TURNS) / 2;

int targetInput = 127;
long currentPos = 0;
int lastDir = -1;
unsigned long lastTelemetry = 0;
unsigned long lastPacketTime = 0;

// Логіка станів
bool pilotConnected = false; 
bool systemVerified = false; 
bool checkDone = false; // "Замок" для одноразової перевірки

void runMechanicalCheck() {
  Serial.println("SYSTEM: STARTING PRE-FLIGHT CHECK...");
  
  if (digitalRead(alarmPin) == LOW) { 
    systemVerified = false;
    return;
  }

  // Тестовий рух вліво
  digitalWrite(dirPin, LOW);
  for(int i = 0; i < 200; i++) {
    digitalWrite(stepPin, LOW); delayMicroseconds(10);
    digitalWrite(stepPin, HIGH); delayMicroseconds(1000);
    if(digitalRead(alarmPin) == LOW) { systemVerified = false; return; }
  }
  delay(150);
  
  // Тестовий рух вправо
  digitalWrite(dirPin, HIGH);
  for(int i = 0; i < 200; i++) {
    digitalWrite(stepPin, LOW); delayMicroseconds(10);
    digitalWrite(stepPin, HIGH); delayMicroseconds(1000);
  }
  
  systemVerified = true;
  checkDone = true; // Блокуємо повторний тест
  Serial.println("SYSTEM: CHECK PASSED.");
}

void setup() {
  Serial.begin(115200);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(alarmPin, INPUT); 
  pinMode(ledPin, OUTPUT);
  digitalWrite(stepPin, HIGH);
}

void loop() {
  // 1. ПРИЙОМ ДАНИХ
  if (Serial.available() >= 3) {
    uint8_t packet[3];
    Serial.readBytes(packet, 3);
    targetInput = packet[0];
    lastPacketTime = millis();

    // Якщо це перший пакет і перевірка ще не робилася
    if (!pilotConnected && !checkDone) {
      pilotConnected = true;
      runMechanicalCheck();
    }
    pilotConnected = true; // Підтверджуємо, що пілот на зв'язку
  }

  // 2. FAILSAFE (ВТРАТА ЗВ'ЯЗКУ)
  if (millis() - lastPacketTime > 1000) { // Якщо секунду немає даних
    if (pilotConnected) {
      Serial.println("SYSTEM: PILOT DISCONNECTED");
      pilotConnected = false;
      checkDone = false; // Скидаємо прапорець, щоб при новому вході знову перевірити
    }
    targetInput = 127;
    digitalWrite(ledPin, LOW);
  } else {
    digitalWrite(ledPin, HIGH);
  }

  // 3. ТЕЛЕМЕТРІЯ
  if (millis() - lastTelemetry > 1000) {
    bool isAlarm = (digitalRead(alarmPin) == LOW);
    
    Serial.print("T:");
    Serial.print(temperatureRead(), 1); Serial.print(";");
    Serial.print(millis() / 1000); Serial.print(";");

    if (isAlarm) Serial.println("1");               // Аварія
    else if (!pilotConnected) Serial.println("3");   // Чекаємо коннект
    else if (!systemVerified) Serial.println("2");   // Помилка тесту
    else Serial.println("0");                       // ОК
    
    lastTelemetry = millis();
  }

  // 4. РУХ (Тільки якщо перевірка успішна)
  if (systemVerified && pilotConnected && digitalRead(alarmPin) == HIGH) {
    long targetPos = map(targetInput, 0, 254, -maxSteps, maxSteps);
    
    if (currentPos != targetPos) {
      int newDir = (currentPos < targetPos) ? HIGH : LOW;
      if (newDir != lastDir) {
        delayMicroseconds(5000);
        digitalWrite(dirPin, newDir);
        lastDir = newDir;
      }
      
      if (newDir == HIGH) currentPos++; else currentPos--;
      
      digitalWrite(stepPin, LOW); delayMicroseconds(15);
      digitalWrite(stepPin, HIGH);
      
      int wait = (abs(targetPos - currentPos) > 100) ? 150 : 400;
      delayMicroseconds(wait);
    }
  }
}