import type { Meta, StoryObj } from "@storybook/react";
import { Loading } from "./Loading";

// Custom styles to override fixed positioning in Storybook
const storybookOverride = "!relative !inset-auto !h-full";

const meta: Meta<typeof Loading> = {
  title: "Core/Loading",
  component: Loading,
  tags: ["autodocs"],
  parameters: {
    layout: "fullscreen",
  },
  // Wrap in a container to contain the loading component
  decorators: [
    (Story) => (
      <div className="relative h-[500px] w-full overflow-hidden rounded-xl border border-theme">
        <Story />
      </div>
    ),
  ],
};

export default meta;
type Story = StoryObj<typeof Loading>;

export const Default: Story = {
  args: {
    className: storybookOverride,
  },
};

export const WithMessage: Story = {
  args: {
    message: "Connecting to your espresso machine...",
    className: storybookOverride,
  },
};

export const WithLongMessage: Story = {
  args: {
    message: "Establishing connection to your BrewOS device. This may take a few moments while we synchronize your settings.",
    className: storybookOverride,
  },
};

export const ErrorState: Story = {
  args: {
    message: "Connection error: Unable to reach your machine",
    showRetry: true,
    onRetry: () => alert("Retrying connection..."),
    className: storybookOverride,
  },
};

export const AllStates: Story = {
  decorators: [
    // Use a grid layout without the default container
    (Story) => <Story />,
  ],
  render: () => (
    <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
      <div className="relative h-[400px] rounded-xl border border-theme overflow-hidden">
        <Loading message="Loading..." className={storybookOverride} />
      </div>
      <div className="relative h-[400px] rounded-xl border border-theme overflow-hidden">
        <Loading
          message="Connection error"
          showRetry
          onRetry={() => console.log("Retry clicked")}
          className={storybookOverride}
        />
      </div>
    </div>
  ),
};

