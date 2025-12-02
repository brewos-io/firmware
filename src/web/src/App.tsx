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
import { initConnection, getConnection, setActiveConnection } from '@/lib/connection';
import { initializeStore } from '@/lib/store';
import { useAppStore } from '@/lib/mode';
import { useThemeStore } from '@/lib/themeStore';
import { Loading } from '@/components/Loading';
import { DemoBanner } from '@/components/DemoBanner';
import { getDemoConnection, clearDemoConnection } from '@/lib/demo-connection';
import { isDemoMode } from '@/lib/demo-mode';

function App() {
  const [loading, setLoading] = useState(true);
  const [setupComplete, setSetupComplete] = useState(true); // Default true to avoid flash
  const [inDemoMode] = useState(() => isDemoMode());
  
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

  // Initialize demo mode
  useEffect(() => {
    if (!inDemoMode) return;

    const initDemo = async () => {
      initTheme();
      
      const demoConnection = getDemoConnection();
      
      // Set as active connection so useCommand works
      setActiveConnection(demoConnection);
      
      initializeStore(demoConnection);
      
      await demoConnection.connect();
      setLoading(false);
    };

    initDemo();

    return () => {
      setActiveConnection(null);
      clearDemoConnection();
    };
  }, [inDemoMode, initTheme]);

  // Initialize normal mode
  useEffect(() => {
    if (inDemoMode) return;
    
    const init = async () => {
      // Initialize theme first for immediate visual consistency
      initTheme();
      
      // Initialize app - this fetches mode from server
      await initialize();
    };

    init();
  }, [initialize, initTheme, inDemoMode]);

  // Setup local mode connection after initialization
  useEffect(() => {
    if (inDemoMode) return; // Skip if in demo mode
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
  }, [initialized, mode, apMode, inDemoMode]);

  // Handle setup completion
  const handleSetupComplete = () => {
    setSetupComplete(true);
  };

  // Handle demo exit
  const handleExitDemo = () => {
    // Navigate to login page - the login page will clear demo mode
    // Don't clear here to avoid connection errors during navigation
    window.location.href = '/login?exitDemo=true';
  };

  // Show loading state
  if (loading || (!inDemoMode && !initialized)) {
    return <Loading />;
  }

  // ===== DEMO MODE =====
  if (inDemoMode) {
    return (
      <>
        <DemoBanner onExit={handleExitDemo} />
        <Routes>
          <Route path="/" element={<Layout />}>
            <Route index element={<Dashboard />} />
            <Route path="brewing" element={<Brewing />} />
            <Route path="stats" element={<Stats />} />
            <Route path="schedules" element={<Schedules />} />
            <Route path="settings" element={<Settings />} />
            <Route path="*" element={<Navigate to="/" replace />} />
          </Route>
        </Routes>
      </>
    );
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
