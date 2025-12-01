import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { Card } from '@/components/Card';
import { Button } from '@/components/Button';
import { Input } from '@/components/Input';
import { Logo } from '@/components/Logo';
import { useAppStore } from '@/lib/mode';
import { 
  Coffee, 
  QrCode, 
  Wifi, 
  Check, 
  ArrowRight,
  Smartphone,
  Loader2,
} from 'lucide-react';

export function Onboarding() {
  const navigate = useNavigate();
  const { claimDevice, fetchDevices } = useAppStore();
  
  const [step, setStep] = useState<'welcome' | 'scan' | 'manual' | 'success'>('welcome');
  const [claimCode, setClaimCode] = useState('');
  const [deviceName, setDeviceName] = useState('');
  const [claiming, setClaiming] = useState(false);
  const [error, setError] = useState('');

  const handleClaim = async () => {
    if (!claimCode) return;
    
    setClaiming(true);
    setError('');
    
    try {
      // Parse QR code URL or manual entry
      let deviceId = '';
      let token = '';
      
      if (claimCode.includes('?')) {
        // URL format
        try {
          const url = new URL(claimCode);
          deviceId = url.searchParams.get('id') || '';
          token = url.searchParams.get('token') || '';
        } catch {
          // Try parsing as query string
          const params = new URLSearchParams(claimCode.split('?')[1]);
          deviceId = params.get('id') || '';
          token = params.get('token') || '';
        }
      } else if (claimCode.includes(':')) {
        // Manual format: DEVICE_ID:TOKEN
        const parts = claimCode.split(':');
        deviceId = parts[0];
        token = parts[1] || '';
      } else {
        setError('Invalid code format. Scan QR or enter DEVICE_ID:TOKEN');
        setClaiming(false);
        return;
      }
      
      if (!deviceId || !token) {
        setError('Invalid code format');
        setClaiming(false);
        return;
      }
      
      const success = await claimDevice(deviceId, token, deviceName || undefined);
      
      if (success) {
        setStep('success');
        await fetchDevices();
        setTimeout(() => navigate('/'), 2000);
      } else {
        setError('Failed to add device. The code may have expired.');
      }
    } catch {
      setError('An error occurred');
    }
    
    setClaiming(false);
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-coffee-800 via-coffee-900 to-coffee-950 flex items-center justify-center p-4">
      <div className="w-full max-w-lg">
        {step === 'welcome' && (
          <Card className="text-center">
            <div className="py-8">
              <div className="flex justify-center mb-6">
                <Logo size="lg" />
              </div>
              
              <h1 className="text-3xl font-bold text-theme mb-2">
                Welcome to BrewOS
              </h1>
              <p className="text-theme-muted mb-8 max-w-sm mx-auto">
                Control your espresso machine from anywhere. Let's add your first machine.
              </p>

              <div className="space-y-4">
                <Button 
                  className="w-full py-4 text-lg"
                  onClick={() => setStep('scan')}
                >
                  <QrCode className="w-5 h-5" />
                  Scan QR Code
                </Button>
                
                <p className="text-sm text-coffee-400">
                  or{' '}
                  <button 
                    className="text-accent hover:underline"
                    onClick={() => setStep('manual')}
                  >
                    enter code manually
                  </button>
                </p>
              </div>
            </div>
          </Card>
        )}

        {step === 'scan' && (
          <Card>
            <div className="text-center mb-6">
              <h2 className="text-2xl font-bold text-coffee-900 mb-2">
                Scan QR Code
              </h2>
              <p className="text-coffee-500">
                Find the QR code on your BrewOS display or web interface.
              </p>
            </div>

            <div className="bg-cream-100 rounded-2xl p-8 mb-6">
              <div className="flex flex-col items-center gap-4">
                <div className="w-48 h-48 bg-white rounded-xl flex items-center justify-center border-4 border-dashed border-cream-300">
                  <div className="text-center text-coffee-400">
                    <Smartphone className="w-12 h-12 mx-auto mb-2 opacity-50" />
                    <p className="text-sm">Point camera at QR</p>
                  </div>
                </div>
                
                <div className="flex items-center gap-2 text-sm text-coffee-500">
                  <Wifi className="w-4 h-4" />
                  <span>Make sure your machine is connected to WiFi</span>
                </div>
              </div>
            </div>

            <div className="space-y-4">
              <Input
                label="Or paste the pairing URL"
                placeholder="https://brewos.app/pair?id=BRW-..."
                value={claimCode}
                onChange={(e) => setClaimCode(e.target.value)}
              />

              <Input
                label="Machine Name (optional)"
                placeholder="Kitchen Espresso"
                value={deviceName}
                onChange={(e) => setDeviceName(e.target.value)}
              />

              {error && (
                <p className="text-sm text-red-600 bg-red-50 p-3 rounded-lg">{error}</p>
              )}

              <div className="flex gap-3">
                <Button 
                  variant="secondary" 
                  className="flex-1"
                  onClick={() => setStep('welcome')}
                >
                  Back
                </Button>
                <Button 
                  className="flex-1"
                  onClick={handleClaim}
                  loading={claiming}
                  disabled={!claimCode}
                >
                  Add Machine
                  <ArrowRight className="w-4 h-4" />
                </Button>
              </div>
            </div>
          </Card>
        )}

        {step === 'manual' && (
          <Card>
            <div className="text-center mb-6">
              <h2 className="text-2xl font-bold text-coffee-900 mb-2">
                Enter Pairing Code
              </h2>
              <p className="text-coffee-500">
                Find the code on your BrewOS display under System → Cloud Access.
              </p>
            </div>

            <div className="space-y-4">
              <Input
                label="Pairing Code"
                placeholder="BRW-XXXXXXXX:token..."
                value={claimCode}
                onChange={(e) => setClaimCode(e.target.value)}
                hint="Format: DEVICE_ID:TOKEN or full URL"
              />

              <Input
                label="Machine Name (optional)"
                placeholder="Kitchen Espresso"
                value={deviceName}
                onChange={(e) => setDeviceName(e.target.value)}
              />

              {error && (
                <p className="text-sm text-red-600 bg-red-50 p-3 rounded-lg">{error}</p>
              )}

              <div className="flex gap-3">
                <Button 
                  variant="secondary" 
                  className="flex-1"
                  onClick={() => setStep('welcome')}
                >
                  Back
                </Button>
                <Button 
                  className="flex-1"
                  onClick={handleClaim}
                  loading={claiming}
                  disabled={!claimCode}
                >
                  Add Machine
                  <ArrowRight className="w-4 h-4" />
                </Button>
              </div>
            </div>
          </Card>
        )}

        {step === 'success' && (
          <Card className="text-center py-12">
            <div className="w-16 h-16 bg-emerald-100 rounded-full flex items-center justify-center mx-auto mb-4">
              <Check className="w-8 h-8 text-emerald-600" />
            </div>
            <h2 className="text-2xl font-bold text-coffee-900 mb-2">
              Machine Added!
            </h2>
            <p className="text-coffee-500 mb-4">
              {deviceName || 'Your machine'} is now connected to your account.
            </p>
            <div className="flex items-center justify-center gap-2 text-sm text-coffee-400">
              <Loader2 className="w-4 h-4 animate-spin" />
              Redirecting to dashboard...
            </div>
          </Card>
        )}

        {/* How it works */}
        {(step === 'welcome' || step === 'scan') && (
          <div className="mt-8 text-center">
            <h3 className="text-sm font-semibold text-cream-300 mb-4">How it works</h3>
            <div className="grid grid-cols-3 gap-4 text-cream-400 text-xs">
              <div className="flex flex-col items-center gap-2">
                <div className="w-10 h-10 bg-cream-800/30 rounded-full flex items-center justify-center">
                  <Wifi className="w-5 h-5" />
                </div>
                <span>Connect machine to WiFi</span>
              </div>
              <div className="flex flex-col items-center gap-2">
                <div className="w-10 h-10 bg-cream-800/30 rounded-full flex items-center justify-center">
                  <QrCode className="w-5 h-5" />
                </div>
                <span>Scan QR code on display</span>
              </div>
              <div className="flex flex-col items-center gap-2">
                <div className="w-10 h-10 bg-cream-800/30 rounded-full flex items-center justify-center">
                  <Coffee className="w-5 h-5" />
                </div>
                <span>Control from anywhere</span>
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}

