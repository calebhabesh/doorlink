'use client';

import { useState } from 'react';
import MainLayout from '../../components/MainLayout';
import { Calendar as CalendarIcon, ChevronLeft, ChevronRight } from 'lucide-react';

export default function CalendarView() {
  const [toastMessage, setToastMessage] = useState<string | null>(null);

  const showToast = (date: number) => {
    setToastMessage(`No events found for May ${date}, 2026`);
    setTimeout(() => setToastMessage(null), 3000);
  };

  // Mock data for May 2026
  const daysInMonth = 31;
  const startDayOfWeek = 5; // Friday (0=Sun, 1=Mon, ..., 5=Fri)
  
  const daysArray = Array.from({ length: daysInMonth }, (_, i) => i + 1);
  const emptyDays = Array.from({ length: startDayOfWeek }, (_, i) => i);

  // Mock days with events
  const daysWithEvents = [2, 14, 21, 28];

  return (
    <MainLayout status="connected" breadcrumbs={[{ label: 'Dashboard' }, { label: 'Calendar View', active: true }]}>
      <div className="w-full max-w-[1200px] mx-auto flex flex-col h-full relative">
        
        {/* Toast Notification */}
        {toastMessage && (
          <div className="absolute top-0 right-0 z-50 animate-slide-in">
            <div className="bg-zinc-900 border border-zinc-800 shadow-2xl rounded-xl px-6 py-4 flex items-center gap-3">
              <div className="w-2 h-2 bg-emerald-500 rounded-full animate-pulse"></div>
              <p className="text-zinc-200 text-sm font-medium">{toastMessage}</p>
            </div>
          </div>
        )}

        {/* Header */}
        <div className="flex justify-between items-center mb-8 bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-2xl">
          <h1 className="text-2xl font-black tracking-tight text-white flex items-center gap-3">
            <CalendarIcon className="w-6 h-6 text-emerald-500" />
            May 2026
          </h1>
          <div className="flex gap-2">
            <button className="p-2 bg-zinc-900 hover:bg-zinc-800 border border-zinc-800 rounded-lg text-zinc-400 transition-colors">
              <ChevronLeft className="w-5 h-5" />
            </button>
            <button className="p-2 bg-zinc-900 hover:bg-zinc-800 border border-zinc-800 rounded-lg text-zinc-400 transition-colors">
              <ChevronRight className="w-5 h-5" />
            </button>
          </div>
        </div>

        {/* Calendar Grid */}
        <div className="grid grid-cols-7 gap-px bg-zinc-800 rounded-2xl overflow-hidden border border-zinc-800">
          {/* Days of week */}
          {['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'].map(day => (
            <div key={day} className="bg-zinc-950 p-4 text-center text-xs font-bold uppercase tracking-widest text-zinc-500">
              {day}
            </div>
          ))}
          
          {/* Empty cells */}
          {emptyDays.map(i => (
            <div key={`empty-${i}`} className="bg-zinc-950/50 min-h-[120px]"></div>
          ))}

          {/* Days cells */}
          {daysArray.map(day => {
            const hasEvent = daysWithEvents.includes(day);
            return (
              <button
                key={day}
                onClick={() => showToast(day)}
                className="bg-zinc-950 min-h-[120px] p-4 flex flex-col items-start justify-start hover:bg-zinc-900 transition-colors group relative border-t border-transparent hover:border-zinc-800"
              >
                <span className={`text-lg font-bold ${hasEvent ? 'text-zinc-100' : 'text-zinc-500'}`}>
                  {day}
                </span>
                
                {hasEvent && (
                  <div className="mt-auto self-center flex items-center justify-center w-full">
                     <span className="relative flex h-3 w-3">
                       <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>
                       <span className="relative inline-flex rounded-full h-3 w-3 bg-emerald-500 shadow-[0_0_10px_rgba(16,185,129,0.8)]"></span>
                     </span>
                  </div>
                )}
              </button>
            );
          })}
        </div>
      </div>
    </MainLayout>
  );
}