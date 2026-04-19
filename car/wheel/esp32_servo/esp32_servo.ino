#include <Arduino.h>

const int stepPin = 12;
const int dirPin = 13;
const int alarmPin = 14; 
const int ledPin = 2;

long STEPS_PER_REV = 1600; 
const float TOTAL_TURNS = 4.0;
long maxSteps = (STEPS_PER_REV * TOTAL_TURNS) / 2;

int targetSteeringInput = 127;
long currentPosition = 0;
int lastDir = -1;
unsigned long lastPacketTime = 0;
unsigned long lastTelemetryTime = 0;
bool systemVerified = false; 

void runMechanicalCheck() {
  Serial.println("SYSTEM: STARTING MECHANICAL CHECK...");
  
  // Якщо при старті вже горить Alarm - тест провалено відразу
  if (digitalRead(alarmPin) == LOW) { // При P-10=0, LOW це помилка
    systemVerified = false;
    return;
  }

  // Рух для перевірки
  digitalWrite(dirPin, LOW);
  for(int i = 0; i < 200; i++) {
    digitalWrite(stepPin, LOW); delayMicroseconds(10);
    digitalWrite(stepPin, HIGH); delayMicroseconds(1000);
    // Якщо під час руху виб'є помилку - фіксуємо
    if(digitalRead(alarmPin) == LOW) { systemVerified = false; return; }
  }
  
  delay(200);
  digitalWrite(dirPin, HIGH);
  for(int i = 0; i < 200; i++) {
    digitalWrite(stepPin, LOW); delayMicroseconds(10);
    digitalWrite(stepPin, HIGH); delayMicroseconds(1000);
  }

  systemVerified = true; // Якщо доїхали сюди і Alarm не мигнув - все ОК
  Serial.println("SYSTEM: MECHANICAL CHECK PASSED");
}

void setup() {
  Serial.begin(115200);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(alarmPin, INPUT); 
  pinMode(ledPin, OUTPUT);

  digitalWrite(stepPin, HIGH);
  delay(2000); // Даємо драйверу час "прокинутись"
  runMechanicalCheck();
}

void loop() {
  if (Serial.available() >= 3) {
    uint8_t packet[3];
    Serial.readBytes(packet, 3);
    targetSteeringInput = packet[0];
    lastPacketTime = millis();
  }

  // Failsafe
  if (millis() - lastPacketTime > 500) {
    targetSteeringInput = 127;
    digitalWrite(ledPin, LOW);
  } else {
    digitalWrite(ledPin, HIGH);
  }

  // Телеметрія
  if (millis() - lastTelemetryTime > 1000) {
    bool isAlarm = (digitalRead(alarmPin) == LOW); 
    
    Serial.print("T:");
    Serial.print(temperatureRead(), 1); Serial.print(";");
    Serial.print(millis() / 1000); Serial.print(";");

    if (isAlarm) Serial.println("1");               // Код 1: Реальна аварія
    else if (!systemVerified) Serial.println("2");   // Код 2: Тест не пройшов
    else Serial.println("0");                       // Код 0: ВСЕ ГУД
    
    lastTelemetryTime = millis();
  }

  // Рух (тільки якщо немає Alarm)
  if (digitalRead(alarmPin) == HIGH) {
    long targetPosition = map(targetSteeringInput, 0, 254, -maxSteps, maxSteps);
    if (currentPosition != targetPosition) {
      int newDir = (currentPosition < targetPosition) ? HIGH : LOW;
      if (newDir != lastDir) {
        delayMicroseconds(5000);
        digitalWrite(dirPin, newDir);
        lastDir = newDir;
      }
      if (newDir == HIGH) currentPosition++; else currentPosition--;

      digitalWrite(stepPin, LOW);
      delayMicroseconds(15);
      digitalWrite(stepPin, HIGH);
      
      long dist = abs(targetPosition - currentPosition);
      int wait = (dist > 100) ? 150 : 400;
      delayMicroseconds(wait);
    }
  }
}