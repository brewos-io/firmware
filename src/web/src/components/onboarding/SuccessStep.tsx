import { Check, Loader2, Sparkles, Coffee } from "lucide-react";

interface SuccessStepProps {
  deviceName?: string;
}

export function SuccessStep({ deviceName }: SuccessStepProps) {
  return (
    <div className="text-center py-4 sm:py-10 animate-in fade-in zoom-in-95 duration-700">
      {/* Success icon with animation */}
      <div className="relative mb-4 sm:mb-8">
        <div className="absolute inset-0 flex items-center justify-center">
          <div className="w-20 h-20 sm:w-32 sm:h-32 bg-success/20 rounded-full animate-ping opacity-75" />
        </div>
        <div className="relative w-16 h-16 sm:w-24 sm:h-24 bg-gradient-to-br from-success/20 to-success/10 rounded-full flex items-center justify-center mx-auto border-4 border-success/30">
          <div className="w-11 h-11 sm:w-16 sm:h-16 bg-success rounded-full flex items-center justify-center shadow-lg">
            <Check className="w-6 h-6 sm:w-10 sm:h-10 text-white animate-in zoom-in duration-500 delay-300" />
          </div>
        </div>
      </div>

      {/* Success message */}
      <div className="mb-3 sm:mb-6">
        <div className="inline-flex items-center gap-1.5 sm:gap-2 mb-1.5 sm:mb-3">
          <Sparkles className="w-4 h-4 sm:w-6 sm:h-6 text-accent animate-pulse" />
          <h2 className="text-xl sm:text-3xl font-bold text-theme">Machine Added!</h2>
        </div>
        <p className="text-theme-muted text-xs sm:text-base max-w-md mx-auto leading-relaxed">
          <span className="font-semibold text-theme">{deviceName || "Your machine"}</span> is now connected and ready to brew.
        </p>
      </div>

      {/* Celebration details */}
      <div className="max-w-sm mx-auto mb-4 sm:mb-8">
        <div className="bg-white/5 sm:bg-theme-secondary rounded-lg sm:rounded-xl p-3 sm:p-5 border border-white/10 sm:border-theme/20">
          <div className="flex items-center justify-center gap-1.5 sm:gap-2 mb-1.5 sm:mb-3">
            <Coffee className="w-4 h-4 sm:w-5 sm:h-5 text-accent" />
            <span className="font-semibold text-xs sm:text-base text-theme">You're all set!</span>
          </div>
          <p className="text-[10px] sm:text-sm text-theme-muted leading-relaxed">
            Your machine is now accessible and ready to create the perfect espresso.
          </p>
        </div>
      </div>

      {/* Loading indicator */}
      <div className="flex items-center justify-center gap-2 text-[10px] sm:text-sm text-theme-muted animate-in fade-in delay-500">
        <Loader2 className="w-3.5 h-3.5 sm:w-4 sm:h-4 animate-spin" />
        <span>Redirecting to dashboard...</span>
      </div>
    </div>
  );
}
