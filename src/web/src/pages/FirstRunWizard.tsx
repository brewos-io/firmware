import { useState, useEffect } from 'react';
import { Card } from '@/components/Card';
import { Button } from '@/components/Button';
import { Input } from '@/components/Input';
import { getConnection } from '@/lib/connection';
import { 
  Coffee, 
  Settings, 
  Cloud, 
  Check, 
  ArrowRight, 
  ArrowLeft,
  Loader2,
  Copy,
} from 'lucide-react';
import { QRCodeSVG } from 'qrcode.react';

interface WizardStep {
  id: string;
  title: string;
  icon: React.ReactNode;
}

interface PairingData {
  deviceId: string;
  token: string;
  url: string;
  expiresIn: number;
}

const STEPS: WizardStep[] = [
  { id: 'welcome', title: 'Welcome', icon: <Coffee className="w-5 h-5" /> },
  { id: 'machine', title: 'Your Machine', icon: <Settings className="w-5 h-5" /> },
  { id: 'cloud', title: 'Cloud Access', icon: <Cloud className="w-5 h-5" /> },
  { id: 'done', title: 'All Set!', icon: <Check className="w-5 h-5" /> },
];

interface FirstRunWizardProps {
  onComplete: () => void;
}

export function FirstRunWizard({ onComplete }: FirstRunWizardProps) {
  const [currentStep, setCurrentStep] = useState(0);
  const [saving, setSaving] = useState(false);
  
  // Machine info
  const [machineName, setMachineName] = useState('');
  const [machineBrand, setMachineBrand] = useState('');
  const [machineModel, setMachineModel] = useState('');
  
  // Cloud pairing
  const [pairing, setPairing] = useState<PairingData | null>(null);
  const [loadingQR, setLoadingQR] = useState(false);
  const [copied, setCopied] = useState(false);
  
  // Validation
  const [errors, setErrors] = useState<Record<string, string>>({});

  // Fetch pairing QR on cloud step
  useEffect(() => {
    if (STEPS[currentStep].id === 'cloud') {
      fetchPairingQR();
    }
  }, [currentStep]);

  const fetchPairingQR = async () => {
    setLoadingQR(true);
    try {
      const response = await fetch('/api/pairing/qr');
      if (response.ok) {
        const data = await response.json();
        setPairing(data);
      }
    } catch {
      console.log('Failed to fetch pairing QR');
    }
    setLoadingQR(false);
  };

  const copyPairingCode = () => {
    if (pairing) {
      navigator.clipboard.writeText(`${pairing.deviceId}:${pairing.token}`);
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    }
  };

  const validateMachineStep = (): boolean => {
    const newErrors: Record<string, string> = {};
    
    if (!machineName.trim()) {
      newErrors.machineName = 'Please give your machine a name';
    }
    if (!machineBrand.trim()) {
      newErrors.machineBrand = 'Please enter the machine brand';
    }
    if (!machineModel.trim()) {
      newErrors.machineModel = 'Please enter the machine model';
    }
    
    setErrors(newErrors);
    return Object.keys(newErrors).length === 0;
  };

  const saveMachineInfo = async () => {
    setSaving(true);
    try {
      // Send to ESP32 via WebSocket
      getConnection()?.sendCommand('set_device_info', {
        name: machineName,
        machineBrand,
        machineModel,
      });
      
      // Also save via REST API for immediate persistence
      await fetch('/api/settings', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          machineName,
          machineBrand,
          machineModel,
        }),
      });
    } catch (error) {
      console.error('Failed to save machine info:', error);
    }
    setSaving(false);
  };

  const completeSetup = async () => {
    setSaving(true);
    try {
      // Mark setup as complete
      await fetch('/api/setup/complete', { method: 'POST' });
      onComplete();
    } catch {
      // Still complete even if API fails
      onComplete();
    }
    setSaving(false);
  };

  const nextStep = async () => {
    const stepId = STEPS[currentStep].id;
    
    if (stepId === 'machine') {
      if (!validateMachineStep()) return;
      await saveMachineInfo();
    }
    
    if (stepId === 'done') {
      await completeSetup();
      return;
    }
    
    setCurrentStep((prev) => Math.min(prev + 1, STEPS.length - 1));
  };

  const prevStep = () => {
    setCurrentStep((prev) => Math.max(prev - 1, 0));
  };

  const skipCloud = () => {
    setCurrentStep(STEPS.length - 1); // Go to done
  };

  const renderStepContent = () => {
    const stepId = STEPS[currentStep].id;

    switch (stepId) {
      case 'welcome':
        return (
          <div className="text-center py-8">
            <img src="/logo.png" alt="BrewOS" className="h-20 mx-auto mb-6" />
            <h1 className="text-3xl font-bold text-coffee-900 mb-3">
              Welcome to BrewOS
            </h1>
            <p className="text-coffee-500 max-w-md mx-auto mb-8">
              Let's set up your espresso machine. This will only take a minute.
            </p>
            
            <div className="flex flex-col items-center gap-4 text-sm text-coffee-400">
              <div className="flex items-center gap-3">
                <div className="w-8 h-8 rounded-full bg-accent/10 flex items-center justify-center">
                  <Settings className="w-4 h-4 text-accent" />
                </div>
                <span>Name your machine</span>
              </div>
              <div className="flex items-center gap-3">
                <div className="w-8 h-8 rounded-full bg-accent/10 flex items-center justify-center">
                  <Cloud className="w-4 h-4 text-accent" />
                </div>
                <span>Optional: Connect to cloud for remote access</span>
              </div>
            </div>
          </div>
        );

      case 'machine':
        return (
          <div className="py-6">
            <div className="text-center mb-8">
              <div className="w-16 h-16 bg-accent/10 rounded-2xl flex items-center justify-center mx-auto mb-4">
                <Coffee className="w-8 h-8 text-accent" />
              </div>
              <h2 className="text-2xl font-bold text-coffee-900 mb-2">
                About Your Machine
              </h2>
              <p className="text-coffee-500">
                Tell us about your espresso machine
              </p>
            </div>

            <div className="space-y-4 max-w-sm mx-auto">
              <Input
                label="Machine Name"
                placeholder="Kitchen Espresso"
                value={machineName}
                onChange={(e) => setMachineName(e.target.value)}
                error={errors.machineName}
                hint="Give it a friendly name"
              />
              
              <Input
                label="Brand"
                placeholder="ECM"
                value={machineBrand}
                onChange={(e) => setMachineBrand(e.target.value)}
                error={errors.machineBrand}
              />
              
              <Input
                label="Model"
                placeholder="Synchronika"
                value={machineModel}
                onChange={(e) => setMachineModel(e.target.value)}
                error={errors.machineModel}
              />
            </div>
          </div>
        );

      case 'cloud':
        return (
          <div className="py-6">
            <div className="text-center mb-6">
              <div className="w-16 h-16 bg-accent/10 rounded-2xl flex items-center justify-center mx-auto mb-4">
                <Cloud className="w-8 h-8 text-accent" />
              </div>
              <h2 className="text-2xl font-bold text-coffee-900 mb-2">
                Connect to Cloud
              </h2>
              <p className="text-coffee-500">
                Control your machine from anywhere (optional)
              </p>
            </div>

            <div className="bg-cream-100 rounded-2xl p-6 mb-6">
              <div className="flex flex-col items-center">
                {loadingQR ? (
                  <div className="w-48 h-48 flex items-center justify-center">
                    <Loader2 className="w-8 h-8 animate-spin text-accent" />
                  </div>
                ) : pairing ? (
                  <>
                    <div className="bg-white p-3 rounded-xl mb-4">
                      <QRCodeSVG value={pairing.url} size={160} level="M" />
                    </div>
                    <p className="text-sm text-coffee-500 mb-3">
                      Scan with your phone or visit{' '}
                      <a 
                        href="https://cloud.brewos.io" 
                        target="_blank"
                        rel="noopener noreferrer"
                        className="text-accent hover:underline"
                      >
                        cloud.brewos.io
                      </a>
                    </p>
                    
                    {/* Manual code */}
                    <div className="w-full max-w-xs">
                      <div className="flex items-center gap-2">
                        <code className="flex-1 bg-white px-3 py-2 rounded-lg text-xs font-mono text-coffee-600 text-center">
                          {pairing.deviceId}:{pairing.token.substring(0, 8)}...
                        </code>
                        <Button 
                          variant="secondary" 
                          size="sm"
                          onClick={copyPairingCode}
                        >
                          {copied ? <Check className="w-4 h-4" /> : <Copy className="w-4 h-4" />}
                        </Button>
                      </div>
                    </div>
                  </>
                ) : (
                  <p className="text-coffee-400">Could not generate pairing code</p>
                )}
              </div>
            </div>

            <div className="text-center">
              <button 
                onClick={skipCloud}
                className="text-sm text-coffee-400 hover:text-coffee-600 hover:underline"
              >
                Skip for now — you can set this up later
              </button>
            </div>
          </div>
        );

      case 'done':
        return (
          <div className="text-center py-12">
            <div className="w-20 h-20 bg-emerald-100 rounded-full flex items-center justify-center mx-auto mb-6">
              <Check className="w-10 h-10 text-emerald-600" />
            </div>
            <h2 className="text-3xl font-bold text-coffee-900 mb-3">
              You're All Set!
            </h2>
            <p className="text-coffee-500 max-w-sm mx-auto mb-8">
              Your {machineName || 'machine'} is ready to brew. 
              Enjoy your perfect espresso!
            </p>
            
            <div className="bg-cream-100 rounded-xl p-4 max-w-sm mx-auto">
              <h3 className="font-semibold text-coffee-800 mb-2">Quick Tips:</h3>
              <ul className="text-sm text-coffee-500 text-left space-y-1">
                <li>• Access locally at <span className="font-mono text-accent">brewos.local</span></li>
                <li>• Adjust brew temperature in Settings</li>
                <li>• Connect a scale for brew-by-weight</li>
              </ul>
            </div>
          </div>
        );
    }
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-coffee-800 via-coffee-900 to-coffee-950 flex items-center justify-center p-4">
      <div className="w-full max-w-xl">
        {/* Progress indicator */}
        <div className="flex justify-center mb-6">
          <div className="flex items-center gap-2">
            {STEPS.map((step, index) => (
              <div key={step.id} className="flex items-center">
                <div 
                  className={`w-10 h-10 rounded-full flex items-center justify-center transition-colors ${
                    index < currentStep 
                      ? 'bg-emerald-500 text-white' 
                      : index === currentStep 
                        ? 'bg-accent text-white' 
                        : 'bg-cream-700/30 text-cream-400'
                  }`}
                >
                  {index < currentStep ? (
                    <Check className="w-5 h-5" />
                  ) : (
                    step.icon
                  )}
                </div>
                {index < STEPS.length - 1 && (
                  <div 
                    className={`w-8 h-0.5 mx-1 transition-colors ${
                      index < currentStep ? 'bg-emerald-500' : 'bg-cream-700/30'
                    }`}
                  />
                )}
              </div>
            ))}
          </div>
        </div>

        {/* Card */}
        <Card>
          {renderStepContent()}

          {/* Navigation buttons */}
          <div className="flex justify-between pt-6 border-t border-cream-200 mt-6">
            {currentStep > 0 && STEPS[currentStep].id !== 'done' ? (
              <Button variant="ghost" onClick={prevStep}>
                <ArrowLeft className="w-4 h-4" />
                Back
              </Button>
            ) : (
              <div />
            )}
            
            <Button onClick={nextStep} loading={saving}>
              {STEPS[currentStep].id === 'done' ? (
                'Start Brewing'
              ) : STEPS[currentStep].id === 'cloud' ? (
                <>
                  Continue
                  <ArrowRight className="w-4 h-4" />
                </>
              ) : (
                <>
                  Next
                  <ArrowRight className="w-4 h-4" />
                </>
              )}
            </Button>
          </div>
        </Card>
      </div>
    </div>
  );
}

