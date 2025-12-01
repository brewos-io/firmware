import { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { Card, CardHeader, CardTitle } from '@/components/Card';
import { Button } from '@/components/Button';
import { Input } from '@/components/Input';
import { Badge } from '@/components/Badge';
import { Logo } from '@/components/Logo';
import { useAppStore } from '@/lib/mode';
import {
  Coffee,
  Plus,
  Trash2,
  Edit2,
  Wifi,
  WifiOff,
  QrCode,
  X,
  Check,
  Loader2,
  LogOut,
  ExternalLink,
} from 'lucide-react';

export function Devices() {
  const navigate = useNavigate();
  const { 
    user, 
    signOut, 
    authLoading, 
    devices, 
    devicesLoading, 
    fetchDevices, 
    claimDevice, 
    removeDevice, 
    renameDevice,
    selectDevice,
  } = useAppStore();
  
  const [showAddModal, setShowAddModal] = useState(false);
  const [claimCode, setClaimCode] = useState('');
  const [deviceName, setDeviceName] = useState('');
  const [claiming, setClaiming] = useState(false);
  const [claimError, setClaimError] = useState('');
  
  const [editingDevice, setEditingDevice] = useState<string | null>(null);
  const [editName, setEditName] = useState('');

  useEffect(() => {
    if (!authLoading && !user) {
      navigate('/login');
    } else if (user) {
      fetchDevices();
    }
  }, [user, authLoading, navigate, fetchDevices]);

  const handleClaim = async () => {
    if (!claimCode) return;
    
    setClaiming(true);
    setClaimError('');
    
    try {
      // Parse QR code URL or manual entry
      let deviceId = '';
      let token = '';
      
      if (claimCode.includes('?')) {
        // URL format
        const url = new URL(claimCode);
        deviceId = url.searchParams.get('id') || '';
        token = url.searchParams.get('token') || '';
      } else {
        // Manual format: DEVICE_ID:TOKEN
        const parts = claimCode.split(':');
        deviceId = parts[0];
        token = parts[1] || '';
      }
      
      if (!deviceId || !token) {
        setClaimError('Invalid code format');
        setClaiming(false);
        return;
      }
      
      const success = await claimDevice(deviceId, token, deviceName || undefined);
      
      if (success) {
        setShowAddModal(false);
        setClaimCode('');
        setDeviceName('');
      } else {
        setClaimError('Failed to claim device. Token may be expired.');
      }
    } catch (error) {
      setClaimError('An error occurred');
    }
    
    setClaiming(false);
  };

  const handleRemove = async (deviceId: string) => {
    if (confirm('Remove this device from your account?')) {
      await removeDevice(deviceId);
    }
  };

  const handleRename = async (deviceId: string) => {
    if (editName.trim()) {
      await renameDevice(deviceId, editName.trim());
    }
    setEditingDevice(null);
    setEditName('');
  };

  const handleLogout = async () => {
    await signOut();
    navigate('/login');
  };

  if (authLoading) {
    return (
      <div className="min-h-screen bg-cream-100 flex items-center justify-center">
        <Loader2 className="w-8 h-8 animate-spin text-accent" />
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-cream-100">
      {/* Header */}
      <header className="bg-theme-card border-b border-theme sticky top-0 z-50">
        <div className="max-w-4xl mx-auto px-4 py-4 flex items-center justify-between">
          <Logo size="md" />
          <div className="flex items-center gap-4">
            <span className="text-sm text-coffee-600">{user?.email}</span>
            <Button variant="ghost" size="sm" onClick={handleLogout}>
              <LogOut className="w-4 h-4" />
            </Button>
          </div>
        </div>
      </header>

      {/* Content */}
      <main className="max-w-4xl mx-auto px-4 py-8">
        <div className="flex items-center justify-between mb-6">
          <h1 className="text-2xl font-bold text-coffee-900">My Devices</h1>
          <Button onClick={() => setShowAddModal(true)}>
            <Plus className="w-4 h-4" />
            Add Device
          </Button>
        </div>

        {devicesLoading ? (
          <div className="flex justify-center py-12">
            <Loader2 className="w-8 h-8 animate-spin text-accent" />
          </div>
        ) : devices.length === 0 ? (
          <Card className="text-center py-12">
            <Coffee className="w-16 h-16 text-coffee-300 mx-auto mb-4" />
            <h2 className="text-xl font-bold text-coffee-900 mb-2">No devices yet</h2>
            <p className="text-coffee-500 mb-6">
              Scan the QR code on your BrewOS display to add your first device.
            </p>
            <Button onClick={() => setShowAddModal(true)}>
              <QrCode className="w-4 h-4" />
              Add Device
            </Button>
          </Card>
        ) : (
          <div className="space-y-4">
            {devices.map((device) => (
              <Card key={device.id} className="flex items-center justify-between">
                <div className="flex items-center gap-4">
                  <div className={`w-12 h-12 rounded-xl flex items-center justify-center ${
                    device.isOnline ? 'bg-emerald-100' : 'bg-cream-200'
                  }`}>
                    {device.isOnline ? (
                      <Wifi className="w-6 h-6 text-emerald-600" />
                    ) : (
                      <WifiOff className="w-6 h-6 text-coffee-400" />
                    )}
                  </div>
                  <div>
                    {editingDevice === device.id ? (
                      <div className="flex items-center gap-2">
                        <Input
                          value={editName}
                          onChange={(e) => setEditName(e.target.value)}
                          className="w-48"
                          autoFocus
                          onKeyDown={(e) => {
                            if (e.key === 'Enter') handleRename(device.id);
                            if (e.key === 'Escape') setEditingDevice(null);
                          }}
                        />
                        <Button size="sm" onClick={() => handleRename(device.id)}>
                          <Check className="w-4 h-4" />
                        </Button>
                        <Button size="sm" variant="ghost" onClick={() => setEditingDevice(null)}>
                          <X className="w-4 h-4" />
                        </Button>
                      </div>
                    ) : (
                      <>
                        <h3 className="font-semibold text-coffee-900">{device.name}</h3>
                        <p className="text-sm text-coffee-500">
                          {device.id}
                          {device.firmwareVersion && ` • v${device.firmwareVersion}`}
                        </p>
                      </>
                    )}
                  </div>
                </div>
                
                <div className="flex items-center gap-2">
                  <Badge variant={device.isOnline ? 'success' : 'default'}>
                    {device.isOnline ? 'Online' : 'Offline'}
                  </Badge>
                  <Button
                    size="sm"
                    onClick={() => {
                      selectDevice(device.id);
                      navigate(`/device/${device.id}`);
                    }}
                  >
                    <ExternalLink className="w-4 h-4" />
                    Connect
                  </Button>
                  <Button
                    variant="ghost"
                    size="sm"
                    onClick={() => {
                      setEditingDevice(device.id);
                      setEditName(device.name);
                    }}
                  >
                    <Edit2 className="w-4 h-4" />
                  </Button>
                  <Button
                    variant="ghost"
                    size="sm"
                    onClick={() => handleRemove(device.id)}
                  >
                    <Trash2 className="w-4 h-4 text-red-500" />
                  </Button>
                </div>
              </Card>
            ))}
          </div>
        )}
      </main>

      {/* Add Device Modal */}
      {showAddModal && (
        <div className="fixed inset-0 bg-black/50 flex items-center justify-center p-4 z-50">
          <Card className="w-full max-w-md">
            <CardHeader>
              <CardTitle>Add Device</CardTitle>
              <Button variant="ghost" size="sm" onClick={() => setShowAddModal(false)}>
                <X className="w-4 h-4" />
              </Button>
            </CardHeader>

            <div className="space-y-4">
              <div className="bg-cream-100 rounded-xl p-4 text-center">
                <QrCode className="w-12 h-12 text-accent mx-auto mb-2" />
                <p className="text-sm text-coffee-600">
                  Scan the QR code on your BrewOS display, or enter the code manually below.
                </p>
              </div>

              <Input
                label="Pairing Code"
                placeholder="Paste URL or enter DEVICE_ID:TOKEN"
                value={claimCode}
                onChange={(e) => setClaimCode(e.target.value)}
              />

              <Input
                label="Device Name (optional)"
                placeholder="Kitchen Espresso"
                value={deviceName}
                onChange={(e) => setDeviceName(e.target.value)}
              />

              {claimError && (
                <p className="text-sm text-red-600">{claimError}</p>
              )}

              <div className="flex gap-3">
                <Button
                  variant="secondary"
                  className="flex-1"
                  onClick={() => setShowAddModal(false)}
                >
                  Cancel
                </Button>
                <Button
                  className="flex-1"
                  onClick={handleClaim}
                  loading={claiming}
                  disabled={!claimCode}
                >
                  Add Device
                </Button>
              </div>
            </div>
          </Card>
        </div>
      )}
    </div>
  );
}

