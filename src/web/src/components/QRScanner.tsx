import { useEffect, useRef, useState, useCallback } from "react";
import { Html5Qrcode, Html5QrcodeScannerState } from "html5-qrcode";
import { Camera, CameraOff, RefreshCw } from "lucide-react";
import { Button } from "./Button";

interface QRScannerProps {
  onScan: (result: string) => void;
  onError?: (error: string) => void;
}

export function QRScanner({ onScan, onError }: QRScannerProps) {
  const scannerRef = useRef<Html5Qrcode | null>(null);
  const [isScanning, setIsScanning] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [hasPermission, setHasPermission] = useState<boolean | null>(null);
  const containerRef = useRef<HTMLDivElement>(null);

  const startScanner = useCallback(async () => {
    if (!containerRef.current) return;

    setError(null);

    try {
      // Create scanner instance if not exists
      if (!scannerRef.current) {
        scannerRef.current = new Html5Qrcode("qr-reader");
      }

      // Check if already scanning
      if (scannerRef.current.getState() === Html5QrcodeScannerState.SCANNING) {
        return;
      }

      await scannerRef.current.start(
        { facingMode: "environment" },
        {
          fps: 10,
          qrbox: { width: 250, height: 250 },
          aspectRatio: 1,
        },
        (decodedText) => {
          // Stop scanning after successful read
          stopScanner();
          onScan(decodedText);
        },
        () => {
          // Ignore scan errors (no QR found yet)
        }
      );

      setIsScanning(true);
      setHasPermission(true);
    } catch (err) {
      const errorMessage =
        err instanceof Error ? err.message : "Failed to start camera";

      if (
        errorMessage.includes("Permission") ||
        errorMessage.includes("NotAllowed")
      ) {
        setHasPermission(false);
        setError("Camera permission denied. Please allow camera access.");
      } else if (errorMessage.includes("NotFound")) {
        setError("No camera found on this device.");
      } else {
        setError(errorMessage);
      }

      onError?.(errorMessage);
    }
  }, [onScan, onError]);

  const stopScanner = async () => {
    if (scannerRef.current) {
      try {
        if (
          scannerRef.current.getState() === Html5QrcodeScannerState.SCANNING
        ) {
          await scannerRef.current.stop();
        }
      } catch {
        // Ignore stop errors
      }
    }
    setIsScanning(false);
  };

  // Auto-start scanner on mount
  useEffect(() => {
    const timer = setTimeout(() => {
      startScanner();
    }, 100);

    return () => {
      clearTimeout(timer);
      stopScanner();
    };
  }, [startScanner]);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      if (scannerRef.current) {
        try {
          scannerRef.current.clear();
        } catch {
          // Ignore cleanup errors
        }
        scannerRef.current = null;
      }
    };
  }, []);

  return (
    <div className="w-full" ref={containerRef}>
      {/* Scanner container */}
      <div
        id="qr-reader"
        className="w-full rounded-xl overflow-hidden bg-black"
        style={{ minHeight: "280px" }}
      />

      {/* Error state */}
      {error && (
        <div className="mt-4 p-4 bg-error-soft border border-error rounded-xl text-center">
          <CameraOff className="w-8 h-8 text-error mx-auto mb-2" />
          <p className="text-sm text-error mb-3">{error}</p>
          <Button variant="secondary" size="sm" onClick={startScanner}>
            <RefreshCw className="w-4 h-4" />
            Try Again
          </Button>
        </div>
      )}

      {/* Permission denied instructions */}
      {hasPermission === false && (
        <div className="mt-4 p-4 bg-warning-soft border border-warning rounded-xl">
          <p className="text-sm text-warning mb-2 font-medium">
            To scan QR codes, allow camera access:
          </p>
          <ol className="text-xs text-warning space-y-1 list-decimal list-inside">
            <li>Click the camera/lock icon in your browser's address bar</li>
            <li>Allow camera permissions for this site</li>
            <li>Refresh the page or click "Try Again"</li>
          </ol>
        </div>
      )}

      {/* Scanning indicator */}
      {isScanning && !error && (
        <div className="mt-4 flex items-center justify-center gap-2 text-sm text-theme-muted">
          <Camera className="w-4 h-4 animate-pulse" />
          <span>Point camera at QR code...</span>
        </div>
      )}
    </div>
  );
}
