import type { Meta, StoryObj } from "@storybook/react";
import { useEffect, useRef } from "react";
import { ToastProvider, useToast } from "./Toast";
import { Button } from "./Button";
import { Card } from "./Card";

const meta: Meta = {
  title: "Core/Toast",
  decorators: [
    (Story) => (
      <ToastProvider>
        <Story />
      </ToastProvider>
    ),
  ],
  tags: ["autodocs"],
};

export default meta;
type Story = StoryObj;

// Demo component to trigger toasts
function ToastDemo() {
  const toast = useToast();

  return (
    <Card className="max-w-md">
      <h2 className="text-xl font-bold text-theme mb-4">Toast Notifications</h2>
      <p className="text-theme-muted mb-6">Click a button to trigger a toast notification.</p>
      <div className="flex flex-wrap gap-3">
        <Button
          variant="primary"
          onClick={() => toast.success("Coffee brewing started!")}
        >
          Success
        </Button>
        <Button
          variant="danger"
          onClick={() => toast.error("Failed to connect to machine")}
        >
          Error
        </Button>
        <Button
          variant="secondary"
          onClick={() => toast.warning("Machine temperature is low")}
        >
          Warning
        </Button>
        <Button
          variant="ghost"
          onClick={() => toast.loading("Connecting...")}
        >
          Loading
        </Button>
      </div>
    </Card>
  );
}

export const Interactive: Story = {
  render: () => <ToastDemo />,
};

// Loading with update demo
function LoadingUpdateDemo() {
  const toast = useToast();

  const handleSave = () => {
    const id = toast.loading("Saving settings...");
    
    // Simulate async operation
    setTimeout(() => {
      toast.update(id, "success", "Settings saved successfully!");
    }, 2000);
  };

  const handleSaveError = () => {
    const id = toast.loading("Saving settings...");
    
    // Simulate async operation that fails
    setTimeout(() => {
      toast.update(id, "error", "Failed to save settings");
    }, 2000);
  };

  return (
    <Card className="max-w-md">
      <h2 className="text-xl font-bold text-theme mb-4">Async Operations</h2>
      <p className="text-theme-muted mb-6">
        Toast updates from loading to success/error state.
      </p>
      <div className="flex gap-3">
        <Button variant="primary" onClick={handleSave}>
          Save (Success)
        </Button>
        <Button variant="secondary" onClick={handleSaveError}>
          Save (Fail)
        </Button>
      </div>
    </Card>
  );
}

export const AsyncOperations: Story = {
  render: () => <LoadingUpdateDemo />,
};

// Multiple toasts demo
function MultipleToastsDemo() {
  const toast = useToast();

  const triggerMultiple = () => {
    toast.success("Connected to machine");
    setTimeout(() => toast.warning("Pre-heating required"), 500);
    setTimeout(() => toast.success("Temperature target reached"), 1000);
    setTimeout(() => toast.error("Water level low"), 1500);
  };

  return (
    <Card className="max-w-md">
      <h2 className="text-xl font-bold text-theme mb-4">Multiple Toasts</h2>
      <p className="text-theme-muted mb-6">
        Multiple toasts stack and animate independently.
      </p>
      <Button variant="accent" onClick={triggerMultiple}>
        Trigger Multiple Toasts
      </Button>
    </Card>
  );
}

export const MultipleToasts: Story = {
  render: () => <MultipleToastsDemo />,
};

// Pre-rendered toasts for visual testing
function StaticToastPreview() {
  const toast = useToast();
  const hasShownToasts = useRef(false);

  useEffect(() => {
    // Only show toasts once to prevent infinite loop
    if (hasShownToasts.current) return;
    hasShownToasts.current = true;
    
    toast.success("Machine is ready to brew");
    toast.warning("Descaling recommended");
    toast.error("Connection lost");
  }, [toast]); // Run only on mount

  return (
    <Card className="max-w-md">
      <h2 className="text-xl font-bold text-theme mb-4">Toast Variants</h2>
      <p className="text-theme-muted">
        All toast types are displayed. Dismiss them to see the animation.
      </p>
    </Card>
  );
}

export const AllTypes: Story = {
  render: () => <StaticToastPreview />,
};

