#include <ESP32Servo.h>

Servo steeringServo;
const int SERVO_PIN = 18;
const int LED_THROTTLE = 4;  // Зелений
const int LED_BRAKE = 5;     // Червоний

const int CENTER_ANGLE = 65;
const int RANGE = 45;

unsigned long lastPacketTime = 0;
unsigned long lastTelemetryTime = 0; // Для таймера зворотного зв'язку
const unsigned long FAILSAFE_TIMEOUT = 1000;  // 1 секунда без зв'язку
bool isFailsafeActive = false;

void setup() {
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  steeringServo.setPeriodHertz(50);
  steeringServo.attach(SERVO_PIN, 500, 2400);
  steeringServo.write(CENTER_ANGLE);

  pinMode(LED_THROTTLE, OUTPUT);
  pinMode(LED_BRAKE, OUTPUT);

  ledcAttach(LED_THROTTLE, 5000, 8);
  ledcAttach(LED_BRAKE, 5000, 8);

  lastPacketTime = millis();
}

void loop() {
  // --- 1. ЧИТАННЯ КОМАНД ВІД СЕРВЕРА ---
  if (Serial.available() >= 3) {
    byte steerRaw = Serial.read();
    byte brakeRaw = Serial.read();
    byte throttleRaw = Serial.read();

    lastPacketTime = millis();
    isFailsafeActive = false;

    int finalThrottle = 0;
    int finalBrake = 0;

    if (brakeRaw > 5) {
      finalBrake = brakeRaw;
      finalThrottle = 0;
    } else {
      finalThrottle = throttleRaw;
      finalBrake = 0;
    }

    int angle = map(steerRaw, 0, 255, CENTER_ANGLE - RANGE, CENTER_ANGLE + RANGE);
    steeringServo.write(angle);
    ledcWrite(LED_THROTTLE, finalThrottle);
    ledcWrite(LED_BRAKE, finalBrake);
  }

  // --- 2. ЛОГІКА FAILSAFE (ЗАХИСТ) ---
  if (millis() - lastPacketTime > FAILSAFE_TIMEOUT) {
    unsigned long timeSinceLoss = millis() - lastPacketTime;
    steeringServo.write(CENTER_ANGLE);
    ledcWrite(LED_THROTTLE, 0);

    if (timeSinceLoss < 3000) {
      ledcWrite(LED_BRAKE, 255); // Активне гальмування 3 сек
    } else {
      ledcWrite(LED_BRAKE, 0);   // Повна зупинка
      isFailsafeActive = true;
    }
  }

  // --- 3. ЗВОРОТНИЙ ЗВ'ЯЗОК (ТЕЛЕМЕТРІЯ) ---
  // Відправляємо дані на сервер раз на 1000 мс (1 секунда)
  if (millis() - lastTelemetryTime > 1000 * 5) {
    float temp_c = temperatureRead(); // Вбудований датчик температури ESP32

    Serial.print("TELE:");
    Serial.print(temp_c);
    Serial.print(";");
    Serial.print(millis() / 1000); // Аптайм (час роботи в сек)
    Serial.print(";");
    Serial.println(isFailsafeActive ? "1" : "0"); // Статус безпеки

    lastTelemetryTime = millis();
  }
}