#include <ESP32Servo.h>

// --- Об'єкти та піни ---
Servo steeringServo;
const int SERVO_PIN = 18;
const int LED_THROTTLE = 4;
const int LED_BRAKE = 5;

// --- Налаштування ---
const int CENTER_ANGLE = 65;
const int RANGE = 45;
const unsigned long FAILSAFE_TIMEOUT = 1000;

// --- Глобальні змінні (спільні для ядер) ---
volatile unsigned long lastPacketTime = 0;
volatile bool isFailsafeActive = false;

// Task Handle (посилання на завдання телеметрії)
TaskHandle_t TelemetryTask;

// --- ФУНКЦІЯ ДЛЯ CORE 0 (Телеметрія) ---
void telemetryTaskCode(void * pvParameters) {
  Serial.print("Telemetry Task started on core: ");
  Serial.println(xPortGetCoreID());

  for (;;) { // Нескінченний цикл завдання
    float temp_c = temperatureRead();
    unsigned long uptime = millis() / 1000;

    // Відправка даних
    Serial.print("TELE:");
    Serial.print(temp_c);
    Serial.print(";");
    Serial.print(uptime);
    Serial.print(";");
    Serial.println(isFailsafeActive ? "1" : "0");

    // Затримка саме для цього завдання (1 секунда)
    // vTaskDelay не блокує процесор, а дає йому "подихати"
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  // Налаштування заліза
  ESP32PWM::allocateTimer(0);
  steeringServo.setPeriodHertz(50);
  steeringServo.attach(SERVO_PIN, 500, 2400);
  steeringServo.write(CENTER_ANGLE);

  pinMode(LED_THROTTLE, OUTPUT);
  pinMode(LED_BRAKE, OUTPUT);
  ledcAttach(LED_THROTTLE, 5000, 8);
  ledcAttach(LED_BRAKE, 5000, 8);

  // --- ЗАПУСК ДРУГОГО ЯДРА ---
  xTaskCreatePinnedToCore(
    telemetryTaskCode,   // Функція завдання
    "TelemetryTask",     // Назва
    10000,               // Розмір стеку (пам'ять)
    NULL,                // Параметри
    1,                   // Пріоритет
    &TelemetryTask,      // Хендл
    0                    // НОМЕР ЯДРА (Core 0)
  );

  lastPacketTime = millis();
}

// --- ЦЕЙ ЦИКЛ ПРАЦЮЄ НА CORE 1 АВТОМАТИЧНО ---
void loop() {
  // 1. Читання команд
  if (Serial.available() >= 3) {
    byte steerRaw = Serial.read();
    byte brakeRaw = Serial.read();
    byte throttleRaw = Serial.read();

    lastPacketTime = millis();
    isFailsafeActive = false;

    int finalThrottle = (brakeRaw > 5) ? 0 : throttleRaw;
    int finalBrake = (brakeRaw > 5) ? brakeRaw : 0;

    int angle = map(steerRaw, 0, 255, CENTER_ANGLE - RANGE, CENTER_ANGLE + RANGE);
    steeringServo.write(angle);
    ledcWrite(LED_THROTTLE, finalThrottle);
    ledcWrite(LED_BRAKE, finalBrake);
  }

  // 2. Failsafe
  if (millis() - lastPacketTime > FAILSAFE_TIMEOUT) {
    unsigned long timeSinceLoss = millis() - lastPacketTime;
    steeringServo.write(CENTER_ANGLE);
    ledcWrite(LED_THROTTLE, 0);

    if (timeSinceLoss < 3000) {
      ledcWrite(LED_BRAKE, 255);
    } else {
      ledcWrite(LED_BRAKE, 0);
      isFailsafeActive = true;
    }
  }
  
  // В loop() більше немає коду телеметрії! Він тепер незалежний.
}