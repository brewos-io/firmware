/**
 * Demo statistics data generator
 * Creates realistic-looking statistics for demo mode
 */

import type { ExtendedStatsResponse, BrewRecord, PowerSample, DailySummary, Statistics } from "./types";

// Generate timestamps
const now = Math.floor(Date.now() / 1000);
const day = 86400;
const hour = 3600;

/**
 * Generate realistic demo brew history
 */
function generateBrewHistory(): BrewRecord[] {
  const brews: BrewRecord[] = [];
  
  // Generate ~20 brews over last few days
  const brewTimes = [7, 7.5, 8, 12, 14, 15, 16, 17, 18]; // Typical brew hours
  
  for (let daysAgo = 0; daysAgo < 5; daysAgo++) {
    const numBrews = daysAgo === 0 ? 3 : Math.floor(Math.random() * 4) + 2;
    
    for (let i = 0; i < numBrews; i++) {
      const brewHour = brewTimes[Math.floor(Math.random() * brewTimes.length)];
      const timestamp = now - (daysAgo * day) - ((24 - brewHour) * hour) + Math.floor(Math.random() * 1800);
      
      const durationMs = 25000 + Math.floor(Math.random() * 10000); // 25-35 seconds
      const doseWeight = 18 + Math.random() * 2; // 18-20g dose
      const yieldWeight = doseWeight * (1.8 + Math.random() * 0.6); // 1.8-2.4 ratio
      
      brews.push({
        timestamp,
        durationMs,
        yieldWeight: Math.round(yieldWeight * 10) / 10,
        doseWeight: Math.round(doseWeight * 10) / 10,
        peakPressure: 8.5 + Math.random(),
        avgTemperature: 93 + Math.random() * 2,
        avgFlowRate: 2 + Math.random() * 0.5,
        rating: Math.random() > 0.7 ? Math.floor(Math.random() * 2) + 4 : 0, // 30% rated 4-5 stars
        ratio: Math.round((yieldWeight / doseWeight) * 10) / 10,
      });
    }
  }
  
  // Sort by timestamp descending (most recent first)
  return brews.sort((a, b) => b.timestamp - a.timestamp);
}

/**
 * Generate power consumption history (24 hours at 5-minute intervals)
 */
function generatePowerHistory(): PowerSample[] {
  const samples: PowerSample[] = [];
  const intervalSeconds = 300; // 5 minutes
  const numSamples = 288; // 24 hours worth
  
  for (let i = numSamples - 1; i >= 0; i--) {
    const timestamp = now - (i * intervalSeconds);
    const hourOfDay = new Date(timestamp * 1000).getHours();
    
    // Simulate typical usage patterns
    let avgWatts = 5; // Standby
    let maxWatts = 10;
    let kwhConsumed = 0.0004; // Very low standby
    
    // Morning peak (6-9am)
    if (hourOfDay >= 6 && hourOfDay < 9) {
      const intensity = Math.random();
      if (intensity > 0.6) {
        avgWatts = 800 + Math.random() * 600;
        maxWatts = 1500 + Math.random() * 400;
        kwhConsumed = (avgWatts * 300) / 3600000; // 5 min interval
      } else if (intensity > 0.3) {
        avgWatts = 200 + Math.random() * 300;
        maxWatts = 800 + Math.random() * 400;
        kwhConsumed = (avgWatts * 300) / 3600000;
      }
    }
    // Afternoon (2-4pm)
    else if (hourOfDay >= 14 && hourOfDay < 16) {
      const intensity = Math.random();
      if (intensity > 0.7) {
        avgWatts = 600 + Math.random() * 500;
        maxWatts = 1400 + Math.random() * 300;
        kwhConsumed = (avgWatts * 300) / 3600000;
      }
    }
    // Evening (6-8pm)
    else if (hourOfDay >= 18 && hourOfDay < 20) {
      const intensity = Math.random();
      if (intensity > 0.6) {
        avgWatts = 500 + Math.random() * 600;
        maxWatts = 1300 + Math.random() * 400;
        kwhConsumed = (avgWatts * 300) / 3600000;
      }
    }
    
    samples.push({
      timestamp,
      avgWatts: Math.round(avgWatts),
      maxWatts: Math.round(maxWatts),
      kwhConsumed: Math.round(kwhConsumed * 10000) / 10000,
    });
  }
  
  return samples;
}

/**
 * Generate daily summaries for last 30 days
 */
function generateDailyHistory(): DailySummary[] {
  const summaries: DailySummary[] = [];
  
  for (let daysAgo = 0; daysAgo < 30; daysAgo++) {
    const date = now - (daysAgo * day);
    const midnight = Math.floor(date / day) * day;
    
    // Weekend vs weekday patterns
    const dayOfWeek = new Date(midnight * 1000).getDay();
    const isWeekend = dayOfWeek === 0 || dayOfWeek === 6;
    
    const baseShots = isWeekend ? 4 : 3;
    const shotVariance = Math.floor(Math.random() * 3);
    const shotCount = baseShots + shotVariance + (daysAgo === 0 ? 3 : 0);
    
    const avgBrewTimeMs = 27000 + Math.floor(Math.random() * 6000);
    const totalBrewTimeMs = shotCount * avgBrewTimeMs;
    
    // Energy varies based on shots and machine-on time
    const baseKwh = 0.3; // Base heating
    const perShotKwh = 0.08;
    const totalKwh = baseKwh + (shotCount * perShotKwh) + Math.random() * 0.2;
    
    const onTimeMinutes = 60 + shotCount * 15 + Math.floor(Math.random() * 30);
    const steamCycles = Math.floor(Math.random() * 3);
    
    summaries.push({
      date: midnight,
      shotCount,
      totalBrewTimeMs,
      totalKwh: Math.round(totalKwh * 100) / 100,
      onTimeMinutes,
      steamCycles,
      avgBrewTimeMs,
    });
  }
  
  // Sort oldest first
  return summaries.reverse();
}

