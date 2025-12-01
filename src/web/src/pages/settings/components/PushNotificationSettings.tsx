import { useState, useEffect } from 'react';
import { useAppStore } from '@/lib/mode';
import { usePushNotifications } from '@/hooks/usePushNotifications';
import { Card, CardHeader, CardTitle, CardDescription } from '@/components/Card';
import { Button } from '@/components/Button';
import { Badge } from '@/components/Badge';
import { Toggle } from '@/components/Toggle';
import {
  Bell,
  BellOff,
  Check,
  X,
  AlertCircle,
  Coffee,
  Droplet,
  Wrench,
  AlertTriangle,
  Clock,
  Wifi,
  Settings2,
  Loader2,
} from 'lucide-react';

// Notification preference types
interface NotificationPreferences {
  machineReady: boolean;
  waterEmpty: boolean;
  descaleDue: boolean;
  serviceDue: boolean;
  backflushDue: boolean;
  machineError: boolean;
  picoOffline: boolean;
  scheduleTriggered: boolean;
  brewComplete: boolean;
}

const defaultPreferences: NotificationPreferences = {
  machineReady: true,
  waterEmpty: true,
  descaleDue: true,
  serviceDue: true,
  backflushDue: true,
  machineError: true,
  picoOffline: true,
  scheduleTriggered: true,
  brewComplete: false,
};

// Notification categories for better UX
const notificationCategories = [
  {
    title: 'Machine Status',
    description: 'Get notified about your machine state',
    items: [
      {
        key: 'machineReady' as const,
        label: 'Machine Ready',
        description: 'When your machine reaches brewing temperature',
        icon: Coffee,
        recommended: true,
      },
      {
        key: 'brewComplete' as const,
        label: 'Brew Complete',
        description: 'When a brew finishes (brew-by-weight)',
        icon: Coffee,
        recommended: false,
      },
    ],
  },
  {
    title: 'Alerts',
    description: 'Important alerts that need attention',
    items: [
      {
        key: 'waterEmpty' as const,
        label: 'Water Tank Empty',
        description: 'When the water tank needs refilling',
        icon: Droplet,
        recommended: true,
      },
      {
        key: 'machineError' as const,
        label: 'Machine Errors',
        description: 'When something goes wrong with your machine',
        icon: AlertTriangle,
        recommended: true,
      },
      {
        key: 'picoOffline' as const,
        label: 'Control Board Offline',
        description: 'When the control board stops responding',
        icon: Wifi,
        recommended: true,
      },
    ],
  },
  {
    title: 'Maintenance',
    description: 'Reminders for machine maintenance',
    items: [
      {
        key: 'descaleDue' as const,
        label: 'Descale Reminder',
        description: 'When it\'s time to descale your machine',
        icon: Wrench,
        recommended: true,
      },
      {
        key: 'serviceDue' as const,
        label: 'Service Reminder',
        description: 'When general service is recommended',
        icon: Wrench,
        recommended: true,
      },
      {
        key: 'backflushDue' as const,
        label: 'Backflush Reminder',
        description: 'When it\'s time to backflush',
        icon: Wrench,
        recommended: true,
      },
    ],
  },
  {
    title: 'Schedules',
    description: 'Notifications related to schedules',
    items: [
      {
        key: 'scheduleTriggered' as const,
        label: 'Schedule Triggered',
        description: 'When a schedule turns your machine on/off',
        icon: Clock,
        recommended: true,
      },
    ],
  },
];

