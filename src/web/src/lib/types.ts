// Connection types
export type ConnectionState = 
  | 'disconnected' 
  | 'connecting' 
  | 'connected' 
  | 'reconnecting' 
  | 'error';

export type ConnectionMode = 'local' | 'cloud' | 'demo';

export interface ConnectionConfig {
  mode: ConnectionMode;
  endpoint?: string;
  cloudUrl?: string;
  authToken?: string;
  deviceId?: string;
}

// Common interface for connection classes (real and demo)
export interface IConnection {
  connect(): Promise<void>;
  disconnect(): void;
  send(type: string, payload?: Record<string, unknown>): void;
  sendCommand(cmd: string, data?: Record<string, unknown>): void;
  onMessage(handler: (message: WebSocketMessage) => void): () => void;
  onStateChange(handler: (state: ConnectionState) => void): () => void;
  getState(): ConnectionState;
}

// Message types from ESP32
export type MessageType = 
  | 'status'
  | 'esp_status'
  | 'pico_status'
  | 'scale_status'
  | 'stats'
  | 'event'
  | 'log'
  | 'error'
  | 'scan_result'
  | 'scan_complete'
  | 'device_info';

export interface WebSocketMessage {
  type: MessageType;
  [key: string]: unknown;
}

// Machine state
export type MachineState = 
  | 'unknown'
  | 'init'
  | 'idle'
  | 'heating'
  | 'ready'
  | 'brewing'
  | 'steaming'
  | 'cooldown'
  | 'fault';

export type MachineMode = 'standby' | 'on' | 'eco';

export type HeatingStrategy = 0 | 1 | 2 | 3; // 0=Brew Only, 1=Sequential, 2=Parallel, 3=Smart Stagger

export interface MachineStatus {
  state: MachineState;
  mode: MachineMode;
  isHeating: boolean;
  isBrewing: boolean;
  machineOnTimestamp: number | null; // Unix timestamp (ms) when machine was turned ON, null if off
  heatingStrategy: HeatingStrategy | null; // Active heating strategy when machine is on
  lastShotTimestamp: number | null; // Unix timestamp (ms) of last completed shot
}

// Temperature
export interface BoilerTemp {
  current: number;
  setpoint: number;
  max: number;
}

export interface Temperatures {
  brew: BoilerTemp;
  steam: BoilerTemp;
  group: number;  // Group head temp for HX machines (°C)
}

// Power
export interface PowerStatus {
  current: number;
  voltage: number;
  todayKwh: number;
  totalKwh: number;
}

// Cleaning
export interface CleaningStatus {
  brewCount: number;     // Brews since last cleaning
  reminderDue: boolean;  // True when cleaning is recommended
}

// Water
export type WaterLevel = 'ok' | 'low' | 'empty';

export interface WaterStatus {
  tankLevel: WaterLevel;
  dripTrayFull: boolean;
}

// Scale
export interface ScaleStatus {
  connected: boolean;
  name: string;
  type: string;
  weight: number;
  flowRate: number;
  stable: boolean;
  battery: number;
}

export interface ScaleScanResult {
  address: string;
  name: string;
  rssi: number;
  type?: string;
}

// Brew-by-weight
export interface BBWSettings {
  enabled: boolean;
  targetWeight: number;
  doseWeight: number;
  stopOffset: number;
  autoTare: boolean;
}

// Shot
export interface ShotStatus {
  active: boolean;
  startTime: number;
  duration: number;
  weight: number;
  flowRate: number;
}

// WiFi
export interface WiFiStatus {
  connected: boolean;
  ssid: string;
  ip: string;
  rssi: number;
  apMode: boolean;
}

// MQTT
export interface MQTTConfig {
  enabled: boolean;
  broker: string;
  port: number;
  username: string;
  password: string;
  topic: string;
  discovery: boolean;
}

export interface MQTTStatus {
  enabled: boolean;
  connected: boolean;
  broker: string;
  topic: string;
}

// Cloud
export interface CloudConfig {
  enabled: boolean;
  serverUrl: string;
  connected: boolean;
  deviceId: string;
}

// Machine type enum for type-safe comparisons
export type MachineType = 'dual_boiler' | 'single_boiler' | 'heat_exchanger' | '';

// Device info
export interface DeviceInfo {
  deviceId: string;
  deviceName: string;
  machineBrand: string;
  machineModel: string;
  machineType: MachineType;  // dual_boiler, single_boiler, heat_exchanger
  firmwareVersion: string;
}

export interface ESP32Info {
  version: string;
  freeHeap: number;
}

export interface PicoInfo {
  connected: boolean;
  version: string;
}

