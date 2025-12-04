import { Button } from "@/components/Button";
import { Input } from "@/components/Input";
import { ArrowRight, KeyRound, AlertCircle, Info } from "lucide-react";

interface ManualStepProps {
  claimCode?: string;
  deviceName?: string;
  onClaimCodeChange?: (code: string) => void;
  onDeviceNameChange?: (name: string) => void;
  error?: string;
  onBack?: () => void;
  onAdd?: () => void;
  disabled?: boolean;
  loading?: boolean;
}

export function ManualStep({
  claimCode,
  deviceName,
  onClaimCodeChange,
  onDeviceNameChange,
  error,
  onBack,
  onAdd,
  disabled = false,
  loading = false,
}: ManualStepProps) {
  const isValidCode = claimCode && claimCode.length >= 8;
  
  return (
    <div className="animate-in fade-in slide-in-from-bottom-4 duration-500">
      {/* Header */}
      <div className="text-center mb-8">
        <div className="w-16 h-16 bg-accent/10 rounded-2xl flex items-center justify-center mx-auto mb-4">
          <KeyRound className="w-8 h-8 text-accent" />
        </div>
        <h2 className="text-3xl font-bold text-theme mb-2">
          Enter Pairing Code
        </h2>
        <p className="text-theme-muted text-base max-w-md mx-auto">
          Find the code on your BrewOS display under <span className="font-semibold text-theme">System → Cloud Access</span>
        </p>
      </div>

      {/* Info box */}
      <div className="mb-6 p-4 bg-accent/5 border border-accent/20 rounded-xl">
        <div className="flex items-start gap-3">
          <Info className="w-5 h-5 text-accent flex-shrink-0 mt-0.5" />
          <div className="flex-1">
            <p className="text-sm font-medium text-theme mb-1">Where to find your code</p>
            <p className="text-xs text-theme-muted leading-relaxed">
              Navigate to your machine's display, go to <span className="font-mono text-accent">System</span> → <span className="font-mono text-accent">Cloud Access</span> to find your 8-character pairing code.
            </p>
          </div>
        </div>
      </div>

      {/* Form */}
      <div className="space-y-5">
        <div>
          <Input
            label="Pairing Code"
            placeholder="X6ST-AP3G"
            value={claimCode}
            onChange={(e) =>
              onClaimCodeChange?.(e.target.value.toUpperCase().replace(/[^A-Z0-9-]/g, ''))
            }
            hint="Enter the 8-character code (letters and numbers)"
            maxLength={9}
            className="text-center text-lg font-mono tracking-wider"
          />
          {isValidCode && !error && (
            <div className="mt-2 flex items-center gap-2 text-xs text-emerald-500">
              <div className="w-1.5 h-1.5 rounded-full bg-emerald-500 animate-pulse" />
              <span>Code format looks good</span>
            </div>
          )}
        </div>

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
                <p className="text-sm font-medium text-red-500 mb-1">Invalid Code</p>
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
            disabled={disabled || !isValidCode}
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

