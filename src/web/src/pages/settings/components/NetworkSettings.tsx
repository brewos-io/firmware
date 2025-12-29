import { useState, useEffect } from 'react';
import { useStore } from '@/lib/store';
import { useCommand } from '@/lib/useCommand';
import { Card, CardHeader, CardTitle } from '@/components/Card';
import { Input } from '@/components/Input';
import { Button } from '@/components/Button';
import { Toggle } from '@/components/Toggle';
import { Badge } from '@/components/Badge';
import { ConfirmDialog } from '@/components/ConfirmDialog';
import { Wifi, Radio, Network, Settings, ChevronRight, Trash2, AlertTriangle } from 'lucide-react';
import { StatusRow } from './StatusRow';

export function NetworkSettings() {
  const wifi = useStore((s) => s.wifi);
  const mqtt = useStore((s) => s.mqtt);
  const { sendCommand } = useCommand();
  const [showForgetWarning, setShowForgetWarning] = useState(false);

  // IP configuration state
  const [ipConfig, setIpConfig] = useState({
    staticIp: wifi.staticIp,
    ip: wifi.ip || '',
    gateway: wifi.gateway || '',
    subnet: wifi.subnet || '255.255.255.0',
    dns1: wifi.dns1 || '',
    dns2: wifi.dns2 || '',
  });
  const [savingIp, setSavingIp] = useState(false);
  const [editingIp, setEditingIp] = useState(false);

  // Update local state when wifi state changes from server
  useEffect(() => {
    setIpConfig({
      staticIp: wifi.staticIp,
      ip: wifi.staticIp ? wifi.ip : ipConfig.ip || wifi.ip,
      gateway: wifi.gateway || ipConfig.gateway,
      subnet: wifi.subnet || ipConfig.subnet || '255.255.255.0',
      dns1: wifi.dns1 || ipConfig.dns1,
      dns2: wifi.dns2 || ipConfig.dns2,
    });
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [wifi.staticIp, wifi.gateway, wifi.subnet, wifi.dns1, wifi.dns2]);

  const [mqttConfig, setMqttConfig] = useState({
    enabled: mqtt.enabled,
    broker: mqtt.broker || '',
    port: 1883,
    username: '',
    password: '',
    topic: mqtt.topic || 'brewos',
    discovery: true,
  });
  const [testingMqtt, setTestingMqtt] = useState(false);
  const [mqttTestResult, setMqttTestResult] = useState<{ success: boolean; message: string } | null>(null);
  const [savingMqtt, setSavingMqtt] = useState(false);
  const [editingMqtt, setEditingMqtt] = useState(false);

  // Sync local MQTT config with store when store changes (e.g., after save)
  useEffect(() => {
    // Only sync when not editing to avoid overwriting user's current edits
    if (!editingMqtt) {
      setMqttConfig(prev => ({
        ...prev,
        enabled: mqtt.enabled,
        broker: mqtt.broker || prev.broker,
        topic: mqtt.topic || prev.topic,
      }));
    }
  }, [mqtt.enabled, mqtt.broker, mqtt.topic, editingMqtt]);

  // Listen for MQTT test results
  useEffect(() => {
    const handleTestResult = (event: CustomEvent<{ success: boolean; message: string; error?: string }>) => {
      setTestingMqtt(false);
      setMqttTestResult({
        success: event.detail.success,
        message: event.detail.message,
      });
      // Clear result after 5 seconds
      setTimeout(() => setMqttTestResult(null), 5000);
    };

    window.addEventListener('mqtt_test_result', handleTestResult as EventListener);
    return () => window.removeEventListener('mqtt_test_result', handleTestResult as EventListener);
  }, []);

  const testMqtt = () => {
    if (testingMqtt) return;
    setTestingMqtt(true);
    setMqttTestResult(null);
    sendCommand('mqtt_test', mqttConfig);
    // Timeout fallback in case no response received
    setTimeout(() => {
      setTestingMqtt(false);
    }, 10000);
  };

  const saveMqtt = () => {
    if (savingMqtt) return; // Prevent double-click
    setSavingMqtt(true);
    sendCommand('mqtt_config', mqttConfig, { successMessage: 'MQTT settings saved' });
    // Brief visual feedback for fire-and-forget WebSocket command
    setTimeout(() => setSavingMqtt(false), 600);
  };

  const confirmForgetWifi = () => {
    sendCommand('wifi_forget', undefined, { successMessage: 'WiFi forgotten. Device will restart...' });
    setShowForgetWarning(false);
  };

  const saveIpConfig = () => {
    if (savingIp) return;
    setSavingIp(true);
    sendCommand('wifi_config', ipConfig, { successMessage: 'IP settings saved. Reconnecting...' });
    setTimeout(() => setSavingIp(false), 600);
  };

  // Validate IP address format
  const isValidIp = (ip: string) => {
    if (!ip) return true; // Empty is OK for optional fields
    const parts = ip.split('.');
    if (parts.length !== 4) return false;
    return parts.every(part => {
      const num = parseInt(part, 10);
      return !isNaN(num) && num >= 0 && num <= 255 && part === num.toString();
    });
  };

  const ipConfigValid = !ipConfig.staticIp || (
    isValidIp(ipConfig.ip) && ipConfig.ip !== '' &&
    isValidIp(ipConfig.gateway) && ipConfig.gateway !== '' &&
    isValidIp(ipConfig.subnet) && ipConfig.subnet !== '' &&
    isValidIp(ipConfig.dns1)
  );

  return (
    <>
      {/* WiFi Status */}
      <Card>
        <CardHeader>
          <CardTitle icon={<Wifi className="w-5 h-5" />}>WiFi</CardTitle>
        </CardHeader>

        <div className="space-y-0">
          <StatusRow 
            label="Status" 
            value={
              <Badge variant={wifi.connected ? 'success' : wifi.apMode ? 'warning' : 'error'}>
                {wifi.connected ? 'Connected' : wifi.apMode ? 'AP Mode' : 'Disconnected'}
              </Badge>
            }
          />
          <StatusRow label="Network" value={wifi.ssid || '—'} />
          <StatusRow label="IP Address" value={wifi.ip || '—'} mono />
          <StatusRow 
            label="Signal" 
            value={wifi.rssi ? `${wifi.rssi} dBm` : '—'} 
          />

          {/* Forget Network button */}
          <button
            onClick={() => setShowForgetWarning(true)}
            className="w-full flex items-center justify-between py-2.5 border-t border-theme text-left group transition-colors hover:opacity-80 mt-2"
          >
            <div className="flex items-center gap-3">
              <Trash2 className="w-4 h-4 text-red-500" />
              <span className="text-sm font-medium text-red-500">Forget Network</span>
            </div>
            <ChevronRight className="w-4 h-4 text-red-500/50 group-hover:text-red-500 transition-colors" />
          </button>
        </div>
      </Card>

      {/* IP Configuration */}
      <Card>
        <CardHeader>
          <CardTitle icon={<Network className="w-5 h-5" />}>
            IP Configuration
          </CardTitle>
        </CardHeader>

        {!editingIp ? (
          /* View mode */
          <div className="space-y-0">
            <StatusRow label="Mode" value={wifi.staticIp ? 'Static' : 'DHCP'} />
            <StatusRow label="Gateway" value={wifi.gateway || '—'} mono />
            <StatusRow label="Subnet" value={wifi.subnet || '—'} mono />
            <StatusRow label="DNS" value={wifi.dns1 ? `${wifi.dns1}${wifi.dns2 ? `, ${wifi.dns2}` : ''}` : '—'} mono />
            
            {!wifi.apMode && (
              <button
                onClick={() => setEditingIp(true)}
                className="w-full flex items-center justify-between py-2.5 border-t border-theme text-left group transition-colors hover:opacity-80 mt-2"
              >
                <div className="flex items-center gap-3">
                  <Settings className="w-4 h-4 text-theme-muted" />
                  <span className="text-sm font-medium text-theme">Configure IP Settings</span>
                </div>
                <ChevronRight className="w-4 h-4 text-theme-muted group-hover:text-theme transition-colors" />
              </button>
            )}
          </div>
        ) : (
          /* Edit mode */
          <div className="space-y-4">
            <div className="flex items-center justify-between py-2 border-b border-theme">
              <span className="text-sm text-theme-muted">Mode</span>
              <Toggle
                checked={ipConfig.staticIp}
                onChange={(checked) => setIpConfig({ ...ipConfig, staticIp: checked })}
                label={ipConfig.staticIp ? 'Static' : 'DHCP'}
              />
            </div>

            {ipConfig.staticIp && (
              <>
                <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
                  <Input
                    label="IP Address"
                    placeholder="192.168.1.100"
                    value={ipConfig.ip}
                    onChange={(e) => setIpConfig({ ...ipConfig, ip: e.target.value })}
                    error={ipConfig.ip && !isValidIp(ipConfig.ip) ? 'Invalid format' : undefined}
                  />
                  <Input
                    label="Gateway"
                    placeholder="192.168.1.1"
                    value={ipConfig.gateway}
                    onChange={(e) => setIpConfig({ ...ipConfig, gateway: e.target.value })}
                    error={ipConfig.gateway && !isValidIp(ipConfig.gateway) ? 'Invalid format' : undefined}
                  />
                </div>
                <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
                  <Input
                    label="Subnet Mask"
                    placeholder="255.255.255.0"
                    value={ipConfig.subnet}
                    onChange={(e) => setIpConfig({ ...ipConfig, subnet: e.target.value })}
                    error={ipConfig.subnet && !isValidIp(ipConfig.subnet) ? 'Invalid format' : undefined}
                  />
                  <Input
                    label="Primary DNS"
                    placeholder="8.8.8.8"
                    value={ipConfig.dns1}
                    onChange={(e) => setIpConfig({ ...ipConfig, dns1: e.target.value })}
                    error={ipConfig.dns1 && !isValidIp(ipConfig.dns1) ? 'Invalid format' : undefined}
                  />
                  <Input
                    label="Secondary DNS"
                    placeholder="8.8.4.4"
                    value={ipConfig.dns2}
                    onChange={(e) => setIpConfig({ ...ipConfig, dns2: e.target.value })}
                    error={ipConfig.dns2 && !isValidIp(ipConfig.dns2) ? 'Invalid format' : undefined}
                    hint="Optional"
                  />
                </div>
              </>
            )}

            <div className="flex justify-end gap-3">
              <Button variant="ghost" onClick={() => setEditingIp(false)}>
                Cancel
              </Button>
              <Button 
                onClick={() => {
                  saveIpConfig();
                  setEditingIp(false);
                }} 
                loading={savingIp} 
                disabled={savingIp || !ipConfigValid}
              >
                Save
              </Button>
            </div>
          </div>
        )}
      </Card>

      {/* MQTT Settings */}
      <Card>
        <CardHeader>
          <CardTitle icon={<Radio className="w-5 h-5" />}>
            MQTT / Home Assistant
          </CardTitle>
        </CardHeader>

        {!editingMqtt ? (
          /* View mode */
          <div className="space-y-0">
            <StatusRow 
              label="Status" 
              value={
                <Badge variant={mqtt.connected ? 'success' : mqtt.enabled ? 'error' : 'default'}>
                  {mqtt.connected ? 'Connected' : mqtt.enabled ? 'Disconnected' : 'Disabled'}
                </Badge>
              }
            />
            <StatusRow label="Broker" value={mqtt.broker || '—'} mono />
            <StatusRow label="Topic" value={mqtt.topic || '—'} mono />
            
            <button
              onClick={() => setEditingMqtt(true)}
              className="w-full flex items-center justify-between py-2.5 border-t border-theme text-left group transition-colors hover:opacity-80 mt-2"
            >
              <div className="flex items-center gap-3">
                <Settings className="w-4 h-4 text-theme-muted" />
                <span className="text-sm font-medium text-theme">Configure MQTT</span>
              </div>
              <ChevronRight className="w-4 h-4 text-theme-muted group-hover:text-theme transition-colors" />
            </button>
          </div>
        ) : (
          /* Edit mode */
          <div className="space-y-4">
            <div className="flex items-center justify-between py-2 border-b border-theme">
              <span className="text-sm text-theme-muted">Enabled</span>
              <Toggle
                checked={mqttConfig.enabled}
                onChange={(enabled) => setMqttConfig({ ...mqttConfig, enabled })}
              />
            </div>

            {mqttConfig.enabled && (
              <>
                <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
                  <Input
                    label="Broker Address"
                    placeholder="homeassistant.local"
                    value={mqttConfig.broker}
                    onChange={(e) => setMqttConfig({ ...mqttConfig, broker: e.target.value })}
                  />
                  <Input
                    label="Topic Prefix"
                    placeholder="brewos"
                    value={mqttConfig.topic}
                    onChange={(e) => setMqttConfig({ ...mqttConfig, topic: e.target.value })}
                    hint="Topics: {prefix}/status, {prefix}/command"
                  />
                </div>

                <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
                  <Input
                    label="Port"
                    type="number"
                    value={mqttConfig.port}
                    onChange={(e) => setMqttConfig({ ...mqttConfig, port: parseInt(e.target.value) })}
                  />
                  <Input
                    label="Username"
                    placeholder="Optional"
                    value={mqttConfig.username}
                    onChange={(e) => setMqttConfig({ ...mqttConfig, username: e.target.value })}
                  />
                  <Input
                    label="Password"
                    type="password"
                    placeholder="Optional"
                    value={mqttConfig.password}
                    onChange={(e) => setMqttConfig({ ...mqttConfig, password: e.target.value })}
                  />
                </div>

                <label className="flex items-center gap-2 cursor-pointer">
                  <input
                    type="checkbox"
                    checked={mqttConfig.discovery}
                    onChange={(e) => setMqttConfig({ ...mqttConfig, discovery: e.target.checked })}
                    className="w-4 h-4 rounded border-theme text-accent focus:ring-accent"
                  />
                  <span className="text-sm text-theme-secondary">Enable Home Assistant auto-discovery</span>
                </label>
              </>
            )}

            {/* Test Result Feedback */}
            {mqttTestResult && (
              <div className={`p-3 rounded-lg text-sm ${
                mqttTestResult.success 
                  ? 'bg-green-500/10 border border-green-500/20 text-green-600 dark:text-green-400' 
                  : 'bg-red-500/10 border border-red-500/20 text-red-600 dark:text-red-400'
              }`}>
                {mqttTestResult.message}
              </div>
            )}

            <div className="flex justify-end gap-3">
              <Button variant="ghost" onClick={() => {
                setEditingMqtt(false);
                setMqttTestResult(null);
              }}>
                Cancel
              </Button>
              {mqttConfig.enabled && (
                <Button variant="secondary" onClick={testMqtt} loading={testingMqtt}>
                  Test
                </Button>
              )}
              <Button 
                onClick={() => {
                  saveMqtt();
                  setEditingMqtt(false);
                  setMqttTestResult(null);
                }} 
                loading={savingMqtt} 
                disabled={savingMqtt}
              >
                Save
              </Button>
            </div>
          </div>
        )}
      </Card>

      {/* Forget Network Warning Dialog */}
      <ConfirmDialog
        isOpen={showForgetWarning}
        onClose={() => setShowForgetWarning(false)}
        onConfirm={confirmForgetWifi}
        title="Forget Network"
        description="You are about to remove the saved WiFi network."
        variant="danger"
        confirmText="Forget Network"
        cancelText="Cancel"
      >
        <div className="space-y-3 text-sm">
          <div className="flex gap-3 p-3 rounded-lg bg-red-500/10 border border-red-500/20">
            <AlertTriangle className="w-5 h-5 text-red-500 flex-shrink-0 mt-0.5" />
            <div>
              <p className="font-semibold text-red-500">Device Will Restart</p>
              <p className="text-theme-muted mt-1">
                The device will restart in <span className="text-red-500 font-medium">Access Point (AP) mode</span>.
                You will need to reconnect to the device's WiFi hotspot to configure a new network.
              </p>
            </div>
          </div>
          <p className="text-theme-muted text-xs">
            Current network: <span className="font-medium text-theme">{wifi.ssid || 'Unknown'}</span>
          </p>
        </div>
      </ConfirmDialog>
    </>
  );
}

