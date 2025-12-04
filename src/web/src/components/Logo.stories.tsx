import type { Meta, StoryObj } from "@storybook/react";
import { Logo, LogoIcon } from "./Logo";

const meta: Meta<typeof Logo> = {
  title: "Core/Logo",
  component: Logo,
  tags: ["autodocs"],
  argTypes: {
    size: {
      control: "select",
      options: ["sm", "md", "lg", "xl"],
    },
    iconOnly: { control: "boolean" },
    forceLight: { control: "boolean" },
    forceDark: { control: "boolean" },
  },
};

export default meta;
type Story = StoryObj<typeof Logo>;

export const Default: Story = {
  args: {
    size: "md",
  },
};

export const Small: Story = {
  args: {
    size: "sm",
  },
};

export const Large: Story = {
  args: {
    size: "lg",
  },
};

export const ExtraLarge: Story = {
  args: {
    size: "xl",
  },
};

export const IconOnly: Story = {
  args: {
    iconOnly: true,
    size: "lg",
  },
};

export const ForcedLight: Story = {
  args: {
    forceLight: true,
    size: "lg",
  },
  decorators: [
    (Story) => (
      <div className="bg-coffee-800 p-8 rounded-xl">
        <Story />
      </div>
    ),
  ],
};

export const ForcedDark: Story = {
  args: {
    forceDark: true,
    size: "lg",
  },
  decorators: [
    (Story) => (
      <div className="bg-cream-100 p-8 rounded-xl">
        <Story />
      </div>
    ),
  ],
};

export const AllSizes: Story = {
  render: () => (
    <div className="space-y-6">
      <div>
        <p className="text-sm text-theme-muted mb-2">Small (sm)</p>
        <Logo size="sm" />
      </div>
      <div>
        <p className="text-sm text-theme-muted mb-2">Medium (md)</p>
        <Logo size="md" />
      </div>
      <div>
        <p className="text-sm text-theme-muted mb-2">Large (lg)</p>
        <Logo size="lg" />
      </div>
      <div>
        <p className="text-sm text-theme-muted mb-2">Extra Large (xl)</p>
        <Logo size="xl" />
      </div>
    </div>
  ),
};

export const IconSizes: Story = {
  render: () => (
    <div className="flex items-end gap-6">
      <div className="text-center">
        <LogoIcon size="sm" />
        <p className="text-xs text-theme-muted mt-2">sm</p>
      </div>
      <div className="text-center">
        <LogoIcon size="md" />
        <p className="text-xs text-theme-muted mt-2">md</p>
      </div>
      <div className="text-center">
        <LogoIcon size="lg" />
        <p className="text-xs text-theme-muted mt-2">lg</p>
      </div>
      <div className="text-center">
        <LogoIcon size="xl" />
        <p className="text-xs text-theme-muted mt-2">xl</p>
      </div>
    </div>
  ),
};

export const OnBackgrounds: Story = {
  render: () => (
    <div className="space-y-4">
      <div className="bg-theme-card p-6 rounded-xl">
        <Logo size="lg" />
        <p className="text-sm text-theme-muted mt-2">On card background (theme-aware)</p>
      </div>
      <div className="bg-coffee-800 p-6 rounded-xl">
        <Logo size="lg" forceLight />
        <p className="text-sm text-cream-200 mt-2">On dark background (forceLight)</p>
      </div>
      <div className="bg-cream-100 p-6 rounded-xl border border-theme">
        <Logo size="lg" forceDark />
        <p className="text-sm text-coffee-800 mt-2">On light background (forceDark)</p>
      </div>
    </div>
  ),
};

