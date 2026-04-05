#include <SoftwareSerial.h>
#include <Servo.h>

SoftwareSerial MyInput(10, 11); // RX на 10, TX на 11
Servo steeringServo;

const int CENTER_ANGLE = 65; // Твій центр
const int RANGE = 40;        // Радіус повороту (вліво/вправо на 40 град)

void setup() {
MyInput.begin(38400);
  steeringServo.attach(9); // Серво на 9 піні
  steeringServo.write(CENTER_ANGLE);
}

unsigned long lastUpdate = 0;

void loop() {
  if (MyInput.available() >= 3) {
    byte steer = MyInput.read();
    byte brake = MyInput.read();
    byte throttle = MyInput.read();

    // Оновлюємо серво не частіше ніж раз на 20 мс
    if (millis() - lastUpdate > 20) {
      int angle = map(steer, 0, 255, CENTER_ANGLE - RANGE, CENTER_ANGLE + RANGE);
      steeringServo.write(angle);
      lastUpdate = millis();
    }
  }
}