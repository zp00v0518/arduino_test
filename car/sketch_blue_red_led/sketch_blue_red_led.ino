#include <Adafruit_NeoPixel.h>

#define PIN        48
#define NUMPIXELS  1

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // На S3 Native USB (COM7) іноді стартує довше
  Serial.begin(115200); 
  
  pixels.begin();
  pixels.setBrightness(50);
  
  // Фіолетовий колір при старті - чекаємо на ініціалізацію
  pixels.setPixelColor(0, pixels.Color(100, 0, 100)); 
  pixels.show();
}

void loop() {
  // Перевіряємо вхідний буфер
  if (Serial.available() >= 3) {
    byte s = Serial.read();
    byte b = Serial.read();
    byte t = Serial.read();

    // Робимо колір:
    int r = t;           // Червоний від газу
    int g = (t < 10 && b < 10) ? 50 : 0; // Зелений, тільки якщо нічого не тиснемо
    int b_val = b;       // Синій від гальм

    pixels.setPixelColor(0, pixels.Color(r, g, b_val));
    pixels.show();
  }
}