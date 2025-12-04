import type { Meta, StoryObj } from "@storybook/react";
import { TemperatureGauges } from "./TemperatureGauges";

const meta: Meta<typeof TemperatureGauges> = {
  title: "Dashboard/TemperatureGauges",
  component: TemperatureGauges,
  tags: ["autodocs"],
  argTypes: {
    machineType: {
      control: "select",
      options: ["dual_boiler", "single_boiler", "heat_exchanger", undefined],
    },
  },
};

export default meta;
type Story = StoryObj<typeof TemperatureGauges>;

export const DualBoiler: Story = {
  args: {
    machineType: "dual_boiler",
    brewTemp: { current: 93, setpoint: 93, max: 105 },
    steamTemp: { current: 140, setpoint: 145, max: 160 },
    groupTemp: 88,
  },
};

export const DualBoilerHeating: Story = {
  args: {
    machineType: "dual_boiler",
    brewTemp: { current: 65, setpoint: 93, max: 105 },
    steamTemp: { current: 80, setpoint: 145, max: 160 },
    groupTemp: 45,
  },
};

export const SingleBoiler: Story = {
  args: {
    machineType: "single_boiler",
    brewTemp: { current: 93, setpoint: 93, max: 105 },
    steamTemp: { current: 0, setpoint: 0, max: 160 },
    groupTemp: 0,
  },
};

export const HeatExchanger: Story = {
  args: {
    machineType: "heat_exchanger",
    brewTemp: { current: 0, setpoint: 0, max: 105 },
    steamTemp: { current: 140, setpoint: 145, max: 160 },
    groupTemp: 93,
  },
};

export const Unknown: Story = {
  args: {
    machineType: undefined,
    brewTemp: { current: 93, setpoint: 93, max: 105 },
    steamTemp: { current: 140, setpoint: 145, max: 160 },
    groupTemp: 88,
  },
};

