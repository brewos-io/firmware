import type { Meta, StoryObj } from "@storybook/react";
import { Card, CardHeader, CardTitle, CardDescription } from "./Card";
import { Button } from "./Button";
import { Badge } from "./Badge";
import {
  Thermometer,
  Coffee,
  Gauge,
  Settings,
  Wifi,
  Power,
} from "lucide-react";

const meta: Meta<typeof Card> = {
  title: "Core/Card",
  component: Card,
  tags: ["autodocs"],
};

export default meta;
type Story = StoryObj<typeof Card>;

export const Default: Story = {
  render: () => (
    <Card>
      <p className="text-theme-secondary">This is a basic card component.</p>
    </Card>
  ),
};

export const WithHeader: Story = {
  render: () => (
    <Card>
      <CardHeader>
        <CardTitle icon={<Thermometer className="w-5 h-5" />}>
          Temperature
        </CardTitle>
      </CardHeader>
      <p className="text-theme-secondary">
        Monitor and control your brew temperature.
      </p>
    </Card>
  ),
};

export const WithHeaderAction: Story = {
  render: () => (
    <Card>
      <CardHeader action={<Button size="sm">Configure</Button>}>
        <CardTitle icon={<Settings className="w-5 h-5" />}>Settings</CardTitle>
      </CardHeader>
      <p className="text-theme-secondary">
        Configure your machine settings and preferences.
      </p>
    </Card>
  ),
};

export const WithDescription: Story = {
  render: () => (
    <Card>
      <CardHeader>
        <div>
          <CardTitle icon={<Coffee className="w-5 h-5" />}>
            Brew Session
          </CardTitle>
          <CardDescription>Last brew: 2 hours ago</CardDescription>
        </div>
      </CardHeader>
      <p className="text-theme-secondary">
        Start a new brew session with your preferred settings.
      </p>
    </Card>
  ),
};

export const StatusCard: Story = {
  render: () => (
    <Card>
      <CardHeader action={<Badge variant="success">Online</Badge>}>
        <CardTitle icon={<Wifi className="w-5 h-5" />}>Connection</CardTitle>
      </CardHeader>
      <div className="space-y-3">
        <div className="flex justify-between items-center">
          <span className="text-theme-muted">IP Address</span>
          <span className="text-theme font-medium">192.168.1.100</span>
        </div>
        <div className="flex justify-between items-center">
          <span className="text-theme-muted">Signal</span>
          <span className="text-theme font-medium">-45 dBm</span>
        </div>
        <div className="flex justify-between items-center">
          <span className="text-theme-muted">Uptime</span>
          <span className="text-theme font-medium">4h 32m</span>
        </div>
      </div>
    </Card>
  ),
};

export const TemperatureCard: Story = {
  render: () => (
    <Card>
      <CardHeader action={<Badge variant="success">Ready</Badge>}>
        <CardTitle icon={<Thermometer className="w-5 h-5" />}>
          Brew Temperature
        </CardTitle>
      </CardHeader>
      <div className="flex items-end gap-2 mb-4">
        <span className="text-5xl font-bold text-theme">93</span>
        <span className="text-2xl text-theme-muted mb-1">°C</span>
      </div>
      <div className="flex justify-between text-sm">
        <span className="text-theme-muted">Target: 93°C</span>
        <span className="text-success">± 0.5°C</span>
      </div>
    </Card>
  ),
};

export const QuickActionCard: Story = {
  render: () => (
    <Card>
      <CardHeader>
        <CardTitle icon={<Power className="w-5 h-5" />}>Quick Actions</CardTitle>
      </CardHeader>
      <div className="grid grid-cols-2 gap-3">
        <Button variant="primary" className="w-full">
          <Coffee className="w-4 h-4" />
          Brew
        </Button>
        <Button variant="secondary" className="w-full">
          <Gauge className="w-4 h-4" />
          Steam
        </Button>
        <Button variant="secondary" className="w-full">
          <Thermometer className="w-4 h-4" />
          Preheat
        </Button>
        <Button variant="ghost" className="w-full">
          <Power className="w-4 h-4" />
          Standby
        </Button>
      </div>
    </Card>
  ),
};

export const CardGrid: Story = {
  render: () => (
    <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
      <Card>
        <CardHeader>
          <CardTitle icon={<Thermometer className="w-5 h-5" />}>Boiler</CardTitle>
        </CardHeader>
        <div className="text-3xl font-bold text-theme">93°C</div>
        <div className="text-sm text-theme-muted mt-1">Target: 93°C</div>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle icon={<Gauge className="w-5 h-5" />}>Pressure</CardTitle>
        </CardHeader>
        <div className="text-3xl font-bold text-theme">9.0 bar</div>
        <div className="text-sm text-theme-muted mt-1">Brewing range</div>
      </Card>

      <Card>
        <CardHeader>
          <CardTitle icon={<Coffee className="w-5 h-5" />}>
            Today's Brews
          </CardTitle>
        </CardHeader>
        <div className="text-3xl font-bold text-theme">12</div>
        <div className="text-sm text-theme-muted mt-1">+3 from yesterday</div>
      </Card>
    </div>
  ),
};

