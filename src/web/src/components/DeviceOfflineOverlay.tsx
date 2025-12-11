import { Link } from "react-router-dom";
import { WifiOff, ArrowRight, Coffee, RefreshCw } from "lucide-react";
import { Button } from "./Button";
import { useThemeStore } from "@/lib/themeStore";

interface DeviceOfflineOverlayProps {
  deviceName?: string;
  onRetry?: () => void;
}

/**
 * Full-screen overlay shown when a cloud-connected device is offline.
 * Uses z-30, lower than header (z-50) and nav (z-40), so those remain accessible
 * for navigation (machines page, sign out).
 */
export function DeviceOfflineOverlay({
  deviceName,
  onRetry,
}: DeviceOfflineOverlayProps) {
  const { theme } = useThemeStore();
  const isDark = theme.isDark;

  return (
    <div
      className={`
        fixed inset-0 z-30
        flex items-center justify-center p-6
        ${
          isDark
            ? "bg-zinc-950/95 backdrop-blur-sm"
            : "bg-white/95 backdrop-blur-sm"
        }
      `}
    >
      <div className="max-w-md w-full text-center space-y-8 relative z-10">
        {/* Animated Icon */}
        <div className="relative inline-flex items-center justify-center">
          {/* Pulsing rings */}
          <div
            className={`absolute w-32 h-32 rounded-full animate-ping opacity-20 ${
              isDark ? "bg-red-500" : "bg-red-400"
            }`}
            style={{ animationDuration: "2s" }}
          />
          <div
            className={`absolute w-28 h-28 rounded-full animate-pulse opacity-30 ${
              isDark ? "bg-red-500" : "bg-red-400"
            }`}
          />

          {/* Icon container */}
          <div
            className={`
              relative w-24 h-24 rounded-full flex items-center justify-center
              ${
                isDark
                  ? "bg-gradient-to-br from-red-500/30 to-red-600/20 border-2 border-red-400/40"
                  : "bg-gradient-to-br from-red-100 to-red-200 border-2 border-red-300"
              }
            `}
          >
            <WifiOff
              className={`w-10 h-10 ${
                isDark ? "text-red-400" : "text-red-500"
              }`}
            />
          </div>
        </div>

        {/* Status Text */}
        <div className="space-y-3">
          <h2
            className={`text-2xl font-bold ${
              isDark ? "text-white" : "text-zinc-900"
            }`}
          >
            Machine Offline
          </h2>
          <p
            className={`text-base leading-relaxed ${
              isDark ? "text-zinc-400" : "text-zinc-600"
            }`}
          >
            {deviceName ? (
              <>
                <span
                  className={`font-medium ${
                    isDark ? "text-zinc-200" : "text-zinc-800"
                  }`}
                >
                  {deviceName}
                </span>{" "}
                is currently not connected. Check that your machine is powered
                on and connected to the internet.
              </>
            ) : (
              <>
                Your machine is currently not connected. Check that it's powered
                on and connected to the internet.
              </>
            )}
          </p>
        </div>

        {/* Action Buttons */}
        <div className="flex flex-col sm:flex-row gap-3 justify-center">
          <Link to="/machines">
            <Button
              variant="primary"
              size="lg"
              className="w-full sm:w-auto min-w-[180px]"
            >
              <Coffee className="w-5 h-5" />
              Select Machine
              <ArrowRight className="w-4 h-4" />
            </Button>
          </Link>

          {onRetry && (
            <Button
              variant="secondary"
              size="lg"
              onClick={onRetry}
              className="w-full sm:w-auto min-w-[140px]"
            >
              <RefreshCw className="w-5 h-5" />
              Retry
            </Button>
          )}
        </div>

        {/* Help text with arrow pointing up */}
        <div
          className={`
            text-sm pt-6 
            ${isDark ? "text-zinc-500" : "text-zinc-500"}
          `}
        >
          <div className="flex items-center justify-center gap-2 mb-2">
            <div
              className={`w-8 h-px ${isDark ? "bg-zinc-700" : "bg-zinc-300"}`}
            />
            <span className="text-xs uppercase tracking-wider font-medium">
              or
            </span>
            <div
              className={`w-8 h-px ${isDark ? "bg-zinc-700" : "bg-zinc-300"}`}
            />
          </div>
          <p>Use the header above to switch machines or sign out</p>
        </div>
      </div>

      {/* Subtle decorative elements */}
      <div className="absolute inset-0 overflow-hidden pointer-events-none">
        <div
          className={`absolute -top-32 -right-32 w-64 h-64 rounded-full opacity-5 ${
            isDark ? "bg-red-500" : "bg-red-400"
          }`}
        />
        <div
          className={`absolute -bottom-20 -left-20 w-48 h-48 rounded-full opacity-5 ${
            isDark ? "bg-red-500" : "bg-red-400"
          }`}
        />
      </div>
    </div>
  );
}