// =============================================================================
// STATISTICS TYPES - Enhanced statistics from ESP32
// =============================================================================

/**
 * Individual brew record with detailed metrics
 */
export interface BrewRecord {
  timestamp: number;           // Unix timestamp
  durationMs: number;          // Brew duration in milliseconds
  yieldWeight: number;         // Output weight (g)
  doseWeight: number;          // Input dose (g)
  peakPressure: number;        // Maximum pressure during brew
  avgTemperature: number;      // Average brew temperature
  avgFlowRate: number;         // Average flow rate (g/s)
  rating: number;              // User rating (0-5, 0=unrated)
  ratio?: number;              // Brew ratio (yield/dose)
}

/**
 * Power consumption sample
 */
export interface PowerSample {
  timestamp: number;           // Unix timestamp
  avgWatts: number;            // Average power during interval
  maxWatts: number;            // Peak power during interval
  kwhConsumed: number;         // Energy consumed during interval
}

/**
 * Daily summary for trend analysis
 */
export interface DailySummary {
  date: number;                // Unix timestamp at midnight
  shotCount: number;           // Shots that day
  totalBrewTimeMs: number;     // Total brew time
  totalKwh: number;            // Total energy consumed
  onTimeMinutes: number;       // Minutes machine was on
  steamCycles: number;         // Steam cycle count
  avgBrewTimeMs: number;       // Average brew time
}

/**
 * Statistics for a time period
 */
export interface PeriodStats {
  shotCount: number;
  totalBrewTimeMs: number;
  avgBrewTimeMs: number;
  minBrewTimeMs: number;
  maxBrewTimeMs: number;
  totalKwh: number;
}

/**
 * Lifetime statistics
 */
export interface LifetimeStats {
  totalShots: number;
  totalSteamCycles: number;
  totalKwh: number;
  totalOnTimeMinutes: number;
  totalBrewTimeMs: number;
  avgBrewTimeMs: number;
  minBrewTimeMs: number;
  maxBrewTimeMs: number;
  firstShotTimestamp: number;
}

/**
 * Maintenance tracking
 */
export interface MaintenanceStats {
  shotsSinceBackflush: number;
  shotsSinceGroupClean: number;
  shotsSinceDescale: number;
  lastBackflushTimestamp: number;
  lastGroupCleanTimestamp: number;
  lastDescaleTimestamp: number;
}

/**
 * Complete statistics package
 */
export interface Statistics {
  // Lifetime stats
  lifetime: LifetimeStats;
  
  // Period stats (calculated from history)
  daily: PeriodStats;
  weekly: PeriodStats;
  monthly: PeriodStats;
  
  // Maintenance
  maintenance: MaintenanceStats;
  
  // Session
  sessionShots: number;
  sessionStartTimestamp: number;
  
  // Legacy compatibility fields (derived from new structure)
  totalShots: number;
  totalSteamCycles: number;
  totalKwh: number;
  totalOnTimeMinutes: number;
  shotsToday: number;
  kwhToday: number;
  onTimeToday: number;
  shotsSinceDescale: number;
  shotsSinceGroupClean: number;
  shotsSinceBackflush: number;
  lastDescaleTimestamp: number;
  lastGroupCleanTimestamp: number;
  lastBackflushTimestamp: number;
  avgBrewTimeMs: number;
  minBrewTimeMs: number;
  maxBrewTimeMs: number;
  dailyCount: number;
  weeklyCount: number;
  monthlyCount: number;
}

/**
 * Extended statistics response (from /api/stats/extended)
 */
export interface ExtendedStatsResponse {
  stats: Statistics;
  weekly: { day: string; shots: number }[];
  hourlyDistribution: { hour: number; count: number }[];
  brewHistory: BrewRecord[];
  powerHistory: PowerSample[];
  dailyHistory: DailySummary[];
}

// Alerts & Logs
export interface Alert {
  id: number;
  time: string;
  level: 'info' | 'warning' | 'error';
  message: string;
  dismissed: boolean;
}

export interface LogEntry {
  id: number;
  time: string;
  level: string;
  message: string;
}

// Cloud device (from API)
export interface CloudDevice {
  id: string;
  name: string;
  isOnline: boolean;
  lastSeen: string | null;
  firmwareVersion: string | null;
  machineType: string | null;
  claimedAt: string;
}

// User preferences (stored in localStorage)
export type FirstDayOfWeek = 'sunday' | 'monday';
export type TemperatureUnit = 'celsius' | 'fahrenheit';

export interface UserPreferences {
  firstDayOfWeek: FirstDayOfWeek;
  use24HourTime: boolean;
  temperatureUnit: TemperatureUnit;
}