/**
 * Generate weekly chart data
 */
function generateWeeklyData(): { day: string; shots: number }[] {
  const days = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"];
  const today = new Date().getDay();
  
  return days.map((day, index) => {
    const dayIndex = (index + 1) % 7; // Monday = 0 in our array, but getDay() returns 0 for Sunday
    const isWeekend = dayIndex === 6 || dayIndex === 0;
    
    // Today gets actual value, other days get estimates
    if (dayIndex === today) {
      return { day, shots: 3 };
    }
    
    const baseShots = isWeekend ? 5 : 3;
    const variance = Math.floor(Math.random() * 3) - 1;
    return { day, shots: Math.max(1, baseShots + variance) };
  });
}

/**
 * Generate hourly distribution (when user typically brews)
 */
function generateHourlyDistribution(): { hour: number; count: number }[] {
  const distribution: { hour: number; count: number }[] = [];
  
  // Typical coffee drinking patterns
  const hourPatterns: Record<number, number> = {
    6: 3,
    7: 12,
    8: 15,
    9: 8,
    10: 4,
    11: 2,
    12: 3,
    13: 2,
    14: 5,
    15: 7,
    16: 4,
    17: 3,
    18: 4,
    19: 2,
    20: 1,
  };
  
  for (let hour = 0; hour < 24; hour++) {
    const baseCount = hourPatterns[hour] || 0;
    const variance = Math.floor(Math.random() * 3) - 1;
    distribution.push({
      hour,
      count: Math.max(0, baseCount + variance),
    });
  }
  
  return distribution;
}

/**
 * Generate complete demo statistics
 */
export function generateDemoStats(): Statistics {
  return {
    // New structured fields
    lifetime: {
      totalShots: 1247,
      totalSteamCycles: 234,
      totalKwh: 89.3,
      totalOnTimeMinutes: 15420,
      totalBrewTimeMs: 35539500,
      avgBrewTimeMs: 28500,
      minBrewTimeMs: 18000,
      maxBrewTimeMs: 42000,
      firstShotTimestamp: now - (180 * day), // Started 6 months ago
    },
    daily: {
      shotCount: 3,
      totalBrewTimeMs: 85500,
      avgBrewTimeMs: 28500,
      minBrewTimeMs: 26000,
      maxBrewTimeMs: 31000,
      totalKwh: 0.95,
    },
    weekly: {
      shotCount: 28,
      totalBrewTimeMs: 798000,
      avgBrewTimeMs: 28500,
      minBrewTimeMs: 22000,
      maxBrewTimeMs: 35000,
      totalKwh: 7.2,
    },
    monthly: {
      shotCount: 124,
      totalBrewTimeMs: 3534000,
      avgBrewTimeMs: 28500,
      minBrewTimeMs: 21000,
      maxBrewTimeMs: 38000,
      totalKwh: 28.5,
    },
    maintenance: {
      shotsSinceBackflush: 45,
      shotsSinceGroupClean: 12,
      shotsSinceDescale: 145,
      lastBackflushTimestamp: now - (7 * day),
      lastGroupCleanTimestamp: now - (2 * day),
      lastDescaleTimestamp: now - (30 * day),
    },
    sessionShots: 3,
    sessionStartTimestamp: now - 2700, // 45 min ago
    
    // Legacy compatibility fields
    totalShots: 1247,
    totalSteamCycles: 234,
    totalKwh: 89.3,
    totalOnTimeMinutes: 15420,
    shotsToday: 3,
    kwhToday: 0.95,
    onTimeToday: 45,
    shotsSinceDescale: 145,
    shotsSinceGroupClean: 12,
    shotsSinceBackflush: 45,
    lastDescaleTimestamp: now - (30 * day),
    lastGroupCleanTimestamp: now - (2 * day),
    lastBackflushTimestamp: now - (7 * day),
    avgBrewTimeMs: 28500,
    minBrewTimeMs: 22000,
    maxBrewTimeMs: 35000,
    dailyCount: 3,
    weeklyCount: 28,
    monthlyCount: 124,
  };
}

/**
 * Get complete extended stats response for demo mode
 */
export function getDemoExtendedStats(): ExtendedStatsResponse {
  return {
    stats: generateDemoStats(),
    weekly: generateWeeklyData(),
    hourlyDistribution: generateHourlyDistribution(),
    brewHistory: generateBrewHistory(),
    powerHistory: generatePowerHistory(),
    dailyHistory: generateDailyHistory(),
  };
}

