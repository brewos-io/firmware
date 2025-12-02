import { useState, useEffect, useMemo } from 'react';
import { useStore } from '@/lib/store';
import { useCommand } from '@/lib/useCommand';
import { Card, CardHeader, CardTitle } from '@/components/Card';
import { Input } from '@/components/Input';
import { Button } from '@/components/Button';
import { Thermometer } from 'lucide-react';
import { 
  convertFromCelsius, 
  convertToCelsius, 
  getUnitSymbol, 
  getTemperatureStep,
  getTemperatureRanges,
} from '@/lib/temperature';

export function TemperatureSettings() {
  const temps = useStore((s) => s.temps);
  const device = useStore((s) => s.device);
  const temperatureUnit = useStore((s) => s.preferences.temperatureUnit);
  const { sendCommand } = useCommand();
  const [saving, setSaving] = useState(false);

  // Store values in display unit for the UI
  const [brewTempDisplay, setBrewTempDisplay] = useState(() => 
    convertFromCelsius(temps.brew.setpoint, temperatureUnit)
  );
  const [steamTempDisplay, setSteamTempDisplay] = useState(() => 
    convertFromCelsius(temps.steam.setpoint, temperatureUnit)
  );

  // Get temperature ranges in current unit
  const ranges = useMemo(() => getTemperatureRanges(temperatureUnit), [temperatureUnit]);
  const step = getTemperatureStep(temperatureUnit);
  const unitSymbol = getUnitSymbol(temperatureUnit);

  // Update display values when unit changes or temps change from server
  useEffect(() => {
    setBrewTempDisplay(convertFromCelsius(temps.brew.setpoint, temperatureUnit));
    setSteamTempDisplay(convertFromCelsius(temps.steam.setpoint, temperatureUnit));
  }, [temps.brew.setpoint, temps.steam.setpoint, temperatureUnit]);

  const isDualBoiler = device.machineType === 'dual_boiler';
  const isSingleBoiler = device.machineType === 'single_boiler';
  const isHeatExchanger = device.machineType === 'heat_exchanger';

  const saveTemps = () => {
    setSaving(true);
    
    // Convert back to Celsius before sending to backend
    const brewTempCelsius = convertToCelsius(brewTempDisplay, temperatureUnit);
    const steamTempCelsius = convertToCelsius(steamTempDisplay, temperatureUnit);

    let success = true;
    if (isDualBoiler || isSingleBoiler || !device.machineType) {
      success = sendCommand('set_temp', { boiler: 'brew', temp: brewTempCelsius });
    }
    if (success && (isDualBoiler || isHeatExchanger || !device.machineType)) {
      success = sendCommand('set_temp', { boiler: 'steam', temp: steamTempCelsius }, 
        { successMessage: 'Temperatures saved' });
    }
    
    setSaving(false);
  };

  // Determine what controls to show based on machine type
  const showBrewControl = isDualBoiler || isSingleBoiler || !device.machineType;
  const showSteamControl = isDualBoiler || isHeatExchanger || !device.machineType;

  // Get appropriate labels and hints based on machine type and unit
  const brewLabel = isSingleBoiler ? 'Boiler Temperature' : 'Brew Temperature';
  const brewHint = isSingleBoiler 
    ? `Brew mode: ${ranges.brew.recommended.min.toFixed(0)}-${ranges.brew.recommended.max.toFixed(0)}${unitSymbol}`
    : `Recommended: ${ranges.brew.recommended.min.toFixed(0)}-${ranges.brew.recommended.max.toFixed(0)}${unitSymbol}`;
  const steamLabel = isHeatExchanger ? 'Boiler Temperature' : 'Steam Temperature';
  const steamHint = isHeatExchanger 
    ? 'Controls HX brew water temperature' 
    : 'For milk frothing';

  // Calculate min/max for inputs
  const brewMin = ranges.brew.min;
  const brewMax = isSingleBoiler ? ranges.boiler.max : ranges.brew.max;
  const steamMin = isHeatExchanger ? convertFromCelsius(100, temperatureUnit) : ranges.steam.min;
  const steamMax = ranges.steam.max;

  return (
    <Card>
      <CardHeader>
        <CardTitle icon={<Thermometer className="w-5 h-5" />}>
          Temperature
        </CardTitle>
      </CardHeader>

      <div className={`grid grid-cols-1 ${showBrewControl && showSteamControl ? 'sm:grid-cols-2' : ''} gap-4 mb-6`}>
        {showBrewControl && (
          <Input
            label={brewLabel}
            type="number"
            min={brewMin}
            max={brewMax}
            step={step}
            value={brewTempDisplay}
            onChange={(e) => setBrewTempDisplay(parseFloat(e.target.value))}
            unit={unitSymbol}
            hint={brewHint}
          />
        )}
        {showSteamControl && (
          <Input
            label={steamLabel}
            type="number"
            min={steamMin}
            max={steamMax}
            step={step}
            value={steamTempDisplay}
            onChange={(e) => setSteamTempDisplay(parseFloat(e.target.value))}
            unit={unitSymbol}
            hint={steamHint}
          />
        )}
      </div>

      <div className="flex justify-end">
        <Button onClick={saveTemps} loading={saving}>Save Temperatures</Button>
      </div>
    </Card>
  );
}

