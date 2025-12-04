import { Check, Sparkles, Coffee, Settings, Scale, Globe } from "lucide-react";

interface DoneStepProps {
  machineName: string;
}

export function DoneStep({ machineName }: DoneStepProps) {
  const tips = [
    {
      icon: Globe,
      text: "Access locally at",
      highlight: "brewos.local",
    },
    {
      icon: Settings,
      text: "Adjust brew temperature in Settings",
    },
    {
      icon: Scale,
      text: "Connect a scale for brew-by-weight",
    },
  ];

  return (
    <div className="text-center py-4 sm:py-12 animate-in fade-in zoom-in-95 duration-700">
      {/* Success icon with animation */}
      <div className="relative mb-4 sm:mb-8">
        <div className="absolute inset-0 flex items-center justify-center">
          <div className="w-24 h-24 sm:w-40 sm:h-40 bg-success/20 rounded-full animate-ping opacity-75" />
        </div>
        <div className="relative w-16 h-16 sm:w-28 sm:h-28 bg-gradient-to-br from-success/20 to-success/10 rounded-full flex items-center justify-center mx-auto border-4 border-success/30">
          <div className="w-11 h-11 sm:w-20 sm:h-20 bg-success rounded-full flex items-center justify-center shadow-lg">
            <Check className="w-6 h-6 sm:w-12 sm:h-12 text-white animate-in zoom-in duration-500 delay-300" />
          </div>
        </div>
      </div>

      {/* Success message */}
      <div className="mb-4 sm:mb-8">
        <div className="inline-flex items-center gap-1.5 sm:gap-2 mb-1.5 sm:mb-3">
          <Sparkles className="w-4 h-4 sm:w-6 sm:h-6 text-accent animate-pulse" />
          <h2 className="text-xl sm:text-4xl font-bold text-theme">You're All Set!</h2>
        </div>
        <p className="text-theme-muted text-xs sm:text-lg max-w-md mx-auto leading-relaxed">
          Your <span className="font-semibold text-theme">{machineName || "machine"}</span> is ready to brew. Enjoy your perfect espresso!
        </p>
      </div>

      {/* Quick tips */}
      <div className="max-w-md mx-auto">
        <div className="bg-white/5 sm:bg-theme-secondary rounded-lg sm:rounded-xl p-3 sm:p-6 border border-white/10 sm:border-theme/20 shadow-lg">
          <div className="flex items-center gap-1.5 sm:gap-2 mb-2 sm:mb-4 justify-center">
            <Coffee className="w-4 h-4 sm:w-5 sm:h-5 text-accent" />
            <h3 className="font-semibold text-theme text-sm sm:text-base">Quick Tips</h3>
          </div>
          <ul className="text-[10px] sm:text-sm text-theme-muted text-left space-y-2 sm:space-y-3">
            {tips.map((tip, index) => {
              const Icon = tip.icon;
              return (
                <li
                  key={index}
                  className="flex items-start gap-2 sm:gap-3 animate-in fade-in slide-in-from-left-4"
                  style={{ animationDelay: `${index * 100}ms` }}
                >
                  <Icon className="w-3.5 h-3.5 sm:w-4 sm:h-4 text-accent flex-shrink-0 mt-0.5" />
                  <span>
                    {tip.text}{" "}
                    {tip.highlight && (
                      <span className="font-mono text-accent font-semibold">
                        {tip.highlight}
                      </span>
                    )}
                  </span>
                </li>
              );
            })}
          </ul>
        </div>
      </div>
    </div>
  );
}

