import { WebSocketServer } from "ws";
import { SerialPort } from "serialport";
import { ReadlineParser } from "@serialport/parser-readline"; // Додай цей імпорт

// 1. Налаштування порту (Вкажи свій COM7)
const port = new SerialPort({
  path: "COM7", 
  baudRate: 115200,
});

const wss = new WebSocketServer({ port: 3000 });
const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));

// Змінні для збереження стану
let lastSentSteering = 127;
let lastSentBrake = 0;
let lastSentThrottle = 0;
let lastSentTime = Date.now();

// Змінні для калібрування
let initialZero = 127;
let isCalibrated = false;

// Функція обробки значень керма
function processSteering(rawSteering: number): number {
  if (!isCalibrated) {
    initialZero = rawSteering;
    isCalibrated = true;
    console.log(`Систему відкалібровано! Новий нуль: ${initialZero}`);
  }

  // Обчислюємо реальне значення відносно 127 (центр для ESP32)
  let offset = rawSteering - initialZero;
  let steering = 127 + offset;

  // Мертва зона (щоб не дрижало в центрі)
  const deathZone = 5;
  if (Math.abs(steering - 127) < deathZone) {
    steering = 127;
  }

  return Math.max(0, Math.min(254, steering)); // 254, щоб не плутати з маркером 255 (якщо захочемо додати)
}

// Функція відправки пакету на ESP32
function sendToMachine(s: number, b: number, t: number) {
  const packet = Buffer.from([s, b, t]);
  port.write(packet);
  
  lastSentSteering = s;
  lastSentBrake = b;
  lastSentThrottle = t;
  lastSentTime = Date.now();
}

port.on("open", () => {
  console.log("Зв'язок з ESP32 (COM7) встановлено!");
});

// Тепер слухаємо ПАРСЕР, а не сам порт
parser.on('data', (line: string) => {
    const message = line.trim();
    
    // Тепер шукаємо префікс "T:"
    if (message.startsWith("T:")) {
        const parts = message.replace("T:", "").split(";");
        
        if (parts.length >= 3) {
            console.log("\n--- ТЕЛЕМЕТРІЯ (раз на 5 сек) ---");
            console.log(`🌡️ CPU: ${parts[0]}°C`);
            console.log(`⏱️ Uptime: ${parts[1]} сек`);
            console.log(`🚨 Status: ${parts[2] === "1" ? "FAILSAFE" : "OK"}`);
            console.log("---------------------------------\n");
        }
    } else {
        // Якщо це не телеметрія, просто виводимо як лог
        console.log("ESP32:", message);
    }
});

wss.on("connection", (ws) => {
  console.log("ПІЛОТ підключився!");

  ws.on("message", (rawData) => {
    try {
      const command = JSON.parse(rawData.toString());
      const finalSteering = processSteering(command.steering);

      const now = Date.now();
      
      // УМОВА ВІДПРАВКИ: 
      // Або дані змінилися (поріг 2), або минуло 100 мс (Heartbeat)
      const dataChanged = 
        Math.abs(finalSteering - lastSentSteering) > 2 ||
        Math.abs(command.brake - lastSentBrake) > 2 ||
        Math.abs(command.throttle - lastSentThrottle) > 2;

      const timePassed = now - lastSentTime > 100;

      if (dataChanged || timePassed) {
        sendToMachine(finalSteering, command.brake, command.throttle);
        
        // Лог для перевірки (можна закоментувати, якщо заважає)
        // console.log(`STR: ${finalSteering} | BRK: ${command.brake} | THR: ${command.throttle}`);
      }
    } catch (e) {
      console.error("Помилка даних:", e);
    }
  });

  ws.on("close", () => {
    console.log("ПІЛОТ відключився.");
    // При відключенні пілота можемо відправити команду "стоп"
    sendToMachine(127, 0, 0);
  });
});

console.log("Сервер запущено на порту 3000. Чекаю пілота...");