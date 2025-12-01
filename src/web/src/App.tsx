import { useEffect, useState } from 'react';
import { Routes, Route, Navigate } from 'react-router-dom';
import { Layout } from '@/components/Layout';
import { Dashboard } from '@/pages/Dashboard';
import { Brewing } from '@/pages/Brewing';
import { Stats } from '@/pages/Stats';
import { Settings } from '@/pages/settings';
import { Schedules } from '@/pages/Schedules';
import { Setup } from '@/pages/Setup';
import { Login } from '@/pages/Login';
import { Devices } from '@/pages/Devices';
import { AuthCallback } from '@/pages/AuthCallback';
import { Pair } from '@/pages/Pair';
import { Onboarding } from '@/pages/Onboarding';
import { FirstRunWizard } from '@/pages/FirstRunWizard';
import { initConnection, getConnection } from '@/lib/connection';
import { initializeStore } from '@/lib/store';
import { useAppStore } from '@/lib/mode';
import { useThemeStore } from '@/lib/themeStore';
import { Loading } from '@/components/Loading';

function App() {
  const [loading, setLoading] = useState(true);
  const [setupComplete, setSetupComplete] = useState(true); // Default true to avoid flash
  
  const { 
    mode,
    apMode,
    initialized, 
    user, 
    devices, 
    initialize,
    getSelectedDevice,
  } = useAppStore();
  
  const initTheme = useThemeStore((s) => s.initTheme);

  useEffect(() => {
    const init = async () => {
      // Initialize theme first for immediate visual consistency
      initTheme();
      
      // Initialize app - this fetches mode from server
      await initialize();
    };

    init();
  }, [initialize, initTheme]);

  // Setup local mode connection after initialization
  useEffect(() => {
    if (!initialized) return;

    const setupLocalMode = async () => {
      if (mode === 'local' && !apMode) {
        // Check if setup is complete
        try {
          const setupResponse = await fetch('/api/setup/status');
          if (setupResponse.ok) {
            const setupData = await setupResponse.json();
            setSetupComplete(setupData.complete);
          }
        } catch {
          // If endpoint doesn't exist, assume setup is complete
          setSetupComplete(true);
        }

        // Initialize WebSocket connection
        const connection = initConnection({
          mode: 'local',
          endpoint: '/ws',
        });

        initializeStore(connection);

        connection.connect().catch((error) => {
          console.error('Initial connection failed:', error);
        });
      }

      setLoading(false);
    };

    setupLocalMode();

    return () => {
      getConnection()?.disconnect();
    };
  }, [initialized, mode, apMode]);

  // Handle setup completion
  const handleSetupComplete = () => {
    setSetupComplete(true);
  };

  // Show loading state
  if (loading || !initialized) {
    return <Loading />;
  }

  // ===== LOCAL MODE (ESP32) =====
  if (mode === 'local') {
    // Show WiFi setup page in AP mode
    if (apMode) {
      return <Setup />;
    }

    // Show first-run wizard if setup not complete
    if (!setupComplete) {
      return <FirstRunWizard onComplete={handleSetupComplete} />;
    }

    return (
      <Routes>
        <Route path="/" element={<Layout />}>
          <Route index element={<Dashboard />} />
          <Route path="brewing" element={<Brewing />} />
          <Route path="stats" element={<Stats />} />
          <Route path="schedules" element={<Schedules />} />
          <Route path="settings" element={<Settings />} />
          <Route path="setup" element={<Setup />} />
          <Route path="*" element={<Navigate to="/" replace />} />
        </Route>
      </Routes>
    );
  }

  // ===== CLOUD MODE =====
  
  // Not logged in -> Login
  if (!user) {
    return (
      <Routes>
        <Route path="/login" element={<Login />} />
        <Route path="/auth/callback" element={<AuthCallback />} />
        <Route path="/pair" element={<Pair />} />
        <Route path="*" element={<Navigate to="/login" replace />} />
      </Routes>
    );
  }

  // Logged in but no devices -> Onboarding
  if (devices.length === 0) {
    return (
      <Routes>
        <Route path="/onboarding" element={<Onboarding />} />
        <Route path="/auth/callback" element={<AuthCallback />} />
        <Route path="/pair" element={<Pair />} />
        <Route path="*" element={<Navigate to="/onboarding" replace />} />
      </Routes>
    );
  }

  // Logged in with devices -> Full app
  const selectedDevice = getSelectedDevice();
  
  return (
    <Routes>
      {/* Auth routes */}
      <Route path="/auth/callback" element={<AuthCallback />} />
      <Route path="/pair" element={<Pair />} />
      <Route path="/onboarding" element={<Onboarding />} />
      
      {/* Device management */}
      <Route path="/devices" element={<Devices />} />
      <Route path="/login" element={<Navigate to="/devices" replace />} />
      
      {/* Device control (when connected via cloud) */}
      <Route path="/device/:deviceId" element={<Layout />}>
        <Route index element={<Dashboard />} />
        <Route path="brewing" element={<Brewing />} />
        <Route path="stats" element={<Stats />} />
        <Route path="schedules" element={<Schedules />} />
        <Route path="settings" element={<Settings />} />
      </Route>
      
      {/* Root: redirect to selected device or first device */}
      <Route 
        path="/" 
        element={
          <Navigate 
            to={selectedDevice ? `/device/${selectedDevice.id}` : '/devices'} 
            replace 
          />
        } 
      />
      
      {/* Default: redirect to devices */}
      <Route path="*" element={<Navigate to="/" replace />} />
    </Routes>
  );
}

export default App;
