import type { Meta, StoryObj } from "@storybook/react";
import { PowerCard } from "./PowerCard";

const meta: Meta<typeof PowerCard> = {
  title: "Dashboard/PowerCard",
  component: PowerCard,
  tags: ["autodocs"],
  decorators: [
    (Story) => (
      <div className="max-w-sm">
        <Story />
      </div>
    ),
  ],
};

export default meta;
type Story = StoryObj<typeof PowerCard>;

export const Idle: Story = {
  args: {
    current: 5,
    todayKwh: 0.3,
    voltage: 120,
  },
};

export const Heating: Story = {
  args: {
    current: 1200,
    todayKwh: 1.8,
    voltage: 120,
  },
};

export const Ready: Story = {
  args: {
    current: 45,
    todayKwh: 2.4,
    voltage: 120,
  },
};

export const Brewing: Story = {
  args: {
    current: 850,
    todayKwh: 3.1,
    voltage: 120,
  },
};

export const EU_Voltage: Story = {
  args: {
    current: 1800,
    todayKwh: 4.2,
    voltage: 230,
  },
};

export const HighUsage: Story = {
  args: {
    current: 2100,
    todayKwh: 8.7,
    voltage: 230,
  },
};

