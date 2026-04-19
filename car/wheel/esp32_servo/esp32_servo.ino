#include <Arduino.h>

const int stepPin = 12;
const int dirPin = 13;
const int alarmPin = 14; 
const int ledPin = 2;

long STEPS_PER_REV = 1600; 
const float TOTAL_TURNS = 3.6;
long maxSteps = (STEPS_PER_REV * TOTAL_TURNS) / 2;

int targetInput = 127;
long currentPos = 0;
int lastDir = -1;
unsigned long lastTelemetry = 0;
unsigned long lastPacketTime = 0;

// Статуси системи
bool systemVerified = false; 
bool isTestingNow = false;

// --- ФУНКЦІЯ ПЕРЕВІРКИ (Винесена) ---
void executeMechanicalTest() {
  isTestingNow = true;
  systemVerified = false; // Скидаємо перед перевіркою
  
  Serial.println("SYSTEM: EXECUTING REMOTE TEST COMMAND...");

  // 1. Перевірка на старті
  if (digitalRead(alarmPin) == LOW) {
    isTestingNow = false;
    return;
  }

  // 2. Рух вліво (200 кроків)
  digitalWrite(dirPin, LOW);
  for(int i = 0; i < 200; i++) {
    digitalWrite(stepPin, LOW); delayMicroseconds(10);
    digitalWrite(stepPin, HIGH); delayMicroseconds(1000);
    if(digitalRead(alarmPin) == LOW) { isTestingNow = false; return; }
  }
  
  delay(200);

  // 3. Рух вправо (200 кроків)
  digitalWrite(dirPin, HIGH);
  for(int i = 0; i < 200; i++) {
    digitalWrite(stepPin, LOW); delayMicroseconds(10);
    digitalWrite(stepPin, HIGH); delayMicroseconds(1000);
    if(digitalRead(alarmPin) == LOW) { isTestingNow = false; return; }
  }

  systemVerified = true;
  isTestingNow = false;
  Serial.println("SYSTEM: TEST COMPLETED SUCCESSFULLY.");
}

void setup() {
  Serial.begin(115200);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(alarmPin, INPUT); 
  pinMode(ledPin, OUTPUT);
  digitalWrite(stepPin, HIGH);
  Serial.println("SYSTEM: READY. WAITING FOR SERVER COMMAND...");
}

void loop() {
  // 1. ПРИЙОМ ДАНИХ ТА КОМАНД
  if (Serial.available() >= 3) {
    uint8_t packet[3];
    Serial.readBytes(packet, 3);
    
    lastPacketTime = millis();

    // ПЕРЕВІРКА НА КОМАНДУ ТЕСТУ (Код 250)
    if (packet[0] == 250) {
      executeMechanicalTest();
    } else {
      targetInput = packet[0];
    }
  }

  // 2. ТЕЛЕМЕТРІЯ
  if (millis() - lastTelemetry > 1000) {
    bool isAlarm = (digitalRead(alarmPin) == LOW);
    
    Serial.print("T:");
    Serial.print(temperatureRead(), 1); Serial.print(";");
    Serial.print(millis() / 1000); Serial.print(";");

    if (isAlarm) Serial.println("1");               // Аварія (Червоний)
    else if (isTestingNow) Serial.println("4");      // Процес тесту (Жовтий/Миготить)
    else if (!systemVerified) Serial.println("2");   // Потрібна перевірка (Помаранчевий)
    else Serial.println("0");                       // ГОТОВИЙ (Зелений)
    
    lastTelemetry = millis();
  }

  // 3. FAILSAFE (Втрата зв'язку)
  if (millis() - lastPacketTime > 1000) {
    targetInput = 127;
    digitalWrite(ledPin, LOW);
  } else {
    digitalWrite(ledPin, HIGH);
  }

  // 4. РУХ (Блокується, якщо йде тест або є Alarm)
  if (systemVerified && !isTestingNow && digitalRead(alarmPin) == HIGH) {
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