import type { Meta, StoryObj } from "@storybook/react";
import { LogoIcon, Logo } from "@/components/Logo";
import {
  AlertCircle,
  Coffee,
  Gauge,
  Clock,
  Wifi,
  ChevronRight,
} from "lucide-react";

// Since Login uses auth context and navigation, we create presentational stories
// that show the different visual states of the login page

// Wrapper component for stories
function LoginStoryWrapper({ children }: { children?: React.ReactNode }) {
  return <>{children}</>;
}

const meta = {
  title: "Pages/Onboarding/Login",
  component: LoginStoryWrapper,
  tags: ["autodocs"],
  parameters: {
    layout: "fullscreen",
  },
} satisfies Meta<typeof LoginStoryWrapper>;

export default meta;
type Story = StoryObj<typeof meta>;

// Feature card component
function FeatureCard({
  icon: Icon,
  title,
  description,
}: {
  icon: React.ElementType;
  title: string;
  description: string;
}) {
  return (
    <div className="flex items-start gap-4">
      <div className="flex-shrink-0 w-10 h-10 rounded-xl bg-white/10 backdrop-blur-sm flex items-center justify-center">
        <Icon className="w-5 h-5 text-accent-light" />
      </div>
      <div>
        <h3 className="text-white font-semibold text-sm mb-1">{title}</h3>
        <p className="text-white/60 text-xs leading-relaxed">{description}</p>
      </div>
    </div>
  );
}

// Floating orbs for ambient background animation
function FloatingOrb({ size, x, y }: { size: number; x: number; y: number }) {
  return (
    <div
      className="absolute rounded-full blur-3xl opacity-20"
      style={{
        width: size,
        height: size,
        left: `${x}%`,
        top: `${y}%`,
        background:
          "radial-gradient(circle, var(--accent-glow) 0%, transparent 70%)",
      }}
    />
  );
}

