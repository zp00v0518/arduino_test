import WebSocket from 'ws';
import HID from 'node-hid';
import type { DriveCommand } from '../types.ts';

const URL = 'ws://localhost:3000';
let ws: WebSocket;
let lastSent = 0;
const SEND_INTERVAL = 30; // ~33 кадри на секунду — ідеально для керування

function connect() {
    ws = new WebSocket(URL);
    ws.on('open', () => console.log("ПІЛОТ: Зв'язок встановлено. Поїхали!"));
    ws.on('error', () => {}); // Ігноруємо помилки підключення
    ws.on('close', () => setTimeout(connect, 2000));
}

connect();

const devices = HID.devices();
const deviceInfo = devices.find(d => d.vendorId === 1356 && d.productId === 3302);

if (deviceInfo?.path) {
    const device = new HID.HID(deviceInfo.path);
    console.log("DualSense активований. Чекаю на ввід...");

    device.on("data", (data: Buffer) => {
        const now = Date.now();
        if (now - lastSent < SEND_INTERVAL) return;

        if (ws && ws.readyState === WebSocket.OPEN) {
            const command: DriveCommand = {
                steering: data[1],
                brake:    data[5], // Твій L2
                throttle: data[6]  // Твій R2
            };

            ws.send(JSON.stringify(command));
            lastSent = now;
        }
    });
} else {
    console.error("Джойстик не знайдено. Перевір USB-кабель.");
}