export function PushNotificationSettings() {
  const { mode } = useAppStore();
  const {
    isSupported,
    isRegistered,
    isSubscribed,
    permission,
    subscribe,
    unsubscribe,
  } = usePushNotifications();

  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [preferences, setPreferences] = useState<NotificationPreferences>(defaultPreferences);
  const [loadingPrefs, setLoadingPrefs] = useState(false);
  const [savingPrefs, setSavingPrefs] = useState(false);

  // Only show in cloud mode
  if (mode !== 'cloud') {
    return null;
  }

  // Fetch notification preferences
  useEffect(() => {
    if (isSubscribed) {
      fetchPreferences();
    }
  }, [isSubscribed]);

  const fetchPreferences = async () => {
    setLoadingPrefs(true);
    try {
      const response = await fetch('/api/push/preferences', {
        credentials: 'include',
      });
      if (response.ok) {
        const data = await response.json();
        setPreferences(data.preferences);
      }
    } catch (err) {
      console.error('Failed to fetch notification preferences:', err);
    } finally {
      setLoadingPrefs(false);
    }
  };

  const updatePreference = async (key: keyof NotificationPreferences, value: boolean) => {
    // Optimistically update UI
    setPreferences(prev => ({ ...prev, [key]: value }));
    setSavingPrefs(true);

    try {
      const response = await fetch('/api/push/preferences', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        credentials: 'include',
        body: JSON.stringify({
          preferences: { [key]: value },
        }),
      });

      if (!response.ok) {
        // Revert on error
        setPreferences(prev => ({ ...prev, [key]: !value }));
        throw new Error('Failed to update preference');
      }
    } catch (err) {
      console.error('Failed to update notification preference:', err);
      setError('Failed to save preference');
      setTimeout(() => setError(null), 3000);
    } finally {
      setSavingPrefs(false);
    }
  };

  const handleSubscribe = async () => {
    setIsLoading(true);
    setError(null);
    try {
      const success = await subscribe();
      if (!success) {
        setError('Failed to subscribe to push notifications');
      } else {
        // Fetch preferences after subscribing
        fetchPreferences();
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to subscribe');
    } finally {
      setIsLoading(false);
    }
  };

  const handleUnsubscribe = async () => {
    setIsLoading(true);
    setError(null);
    try {
      const success = await unsubscribe();
      if (!success) {
        setError('Failed to unsubscribe from push notifications');
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to unsubscribe');
    } finally {
      setIsLoading(false);
    }
  };

  if (!isSupported) {
    return (
      <Card>
        <CardHeader>
          <CardTitle icon={<Bell className="w-5 h-5" />}>Push Notifications</CardTitle>
          <CardDescription>
            Push notifications are not supported in this browser
          </CardDescription>
        </CardHeader>
        <div className="p-6 text-center text-sm text-coffee-500">
          <BellOff className="w-12 h-12 mx-auto mb-3 text-coffee-300" />
          <p>Your browser does not support push notifications.</p>
        </div>
      </Card>
    );
  }

  return (
    <div className="space-y-6">
      {/* Main Push Notification Card */}
      <Card>
        <CardHeader>
          <CardTitle icon={<Bell className="w-5 h-5" />}>Push Notifications</CardTitle>
          <CardDescription>
            Receive notifications on your device when your machine needs attention
          </CardDescription>
        </CardHeader>

        <div className="p-6 space-y-6">
          {/* Status */}
          <div className="space-y-3">
            <div className="flex items-center justify-between">
              <span className="text-sm font-medium text-coffee-700">Service Worker</span>
              <Badge variant={isRegistered ? 'success' : 'warning'}>
                {isRegistered ? (
                  <>
                    <Check className="w-3 h-3 mr-1" />
                    Registered
                  </>
                ) : (
                  <>
                    <X className="w-3 h-3 mr-1" />
                    Not Registered
                  </>
                )}
              </Badge>
            </div>

            <div className="flex items-center justify-between">
              <span className="text-sm font-medium text-coffee-700">Permission</span>
              <Badge
                variant={
                  permission === 'granted'
                    ? 'success'
                    : permission === 'denied'
                      ? 'error'
                      : 'warning'
                }
              >
                {permission === 'granted' ? (
                  <>
                    <Check className="w-3 h-3 mr-1" />
                    Granted
                  </>
                ) : permission === 'denied' ? (
                  <>
                    <X className="w-3 h-3 mr-1" />
                    Denied
                  </>
                ) : (
                  'Default'
                )}
              </Badge>
            </div>

            <div className="flex items-center justify-between">
              <span className="text-sm font-medium text-coffee-700">Subscription</span>
              <Badge variant={isSubscribed ? 'success' : 'default'}>
                {isSubscribed ? (
                  <>
                    <Check className="w-3 h-3 mr-1" />
                    Active
                  </>
                ) : (
                  'Inactive'
                )}
              </Badge>
            </div>
          </div>

          {/* Error Message */}
          {error && (
            <div className="p-3 bg-red-50 border border-red-200 rounded-lg flex items-start gap-2">
              <AlertCircle className="w-4 h-4 text-red-600 mt-0.5 flex-shrink-0" />
              <p className="text-sm text-red-700">{error}</p>
            </div>
          )}

          {/* Actions */}
          <div className="space-y-3">
            {!isSubscribed ? (
              <Button
                onClick={handleSubscribe}
                loading={isLoading}
                disabled={permission === 'denied' || !isRegistered}
                className="w-full"
              >
                <Bell className="w-4 h-4 mr-2" />
                Enable Push Notifications
              </Button>
            ) : (
              <Button
                onClick={handleUnsubscribe}
                loading={isLoading}
                variant="secondary"
                className="w-full"
              >
                <BellOff className="w-4 h-4 mr-2" />
                Disable Push Notifications
              </Button>
            )}

            {permission === 'denied' && (
              <p className="text-xs text-coffee-500 text-center">
                Push notifications are blocked. Please enable them in your browser settings.
              </p>
            )}

            {!isRegistered && (
              <p className="text-xs text-coffee-500 text-center">
                Service worker is not registered. Please refresh the page.
              </p>
            )}
          </div>
        </div>
      </Card>

      {/* Notification Preferences Card - Only show when subscribed */}
      {isSubscribed && (
        <Card>
          <CardHeader>
            <CardTitle icon={<Settings2 className="w-5 h-5" />}>
              Notification Preferences
              {savingPrefs && <Loader2 className="w-4 h-4 ml-2 animate-spin inline" />}
            </CardTitle>
            <CardDescription>
              Choose which notifications you want to receive
            </CardDescription>
          </CardHeader>

          <div className="p-6">
            {loadingPrefs ? (
              <div className="flex items-center justify-center py-8">
                <Loader2 className="w-6 h-6 animate-spin text-accent" />
              </div>
            ) : (
              <div className="space-y-8">
                {notificationCategories.map((category) => (
                  <div key={category.title}>
                    <div className="mb-4">
                      <h3 className="font-semibold text-coffee-900">{category.title}</h3>
                      <p className="text-xs text-coffee-500">{category.description}</p>
                    </div>
                    <div className="space-y-3">
                      {category.items.map((item) => {
                        const Icon = item.icon;
                        return (
                          <div
                            key={item.key}
                            className="flex items-start justify-between p-3 bg-cream-50 rounded-xl"
                          >
                            <div className="flex items-start gap-3">
                              <div className="p-2 bg-cream-200 rounded-lg">
                                <Icon className="w-4 h-4 text-coffee-600" />
                              </div>
                              <div>
                                <div className="flex items-center gap-2">
                                  <span className="font-medium text-coffee-800">
                                    {item.label}
                                  </span>
                                  {item.recommended && (
                                    <Badge variant="info" className="text-[10px] px-1.5 py-0">
                                      Recommended
                                    </Badge>
                                  )}
                                </div>
                                <p className="text-xs text-coffee-500 mt-0.5">
                                  {item.description}
                                </p>
                              </div>
                            </div>
                            <Toggle
                              checked={preferences[item.key]}
                              onChange={(checked) => updatePreference(item.key, checked)}
                            />
                          </div>
                        );
                      })}
                    </div>
                  </div>
                ))}
              </div>
            )}
          </div>
        </Card>
      )}
    </div>
  );
}
