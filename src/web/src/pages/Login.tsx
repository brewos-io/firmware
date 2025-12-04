import { useEffect } from "react";
import { useNavigate, useLocation } from "react-router-dom";
import { GoogleLogin, CredentialResponse } from "@react-oauth/google";
import { Logo, LogoIcon } from "@/components/Logo";
import { useAuth } from "@/lib/auth";
import { isGoogleAuthConfigured } from "@/lib/auth";
import { useThemeStore } from "@/lib/themeStore";
import {
  AlertCircle,
  Coffee,
  Gauge,
  Clock,
  Wifi,
  ChevronRight,
} from "lucide-react";

// Feature highlight card
function FeatureCard({
  icon: Icon,
  title,
  description,
  delay,
}: {
  icon: React.ElementType;
  title: string;
  description: string;
  delay: number;
}) {
  return (
    <div
      className="flex items-start gap-4 opacity-0"
      style={{
        animation: `slideUp 0.6s ease-out ${delay}s forwards`,
      }}
    >
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
function FloatingOrb({
  size,
  x,
  y,
  delay,
  duration,
}: {
  size: number;
  x: number;
  y: number;
  delay: number;
  duration: number;
}) {
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
        animation: `float ${duration}s ease-in-out ${delay}s infinite`,
      }}
    />
  );
}

export function Login() {
  const navigate = useNavigate();
  const location = useLocation();
  const { user, handleGoogleLogin } = useAuth();
  const { theme } = useThemeStore();
  const isDark = theme.isDark;

  const error = (location.state as { error?: string })?.error;

  useEffect(() => {
    if (user) {
      navigate("/machines");
    }
  }, [user, navigate]);

  if (!isGoogleAuthConfigured) {
    return (
      <div className="full-page bg-gradient-to-br from-coffee-800 to-coffee-900 flex items-center justify-center p-4">
        <div
          className="w-full max-w-md text-center p-8 rounded-3xl bg-white/5 backdrop-blur-xl border border-white/10"
          style={{ animation: "fadeIn 0.6s ease-out" }}
        >
          <div className="flex justify-center mb-6">
            <LogoIcon size="xl" />
          </div>
          <h1 className="text-2xl font-bold text-white mb-3">
            Connect Locally
          </h1>
          <p className="text-white/60 mb-6">
            Cloud features are not configured. Connect directly to your device.
          </p>
          <a
            href="http://brewos.local"
            className="inline-flex items-center gap-2 px-6 py-3 rounded-xl bg-accent text-white font-semibold hover:bg-accent-light transition-all duration-300 shadow-lg shadow-accent/30"
          >
            Go to brewos.local
            <ChevronRight className="w-4 h-4" />
          </a>
        </div>
      </div>
    );
  }

  return (
    <div className="full-page flex overflow-hidden">
      {/* CSS Animations */}
      <style>{`
        @keyframes float {
          0%, 100% {
            transform: translateY(0) translateX(0);
          }
          33% {
            transform: translateY(-20px) translateX(10px);
          }
          66% {
            transform: translateY(10px) translateX(-5px);
          }
        }
        
        @keyframes fadeIn {
          from {
            opacity: 0;
          }
          to {
            opacity: 1;
          }
        }
        
        @keyframes slideUp {
          from {
            opacity: 0;
            transform: translateY(30px);
          }
          to {
            opacity: 1;
            transform: translateY(0);
          }
        }
        
        @keyframes slideInLeft {
          from {
            opacity: 0;
            transform: translateX(-40px);
          }
          to {
            opacity: 1;
            transform: translateX(0);
          }
        }
        
        @keyframes slideInRight {
          from {
            opacity: 0;
            transform: translateX(40px);
          }
          to {
            opacity: 1;
            transform: translateX(0);
          }
        }
        
        .login-card {
          animation: slideInRight 0.8s ease-out 0.2s both;
        }
        
        .hero-content {
          animation: slideInLeft 0.8s ease-out 0.1s both;
        }
        
        .google-btn-wrapper {
          animation: slideUp 0.6s ease-out 0.5s both;
        }
      `}</style>

      {/* Left Panel - Hero/Brand Section (hidden on mobile) */}
      <div className="hidden lg:flex lg:w-1/2 xl:w-[55%] relative bg-gradient-to-br from-coffee-900 via-coffee-800 to-coffee-900 overflow-hidden">
        {/* Animated background orbs */}
        <FloatingOrb size={400} x={-10} y={20} delay={0} duration={8} />
        <FloatingOrb size={300} x={60} y={60} delay={2} duration={10} />
        <FloatingOrb size={250} x={30} y={-5} delay={4} duration={12} />

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
          <div className="hero-content">
            <Logo size="lg" forceLight />
          </div>

          {/* Center - Hero messaging */}
          <div className="hero-content flex-1 flex flex-col justify-center max-w-lg">
            <h1 className="text-4xl xl:text-5xl font-bold text-white mb-6 leading-tight">
              Craft the perfect
              <span className="block text-accent-light">
                espresso experience
              </span>
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
                delay={0.6}
              />
              <FeatureCard
                icon={Clock}
                title="Smart Scheduling"
                description="Wake up to a pre-heated machine, ready when you are"
                delay={0.8}
              />
              <FeatureCard
                icon={Wifi}
                title="Remote Monitoring"
                description="Keep an eye on your machine from anywhere via cloud"
                delay={1.0}
              />
            </div>
          </div>

          {/* Bottom - Social proof / tagline */}
          <div
            className="opacity-0"
            style={{ animation: "slideUp 0.6s ease-out 1.2s forwards" }}
          >
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

        <div className="login-card w-full max-w-md relative z-10">
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
              <p className="text-theme-muted">
                Sign in to manage your machines
              </p>
            </div>

            {/* Error message */}
            {error && (
              <div
                className="mb-6 p-4 rounded-xl flex items-start gap-3 bg-error-soft border border-error"
                style={{ animation: "slideUp 0.3s ease-out" }}
              >
                <AlertCircle className="w-5 h-5 text-error flex-shrink-0 mt-0.5" />
                <span className="text-sm text-error">{error}</span>
              </div>
            )}

            {/* Google Sign In */}
            <div className="google-btn-wrapper">
              <div className="flex justify-center mb-6">
                <div className="transform hover:scale-[1.02] transition-transform duration-200">
                  <GoogleLogin
                    onSuccess={(response: CredentialResponse) => {
                      if (response.credential) {
                        handleGoogleLogin(response.credential);
                      }
                    }}
                    onError={() => {
                      console.error("Google login failed");
                    }}
                    theme={isDark ? "filled_black" : "outline"}
                    size="large"
                    width="320"
                    text="continue_with"
                    shape="pill"
                  />
                </div>
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
                href="http://brewos.local"
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
          <p
            className="text-center text-xs text-theme-muted mt-6 opacity-0"
            style={{ animation: "fadeIn 0.6s ease-out 0.8s forwards" }}
          >
            By signing in, you agree to our{" "}
            <a
              href="/terms"
              className="underline hover:text-theme-secondary transition-colors"
            >
              Terms of Service
            </a>{" "}
            and{" "}
            <a
              href="/privacy"
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
