import type { Meta, StoryObj } from "@storybook/react";
import { Badge } from "./Badge";
import {
  Check,
  AlertTriangle,
  XCircle,
  Info,
  Wifi,
  WifiOff,
} from "lucide-react";

const meta: Meta<typeof Badge> = {
  title: "Core/Badge",
  component: Badge,
  tags: ["autodocs"],
  argTypes: {
    variant: {
      control: "select",
      options: ["default", "success", "warning", "error", "info"],
    },
  },
  args: {
    children: "Badge",
    variant: "default",
  },
};

export default meta;
type Story = StoryObj<typeof Badge>;

export const Default: Story = {
  args: {
    variant: "default",
    children: "Default",
  },
};

export const Success: Story = {
  args: {
    variant: "success",
    children: "Online",
  },
};

export const Warning: Story = {
  args: {
    variant: "warning",
    children: "Heating",
  },
};

export const Error: Story = {
  args: {
    variant: "error",
    children: "Error",
  },
};

export const InfoBadge: Story = {
  args: {
    variant: "info",
    children: "Update Available",
  },
};

export const WithIcons: Story = {
  render: () => (
    <div className="flex flex-wrap gap-3">
      <Badge variant="success">
        <Check className="w-3 h-3" />
        Connected
      </Badge>
      <Badge variant="warning">
        <AlertTriangle className="w-3 h-3" />
        Heating
      </Badge>
      <Badge variant="error">
        <XCircle className="w-3 h-3" />
        Offline
      </Badge>
      <Badge variant="info">
        <Info className="w-3 h-3" />
        New Version
      </Badge>
    </div>
  ),
};

export const StatusExamples: Story = {
  render: () => (
    <div className="space-y-4">
      <div>
        <h3 className="text-theme-secondary text-sm font-semibold mb-3">
          Machine Status
        </h3>
        <div className="flex flex-wrap gap-2">
          <Badge variant="success">
            <Wifi className="w-3 h-3" />
            Connected
          </Badge>
          <Badge variant="success">Ready</Badge>
          <Badge variant="warning">Heating</Badge>
          <Badge variant="error">
            <WifiOff className="w-3 h-3" />
            Offline
          </Badge>
        </div>
      </div>

      <div>
        <h3 className="text-theme-secondary text-sm font-semibold mb-3">
          Temperatures
        </h3>
        <div className="flex flex-wrap gap-2">
          <Badge variant="success">93°C</Badge>
          <Badge variant="warning">85°C - Heating</Badge>
          <Badge variant="error">120°C - Too Hot!</Badge>
        </div>
      </div>

      <div>
        <h3 className="text-theme-secondary text-sm font-semibold mb-3">
          Firmware
        </h3>
        <div className="flex flex-wrap gap-2">
          <Badge variant="default">v0.2.0</Badge>
          <Badge variant="info">Update Available</Badge>
          <Badge variant="success">Latest</Badge>
        </div>
      </div>
    </div>
  ),
};

