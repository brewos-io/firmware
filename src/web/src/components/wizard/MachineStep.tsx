import { useMemo } from "react";
import { Input } from "@/components/Input";
import { Coffee, ChevronDown } from "lucide-react";
import { cn } from "@/lib/utils";
import {
  getMachinesGroupedByBrand,
  getMachineById,
  getMachineTypeLabel,
  type MachineDefinition,
} from "@/lib/machines";
import { formatTemperatureWithUnit } from "@/lib/temperature";
import { useStore } from "@/lib/store";

interface MachineStepProps {
  machineName: string;
  selectedMachineId: string;
  errors: Record<string, string>;
  onMachineNameChange: (name: string) => void;
  onMachineIdChange: (id: string) => void;
}

export function MachineStep({
  machineName,
  selectedMachineId,
  errors,
  onMachineNameChange,
  onMachineIdChange,
}: MachineStepProps) {
  const temperatureUnit = useStore((s) => s.preferences.temperatureUnit);
  const machineGroups = useMemo(() => getMachinesGroupedByBrand(), []);
  const selectedMachine = useMemo(
    () => (selectedMachineId ? getMachineById(selectedMachineId) : undefined),
    [selectedMachineId]
  );

  return (
    <div className="py-3 sm:py-6">
      <div className="text-center mb-4 sm:mb-8">
        <div className="w-12 h-12 sm:w-16 sm:h-16 bg-accent/10 rounded-xl sm:rounded-2xl flex items-center justify-center mx-auto mb-2 sm:mb-4">
          <Coffee className="w-6 h-6 sm:w-8 sm:h-8 text-accent" />
        </div>
        <h2 className="text-xl sm:text-2xl font-bold text-theme mb-1 sm:mb-2">Select Your Machine</h2>
        <p className="text-theme-muted text-xs sm:text-base">Choose your espresso machine from our supported list</p>
      </div>

      <div className="space-y-3 sm:space-y-4 max-w-sm mx-auto">
        <Input
          label="Machine Name"
          placeholder="Kitchen Espresso"
          value={machineName}
          onChange={(e) => onMachineNameChange(e.target.value)}
          error={errors.machineName}
          hint="Give it a friendly name"
        />

        <MachineSelector
          selectedMachineId={selectedMachineId}
          machineGroups={machineGroups}
          error={errors.machineModel}
          onChange={onMachineIdChange}
        />

        {selectedMachine && (
          <MachineInfo machine={selectedMachine} temperatureUnit={temperatureUnit} />
        )}
      </div>
    </div>
  );
}

interface MachineSelectorProps {
  selectedMachineId: string;
  machineGroups: ReturnType<typeof getMachinesGroupedByBrand>;
  error?: string;
  onChange: (id: string) => void;
}

function MachineSelector({
  selectedMachineId,
  machineGroups,
  error,
  onChange,
}: MachineSelectorProps) {
  return (
    <div className="space-y-1.5 sm:space-y-2">
      <label className="block text-xs sm:text-sm font-medium text-theme">
        Machine Model <span className="text-red-500">*</span>
      </label>
      <div className="relative">
        <select
          value={selectedMachineId}
          onChange={(e) => onChange(e.target.value)}
          className={cn(
            "w-full px-3 sm:px-4 py-2.5 sm:py-3 pr-10 rounded-lg sm:rounded-xl appearance-none",
            "bg-white/5 sm:bg-theme-secondary border border-white/10 sm:border-theme",
            "text-theme text-xs sm:text-sm",
            "focus:outline-none focus:ring-2 focus:ring-accent focus:border-transparent",
            "transition-all duration-200",
            !selectedMachineId && "text-theme-muted",
            error && "border-red-500"
          )}
        >
          <option value="">Select your machine...</option>
          {machineGroups.map((group) => (
            <optgroup key={group.brand} label={group.brand}>
              {group.machines.map((machine) => (
                <option key={machine.id} value={machine.id}>
                  {machine.model} ({getMachineTypeLabel(machine.type)})
                </option>
              ))}
            </optgroup>
          ))}
        </select>
        <ChevronDown className="absolute right-3 top-1/2 -translate-y-1/2 w-4 h-4 sm:w-5 sm:h-5 text-theme-muted pointer-events-none" />
      </div>
      {error && <p className="text-[10px] sm:text-xs text-red-500">{error}</p>}
    </div>
  );
}

interface MachineInfoProps {
  machine: MachineDefinition;
  temperatureUnit: "celsius" | "fahrenheit";
}

function MachineInfo({ machine, temperatureUnit }: MachineInfoProps) {
  return (
    <div className="p-3 sm:p-4 rounded-lg sm:rounded-xl bg-accent/5 border border-accent/20 space-y-2 sm:space-y-3">
      <div className="flex items-center gap-2">
        <Coffee className="w-4 h-4 sm:w-5 sm:h-5 text-accent" />
        <span className="font-semibold text-theme text-sm sm:text-base">
          {machine.brand} {machine.model}
        </span>
      </div>
      <p className="text-xs sm:text-sm text-theme-muted">{machine.description}</p>
      
      {/* Type and Temperatures */}
      <div className="flex flex-wrap gap-1.5 sm:gap-2 text-[10px] sm:text-xs">
        <span className="px-1.5 sm:px-2 py-0.5 sm:py-1 rounded-full bg-white/10 sm:bg-theme-secondary text-theme-muted">
          {getMachineTypeLabel(machine.type)}
        </span>
        <span className="px-1.5 sm:px-2 py-0.5 sm:py-1 rounded-full bg-white/10 sm:bg-theme-secondary text-theme-muted">
          Brew: {formatTemperatureWithUnit(machine.defaults.brewTemp, temperatureUnit, 0)}
        </span>
        <span className="px-1.5 sm:px-2 py-0.5 sm:py-1 rounded-full bg-white/10 sm:bg-theme-secondary text-theme-muted">
          Steam: {formatTemperatureWithUnit(machine.defaults.steamTemp, temperatureUnit, 0)}
        </span>
      </div>

      {/* Power and Capacity Specs - hidden on mobile for compactness */}
      {(machine.specs.brewPowerWatts || machine.specs.steamPowerWatts || machine.specs.boilerVolumeMl) && (
        <div className="hidden sm:flex flex-wrap gap-2 text-xs">
          {machine.specs.brewPowerWatts && (
            <span className="px-2 py-1 rounded-full bg-theme-tertiary text-theme-muted">
              Brew: {machine.specs.brewPowerWatts}W
            </span>
          )}
          {machine.specs.steamPowerWatts && (
            <span className="px-2 py-1 rounded-full bg-theme-tertiary text-theme-muted">
              Steam: {machine.specs.steamPowerWatts}W
            </span>
          )}
          {machine.specs.boilerVolumeMl && (
            <span className="px-2 py-1 rounded-full bg-theme-tertiary text-theme-muted">
              Boiler: {machine.specs.boilerVolumeMl}ml
            </span>
          )}
        </div>
      )}
    </div>
  );
}

