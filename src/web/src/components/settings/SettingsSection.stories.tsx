import type { Meta, StoryObj } from "@storybook/react";
import { SettingsSection } from "./SettingsSection";
import { SettingsRow } from "./SettingsRow";
import { Toggle } from "../Toggle";
import { Input } from "../Input";
import { Button } from "../Button";
import { Card } from "../Card";
import { useState } from "react";

const meta: Meta<typeof SettingsSection> = {
  title: "Settings/SettingsSection",
  component: SettingsSection,
  tags: ["autodocs"],
  decorators: [
    (Story) => (
      <Card className="max-w-xl">
        <Story />
      </Card>
    ),
  ],
};

export default meta;
type Story = StoryObj<typeof SettingsSection>;

export const Default: Story = {
  args: {
    title: "General Settings",
    description: "Configure basic options for your machine",
    children: (
      <div className="space-y-2">
        <p className="text-theme-secondary">Settings content goes here...</p>
      </div>
    ),
  },
};

export const WithToggleSettings: Story = {
  render: () => {
    function ToggleSettings() {
      const [power, setPower] = useState(true);
      const [eco, setEco] = useState(false);
      const [notifications, setNotifications] = useState(true);

      return (
        <SettingsSection
          title="Machine Settings"
          description="Control your machine behavior"
        >
          <SettingsRow label="Power" description="Turn machine on/off">
            <Toggle checked={power} onChange={setPower} />
          </SettingsRow>
          <SettingsRow label="Eco Mode" description="Save energy when idle">
            <Toggle checked={eco} onChange={setEco} />
          </SettingsRow>
          <SettingsRow label="Notifications" description="Get push notifications">
            <Toggle checked={notifications} onChange={setNotifications} />
          </SettingsRow>
        </SettingsSection>
      );
    }
    return <ToggleSettings />;
  },
};

export const WithInputSettings: Story = {
  render: () => (
    <SettingsSection
      title="Temperature Settings"
      description="Configure your brew temperatures"
    >
      <SettingsRow label="Brew Temperature" vertical>
        <Input type="number" placeholder="93" unit="°C" />
      </SettingsRow>
      <SettingsRow label="Steam Temperature" vertical>
        <Input type="number" placeholder="145" unit="°C" />
      </SettingsRow>
      <SettingsRow label="Pre-infusion Time" vertical>
        <Input type="number" placeholder="3" unit="sec" />
      </SettingsRow>
    </SettingsSection>
  ),
};

export const WithActionButton: Story = {
  render: () => (
    <SettingsSection
      title="System"
      description="System-level settings and maintenance"
    >
      <SettingsRow label="Factory Reset" description="Restore all default settings">
        <Button variant="danger" size="sm">
          Reset
        </Button>
      </SettingsRow>
      <SettingsRow label="Firmware" description="Current version: v0.2.0">
        <Button variant="secondary" size="sm">
          Check Updates
        </Button>
      </SettingsRow>
      <SettingsRow label="Export Data" description="Download machine data">
        <Button variant="ghost" size="sm">
          Export
        </Button>
      </SettingsRow>
    </SettingsSection>
  ),
};

export const NoHeader: Story = {
  render: () => {
    function Settings() {
      const [autoOff, setAutoOff] = useState(true);
      
      return (
        <SettingsSection>
          <SettingsRow label="Auto Power Off" description="Turn off after 2 hours">
            <Toggle checked={autoOff} onChange={setAutoOff} />
          </SettingsRow>
        </SettingsSection>
      );
    }
    return <Settings />;
  },
};

