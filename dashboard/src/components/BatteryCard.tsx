'use client';

import { useState } from 'react';
import { Battery, Info } from 'lucide-react';

interface BatteryCardProps {
  percentage?: number | null;
  voltage?: number | null;
}

export default function BatteryCard({ 
  percentage,
  voltage
}: BatteryCardProps) {
  const [reportedPercentage] = useState(percentage);
  const displayPercentage = reportedPercentage ?? 0;
  
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
          <span className={`flex items-center gap-1.5 px-3 py-1 rounded-full text-[10px] font-black uppercase tracking-wider border ${reportedPercentage === null || reportedPercentage === undefined ? 'bg-zinc-500/10 border-zinc-500/20 text-zinc-500' : `${getBatteryBgColor(displayPercentage)} ${getBatteryTextColor(displayPercentage)}`}`}>
          {reportedPercentage === null || reportedPercentage === undefined ? 'Not reported' : displayPercentage > 20 ? 'Good' : 'Low Battery'}
        </span>
      </div>

      <div className="flex flex-col gap-4">
        {/* Percentage Indicator above the Bar */}
        <div className="flex justify-between items-end">
          <span className="text-2xl sm:text-3xl font-black text-white tracking-tight tabular-nums select-none">
            {reportedPercentage === null || reportedPercentage === undefined ? '—' : `${displayPercentage}%`}
          </span>
          <span className="text-[10px] text-zinc-500 font-bold uppercase tracking-wider">
            Charge Level
          </span>
        </div>

        {/* Visual Battery Bar */}
        <div className="w-full h-3 bg-zinc-900 rounded-full overflow-hidden border border-zinc-850 p-[1px]">
          <div 
            className={`h-full rounded-full transition-all duration-1000 ${getBatteryColor(displayPercentage)}`}
            style={{ width: `${displayPercentage}%` }}
          />
        </div>

        <div className="grid grid-cols-2 gap-4">
          <div className="bg-zinc-900/30 border border-zinc-850 rounded-2xl p-3 text-left">
            <span className="text-[10px] text-zinc-500 font-bold uppercase tracking-wider block mb-0.5">Est. Remaining</span>
            <span className="text-sm font-black text-zinc-200">Not measured</span>
          </div>
          <div className="bg-zinc-900/30 border border-zinc-850 rounded-2xl p-3 text-left">
            <span className="text-[10px] text-zinc-500 font-bold uppercase tracking-wider block mb-0.5">Voltage</span>
            <span className="text-sm font-black text-zinc-200 font-mono">{voltage === null || voltage === undefined ? 'Not reported' : `${voltage.toFixed(2)} V`}</span>
          </div>
        </div>
      </div>
      
      <p className="text-[10px] text-zinc-500 font-mono flex items-center gap-1 leading-normal text-left">
        <Info className="w-3.5 h-3.5 text-zinc-650 shrink-0" />
        Values appear only when reported by device telemetry.
      </p>
    </div>
  );
}
