/**
 * Date/Time Utilities for Backend
 * 
 * CONVENTION:
 * - All dates are stored and transmitted in UTC (ISO 8601 format)
 * - SQLite's datetime('now') returns UTC by default
 * - All API responses should use UTC timestamps
 * - Frontend is responsible for localizing to user's timezone
 */

/**
 * Get current UTC timestamp in ISO 8601 format
 * Use this when creating/updating records
 */
export function nowUTC(): string {
  return new Date().toISOString();
}

/**
 * Get current Unix timestamp in seconds (UTC)
 */
export function nowUnixSeconds(): number {
  return Math.floor(Date.now() / 1000);
}

/**
 * Convert a Date object to UTC ISO string
 */
export function toUTC(date: Date): string {
  return date.toISOString();
}

/**
 * Parse a timestamp (ISO string or Unix seconds) to Date
 */
export function parseTimestamp(timestamp: string | number): Date {
  if (typeof timestamp === 'number') {
    // Detect Unix seconds vs milliseconds
    return new Date(timestamp < 10000000000 ? timestamp * 1000 : timestamp);
  }
  return new Date(timestamp);
}

/**
 * Check if a timestamp has expired
 */
export function isExpired(expiresAt: string | Date): boolean {
  const expiry = typeof expiresAt === 'string' ? new Date(expiresAt) : expiresAt;
  return expiry.getTime() < Date.now();
}

/**
 * Get a future timestamp (for setting expiration times)
 * 
 * @param amount - Amount of time units
 * @param unit - Time unit: 'seconds', 'minutes', 'hours', 'days'
 * @returns ISO 8601 UTC timestamp
 */
export function futureUTC(amount: number, unit: 'seconds' | 'minutes' | 'hours' | 'days'): string {
  const multipliers = {
    seconds: 1000,
    minutes: 60 * 1000,
    hours: 60 * 60 * 1000,
    days: 24 * 60 * 60 * 1000,
  };
  
  const futureMs = Date.now() + amount * multipliers[unit];
  return new Date(futureMs).toISOString();
}

/**
 * Format duration in seconds to human-readable string
 * Used for logging and debugging
 */
export function formatDuration(seconds: number): string {
  if (seconds < 60) return `${seconds}s`;
  if (seconds < 3600) return `${Math.floor(seconds / 60)}m ${seconds % 60}s`;
  const hours = Math.floor(seconds / 3600);
  const mins = Math.floor((seconds % 3600) / 60);
  return `${hours}h ${mins}m`;
}

