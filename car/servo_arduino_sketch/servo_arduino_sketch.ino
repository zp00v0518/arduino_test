#include <Servo.h>
Servo steeringServo;

void setup() {
  // Швидкість як у ESP32
  Serial.begin(115200); 
  steeringServo.attach(9);
  steeringServo.write(65); 
}

void loop() {
  // Чекаємо "листи" від ESP32
  if (Serial.available() >= 3) {
    byte s = Serial.read();
    byte b = Serial.read();
    byte t = Serial.read();

    // Використовуємо твій перевірений діапазон
    int angle = map(s, 0, 255, 0, 130); 
    steeringServo.write(angle);
  }
}