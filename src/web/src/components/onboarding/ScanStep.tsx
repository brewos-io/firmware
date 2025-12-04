import { Button } from "@/components/Button";
import { Input } from "@/components/Input";
import { QRScanner } from "@/components/QRScanner";
import { ArrowRight, Wifi, QrCode, AlertCircle } from "lucide-react";

interface ScanStepProps {
  deviceName?: string;
  onDeviceNameChange?: (name: string) => void;
  onScan?: (result: string) => void;
  onScanError?: (error: string) => void;
  error?: string;
  onBack?: () => void;
  onAdd?: () => void;
  disabled?: boolean;
  loading?: boolean;
}

export function ScanStep({
  deviceName,
  onDeviceNameChange,
  onScan,
  onScanError,
  error,
  onBack,
  onAdd,
  disabled = false,
  loading = false,
}: ScanStepProps) {
  return (
    <div className="animate-in fade-in slide-in-from-bottom-4 duration-500">
      {/* Header */}
      <div className="text-center mb-6">
        <div className="w-16 h-16 bg-accent/10 rounded-2xl flex items-center justify-center mx-auto mb-4">
          <QrCode className="w-8 h-8 text-accent" />
        </div>
        <h2 className="text-3xl font-bold text-theme mb-2">Scan QR Code</h2>
        <p className="text-theme-muted text-base">
          Point your camera at the QR code on your BrewOS display or web interface
        </p>
      </div>

      {/* QR Scanner */}
      <div className="mb-6">
        {onScan ? (
          <div className="relative">
            <QRScanner
              onScan={onScan}
              onError={onScanError || (() => {})}
            />
            <div className="absolute inset-0 pointer-events-none flex items-center justify-center">
              <div className="w-48 h-48 border-2 border-accent/50 rounded-2xl border-dashed animate-pulse" />
            </div>
          </div>
        ) : (
          <div className="aspect-square max-w-xs mx-auto bg-gradient-to-br from-theme-secondary to-theme-tertiary rounded-2xl flex items-center justify-center border-2 border-dashed border-accent/30 relative overflow-hidden">
            <div className="absolute inset-0 bg-accent/5 animate-pulse" />
            <div className="relative text-center text-theme-muted z-10">
              <QrCode className="w-20 h-20 mx-auto mb-3 opacity-40" />
              <p className="text-sm font-medium">Camera view will appear here</p>
              <p className="text-xs mt-1 opacity-70">Allow camera access when prompted</p>
            </div>
          </div>
        )}
      </div>

      {/* WiFi reminder */}
      <div className="flex items-center gap-2 text-sm text-theme-muted justify-center mb-6 p-3 bg-theme-secondary/50 rounded-xl">
        <Wifi className="w-4 h-4 text-accent" />
        <span>Make sure your machine is connected to WiFi</span>
      </div>

      {/* Form section */}
      <div className="border-t border-theme/20 pt-6 space-y-5">
        <Input
          label="Machine Name (optional)"
          placeholder="e.g., Kitchen Espresso"
          value={deviceName}
          onChange={(e) => onDeviceNameChange?.(e.target.value)}
          hint="Give your machine a friendly name"
        />

        {/* Error message */}
        {error && (
          <div className="p-4 rounded-xl bg-red-500/10 border border-red-500/20 animate-in fade-in slide-in-from-top-2">
            <div className="flex items-start gap-3">
              <AlertCircle className="w-5 h-5 text-red-500 flex-shrink-0 mt-0.5" />
              <div>
                <p className="text-sm font-medium text-red-500 mb-1">Scan Failed</p>
                <p className="text-xs text-theme-muted">{error}</p>
              </div>
            </div>
          </div>
        )}

        {/* Action buttons */}
        <div className="flex gap-3 pt-2">
          <Button variant="secondary" className="flex-1" onClick={onBack}>
            Back
          </Button>
          <Button
            className="flex-1 font-semibold"
            onClick={onAdd}
            disabled={disabled}
            loading={loading}
          >
            {loading ? "Adding..." : "Add Machine"}
            <ArrowRight className="w-4 h-4" />
          </Button>
        </div>
      </div>
    </div>
  );
}

