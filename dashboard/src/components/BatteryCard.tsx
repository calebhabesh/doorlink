'use client';

import { useState } from 'react';
import { Battery, Info } from 'lucide-react';

interface BatteryCardProps {
  initialPercentage?: number;
  voltage?: number;
}

export default function BatteryCard({ 
  initialPercentage = 85, 
  voltage = 4.02 
}: BatteryCardProps) {
  const [percentage] = useState(initialPercentage);
  
  // Custom logic to estimate remaining days.
  // 100% capacity is target 27 days. 
  // Let's assume linear discharge based on typical usage.
  const estimatedDays = Math.round((percentage / 100) * 27);
  
  const getBatteryColor = (pct: number) => {
    if (pct > 50) return 'bg-emerald-500';
    if (pct > 20) return 'bg-amber-500';
    return 'bg-rose-500';
  };

  const getBatteryTextColor = (pct: number) => {
    if (pct > 50) return 'text-emerald-500';
    if (pct > 20) return 'text-amber-500';
    return 'text-rose-500';
  };

  const getBatteryBgColor = (pct: number) => {
    if (pct > 50) return 'bg-emerald-500/10 border-emerald-500/20';
    if (pct > 20) return 'bg-amber-500/10 border-amber-500/20';
    return 'bg-rose-500/10 border-rose-500/20';
  };

  return (
    <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-3xl flex flex-col hover:border-zinc-700 transition-all duration-300 shadow-lg group relative overflow-hidden animate-flash-event h-full justify-between">
      <div className="absolute inset-0 bg-gradient-to-br from-emerald-500/[0.01] via-transparent to-transparent pointer-events-none" />
      
      <div className="flex justify-between items-center">
        <h3 className="text-zinc-500 text-xs font-bold uppercase tracking-widest flex items-center gap-1.5 text-left">
          <Battery className="w-3.5 h-3.5 text-zinc-400" />
          Device Power
        </h3>
        <span className={`flex items-center gap-1.5 px-3 py-1 rounded-full text-[10px] font-black uppercase tracking-wider border ${getBatteryBgColor(percentage)} ${getBatteryTextColor(percentage)}`}>
          <span className={`w-1.5 h-1.5 rounded-full ${percentage > 20 ? 'bg-emerald-500 animate-pulse' : 'bg-rose-500'}`}></span>
          {percentage > 20 ? 'Good' : 'Low Battery'}
        </span>
      </div>

      <div className="flex flex-col gap-4">
        {/* Percentage Indicator above the Bar */}
        <div className="flex justify-between items-end">
          <span className="text-2xl sm:text-3xl font-black text-white tracking-tight tabular-nums select-none">
            {percentage}%
          </span>
          <span className="text-[10px] text-zinc-500 font-bold uppercase tracking-wider">
            Charge Level
          </span>
        </div>

        {/* Visual Battery Bar */}
        <div className="w-full h-3 bg-zinc-900 rounded-full overflow-hidden border border-zinc-850 p-[1px]">
          <div 
            className={`h-full rounded-full transition-all duration-1000 ${getBatteryColor(percentage)}`}
            style={{ width: `${percentage}%` }}
          />
        </div>

        <div className="grid grid-cols-2 gap-4">
          <div className="bg-zinc-900/30 border border-zinc-850 rounded-2xl p-3 text-left">
            <span className="text-[10px] text-zinc-500 font-bold uppercase tracking-wider block mb-0.5">Est. Remaining</span>
            <span className="text-sm font-black text-zinc-200">~{estimatedDays} Days</span>
          </div>
          <div className="bg-zinc-900/30 border border-zinc-850 rounded-2xl p-3 text-left">
            <span className="text-[10px] text-zinc-500 font-bold uppercase tracking-wider block mb-0.5">Voltage</span>
            <span className="text-sm font-black text-zinc-200 font-mono">{voltage.toFixed(2)} V</span>
          </div>
        </div>
      </div>
      
      <p className="text-[10px] text-zinc-500 font-mono flex items-center gap-1 leading-normal text-left">
        <Info className="w-3.5 h-3.5 text-zinc-650 shrink-0" />
        Calculated from event-driven deep sleep telemetry.
      </p>
    </div>
  );
}
