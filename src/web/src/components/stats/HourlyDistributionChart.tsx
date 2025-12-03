import { useMemo } from "react";

export interface HourlyData {
  hour: number;
  count: number;
}

export interface HourlyDistributionChartProps {
  data: HourlyData[];
  height?: number;
  emptyMessage?: string;
}

export function HourlyDistributionChart({
  data,
  height = 120,
  emptyMessage = "No data yet",
}: HourlyDistributionChartProps) {
  const chartData = useMemo(() => {
    // Ensure we have 24 hours of data
    const fullDay: HourlyData[] = [];
    for (let h = 0; h < 24; h++) {
      const found = data.find((d) => d.hour === h);
      fullDay.push({ hour: h, count: found?.count ?? 0 });
    }
    
    const maxCount = Math.max(...fullDay.map((d) => d.count), 1);
    const peakHour = fullDay.reduce((prev, curr) =>
      curr.count > prev.count ? curr : prev
    );
    
    return { hours: fullDay, maxCount, peakHour };
  }, [data]);

  const totalShots = data.reduce((sum, d) => sum + d.count, 0);
  
  if (totalShots === 0) {
    return (
      <div
        className="flex items-center justify-center text-theme-muted"
        style={{ height }}
      >
        <p>{emptyMessage}</p>
      </div>
    );
  }

  const formatHour = (hour: number) => {
    if (hour === 0) return "12am";
    if (hour === 12) return "12pm";
    return hour < 12 ? `${hour}am` : `${hour - 12}pm`;
  };

  return (
    <div className="w-full" style={{ height }}>
      {/* Peak hour indicator */}
      <div className="flex justify-between items-center mb-2 px-1">
        <span className="text-xs text-theme-muted">
          Peak: {formatHour(chartData.peakHour.hour)}
        </span>
        <span className="text-xs font-medium text-purple-500">
          {chartData.peakHour.count} shots
        </span>
      </div>
      
      {/* Bar chart */}
      <div className="flex items-end justify-between gap-px h-full max-h-20">
        {chartData.hours.map((item) => {
          const heightPercent = (item.count / chartData.maxCount) * 100;
          const isPeak = item.hour === chartData.peakHour.hour;
          const isMorning = item.hour >= 6 && item.hour < 12;
          const isAfternoon = item.hour >= 12 && item.hour < 18;
          
          return (
            <div
              key={item.hour}
              className="flex-1 flex flex-col items-center group relative"
            >
              {/* Tooltip */}
              <div className="absolute -top-8 left-1/2 -translate-x-1/2 bg-theme-elevated px-2 py-1 rounded text-xs opacity-0 group-hover:opacity-100 transition-opacity whitespace-nowrap z-10 shadow-lg">
                {formatHour(item.hour)}: {item.count}
              </div>
              
              {/* Bar */}
              <div
                className={`
                  w-full rounded-t transition-all duration-300 cursor-pointer
                  ${isPeak ? "bg-purple-500" : 
                    isMorning ? "bg-amber-500/70" : 
                    isAfternoon ? "bg-orange-500/70" : "bg-indigo-500/70"}
                  ${item.count === 0 ? "opacity-20" : ""}
                  group-hover:opacity-100 group-hover:scale-105
                `}
                style={{
                  height: `${Math.max(heightPercent, 4)}%`,
                }}
              />
            </div>
          );
        })}
      </div>
      
      {/* Time labels */}
      <div className="flex justify-between mt-1 text-[10px] text-theme-muted">
        <span>12am</span>
        <span>6am</span>
        <span>12pm</span>
        <span>6pm</span>
        <span>12am</span>
      </div>
    </div>
  );
}

