#include <Arduino.h>

const int stepPin = 12;
const int dirPin = 13;

// Змінні стану
int targetSteeringInput = 127;
unsigned long lastPacketTime = 0;

// Налаштування кроків (перевір перемикачі на драйвері OK2D86ECS)
const long STEPS_PER_REV = 1600;      // Кроків на 360 градусів
const float TOTAL_TURNS = 4.0;        // Всього обертів від упору до упору
const long MAX_STEPS = (STEPS_PER_REV * TOTAL_TURNS) / 2; // Кроків від центру до краю (3200 для 4 обертів)
long currentPosition = 0;             // Поточна позиція в кроках

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
    targetSteeringInput = packet[0];
    lastPacketTime = millis(); // Оновлюємо час останнього пакету
  }

  // 2. Failsafe: якщо зв'язок зник на 0.5 сек - зупиняємося
  if (millis() - lastPacketTime > 500)
  {
    targetSteeringInput = 127;
  }

  // 3. Розрахунок цільової позиції в кроках
  // Мапимо вхід 0-254 на діапазон кроків [-MAX_STEPS, MAX_STEPS]
  long targetPosition = map(targetSteeringInput, 0, 254, -MAX_STEPS, MAX_STEPS);

  // 4. Рух до цілі
  if (currentPosition != targetPosition)
  {
    // Визначаємо напрямок
    if (currentPosition < targetPosition)
    {
      digitalWrite(dirPin, HIGH);
      currentPosition++;
    }
    else
    {
      digitalWrite(dirPin, LOW);
      currentPosition--;
    }

    // Генерація імпульсу кроку
    digitalWrite(stepPin, LOW);
    delayMicroseconds(10); // Дуже короткий імпульс для драйвера
    digitalWrite(stepPin, HIGH);

    // Розрахунок швидкості (wait)
    // Для позиційного керування можна використовувати постійну швидкість
    // або сповільнюватися при наближенні до цілі
    long distance = abs(targetPosition - currentPosition);
    int wait = (distance > 100) ? 80 : 250; // Швидше, якщо далеко, повільніше, якщо близько
    
    delayMicroseconds(wait);
  }
}