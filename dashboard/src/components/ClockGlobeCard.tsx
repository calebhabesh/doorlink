'use client';

import { useEffect, useState } from 'react';
import { Clock } from 'lucide-react';

export default function ClockGlobeCard() {
  const [time, setTime] = useState<Date | null>(null);

  useEffect(() => {
    setTime(new Date());
    const interval = setInterval(() => {
      setTime(new Date());
    }, 1000);
    return () => clearInterval(interval);
  }, []);

  if (!time) {
    return (
      <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-3xl flex items-center justify-center h-28 shadow-lg">
        <span className="text-zinc-500 animate-pulse text-xs font-mono uppercase tracking-widest">Syncing Time...</span>
      </div>
    );
  }

  // Format 12h time
  const timeString = time.toLocaleTimeString(undefined, {
    hour: 'numeric',
    minute: '2-digit',
    second: '2-digit',
    hour12: true,
  });

  const dateString = time.toLocaleDateString(undefined, {
    weekday: 'short',
    month: 'short',
    day: 'numeric',
    year: 'numeric',
  });

  // Timezone matching
  const timeZoneName = Intl.DateTimeFormat().resolvedOptions().timeZone;
  const timeZoneAbbr = time.toLocaleDateString(undefined, { timeZoneName: 'short' }).split(', ')[1] || 'EDT';

  return (
    <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-3xl flex items-center justify-between hover:border-zinc-700 transition-all duration-300 shadow-lg group relative overflow-hidden animate-flash-event">
      {/* Background radial highlight */}
      <div className="absolute inset-0 bg-gradient-to-br from-blue-500/[0.02] via-transparent to-transparent pointer-events-none" />
      
      <div className="flex flex-col text-left">
        <h3 className="text-zinc-500 text-xs font-bold uppercase tracking-widest mb-1 flex items-center gap-1.5">
          <Clock className="w-3.5 h-3.5 text-zinc-400" />
          Local Time
        </h3>
        <p className="text-2xl sm:text-3xl font-black text-white tracking-tight tabular-nums select-none">
          {timeString}
        </p>
        <p className="text-xs text-zinc-400 font-bold uppercase tracking-wider mt-1.5">
          {dateString}
        </p>
        <p className="text-[10px] text-zinc-500 font-mono mt-0.5 uppercase tracking-widest">
          {timeZoneName} ({timeZoneAbbr})
        </p>
      </div>

      {/* Rotating Globe Wrapper */}
      <div className="relative w-16 h-16 shrink-0 flex items-center justify-center overflow-hidden rounded-full border border-zinc-800 bg-zinc-950/90 shadow-inner group">
        <div className="absolute inset-0 rounded-full bg-blue-500/5 blur-md group-hover:bg-blue-500/10 transition-all duration-500" />
        
        {/* Slow rotating SVG globe wireframe */}
        <svg 
          className="w-12 h-12 text-zinc-700 group-hover:text-blue-500/40 transition-colors duration-500"
          style={{ animation: 'spin 25s linear infinite' }}
          viewBox="0 0 100 100" 
          fill="none" 
          stroke="currentColor" 
          strokeWidth="1.5"
        >
          <circle cx="50" cy="50" r="45" stroke="currentColor" strokeWidth="2" className="text-zinc-850" />
          <line x1="5" y1="50" x2="95" y2="50" />
          <line x1="15" y1="30" x2="85" y2="30" />
          <line x1="15" y1="70" x2="85" y2="70" />
          <ellipse cx="50" cy="50" rx="30" ry="45" />
          <ellipse cx="50" cy="50" rx="15" ry="45" />
          <line x1="50" y1="5" x2="50" y2="95" />
        </svg>

        <style jsx global>{`
          @keyframes spin {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
          }
        `}</style>
      </div>
    </div>
  );
}
