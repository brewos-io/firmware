import { Coffee, QrCode, Wifi, ArrowRight } from "lucide-react";

export function HowItWorks() {
  const steps = [
    {
      icon: Wifi,
      title: "Connect to WiFi",
      description: "Ensure your machine is on the same network",
    },
    {
      icon: QrCode,
      title: "Scan QR Code",
      description: "Find it on your machine's display",
    },
    {
      icon: Coffee,
      title: "Start Brewing",
      description: "Control from anywhere",
    },
  ];

  return (
    <div className="mt-8">
      <div className="text-center mb-6">
        <h3 className="text-sm font-semibold uppercase tracking-wider text-cream-300 mb-1">
          How it works
        </h3>
        <div className="w-12 h-0.5 bg-accent/30 mx-auto" />
      </div>
      <div className="grid grid-cols-3 gap-4">
        {steps.map((step, index) => {
          const Icon = step.icon;
          return (
            <div key={index} className="flex flex-col items-center gap-3">
              <div className="relative">
                <div className="w-12 h-12 bg-cream-800/20 backdrop-blur-sm rounded-full flex items-center justify-center border border-cream-700/30 shadow-lg">
                  <Icon className="w-6 h-6 text-cream-300" />
                </div>
                {index < steps.length - 1 && (
                  <div className="hidden md:block absolute top-1/2 left-full w-full -translate-y-1/2 translate-x-2">
                    <ArrowRight className="w-4 h-4 text-cream-500/50" />
                  </div>
                )}
              </div>
              <div className="text-center">
                <p className="text-xs font-semibold text-cream-200 mb-1">
                  {step.title}
                </p>
                <p className="text-xs text-cream-400/80 leading-tight">
                  {step.description}
                </p>
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
}

