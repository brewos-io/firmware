import { Badge } from "@/components/Badge";

export interface MilestoneCardProps {
  label: string;
  achieved: boolean;
  icon: string;
  progress?: number;  // 0-100 percentage for upcoming milestones
}

export function MilestoneCard({ label, achieved, icon, progress }: MilestoneCardProps) {
  return (
    <div
      className={`p-4 rounded-xl text-center transition-all relative overflow-hidden ${
        achieved
          ? "bg-gradient-to-br from-accent/10 to-accent/5 border border-accent/20"
          : "bg-theme-secondary"
      }`}
    >
      {/* Progress bar for upcoming milestones */}
      {!achieved && progress !== undefined && progress > 0 && (
        <div 
          className="absolute inset-0 bg-accent/10 transition-all"
          style={{ width: `${progress}%` }}
        />
      )}
      
      <div className="relative">
        <div className={`text-2xl mb-2 ${!achieved ? "opacity-50 grayscale" : ""}`}>
          {icon}
        </div>
        <div
          className={`text-sm font-medium ${
            achieved ? "text-theme" : "text-theme-muted"
          }`}
        >
          {label}
        </div>
        {achieved ? (
          <Badge variant="success" className="mt-2">
            Achieved
          </Badge>
        ) : progress !== undefined && (
          <div className="text-xs text-theme-muted mt-2">
            {Math.round(progress)}%
          </div>
        )}
      </div>
    </div>
  );
}

