/**
 * PWA detection utilities
 * 
 * When the app runs as an installed PWA (standalone mode), 
 * it should only support cloud mode - not local or demo mode.
 */

/**
 * Check if the app is running as an installed PWA (standalone mode)
 * This includes iOS "Add to Home Screen" and Android/Chrome PWA installs.
 */
export function isRunningAsPWA(): boolean {
  if (typeof window === 'undefined') return false;
  
  // Check for standalone display mode (standard PWA detection)
  const isStandalone = window.matchMedia('(display-mode: standalone)').matches;
  
  // Check for iOS standalone mode (Add to Home Screen)
  const isIOSStandalone = (window.navigator as { standalone?: boolean }).standalone === true;
  
  // Check for TWA (Trusted Web Activity) on Android
  const isTWA = document.referrer.includes('android-app://');
  
  return isStandalone || isIOSStandalone || isTWA;
}

/**
 * Check if demo mode should be allowed
 * Demo mode is NOT allowed when running as a PWA
 */
export function isDemoModeAllowed(): boolean {
  return !isRunningAsPWA();
}

/**
 * Check if local mode should be allowed
 * Local mode is NOT allowed when running as a PWA
 */
export function isLocalModeAllowed(): boolean {
  return !isRunningAsPWA();
}

