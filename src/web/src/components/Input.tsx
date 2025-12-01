import { InputHTMLAttributes, forwardRef } from 'react';
import { cn } from '@/lib/utils';

interface InputProps extends InputHTMLAttributes<HTMLInputElement> {
  label?: string;
  hint?: string;
  error?: string;
  unit?: string;
}

export const Input = forwardRef<HTMLInputElement, InputProps>(
  ({ label, hint, error, unit, className, ...props }, ref) => {
    // Calculate right padding based on unit length
    const unitPadding = unit ? (unit.length > 3 ? 'pr-20' : 'pr-12') : '';
    
    return (
      <div className="space-y-1.5">
        {label && (
          <label className="block text-xs font-semibold uppercase tracking-wider text-theme-muted">
            {label}
          </label>
        )}
        <div className="relative flex items-center">
          <input
            ref={ref}
            className={cn(
              'input flex-1',
              unitPadding,
              error && 'border-red-400 focus:border-red-500',
              className
            )}
            {...props}
          />
          {unit && (
            <span className="absolute right-4 top-1/2 -translate-y-1/2 text-sm font-medium text-theme-muted whitespace-nowrap">
              {unit}
            </span>
          )}
        </div>
        {hint && !error && (
          <p className="text-xs text-theme-muted">{hint}</p>
        )}
        {error && (
          <p className="text-xs text-red-500">{error}</p>
        )}
      </div>
    );
  }
);

Input.displayName = 'Input';
