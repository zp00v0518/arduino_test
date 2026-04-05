export interface DriveCommand {
  steering: number;  // Байт 01 (0-255)
  brake: number;     // Байт 05 (0-255)
  throttle: number;  // Байт 06 (0-255)
}

export interface TelemetryData {
  temperature: number;
  batteryVoltage: number;
  lat?: number;
  lon?: number;
}