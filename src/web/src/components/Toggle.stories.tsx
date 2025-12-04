import type { Meta, StoryObj } from "@storybook/react";
import { useState } from "react";
import { Toggle } from "./Toggle";
import { Card } from "./Card";

const meta: Meta<typeof Toggle> = {
  title: "Core/Toggle",
  component: Toggle,
  tags: ["autodocs"],
  argTypes: {
    checked: { control: "boolean" },
    disabled: { control: "boolean" },
  },
};

export default meta;
type Story = StoryObj<typeof Toggle>;

// Interactive toggle wrapper
function ToggleWrapper({
  initialChecked = false,
  disabled = false,
  label,
}: {
  initialChecked?: boolean;
  disabled?: boolean;
  label?: string;
}) {
  const [checked, setChecked] = useState(initialChecked);
  return (
    <Toggle
      checked={checked}
      onChange={setChecked}
      disabled={disabled}
      label={label}
    />
  );
}

export const Default: Story = {
  render: () => <ToggleWrapper />,
};

export const Checked: Story = {
  render: () => <ToggleWrapper initialChecked />,
};

export const WithLabel: Story = {
  render: () => <ToggleWrapper label="Enable feature" />,
};

export const Disabled: Story = {
  render: () => (
    <div className="space-y-4">
      <Toggle checked={false} onChange={() => {}} disabled label="Disabled Off" />
      <Toggle checked={true} onChange={() => {}} disabled label="Disabled On" />
    </div>
  ),
};

export const SettingsExample: Story = {
  render: () => {
    function Settings() {
      const [power, setPower] = useState(true);
      const [eco, setEco] = useState(false);
      const [notifications, setNotifications] = useState(true);
      const [autoOff, setAutoOff] = useState(true);
      const [preHeat, setPreHeat] = useState(false);

      return (
        <Card className="max-w-md">
          <h2 className="text-xl font-bold text-theme mb-6">Machine Settings</h2>
          <div className="space-y-4">
            <div className="flex items-center justify-between py-2 border-b border-theme">
              <div>
                <div className="font-medium text-theme">Power</div>
                <div className="text-sm text-theme-muted">
                  Turn machine on or off
                </div>
              </div>
              <Toggle checked={power} onChange={setPower} />
            </div>

            <div className="flex items-center justify-between py-2 border-b border-theme">
              <div>
                <div className="font-medium text-theme">Eco Mode</div>
                <div className="text-sm text-theme-muted">
                  Reduce power consumption
                </div>
              </div>
              <Toggle checked={eco} onChange={setEco} />
            </div>

            <div className="flex items-center justify-between py-2 border-b border-theme">
              <div>
                <div className="font-medium text-theme">Notifications</div>
                <div className="text-sm text-theme-muted">
                  Get alerts when ready
                </div>
              </div>
              <Toggle checked={notifications} onChange={setNotifications} />
            </div>

            <div className="flex items-center justify-between py-2 border-b border-theme">
              <div>
                <div className="font-medium text-theme">Auto-Off</div>
                <div className="text-sm text-theme-muted">
                  Automatically turn off after 2 hours
                </div>
              </div>
              <Toggle checked={autoOff} onChange={setAutoOff} />
            </div>

            <div className="flex items-center justify-between py-2">
              <div>
                <div className="font-medium text-theme">Pre-Heat Schedule</div>
                <div className="text-sm text-theme-muted">
                  Automatically start at scheduled times
                </div>
              </div>
              <Toggle checked={preHeat} onChange={setPreHeat} />
            </div>
          </div>
        </Card>
      );
    }
    return <Settings />;
  },
};

export const AllStates: Story = {
  render: () => (
    <div className="space-y-6">
      <div>
        <h3 className="text-theme-secondary text-sm font-semibold mb-3">
          Interactive
        </h3>
        <div className="flex flex-wrap gap-6">
          <ToggleWrapper label="Off by default" />
          <ToggleWrapper initialChecked label="On by default" />
        </div>
      </div>

      <div>
        <h3 className="text-theme-secondary text-sm font-semibold mb-3">
          Disabled States
        </h3>
        <div className="flex flex-wrap gap-6">
          <Toggle
            checked={false}
            onChange={() => {}}
            disabled
            label="Disabled off"
          />
          <Toggle
            checked={true}
            onChange={() => {}}
            disabled
            label="Disabled on"
          />
        </div>
      </div>
    </div>
  ),
};

