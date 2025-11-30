import { create } from 'zustand';
import { persist } from 'zustand/middleware';
import type { CloudDevice, ConnectionMode } from './types';
import { 
  isGoogleAuthConfigured, 
  getStoredSession, 
  clearSession, 
  handleGoogleSuccess,
  type GoogleUser,
} from './google-auth';

/**
 * Detect if running on ESP32 (local) or cloud
 */
export function detectMode(): ConnectionMode {
  // If no Google auth config, we're on ESP32
  if (!isGoogleAuthConfigured) return 'local';
  
  // If hostname is brewos.local or IP, we're on ESP32
  const host = window.location.hostname;
  if (host === 'brewos.local' || host === 'localhost') return 'local';
  if (host.match(/^192\.168\./) || host.match(/^10\./) || host.match(/^172\./)) return 'local';
  
  return 'cloud';
}

interface AppState {
  // Mode
  mode: ConnectionMode;
  initialized: boolean;
  
  // Auth (cloud only)
  user: GoogleUser | null;
  idToken: string | null;
  authLoading: boolean;
  
  // Devices (cloud only)
  devices: CloudDevice[];
  selectedDeviceId: string | null;
  devicesLoading: boolean;
  
  // Actions
  initialize: () => Promise<void>;
  handleGoogleLogin: (credential: string) => void;
  signOut: () => void;
  
  // Device management
  fetchDevices: () => Promise<void>;
  selectDevice: (deviceId: string) => void;
  claimDevice: (deviceId: string, token: string, name?: string) => Promise<boolean>;
  removeDevice: (deviceId: string) => Promise<boolean>;
  renameDevice: (deviceId: string, name: string) => Promise<boolean>;
  
  // Helpers
  getSelectedDevice: () => CloudDevice | null;
  getAccessToken: () => string | null;
}

export const useAppStore = create<AppState>()(
  persist(
    (set, get) => ({
      // Initial state
      mode: detectMode(),
      initialized: false,
      user: null,
      idToken: null,
      authLoading: true,
      devices: [],
      selectedDeviceId: null,
      devicesLoading: false,

      initialize: async () => {
        const mode = detectMode();
        set({ mode });

        if (mode === 'local') {
          set({ initialized: true, authLoading: false });
          return;
        }

        // Cloud mode: check for stored session
        const session = getStoredSession();
        
        if (session) {
          set({
            user: session.user,
            idToken: session.idToken,
            authLoading: false,
            initialized: true,
          });
          
          // Fetch devices
          get().fetchDevices();
        } else {
          set({ authLoading: false, initialized: true });
        }
      },

      handleGoogleLogin: (credential: string) => {
        const session = handleGoogleSuccess(credential);
        
        if (session) {
          set({
            user: session.user,
            idToken: session.idToken,
            authLoading: false,
          });
          
          // Fetch devices after login
          get().fetchDevices();
        }
      },

      signOut: () => {
        clearSession();
        set({ 
          user: null, 
          idToken: null, 
          devices: [], 
          selectedDeviceId: null 
        });
      },

      fetchDevices: async () => {
        const token = get().getAccessToken();
        if (!token) return;

        set({ devicesLoading: true });

        try {
          const response = await fetch('/api/devices', {
            headers: {
              Authorization: `Bearer ${token}`,
            },
          });

          if (response.ok) {
            const data = await response.json();
            const devices = data.devices as CloudDevice[];
            
            set({ devices });
            
            // Auto-select first device if none selected
            if (!get().selectedDeviceId && devices.length > 0) {
              set({ selectedDeviceId: devices[0].id });
            }
          } else if (response.status === 401) {
            // Token expired, sign out
            get().signOut();
          }
        } catch (error) {
          console.error('Failed to fetch devices:', error);
        }

        set({ devicesLoading: false });
      },

      selectDevice: (deviceId) => {
        set({ selectedDeviceId: deviceId });
      },

      claimDevice: async (deviceId, token, name) => {
        const accessToken = get().getAccessToken();
        if (!accessToken) return false;

        try {
          const response = await fetch('/api/devices/claim', {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json',
              Authorization: `Bearer ${accessToken}`,
            },
            body: JSON.stringify({ deviceId, token, name }),
          });

          if (response.ok) {
            await get().fetchDevices();
            return true;
          }
        } catch (error) {
          console.error('Failed to claim device:', error);
        }

        return false;
      },

      removeDevice: async (deviceId) => {
        const token = get().getAccessToken();
        if (!token) return false;

        try {
          const response = await fetch(`/api/devices/${deviceId}`, {
            method: 'DELETE',
            headers: {
              Authorization: `Bearer ${token}`,
            },
          });

          if (response.ok) {
            set((state) => ({
              devices: state.devices.filter(d => d.id !== deviceId),
              selectedDeviceId: state.selectedDeviceId === deviceId 
                ? (state.devices[0]?.id ?? null)
                : state.selectedDeviceId,
            }));
            return true;
          }
        } catch (error) {
          console.error('Failed to remove device:', error);
        }

        return false;
      },

      renameDevice: async (deviceId, name) => {
        const token = get().getAccessToken();
        if (!token) return false;

        try {
          const response = await fetch(`/api/devices/${deviceId}`, {
            method: 'PATCH',
            headers: {
              'Content-Type': 'application/json',
              Authorization: `Bearer ${token}`,
            },
            body: JSON.stringify({ name }),
          });

          if (response.ok) {
            set((state) => ({
              devices: state.devices.map(d =>
                d.id === deviceId ? { ...d, name } : d
              ),
            }));
            return true;
          }
        } catch (error) {
          console.error('Failed to rename device:', error);
        }

        return false;
      },

      getSelectedDevice: () => {
        const { devices, selectedDeviceId } = get();
        return devices.find(d => d.id === selectedDeviceId) ?? null;
      },

      getAccessToken: () => {
        return get().idToken;
      },
    }),
    {
      name: 'brewos-app-state',
      partialize: (state) => ({
        selectedDeviceId: state.selectedDeviceId,
      }),
    }
  )
);
