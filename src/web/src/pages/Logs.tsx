import { useNavigate } from "react-router-dom";
import { useStore } from "@/lib/store";
import { PageHeader } from "@/components/PageHeader";
import { Button } from "@/components/Button";
import { LogViewer } from "@/components/LogViewer";
import { Trash2, ArrowLeft } from "lucide-react";

export function Logs() {
  const navigate = useNavigate();
  const clearLogs = useStore((s) => s.clearLogs);

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-4">
          <Button
            variant="ghost"
            size="sm"
            onClick={() => navigate(-1)}
            className="flex items-center gap-2"
          >
            <ArrowLeft className="w-4 h-4" />
            Back
          </Button>
          <PageHeader
            title="System Logs"
            subtitle="View system logs and diagnostics"
          />
        </div>
        <Button variant="secondary" size="sm" onClick={clearLogs}>
          <Trash2 className="w-4 h-4" />
          Clear Logs
        </Button>
      </div>

      {/* Full-screen log viewer */}
      <div className="h-[calc(100vh-12rem)]">
        <LogViewer maxHeight="h-full" />
      </div>
    </div>
  );
}

