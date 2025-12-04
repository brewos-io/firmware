import type { Meta, StoryObj } from "@storybook/react";
import { Gauge } from "./Gauge";
import { Thermometer, Wind, Gauge as GaugeIcon } from "lucide-react";

const meta: Meta<typeof Gauge> = {
  title: "Core/Gauge",
  component: Gauge,
  tags: ["autodocs"],
  argTypes: {
    variant: {
      control: "select",
      options: ["default", "steam", "pressure"],
    },
    isTemperature: { control: "boolean" },
    showSetpoint: { control: "boolean" },
  },
  decorators: [
    (Story) => (
      <div className="max-w-xs">
        <Story />
      </div>
    ),
  ],
};

export default meta;
type Story = StoryObj<typeof Gauge>;

export const Default: Story = {
  args: {
    value: 93,
    max: 120,
    setpoint: 93,
    label: "Brew Temperature",
    icon: <Thermometer className="w-5 h-5" />,
    variant: "default",
    isTemperature: true,
    showSetpoint: true,
  },
};

export const SteamTemperature: Story = {
  args: {
    value: 140,
    max: 160,
    setpoint: 145,
    label: "Steam Temperature",
    icon: <Wind className="w-5 h-5" />,
    variant: "steam",
    isTemperature: true,
    showSetpoint: true,
  },
};

export const Pressure: Story = {
  args: {
    value: 9.0,
    max: 15,
    setpoint: 9,
    label: "Brew Pressure",
    unit: "bar",
    icon: <GaugeIcon className="w-5 h-5" />,
    variant: "pressure",
    isTemperature: false,
    showSetpoint: true,
  },
};

export const Heating: Story = {
  args: {
    value: 75,
    max: 120,
    setpoint: 93,
    label: "Heating...",
    icon: <Thermometer className="w-5 h-5" />,
    variant: "default",
    isTemperature: true,
    showSetpoint: true,
  },
};

export const OverTarget: Story = {
  args: {
    value: 98,
    max: 120,
    setpoint: 93,
    label: "Temperature",
    icon: <Thermometer className="w-5 h-5" />,
    variant: "default",
    isTemperature: true,
    showSetpoint: true,
  },
};

export const NoSetpoint: Story = {
  args: {
    value: 85,
    max: 120,
    label: "Temperature",
    icon: <Thermometer className="w-5 h-5" />,
    variant: "default",
    isTemperature: true,
    showSetpoint: false,
  },
};

export const AllVariants: Story = {
  render: () => (
    <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
      <Gauge
        value={93}
        max={120}
        setpoint={93}
        label="Brew Temperature"
        icon={<Thermometer className="w-5 h-5" />}
        variant="default"
        isTemperature={true}
      />
      <Gauge
        value={140}
        max={160}
        setpoint={145}
        label="Steam Temperature"
        icon={<Wind className="w-5 h-5" />}
        variant="steam"
        isTemperature={true}
      />
      <Gauge
        value={9.0}
        max={15}
        setpoint={9}
        label="Brew Pressure"
        unit="bar"
        icon={<GaugeIcon className="w-5 h-5" />}
        variant="pressure"
        isTemperature={false}
      />
    </div>
  ),
};

export const HeatingProgress: Story = {
  render: () => (
    <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
      <Gauge
        value={25}
        max={120}
        setpoint={93}
        label="Starting up..."
        icon={<Thermometer className="w-5 h-5" />}
        variant="default"
        isTemperature={true}
      />
      <Gauge
        value={60}
        max={120}
        setpoint={93}
        label="Heating..."
        icon={<Thermometer className="w-5 h-5" />}
        variant="default"
        isTemperature={true}
      />
      <Gauge
        value={93}
        max={120}
        setpoint={93}
        label="Ready!"
        icon={<Thermometer className="w-5 h-5" />}
        variant="default"
        isTemperature={true}
      />
    </div>
  ),
};

