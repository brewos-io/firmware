import { cn } from '@/lib/utils';

interface ToggleProps {
  checked: boolean;
  onChange: (checked: boolean) => void;
  disabled?: boolean;
  label?: string;
}

export function Toggle({ checked, onChange, disabled, label }: ToggleProps) {
  return (
    <label className={cn(
      'inline-flex items-center gap-3 cursor-pointer',
      disabled && 'opacity-50 cursor-not-allowed'
    )}>
      <button
        type="button"
        role="switch"
        aria-checked={checked}
        disabled={disabled}
        onClick={() => !disabled && onChange(!checked)}
        className={cn(
          'relative inline-flex h-6 w-11 shrink-0 rounded-full transition-colors duration-200',
          checked ? 'bg-accent' : 'bg-theme-tertiary'
        )}
      >
        <span
          className={cn(
            'pointer-events-none inline-block h-5 w-5 rounded-full shadow-md transform transition-transform duration-200 mt-0.5',
            checked ? 'translate-x-5 bg-white' : 'translate-x-0.5 bg-theme-card'
          )}
        />
      </button>
      {label && <span className="text-sm font-medium text-theme-secondary">{label}</span>}
    </label>
  );
}
