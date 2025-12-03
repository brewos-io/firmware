import { useMemo } from "react";
import type { PowerSample } from "@/lib/types";

export interface PowerChartProps {
  data: PowerSample[];
  height?: number;
  showLabels?: boolean;
  emptyMessage?: string;
}

export function PowerChart({
  data,
  height = 200,
  showLabels = true,
  emptyMessage = "No power data available",
}: PowerChartProps) {
  const chartData = useMemo(() => {
    if (data.length === 0) return { points: [], maxWatts: 1000, totalKwh: 0 };
    
    const maxWatts = Math.max(...data.map((d) => d.maxWatts), 1);
    const totalKwh = data.reduce((sum, d) => sum + d.kwhConsumed, 0);
    
    const points = data.map((sample, index) => {
      const x = (index / Math.max(data.length - 1, 1)) * 100;
      const y = 100 - (sample.avgWatts / maxWatts) * 100;
      return { x, y, sample };
    });
    
    return { points, maxWatts, totalKwh };
  }, [data]);

  if (data.length === 0) {
    return (
      <div
        className="flex items-center justify-center text-theme-muted"
        style={{ height }}
      >
        <p>{emptyMessage}</p>
      </div>
    );
  }

  // Create SVG path for area chart
  const areaPath = useMemo(() => {
    if (chartData.points.length < 2) return "";
    
    let path = `M ${chartData.points[0].x} ${chartData.points[0].y}`;
    for (let i = 1; i < chartData.points.length; i++) {
      path += ` L ${chartData.points[i].x} ${chartData.points[i].y}`;
    }
    // Close the path to create area
    path += ` L 100 100 L 0 100 Z`;
    return path;
  }, [chartData.points]);

  // Create line path
  const linePath = useMemo(() => {
    if (chartData.points.length < 2) return "";
    
    let path = `M ${chartData.points[0].x} ${chartData.points[0].y}`;
    for (let i = 1; i < chartData.points.length; i++) {
      path += ` L ${chartData.points[i].x} ${chartData.points[i].y}`;
    }
    return path;
  }, [chartData.points]);

  const formatTime = (timestamp: number) => {
    const date = new Date(timestamp * 1000);
    return date.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
  };

  return (
    <div className="w-full" style={{ height }}>
      {/* Chart header with summary */}
      {showLabels && (
        <div className="flex justify-between items-center mb-2 px-1">
          <span className="text-xs text-theme-muted">
            Peak: {Math.round(chartData.maxWatts)}W
          </span>
          <span className="text-xs font-medium text-accent">
            Total: {chartData.totalKwh.toFixed(2)} kWh
          </span>
        </div>
      )}
      
      {/* SVG Chart */}
      <div className="relative w-full" style={{ height: height - (showLabels ? 48 : 16) }}>
        <svg
          viewBox="0 0 100 100"
          preserveAspectRatio="none"
          className="w-full h-full"
        >
          {/* Grid lines */}
          <defs>
            <linearGradient id="powerGradient" x1="0%" y1="0%" x2="0%" y2="100%">
              <stop offset="0%" stopColor="var(--color-accent)" stopOpacity="0.4" />
              <stop offset="100%" stopColor="var(--color-accent)" stopOpacity="0.05" />
            </linearGradient>
          </defs>
          
          {/* Horizontal grid lines */}
          {[25, 50, 75].map((y) => (
            <line
              key={y}
              x1="0"
              y1={y}
              x2="100"
              y2={y}
              stroke="currentColor"
              strokeOpacity="0.1"
              strokeDasharray="2,2"
            />
          ))}
          
          {/* Area fill */}
          <path
            d={areaPath}
            fill="url(#powerGradient)"
            className="transition-all duration-500"
          />
          
          {/* Line */}
          <path
            d={linePath}
            fill="none"
            stroke="var(--color-accent)"
            strokeWidth="2"
            strokeLinecap="round"
            strokeLinejoin="round"
            vectorEffect="non-scaling-stroke"
            className="transition-all duration-500"
          />
          
          {/* Data points */}
          {chartData.points.length <= 48 && chartData.points.map((point, i) => (
            <circle
              key={i}
              cx={point.x}
              cy={point.y}
              r="1.5"
              fill="var(--color-accent)"
              className="hover:r-3 transition-all cursor-pointer"
            />
          ))}
        </svg>
        
        {/* Y-axis labels */}
        {showLabels && (
          <div className="absolute left-0 top-0 h-full flex flex-col justify-between text-[10px] text-theme-muted -ml-1">
            <span>{Math.round(chartData.maxWatts)}W</span>
            <span>{Math.round(chartData.maxWatts / 2)}W</span>
            <span>0W</span>
          </div>
        )}
      </div>
      
      {/* X-axis labels */}
      {showLabels && data.length > 0 && (
        <div className="flex justify-between mt-1 px-1 text-[10px] text-theme-muted">
          <span>{formatTime(data[0].timestamp)}</span>
          {data.length > 2 && (
            <span>{formatTime(data[Math.floor(data.length / 2)].timestamp)}</span>
          )}
          <span>{formatTime(data[data.length - 1].timestamp)}</span>
        </div>
      )}
    </div>
  );
}

