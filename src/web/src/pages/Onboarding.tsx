import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { Card } from "@/components/Card";
import {
  WelcomeStep,
  ScanStep,
  ManualStep,
  SuccessStep,
} from "@/components/onboarding";
import { useAppStore } from "@/lib/mode";

export function Onboarding() {
  const navigate = useNavigate();
  const { claimDevice, fetchDevices } = useAppStore();

  const [step, setStep] = useState<"welcome" | "scan" | "manual" | "success">(
    "welcome"
  );
  const [claimCode, setClaimCode] = useState("");
  const [deviceName, setDeviceName] = useState("");
  const [claiming, setClaiming] = useState(false);
  const [error, setError] = useState("");

  const handleClaim = async (code?: string) => {
    const codeToUse = code || claimCode;
    if (!codeToUse) return;

    setClaiming(true);
    setError("");

    try {
      // Parse QR code URL or manual entry
      let deviceId = "";
      let token = "";
      let manualCode = "";

      if (codeToUse.includes("?")) {
        // URL format
        try {
          const url = new URL(codeToUse);
          deviceId = url.searchParams.get("id") || "";
          token = url.searchParams.get("token") || "";
        } catch {
          // Try parsing as query string
          const params = new URLSearchParams(codeToUse.split("?")[1]);
          deviceId = params.get("id") || "";
          token = params.get("token") || "";
        }
      } else if (codeToUse.includes(":")) {
        // Legacy format: DEVICE_ID:TOKEN
        const parts = codeToUse.split(":");
        deviceId = parts[0];
        token = parts[1] || "";
      } else if (/^[A-Z0-9]{4}-[A-Z0-9]{4}$/i.test(codeToUse.trim())) {
        // Short manual code format: X6ST-AP3G
        manualCode = codeToUse.trim().toUpperCase();
      } else {
        setError("Invalid code format");
        setClaiming(false);
        return;
      }

      // If we have a manual code, use it directly (backend will resolve it)
      if (manualCode) {
        const success = await claimDevice(
          manualCode,
          "",
          deviceName || undefined
        );
        if (success) {
          setStep("success");
          await fetchDevices();
          setTimeout(() => navigate("/"), 2000);
        } else {
          setError("Invalid or expired code");
        }
        setClaiming(false);
        return;
      }

      if (!deviceId || !token) {
        setError("Invalid code format");
        setClaiming(false);
        return;
      }

      const success = await claimDevice(
        deviceId,
        token,
        deviceName || undefined
      );

      if (success) {
        setStep("success");
        await fetchDevices();
        setTimeout(() => navigate("/"), 2000);
      } else {
        setError("Failed to add device. The code may have expired.");
      }
    } catch {
      setError("An error occurred");
    }

    setClaiming(false);
  };

  return (
    <div
      className={`full-page-scroll bg-gradient-to-br from-coffee-800 via-coffee-900 to-coffee-950 p-4 transition-all duration-300 ${
        step === "welcome"
          ? "flex flex-col items-center justify-center"
          : "flex justify-center"
      }`}
    >
      {step === "welcome" ? (
        <div className="w-full max-w-lg">
          <Card className="text-center animate-in fade-in slide-in-from-bottom-4 duration-500">
            <WelcomeStep
              onScanClick={() => setStep("scan")}
              onManualClick={() => setStep("manual")}
            />
          </Card>
        </div>
      ) : (
        <div className={`w-full max-w-lg transition-all duration-300 pt-16`}>
          {step === "scan" && (
            <Card className="animate-in fade-in slide-in-from-right-4 duration-300">
              <ScanStep
                deviceName={deviceName}
                onDeviceNameChange={setDeviceName}
                onScan={(result) => {
                  setClaimCode(result);
                  setError(""); // Clear any previous errors
                }}
                onScanError={(err) => {
                  setError(err || "Failed to scan QR code");
                }}
                error={error}
                onBack={() => {
                  setStep("welcome");
                  setError("");
                  setClaimCode("");
                }}
                onAdd={() => handleClaim()}
                disabled={!claimCode || claiming}
                loading={claiming}
              />
            </Card>
          )}

          {step === "manual" && (
            <Card className="animate-in fade-in slide-in-from-right-4 duration-300">
              <ManualStep
                claimCode={claimCode}
                deviceName={deviceName}
                onClaimCodeChange={setClaimCode}
                onDeviceNameChange={setDeviceName}
                error={error}
                onBack={() => {
                  setStep("welcome");
                  setError("");
                }}
                onAdd={() => handleClaim()}
                disabled={!claimCode}
                loading={claiming}
              />
            </Card>
          )}

          {step === "success" && (
            <Card className="animate-in fade-in zoom-in-95 duration-500">
              <SuccessStep deviceName={deviceName} />
            </Card>
          )}
        </div>
      )}
    </div>
  );
}
