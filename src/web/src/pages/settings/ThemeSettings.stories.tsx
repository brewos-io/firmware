import type { Meta, StoryObj } from "@storybook/react";
import { ThemeSettings } from "./components/ThemeSettings";

const meta: Meta<typeof ThemeSettings> = {
  title: "Pages/Settings/ThemeSettings",
  component: ThemeSettings,
  tags: ["autodocs"],
  decorators: [
    (Story) => (
      <div className="max-w-3xl">
        <Story />
      </div>
    ),
  ],
};

export default meta;
type Story = StoryObj<typeof ThemeSettings>;

export const Default: Story = {};

