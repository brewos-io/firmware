import { Button } from "@/components/Button";
import { Logo } from "@/components/Logo";
import { QrCode, Sparkles, ArrowRight, Coffee, Wifi } from "lucide-react";

interface WelcomeStepProps {
  onScanClick?: () => void;
  onManualClick?: () => void;
}

export function WelcomeStep({ onScanClick, onManualClick }: WelcomeStepProps) {
  const steps = [
    {
      icon: Wifi,
      title: "Connect to WiFi",
      description: "Same network",
    },
    {
      icon: QrCode,
      title: "Scan QR Code",
      description: "On display",
    },
    {
      icon: Coffee,
      title: "Start Brewing",
      description: "Control anywhere",
    },
  ];

  return (
    <div className="py-8 animate-in fade-in slide-in-from-bottom-4 duration-500">
      {/* Logo with subtle animation */}
      <div className="flex justify-center mb-8">
        <div className="relative">
          <div className="absolute inset-0 bg-accent/20 rounded-full blur-2xl animate-pulse" />
          <div className="relative">
            <Logo size="lg" />
          </div>
        </div>
      </div>

      {/* Welcome heading */}
      <div className="text-center mb-8">
        <div className="inline-flex items-center gap-2 mb-3">
          <Sparkles className="w-5 h-5 text-accent animate-pulse" />
          <h1 className="text-4xl font-bold text-theme tracking-tight">
            Welcome to BrewOS
          </h1>
        </div>
        <p className="text-theme-muted text-lg max-w-md mx-auto leading-relaxed">
          Control your espresso machine from anywhere. Let's add your first
          machine in just a few moments.
        </p>
      </div>

      {/* Action buttons */}
      <div className="space-y-4 max-w-sm mx-auto mb-10">
        <Button
          className="w-full py-5 text-lg font-semibold shadow-lg hover:shadow-xl transition-all duration-300 hover:scale-[1.02]"
          onClick={onScanClick}
        >
          <QrCode className="w-5 h-5" />
          Scan QR Code
          <ArrowRight className="w-5 h-5 ml-auto" />
        </Button>

        <div className="relative">
          <div className="absolute inset-0 flex items-center">
            <div className="w-full border-t border-theme/20"></div>
          </div>
          <div className="relative flex justify-center text-xs uppercase">
            <span className="bg-theme-card px-3 text-theme-muted">or</span>
          </div>
        </div>

        <button
          className="w-full py-3 text-theme-muted hover:text-theme transition-colors duration-200 font-medium"
          onClick={onManualClick}
        >
          Enter code manually
        </button>
      </div>

      {/* How it works - integrated */}
      <div className="border-t border-theme/10 pt-8 mt-8">
        <div className="text-center mb-6">
          <h3 className="text-xs font-semibold uppercase tracking-wider text-theme-muted mb-2">
            How it works
          </h3>
        </div>
        <div className="grid grid-cols-3 gap-4 max-w-md mx-auto">
          {steps.map((step, index) => {
            const Icon = step.icon;
            return (
              <div
                key={index}
                className="flex flex-col items-center gap-2 animate-in fade-in slide-in-from-bottom-4"
                style={{ animationDelay: `${index * 100}ms` }}
              >
                <div className="w-12 h-12 bg-accent/10 rounded-xl flex items-center justify-center border border-accent/20 shadow-sm">
                  <Icon className="w-6 h-6 text-accent" />
                </div>
                <div className="text-center">
                  <p className="text-xs font-semibold text-theme mb-0.5">
                    {step.title}
                  </p>
                  <p className="text-xs text-theme-muted leading-tight">
                    {step.description}
                  </p>
                </div>
              </div>
            );
          })}
        </div>
      </div>

      {/* Quick tip */}
      <div className="mt-6 text-center">
        <p className="text-xs text-theme-muted">
          💡 Tip: Find the QR code on your machine's display or web interface
        </p>
      </div>
    </div>
  );
}
