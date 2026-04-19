import { WebSocketServer } from "ws";
import { SerialPort } from "serialport";
import { ReadlineParser } from "@serialport/parser-readline";

// 1. Налаштування порту
const port = new SerialPort({
  path: "COM7", 
  baudRate: 115200,
});

port.on("error", (err) => {
  console.error("Помилка Serial порту: ", err.message);
});

const wss = new WebSocketServer({ port: 3000 });
const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));

// Змінні стану
let lastSentSteering = 127;
let lastSentBrake = 0;
let lastSentThrottle = 0;
let lastSentTime = Date.now();
let currentMachineStatus = "2"; // По замовчуванню: Потрібна перевірка

// Змінні калібрування
let initialZero = 127;
let isCalibrated = false;

function processSteering(rawSteering: number): number {
  if (!isCalibrated && rawSteering > 110 && rawSteering < 145) {
    initialZero = rawSteering;
    isCalibrated = true;
    console.log(`Систему відкалібровано! Нуль: ${initialZero}`);
  }
  
  let offset = rawSteering - initialZero;
  let steering = 127 + offset;

  const deathZone = 5;
  if (Math.abs(steering - 127) < deathZone) steering = 127;

  return Math.max(0, Math.min(254, steering));
}

function sendToMachine(s: number, b: number, t: number) {
  if (port.isOpen) {
    port.write(Buffer.from([s, b, t]));
    lastSentSteering = s;
    lastSentBrake = b;
    lastSentThrottle = t;
    lastSentTime = Date.now();
  }
}

// Функція запиту на Self-Test
function triggerSelfTest() {
  console.log("📡 Відправка команди на Self-Test (250)...");
  if (port.isOpen) {
    port.write(Buffer.from([250, 0, 0]));
  }
}

port.on("open", () => {
  console.log("Зв'язок з ESP32 (COM7) встановлено!");
});

// ОБРОБКА ТЕЛЕМЕТРІЇ
parser.on('data', (line: string) => {
    const message = line.trim();
    
    if (message.startsWith("T:")) {
        const parts = message.replace("T:", "").split(";");
        
        if (parts.length >= 3) {
            currentMachineStatus = parts[2]; // Оновлюємо глобальний статус

            let statusText = "";
            switch (currentMachineStatus) {
                case "0": statusText = "✅ ГОТОВИЙ"; break;
                case "1": statusText = "🚨 АВАРІЯ ДРАЙВЕРА (ALARM)"; break;
                case "2": statusText = "⚠️ ПОТРІБНА ПЕРЕВІРКА"; break;
                case "4": statusText = "⚙️ ЙДЕ ТЕСТ КЕРМА..."; break;
                default: statusText = "❓ НЕВІДОМИЙ СТАН";
            }

            process.stdout.write(`\r🌡️ ${parts[0]}°C | ⏱️ ${parts[1]}s | Стан: ${statusText}   `);
        }
    } else {
        console.log("\nESP32 Log:", message);
    }
});

wss.on("connection", (ws) => {
  console.log("\n🛸 ПІЛОТ підключився! Запускаємо діагностику...");
  
  // ЯК ТІЛЬКИ ПІЛОТ ЗАЙШОВ - ДАЄМО КОМАНДУ НА ТЕСТ
  setTimeout(() => {
    triggerSelfTest();
  }, 1000); 

  ws.on("message", (rawData) => {
    try {
      const command = JSON.parse(rawData.toString());
      
      // Якщо система ще не пройшла тест, ігноруємо керування
      if (currentMachineStatus !== "0") {
          return; 
      }

      const finalSteering = processSteering(command.steering);
      const now = Date.now();
      
      const dataChanged = 
        Math.abs(finalSteering - lastSentSteering) > 1 ||
        Math.abs(command.brake - lastSentBrake) > 2 ||
        Math.abs(command.throttle - lastSentThrottle) > 2;

      const timePassed = now - lastSentTime > 100;

      if (dataChanged || timePassed) {
        sendToMachine(finalSteering, command.brake, command.throttle);
      }
    } catch (e) {
      console.error("Помилка даних:", e);
    }
  });

  ws.on("close", () => {
    console.log("\n🔴 ПІЛОТ відключився.");
    sendToMachine(127, 0, 0);
    isCalibrated = false; // Скидаємо калібрування для наступного пілота
  });
});

console.log("Сервер керування кермом запущено на порту 3000.");