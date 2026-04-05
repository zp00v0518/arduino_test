#include <HardwareSerial.h>
HardwareSerial MySerial(1); // Наш канал до Arduino

void setup() {
  Serial.begin(115200); // Порт для зв'язку з Node.js
 MySerial.begin(38400, SERIAL_8N1, 2, 1);; // Піни 2 (RX) та 1 (TX)
}

void loop() {
  if (Serial.available() >= 3) {
    byte data[3];
    Serial.readBytes(data, 3); // Читаємо пакет від Node.js
    
    MySerial.write(data, 3);   // Відправляємо пакет на Arduino
    
    // Для відладки в терміналі Node.js:
    // Serial.print("Sent to Arduino: ");
    // Serial.print(data[0]); Serial.print(",");
    // Serial.print(data[1]); Serial.print(",");
    // Serial.println(data[2]);
  }
}