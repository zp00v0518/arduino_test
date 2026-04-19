#include <Arduino.h>

const int stepPin = 12;
const int dirPin = 13;

// Змінні стану
int currentSteering = 127;
int lastDir = -1;
unsigned long lastPacketTime = 0;

// Змінні для плавності
float smoothSteering = 127.0;
const float accelStep = 0.8; // Швидкість розгону (чим менше, тим плавніше)

void setup()
{
  Serial.begin(115200);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);

  digitalWrite(stepPin, HIGH);
  digitalWrite(dirPin, HIGH);
}

void loop()
{
  // 1. Читаємо дані з Serial
  if (Serial.available() >= 3)
  {
    uint8_t packet[3];
    Serial.readBytes(packet, 3);
    currentSteering = packet[0];
    lastPacketTime = millis(); // Оновлюємо час останнього пакету
  }

  // 2. Failsafe: якщо зв'язок зник на 0.5 сек - зупиняємося
  if (millis() - lastPacketTime > 500)
  {
    currentSteering = 127;
  }

  // 3. Плавний розгін (наближаємо smoothSteering до цілі)
  if (smoothSteering < currentSteering)
  {
    smoothSteering += accelStep;
  }
  else if (smoothSteering > currentSteering)
  {
    smoothSteering -= accelStep;
  }

  // 4. Логіка руху (мертва зона 15 одиниць від центру)
  if (abs(smoothSteering - 127) > 15)
  {
    int newDir = (smoothSteering > 127) ? HIGH : LOW;

    // Захист від різкого реверсу: якщо напрямок змінився - пауза
    if (newDir != lastDir && lastDir != -1)
    {
      delay(60);            // Час на фізичну зупинку валу
      smoothSteering = 127; // Починаємо розгін з нуля в інший бік
    }
    lastDir = newDir;

    digitalWrite(dirPin, newDir);

    // Генерація кроку
    digitalWrite(stepPin, LOW);
    delayMicroseconds(50);
    digitalWrite(stepPin, HIGH);

    // Швидкість: чим більше відхилення, тим менша затримка (wait)
    // 1000 - повільно, 120 - швидко
    int wait = map(abs((int)smoothSteering - 127), 0, 127, 1000, 120);
    delayMicroseconds(wait);
  }
  else
  {
    lastDir = -1; // Скидаємо напрямок, коли стік у центрі
  }
}