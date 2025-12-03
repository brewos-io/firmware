import { useMemo } from "react";
import type { DailySummary } from "@/lib/types";

export interface BrewTrendsChartProps {
  data: DailySummary[];
  height?: number;
  showLabels?: boolean;
  emptyMessage?: string;
}

export function BrewTrendsChart({
  data,
  height = 200,
  showLabels = true,
  emptyMessage = "No trend data available",
}: BrewTrendsChartProps) {
  const chartData = useMemo(() => {
    if (data.length === 0) return { points: [], maxShots: 10, avgShots: 0 };
    
    const maxShots = Math.max(...data.map((d) => d.shotCount), 1);
    const avgShots = data.reduce((sum, d) => sum + d.shotCount, 0) / data.length;
    
    const points = data.map((day, index) => {
      const x = (index / Math.max(data.length - 1, 1)) * 100;
      const y = 100 - (day.shotCount / maxShots) * 100;
      return { x, y, day };
    });
    
    return { points, maxShots, avgShots };
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

  // Create smooth curve path using bezier curves
  const linePath = useMemo(() => {
    if (chartData.points.length < 2) return "";
    
    let path = `M ${chartData.points[0].x} ${chartData.points[0].y}`;
    
    for (let i = 1; i < chartData.points.length; i++) {
      const prev = chartData.points[i - 1];
      const curr = chartData.points[i];
      const cpX = (prev.x + curr.x) / 2;
      path += ` Q ${cpX} ${prev.y} ${curr.x} ${curr.y}`;
    }
    
    return path;
  }, [chartData.points]);

  // Area path
  const areaPath = useMemo(() => {
    if (!linePath) return "";
    return `${linePath} L 100 100 L 0 100 Z`;
  }, [linePath]);

  // Average line Y position
  const avgY = useMemo(() => {
    if (chartData.maxShots === 0) return 50;
    return 100 - (chartData.avgShots / chartData.maxShots) * 100;
  }, [chartData.avgShots, chartData.maxShots]);

  const formatDate = (timestamp: number) => {
    const date = new Date(timestamp * 1000);
    return date.toLocaleDateString([], { month: "short", day: "numeric" });
  };

  return (
    <div className="w-full" style={{ height }}>
      {/* Chart header */}
      {showLabels && (
        <div className="flex justify-between items-center mb-2 px-1">
          <span className="text-xs text-theme-muted">
            Avg: {chartData.avgShots.toFixed(1)} shots/day
          </span>
          <span className="text-xs font-medium text-emerald-500">
            Total: {data.reduce((sum, d) => sum + d.shotCount, 0)} shots
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
          <defs>
            <linearGradient id="trendsGradient" x1="0%" y1="0%" x2="0%" y2="100%">
              <stop offset="0%" stopColor="#10b981" stopOpacity="0.3" />
              <stop offset="100%" stopColor="#10b981" stopOpacity="0.02" />
            </linearGradient>
          </defs>
          
          {/* Grid lines */}
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
          
          {/* Average line */}
          <line
            x1="0"
            y1={avgY}
            x2="100"
            y2={avgY}
            stroke="#f59e0b"
            strokeWidth="1"
            strokeDasharray="4,4"
            strokeOpacity="0.6"
          />
          
          {/* Area fill */}
          <path
            d={areaPath}
            fill="url(#trendsGradient)"
            className="transition-all duration-500"
          />
          
          {/* Line */}
          <path
            d={linePath}
            fill="none"
            stroke="#10b981"
            strokeWidth="2"
            strokeLinecap="round"
            strokeLinejoin="round"
            vectorEffect="non-scaling-stroke"
            className="transition-all duration-500"
          />
          
          {/* Data points */}
          {chartData.points.map((point, i) => (
            <g key={i} className="group cursor-pointer">
              <circle
                cx={point.x}
                cy={point.y}
                r="3"
                fill="#10b981"
                className="transition-all group-hover:r-5"
              />
              <circle
                cx={point.x}
                cy={point.y}
                r="6"
                fill="transparent"
                className="group-hover:fill-emerald-500/20"
              />
            </g>
          ))}
        </svg>
        
        {/* Y-axis labels */}
        {showLabels && (
          <div className="absolute left-0 top-0 h-full flex flex-col justify-between text-[10px] text-theme-muted -ml-1">
            <span>{chartData.maxShots}</span>
            <span>{Math.round(chartData.maxShots / 2)}</span>
            <span>0</span>
          </div>
        )}
      </div>
      
      {/* X-axis labels */}
      {showLabels && data.length > 0 && (
        <div className="flex justify-between mt-1 px-1 text-[10px] text-theme-muted">
          <span>{formatDate(data[0].date)}</span>
          {data.length > 2 && (
            <span>{formatDate(data[Math.floor(data.length / 2)].date)}</span>
          )}
          <span>{formatDate(data[data.length - 1].date)}</span>
        </div>
      )}
    </div>
  );
}

