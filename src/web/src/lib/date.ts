/**
 * Date/Time Utilities
 * 
 * CONVENTION:
 * - All backend dates are stored in UTC (ISO 8601 format)
 * - All frontend dates are displayed in user's local timezone
 * - Use these utilities for consistent formatting across the app
 */

export interface DateFormatOptions {
  /** Include date (default: true) */
  date?: boolean;
  /** Include time (default: false) */
  time?: boolean;
  /** Include seconds when showing time (default: false) */
  seconds?: boolean;
  /** Use relative time like "2 hours ago" (default: false) */
  relative?: boolean;
  /** Date style: 'full', 'long', 'medium', 'short' */
  dateStyle?: 'full' | 'long' | 'medium' | 'short';
  /** Time style: 'full', 'long', 'medium', 'short' */
  timeStyle?: 'full' | 'long' | 'medium' | 'short';
}

/**
 * Format a UTC timestamp for display in user's local timezone
 * 
 * @param timestamp - UTC timestamp (ISO string, Unix seconds, or Unix milliseconds)
 * @param options - Formatting options
 * @returns Localized date/time string
 * 
 * @example
 * formatDate('2024-12-01T10:30:00Z') // "Dec 1, 2024"
 * formatDate('2024-12-01T10:30:00Z', { time: true }) // "Dec 1, 2024, 2:30 AM" (PST)
 * formatDate(1701427800, { time: true }) // Unix seconds
 * formatDate(1701427800000, { time: true }) // Unix milliseconds
 */
export function formatDate(
  timestamp: string | number | Date | null | undefined,
  options: DateFormatOptions = {}
): string {
  if (!timestamp) return 'Never';

  const {
    date = true,
    time = false,
    seconds = false,
    relative = false,
    dateStyle = 'medium',
    timeStyle = 'short',
  } = options;

  // Parse the timestamp
  let dateObj: Date;
  
  if (timestamp instanceof Date) {
    dateObj = timestamp;
  } else if (typeof timestamp === 'string') {
    dateObj = new Date(timestamp);
  } else if (typeof timestamp === 'number') {
    // Detect if Unix seconds or milliseconds
    // Unix seconds are typically < 10 billion, milliseconds are > 1 trillion
    dateObj = timestamp < 10000000000 
      ? new Date(timestamp * 1000) 
      : new Date(timestamp);
  } else {
    return 'Invalid date';
  }

  // Check for invalid date
  if (isNaN(dateObj.getTime())) {
    return 'Invalid date';
  }

  // Relative time formatting
  if (relative) {
    return formatRelativeTime(dateObj);
  }

  // Build format options
  const formatOptions: Intl.DateTimeFormatOptions = {};

  if (date && time) {
    formatOptions.dateStyle = dateStyle;
    formatOptions.timeStyle = seconds ? 'medium' : timeStyle;
  } else if (date) {
    formatOptions.dateStyle = dateStyle;
  } else if (time) {
    formatOptions.timeStyle = seconds ? 'medium' : timeStyle;
  }

  try {
    return new Intl.DateTimeFormat(undefined, formatOptions).format(dateObj);
  } catch {
    // Fallback for older browsers
    if (date && time) {
      return dateObj.toLocaleString();
    } else if (date) {
      return dateObj.toLocaleDateString();
    } else {
      return dateObj.toLocaleTimeString();
    }
  }
}

/**
 * Format time only (no date) in user's local timezone
 */
export function formatTime(
  timestamp: string | number | Date | null | undefined,
  options: { seconds?: boolean } = {}
): string {
  return formatDate(timestamp, { date: false, time: true, ...options });
}

/**
 * Format date only (no time) in user's local timezone
 */
export function formatDateOnly(
  timestamp: string | number | Date | null | undefined,
  style: 'full' | 'long' | 'medium' | 'short' = 'medium'
): string {
  return formatDate(timestamp, { date: true, time: false, dateStyle: style });
}

/**
 * Format as relative time (e.g., "2 hours ago", "in 3 days")
 */