// Cloud login view - Theme-aware Design
function LoginView({ error }: { error?: string }) {
  return (
    <div className="min-h-screen flex overflow-hidden">
      {/* Left Panel - Hero/Brand Section */}
      <div className="hidden lg:flex lg:w-1/2 xl:w-[55%] relative bg-gradient-to-br from-coffee-900 via-coffee-800 to-coffee-900 overflow-hidden">
        {/* Animated background orbs */}
        <FloatingOrb size={400} x={-10} y={20} />
        <FloatingOrb size={300} x={60} y={60} />
        <FloatingOrb size={250} x={30} y={-5} />

        {/* Subtle grid pattern overlay */}
        <div
          className="absolute inset-0 opacity-[0.03]"
          style={{
            backgroundImage: `
              linear-gradient(rgba(255,255,255,0.1) 1px, transparent 1px),
              linear-gradient(90deg, rgba(255,255,255,0.1) 1px, transparent 1px)
            `,
            backgroundSize: "60px 60px",
          }}
        />

        {/* Content */}
        <div className="relative z-10 flex flex-col justify-between p-12 xl:p-16 w-full">
          {/* Top - Logo */}
          <div>
            <Logo size="lg" forceLight />
          </div>

          {/* Center - Hero messaging */}
          <div className="flex-1 flex flex-col justify-center max-w-lg">
            <h1 className="text-4xl xl:text-5xl font-bold text-white mb-6 leading-tight">
              Craft the perfect
              <span className="block text-accent-light">espresso experience</span>
            </h1>
            <p className="text-white/60 text-lg mb-10 leading-relaxed">
              Take complete control of your espresso machine. Monitor
              temperature, pressure, and timing—from anywhere in the world.
            </p>

            {/* Feature highlights */}
            <div className="space-y-5">
              <FeatureCard
                icon={Gauge}
                title="Precision Temperature Control"
                description="PID-controlled heating with 0.1°C accuracy for consistent shots"
              />
              <FeatureCard
                icon={Clock}
                title="Smart Scheduling"
                description="Wake up to a pre-heated machine, ready when you are"
              />
              <FeatureCard
                icon={Wifi}
                title="Remote Monitoring"
                description="Keep an eye on your machine from anywhere via cloud"
              />
            </div>
          </div>

          {/* Bottom - Social proof / tagline */}
          <div>
            <div className="flex items-center gap-3 text-white/40 text-sm">
              <Coffee className="w-4 h-4" />
              <span>Trusted by home baristas worldwide</span>
            </div>
          </div>
        </div>

        {/* Decorative coffee cup silhouette */}
        <div className="absolute right-0 bottom-0 w-80 h-80 xl:w-96 xl:h-96 opacity-5">
          <svg
            viewBox="0 0 200 200"
            fill="currentColor"
            className="text-white w-full h-full"
          >
            <path d="M40 140 Q40 180 80 180 L120 180 Q160 180 160 140 L160 80 L180 80 Q200 80 200 100 L200 120 Q200 140 180 140 L160 140 L160 140 Q160 180 120 180 L80 180 Q40 180 40 140 Z M40 60 L160 60 L160 80 L40 80 Z" />
          </svg>
        </div>
      </div>

      {/* Right Panel - Login Form */}
      <div className="w-full lg:w-1/2 xl:w-[45%] flex items-center justify-center bg-theme p-6 sm:p-8 relative overflow-hidden">
        {/* Subtle decorative elements */}
        <div className="absolute top-0 right-0 w-64 h-64 bg-gradient-to-bl from-accent/5 to-transparent rounded-full blur-3xl" />
        <div className="absolute bottom-0 left-0 w-96 h-96 bg-gradient-to-tr from-accent/5 to-transparent rounded-full blur-3xl" />

        <div className="w-full max-w-md relative z-10">
          {/* Mobile logo (hidden on desktop) */}
          <div className="lg:hidden flex justify-center mb-8">
            <Logo size="lg" />
          </div>

          {/* Card */}
          <div className="bg-theme-card rounded-3xl p-8 sm:p-10 shadow-2xl border border-theme">
            {/* Header */}
            <div className="text-center mb-8">
              <div className="inline-flex items-center justify-center w-16 h-16 rounded-2xl bg-accent shadow-lg shadow-accent/20 mb-5">
                <LogoIcon size="md" className="filter brightness-0 invert" />
              </div>
              <h2 className="text-2xl sm:text-3xl font-bold text-theme mb-2">
                Welcome back
              </h2>
              <p className="text-theme-muted">Sign in to manage your machines</p>
            </div>

            {/* Error message */}
            {error && (
              <div className="mb-6 p-4 rounded-xl flex items-start gap-3 bg-error-soft border border-error">
                <AlertCircle className="w-5 h-5 text-error flex-shrink-0 mt-0.5" />
                <span className="text-sm text-error">{error}</span>
              </div>
            )}

            {/* Google Sign In */}
            <div>
              <div className="flex justify-center mb-6">
                {/* Mock Google Login Button */}
                <button className="flex items-center gap-3 px-8 py-3 bg-white border border-gray-200 rounded-full shadow-sm hover:shadow-md transition-all hover:scale-[1.02]">
                  <svg className="w-5 h-5" viewBox="0 0 24 24">
                    <path
                      fill="#4285F4"
                      d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92c-.26 1.37-1.04 2.53-2.21 3.31v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.09z"
                    />
                    <path
                      fill="#34A853"
                      d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z"
                    />
                    <path
                      fill="#FBBC05"
                      d="M5.84 14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.07H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.93l2.85-2.22.81-.62z"
                    />
                    <path
                      fill="#EA4335"
                      d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.07l3.66 2.84c.87-2.6 3.3-4.53 6.16-4.53z"
                    />
                  </svg>
                  <span className="text-gray-700 font-medium">
                    Continue with Google
                  </span>
                </button>
              </div>

              {/* Divider */}
              <div className="relative my-8">
                <div className="absolute inset-0 flex items-center">
                  <div className="w-full border-t border-theme" />
                </div>
                <div className="relative flex justify-center">
                  <span className="px-4 text-sm text-theme-muted bg-theme-card">
                    or connect locally
                  </span>
                </div>
              </div>

              {/* Local connection option */}
              <a
                href="#"
                className="group flex items-center justify-between w-full p-4 rounded-xl bg-theme-secondary hover:bg-theme-tertiary transition-all duration-200 border border-transparent hover:border-theme"
              >
                <div className="flex items-center gap-3">
                  <div className="w-10 h-10 rounded-lg bg-theme-card flex items-center justify-center shadow-sm border border-theme-light">
                    <Wifi className="w-5 h-5 text-accent" />
                  </div>
                  <div className="text-left">
                    <div className="font-medium text-theme text-sm">
                      Direct Connection
                    </div>
                    <div className="text-xs text-theme-muted">brewos.local</div>
                  </div>
                </div>
                <ChevronRight className="w-5 h-5 text-theme-muted group-hover:text-accent group-hover:translate-x-1 transition-all duration-200" />
              </a>
            </div>
          </div>

          {/* Footer */}
          <p className="text-center text-xs text-theme-muted mt-6">
            By signing in, you agree to our{" "}
            <a
              href="#"
              className="underline hover:text-theme-secondary transition-colors"
            >
              Terms of Service
            </a>{" "}
            and{" "}
            <a
              href="#"
              className="underline hover:text-theme-secondary transition-colors"
            >
              Privacy Policy
            </a>
          </p>
        </div>
      </div>
    </div>
  );
}

