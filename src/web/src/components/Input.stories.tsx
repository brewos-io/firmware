import type { Meta, StoryObj } from "@storybook/react";
import { Input } from "./Input";
import { Card } from "./Card";

const meta: Meta<typeof Input> = {
  title: "Core/Input",
  component: Input,
  tags: ["autodocs"],
  argTypes: {
    type: {
      control: "select",
      options: ["text", "number", "email", "password", "search"],
    },
    disabled: { control: "boolean" },
  },
  args: {
    placeholder: "Enter value...",
    disabled: false,
  },
};

export default meta;
type Story = StoryObj<typeof Input>;

export const Default: Story = {
  args: {
    placeholder: "Enter text...",
  },
};

export const WithLabel: Story = {
  args: {
    label: "Email Address",
    placeholder: "you@example.com",
    type: "email",
  },
};

export const WithHint: Story = {
  args: {
    label: "Password",
    placeholder: "Enter password",
    type: "password",
    hint: "Must be at least 8 characters",
  },
};

export const WithError: Story = {
  args: {
    label: "Temperature",
    placeholder: "Enter temperature",
    type: "number",
    error: "Temperature must be between 85°C and 100°C",
    defaultValue: "120",
  },
};

export const WithUnit: Story = {
  args: {
    label: "Brew Temperature",
    placeholder: "93",
    type: "number",
    unit: "°C",
  },
};

export const NumberWithUnit: Story = {
  args: {
    label: "Pre-infusion Time",
    placeholder: "5",
    type: "number",
    unit: "seconds",
    hint: "How long to pre-wet the coffee before brewing",
  },
};

export const Disabled: Story = {
  args: {
    label: "Locked Setting",
    placeholder: "Cannot be changed",
    disabled: true,
  },
};

export const FormExample: Story = {
  render: () => (
    <Card className="max-w-md">
      <h2 className="text-xl font-bold text-theme mb-6">Brew Settings</h2>
      <div className="space-y-4">
        <Input
          label="Brew Temperature"
          placeholder="93"
          type="number"
          unit="°C"
          hint="Recommended: 92-96°C for espresso"
        />
        <Input
          label="Steam Temperature"
          placeholder="140"
          type="number"
          unit="°C"
        />
        <Input
          label="Pre-infusion Time"
          placeholder="3"
          type="number"
          unit="sec"
        />
        <Input
          label="Brew Ratio"
          placeholder="2.0"
          type="number"
          unit="x"
          hint="Output weight / Input dose"
        />
      </div>
    </Card>
  ),
};

export const ValidationExample: Story = {
  render: () => (
    <Card className="max-w-md">
      <h2 className="text-xl font-bold text-theme mb-6">Form Validation</h2>
      <div className="space-y-4">
        <Input label="Valid Field" placeholder="All good" defaultValue="John" />
        <Input
          label="Field with Warning"
          placeholder="Check this"
          defaultValue="unusual-value"
          hint="This value seems unusual"
        />
        <Input
          label="Invalid Field"
          placeholder="Error here"
          defaultValue="invalid"
          error="This field contains an invalid value"
        />
        <Input
          label="Disabled Field"
          placeholder="Cannot edit"
          defaultValue="Locked value"
          disabled
        />
      </div>
    </Card>
  ),
};

export const AllTypes: Story = {
  render: () => (
    <div className="space-y-4 max-w-md">
      <Input label="Text" type="text" placeholder="Enter text" />
      <Input label="Number" type="number" placeholder="123" />
      <Input label="Email" type="email" placeholder="you@example.com" />
      <Input label="Password" type="password" placeholder="••••••••" />
      <Input label="Search" type="search" placeholder="Search..." />
    </div>
  ),
};