export function formatRelativeTime(date: Date | string | number): string {
  const now = new Date();
  const target = date instanceof Date ? date : new Date(
    typeof date === 'number' && date < 10000000000 ? date * 1000 : date
  );
  
  const diffMs = target.getTime() - now.getTime();
  const diffSecs = Math.round(diffMs / 1000);
  const diffMins = Math.round(diffSecs / 60);
  const diffHours = Math.round(diffMins / 60);
  const diffDays = Math.round(diffHours / 24);
  const diffWeeks = Math.round(diffDays / 7);
  const diffMonths = Math.round(diffDays / 30);
  const diffYears = Math.round(diffDays / 365);

  // Use Intl.RelativeTimeFormat if available
  if (typeof Intl !== 'undefined' && Intl.RelativeTimeFormat) {
    const rtf = new Intl.RelativeTimeFormat(undefined, { numeric: 'auto' });
    
    if (Math.abs(diffSecs) < 60) {
      return rtf.format(diffSecs, 'second');
    } else if (Math.abs(diffMins) < 60) {
      return rtf.format(diffMins, 'minute');
    } else if (Math.abs(diffHours) < 24) {
      return rtf.format(diffHours, 'hour');
    } else if (Math.abs(diffDays) < 7) {
      return rtf.format(diffDays, 'day');
    } else if (Math.abs(diffWeeks) < 4) {
      return rtf.format(diffWeeks, 'week');
    } else if (Math.abs(diffMonths) < 12) {
      return rtf.format(diffMonths, 'month');
    } else {
      return rtf.format(diffYears, 'year');
    }
  }

  // Fallback
  const abs = Math.abs;
  if (abs(diffSecs) < 60) return 'just now';
  if (abs(diffMins) < 60) return `${abs(diffMins)} minute${abs(diffMins) !== 1 ? 's' : ''} ago`;
  if (abs(diffHours) < 24) return `${abs(diffHours)} hour${abs(diffHours) !== 1 ? 's' : ''} ago`;
  if (abs(diffDays) < 7) return `${abs(diffDays)} day${abs(diffDays) !== 1 ? 's' : ''} ago`;
  return formatDate(target, { dateStyle: 'medium' });
}

/**
 * Get current UTC timestamp in ISO format (for sending to backend)
 */
export function nowUTC(): string {
  return new Date().toISOString();
}

/**
 * Convert a local date/time to UTC ISO string (for sending to backend)
 */
export function toUTC(date: Date | string | number): string {
  if (date instanceof Date) {
    return date.toISOString();
  }
  if (typeof date === 'number') {
    return new Date(date < 10000000000 ? date * 1000 : date).toISOString();
  }
  return new Date(date).toISOString();
}

/**
 * Parse a UTC timestamp and return a Date object
 */
export function parseUTC(timestamp: string | number): Date {
  if (typeof timestamp === 'number') {
    return new Date(timestamp < 10000000000 ? timestamp * 1000 : timestamp);
  }
  return new Date(timestamp);
}

/**
 * Format duration in human-readable form
 * 
 * @param seconds - Duration in seconds
 * @returns Human-readable duration string
 * 
 * @example
 * formatDuration(90) // "1m 30s"
 * formatDuration(3661) // "1h 1m"
 */
export function formatDuration(seconds: number): string {
  if (seconds < 0) return '0s';
  
  const hours = Math.floor(seconds / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  const secs = Math.floor(seconds % 60);

  if (hours > 0) {
    return minutes > 0 ? `${hours}h ${minutes}m` : `${hours}h`;
  }
  if (minutes > 0) {
    return secs > 0 ? `${minutes}m ${secs}s` : `${minutes}m`;
  }
  return `${secs}s`;
}

/**
 * Check if a timestamp is today (in user's local timezone)
 */
export function isToday(timestamp: string | number | Date): boolean {
  const date = parseUTC(timestamp as string | number);
  const today = new Date();
  return (
    date.getDate() === today.getDate() &&
    date.getMonth() === today.getMonth() &&
    date.getFullYear() === today.getFullYear()
  );
}

/**
 * Check if a timestamp is within the last N hours
 */
export function isWithinHours(timestamp: string | number | Date, hours: number): boolean {
  const date = parseUTC(timestamp as string | number);
  const cutoff = new Date(Date.now() - hours * 60 * 60 * 1000);
  return date >= cutoff;
}

