/**
 * Demo mode state management
 * Persists demo mode preference to localStorage
 */

const DEMO_MODE_KEY = "brewos-demo-mode";

/**
 * Initialize demo mode from URL parameters
 * This should be called EARLY, before React mounts, to ensure
 * localStorage is set before any components check isDemoMode()
 */
export function initDemoModeFromUrl(): void {
  if (typeof window === "undefined") return;

  const params = new URLSearchParams(window.location.search);

  // Check if we're exiting demo mode
  if (params.get("exitDemo") === "true") {
    disableDemoMode();
    // Clean up the URL
    params.delete("exitDemo");
    const newUrl = params.toString()
      ? `${window.location.pathname}?${params.toString()}`
      : window.location.pathname;
    window.history.replaceState({}, "", newUrl);
    return;
  }

  // Check URL parameter to enter demo mode
  if (params.get("demo") === "true") {
    // Persist to localStorage so it survives navigation
    enableDemoMode();
    console.log("[Demo] Demo mode enabled from URL parameter");
  }
}

/**
 * Check if demo mode is enabled (from localStorage or URL param)
 */
export function isDemoMode(): boolean {
  if (typeof window !== "undefined") {
    const params = new URLSearchParams(window.location.search);

    // Check if we're exiting demo mode - clear it immediately before any other checks
    if (params.get("exitDemo") === "true") {
      disableDemoMode();
      // Clean up the URL
      params.delete("exitDemo");
      const newUrl = params.toString()
        ? `${window.location.pathname}?${params.toString()}`
        : window.location.pathname;
      window.history.replaceState({}, "", newUrl);
      return false;
    }

    // Check URL parameter to enter demo mode
    if (params.get("demo") === "true") {
      // Persist to localStorage so it survives navigation
      enableDemoMode();
      return true;
    }
  }

  // Check localStorage
  if (localStorage.getItem(DEMO_MODE_KEY) === "true") {
    return true;
  }

  return false;
}

/**
 * Enable demo mode and persist to localStorage
 */
export function enableDemoMode(): void {
  localStorage.setItem(DEMO_MODE_KEY, "true");
}

/**
 * Disable demo mode and clear from localStorage
 */
export function disableDemoMode(): void {
  localStorage.removeItem(DEMO_MODE_KEY);
}

/**
 * Get demo schedules data (mock data for demo mode)
 * Days is a bitmask: bit 0 = Sunday, bit 1 = Monday, ... bit 6 = Saturday
 */
export function getDemoSchedules() {
  return {
    schedules: [
      {
        id: 1,
        enabled: true,
        name: "Morning Coffee",
        days: 0b0111110, // Mon-Fri (bits 1-5)
        hour: 6,
        minute: 30,
        action: "on" as const,
        strategy: 0,
      },
      {
        id: 2,
        enabled: true,
        name: "Weekend Brunch",
        days: 0b1000001, // Sat-Sun (bits 0 and 6)
        hour: 9,
        minute: 0,
        action: "on" as const,
        strategy: 0,
      },
      {
        id: 3,
        enabled: false,
        name: "Evening Off",
        days: 0b1111111, // Every day
        hour: 22,
        minute: 0,
        action: "off" as const,
        strategy: 0,
      },
    ],
    autoPowerOffEnabled: true,
    autoPowerOffMinutes: 120,
  };
}