// Mobile login view
function LoginMobileView({ error }: { error?: string }) {
  return (
    <div className="min-h-screen flex items-center justify-center bg-theme p-6 relative overflow-hidden">
      {/* Subtle decorative elements */}
      <div className="absolute top-0 right-0 w-64 h-64 bg-gradient-to-bl from-accent/5 to-transparent rounded-full blur-3xl" />
      <div className="absolute bottom-0 left-0 w-96 h-96 bg-gradient-to-tr from-accent/5 to-transparent rounded-full blur-3xl" />

      <div className="w-full max-w-md relative z-10">
        {/* Mobile logo */}
        <div className="flex justify-center mb-8">
          <Logo size="lg" />
        </div>

        {/* Card */}
        <div className="bg-theme-card rounded-3xl p-8 shadow-2xl border border-theme">
          {/* Header */}
          <div className="text-center mb-8">
            <div className="inline-flex items-center justify-center w-16 h-16 rounded-2xl bg-accent shadow-lg shadow-accent/20 mb-5">
              <LogoIcon size="md" className="filter brightness-0 invert" />
            </div>
            <h2 className="text-2xl font-bold text-theme mb-2">Welcome back</h2>
            <p className="text-theme-muted">Sign in to manage your machines</p>
          </div>

          {error && (
            <div className="mb-6 p-4 rounded-xl flex items-start gap-3 bg-error-soft border border-error">
              <AlertCircle className="w-5 h-5 text-error flex-shrink-0 mt-0.5" />
              <span className="text-sm text-error">{error}</span>
            </div>
          )}

          <div>
            <div className="flex justify-center mb-6">
              <button className="flex items-center gap-3 px-8 py-3 bg-white border border-gray-200 rounded-full shadow-sm">
                <svg className="w-5 h-5" viewBox="0 0 24 24">
                  <path
                    fill="#4285F4"
                    d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92c-.26 1.37-1.04 2.53-2.21 3.31v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.09z"
                  />
                  <path
                    fill="#34A853"
                    d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z"
                  />
                  <path
                    fill="#FBBC05"
                    d="M5.84 14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.07H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.93l2.85-2.22.81-.62z"
                  />
                  <path
                    fill="#EA4335"
                    d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.07l3.66 2.84c.87-2.6 3.3-4.53 6.16-4.53z"
                  />
                </svg>
                <span className="text-gray-700 font-medium">
                  Continue with Google
                </span>
              </button>
            </div>

            <div className="relative my-8">
              <div className="absolute inset-0 flex items-center">
                <div className="w-full border-t border-theme" />
              </div>
              <div className="relative flex justify-center">
                <span className="px-4 text-sm text-theme-muted bg-theme-card">
                  or connect locally
                </span>
              </div>
            </div>

            <a
              href="#"
              className="group flex items-center justify-between w-full p-4 rounded-xl bg-theme-secondary hover:bg-theme-tertiary transition-colors"
            >
              <div className="flex items-center gap-3">
                <div className="w-10 h-10 rounded-lg bg-theme-card flex items-center justify-center shadow-sm border border-theme-light">
                  <Wifi className="w-5 h-5 text-accent" />
                </div>
                <div className="text-left">
                  <div className="font-medium text-theme text-sm">
                    Direct Connection
                  </div>
                  <div className="text-xs text-theme-muted">brewos.local</div>
                </div>
              </div>
              <ChevronRight className="w-5 h-5 text-theme-muted" />
            </a>
          </div>
        </div>

        <p className="text-center text-xs text-theme-muted mt-6">
          By signing in, you agree to our{" "}
          <a href="#" className="underline">
            Terms of Service
          </a>{" "}
          and{" "}
          <a href="#" className="underline">
            Privacy Policy
          </a>
        </p>
      </div>
    </div>
  );
}

// Unconfigured view
function LoginUnconfiguredView() {
  return (
    <div className="min-h-screen bg-gradient-to-br from-coffee-800 to-coffee-900 flex items-center justify-center p-4">
      <div className="w-full max-w-md text-center p-8 rounded-3xl bg-white/5 backdrop-blur-xl border border-white/10">
        <div className="flex justify-center mb-6">
          <LogoIcon size="xl" />
        </div>
        <h1 className="text-2xl font-bold text-white mb-3">Connect Locally</h1>
        <p className="text-white/60 mb-6">
          Cloud features are not configured. Connect directly to your device.
        </p>
        <a
          href="#"
          className="inline-flex items-center gap-2 px-6 py-3 rounded-xl bg-accent text-white font-semibold hover:bg-accent-light transition-all duration-300 shadow-lg shadow-accent/30"
        >
          Go to brewos.local
          <ChevronRight className="w-4 h-4" />
        </a>
      </div>
    </div>
  );
}

export const Default: Story = {
  render: () => <LoginView />,
};

export const WithError: Story = {
  render: () => <LoginView error="Authentication failed. Please try again." />,
};

export const MobileView: Story = {
  render: () => <LoginMobileView />,
  parameters: {
    viewport: {
      defaultViewport: "mobile1",
    },
  },
};

export const MobileViewWithError: Story = {
  render: () => (
    <LoginMobileView error="Invalid credentials. Please try again." />
  ),
  parameters: {
    viewport: {
      defaultViewport: "mobile1",
    },
  },
};

export const CloudNotConfigured: Story = {
  render: () => <LoginUnconfiguredView />,
};
