const int stepPin = 14; 
const int dirPin = 13;
int currentPosition = 0;
const int maxRotationSteps = 800;

void setup() {
  Serial.begin(115200);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  // Початковий стан — LOW
  digitalWrite(stepPin, LOW);
}

void loop() {
  if (Serial.available() >= 3) {
    uint8_t steering = Serial.read();
    Serial.read(); // пропуск brake
    Serial.read(); // пропуск throttle

    int targetPosition = map(steering, 0, 254, -maxRotationSteps, maxRotationSteps);

    if (abs(targetPosition - currentPosition) > 5) {
      if (currentPosition < targetPosition) {
        digitalWrite(dirPin, HIGH);
        currentPosition++;
      } else {
        digitalWrite(dirPin, LOW);
        currentPosition--;
      }
      
      // Чіткий і довгий імпульс для тесту
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(1500); // Дуже повільно! 1.5 мс
      digitalWrite(stepPin, LOW);
      delayMicroseconds(1500);
    }
  }
}