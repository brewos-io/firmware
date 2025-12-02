import { useState, useEffect } from "react";
import { useStore } from "@/lib/store";
import { useCommand } from "@/lib/useCommand";
import { Card, CardHeader, CardTitle } from "@/components/Card";
import { Input } from "@/components/Input";
import { Button } from "@/components/Button";
import { Zap, Leaf } from "lucide-react";
import {
  convertFromCelsius,
  convertToCelsius,
  getUnitSymbol,
  getTemperatureStep,
} from "@/lib/temperature";

export function PowerSettings() {
  const power = useStore((s) => s.power);
  const temperatureUnit = useStore((s) => s.preferences.temperatureUnit);
  const { sendCommand } = useCommand();

  const [voltage, setVoltage] = useState(power.voltage);
  const [maxCurrent, setMaxCurrent] = useState(13);
  const [savingPower, setSavingPower] = useState(false);
  const [savingEco, setSavingEco] = useState(false);

  // Eco temp stored internally in Celsius (80°C default)
  const [ecoBrewTempCelsius] = useState(80);
  const [ecoBrewTempDisplay, setEcoBrewTempDisplay] = useState(() =>
    convertFromCelsius(80, temperatureUnit)
  );
  const [ecoTimeout, setEcoTimeout] = useState(30);

  // Update display when unit changes
  useEffect(() => {
    setEcoBrewTempDisplay(
      convertFromCelsius(ecoBrewTempCelsius, temperatureUnit)
    );
  }, [temperatureUnit, ecoBrewTempCelsius]);

  const unitSymbol = getUnitSymbol(temperatureUnit);
  const step = getTemperatureStep(temperatureUnit);

  // Calculate min/max in display unit
  const ecoTempMin = convertFromCelsius(60, temperatureUnit);
  const ecoTempMax = convertFromCelsius(90, temperatureUnit);

  const savePower = () => {
    setSavingPower(true);
    sendCommand(
      "set_power",
      { voltage, maxCurrent },
      { successMessage: "Power settings saved" }
    );
    setSavingPower(false);
  };

  const saveEco = () => {
    setSavingEco(true);
    // Convert display value back to Celsius for backend
    const brewTempCelsius = convertToCelsius(
      ecoBrewTempDisplay,
      temperatureUnit
    );
    sendCommand(
      "set_eco",
      { brewTemp: brewTempCelsius, timeout: ecoTimeout },
      { successMessage: "Eco settings saved" }
    );
    setSavingEco(false);
  };

  return (
    <>
      {/* Eco Mode */}
      <Card>
        <CardHeader>
          <CardTitle icon={<Leaf className="w-5 h-5" />}>Eco Mode</CardTitle>
        </CardHeader>

        <p className="text-sm text-coffee-500 mb-4">
          Reduce power consumption when idle by lowering boiler temperatures.
        </p>

        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 mb-6">
          <Input
            label="Eco Brew Temp"
            type="number"
            min={ecoTempMin}
            max={ecoTempMax}
            step={step}
            value={ecoBrewTempDisplay}
            onChange={(e) => setEcoBrewTempDisplay(parseFloat(e.target.value))}
            unit={unitSymbol}
          />
          <Input
            label="Auto-Eco After"
            type="number"
            min={5}
            max={120}
            step={5}
            value={ecoTimeout}
            onChange={(e) => setEcoTimeout(parseInt(e.target.value))}
            unit="min"
          />
        </div>

        <div className="flex justify-end">
          <Button onClick={saveEco} loading={savingEco}>
            Save Eco Settings
          </Button>
        </div>
      </Card>

      {/* Power Settings */}
      <Card>
        <CardHeader>
          <CardTitle icon={<Zap className="w-5 h-5" />}>Power</CardTitle>
        </CardHeader>

        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 mb-6">
          <div className="space-y-1.5">
            <label className="block text-xs font-semibold uppercase tracking-wider text-coffee-500">
              Mains Voltage
            </label>
            <select
              value={voltage}
              onChange={(e) => setVoltage(parseInt(e.target.value))}
              className="input"
            >
              <option value="110">110V (US)</option>
              <option value="220">220V (EU/AU)</option>
              <option value="240">240V (UK)</option>
            </select>
          </div>
          <Input
            label="Max Current"
            type="number"
            min={5}
            max={20}
            step={0.5}
            value={maxCurrent}
            onChange={(e) => setMaxCurrent(parseFloat(e.target.value))}
            unit="A"
            hint="Limit for your circuit"
          />
        </div>

        <div className="flex justify-end">
          <Button onClick={savePower} loading={savingPower}>
            Save Power Settings
          </Button>
        </div>
      </Card>
    </>
  );
}
