void setup() {
  pinMode(2, INPUT);   // Вхід PUL від ESP32
  pinMode(3, INPUT);   // Вхід DIR від ESP32
  pinMode(10, OUTPUT); // Вихід PUL на драйвер (5V)
  pinMode(11, OUTPUT); // Вихід DIR на драйвер (5V)
}

void loop() {
  // Перекидаємо сигнали максимально швидко
  digitalWrite(10, digitalRead(2)); 
  digitalWrite(11, digitalRead(3)); 
}