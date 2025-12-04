import { Check } from "lucide-react";
import type { WizardStep } from "./types";

interface ProgressIndicatorProps {
  steps: WizardStep[];
  currentStep: number;
}

export function ProgressIndicator({ steps, currentStep }: ProgressIndicatorProps) {
  return (
    <div className="flex justify-center mb-3 sm:mb-6">
      <div className="flex items-center gap-1 sm:gap-2">
        {steps.map((step, index) => (
          <div key={step.id} className="flex items-center">
            <div
              className={`w-7 h-7 sm:w-10 sm:h-10 rounded-full flex items-center justify-center transition-colors ${
                index < currentStep
                  ? "bg-emerald-500 text-white"
                  : index === currentStep
                  ? "bg-accent text-white"
                  : "bg-white/10 sm:bg-theme-tertiary text-white/50 sm:text-theme-muted"
              }`}
            >
              {index < currentStep ? (
                <Check className="w-3.5 h-3.5 sm:w-5 sm:h-5" />
              ) : (
                <span className="[&>svg]:w-3.5 [&>svg]:h-3.5 sm:[&>svg]:w-5 sm:[&>svg]:h-5">
                  {step.icon}
                </span>
              )}
            </div>
            {index < steps.length - 1 && (
              <div
                className={`w-4 sm:w-8 h-0.5 mx-0.5 sm:mx-1 transition-colors ${
                  index < currentStep ? "bg-emerald-500" : "bg-white/10 sm:bg-theme-tertiary"
                }`}
              />
            )}
          </div>
        ))}
      </div>
    </div>
  );
}

