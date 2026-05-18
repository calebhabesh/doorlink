'use client';

import { useState, useRef, useEffect, useMemo } from 'react';
import MainLayout from '../../components/MainLayout';
import { Calendar as CalendarIcon, ChevronLeft, ChevronRight } from 'lucide-react';

interface DoorbellEvent {
  id: number;
  timestamp: string;
  eventType: string;
  imageKey: string;
}

const API_BASE_URL = `/api/events`;

export default function CalendarView() {
  const [toastMessage, setToastMessage] = useState<string | null>(null);
  const [isExiting, setIsExiting] = useState(false);
  const [events, setEvents] = useState<DoorbellEvent[]>([]);
  const [currentDate] = useState(new Date()); 
  const [viewDate, setViewDate] = useState(new Date(new Date().getFullYear(), new Date().getMonth(), 1)); 
  const [selectedDay, setSelectedDay] = useState<number | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const timeoutRef = useRef<NodeJS.Timeout | null>(null);
  const exitTimeoutRef = useRef<NodeJS.Timeout | null>(null);

  useEffect(() => {
    fetch(`${API_BASE_URL}?size=1000`)
      .then((res) => res.json())
      .then((data) => {
        setEvents(data);
        setIsLoading(false);
      })
      .catch((err) => {
        console.error("Failed to fetch events for calendar", err);
        setIsLoading(false);
      });
  }, []);

  const viewMonth = viewDate.getMonth();
  const viewYear = viewDate.getFullYear();

  const selectedDayEvents = useMemo(() => {
    if (selectedDay === null) return [];
    return events.filter(event => {
      const d = new Date(event.timestamp);
      return d.getDate() === selectedDay && d.getMonth() === viewMonth && d.getFullYear() === viewYear;
    });
  }, [events, selectedDay, viewMonth, viewYear]);

  const handleDayClick = (day: number, hasEvent: boolean) => {
    setSelectedDay(day);
    if (timeoutRef.current) clearTimeout(timeoutRef.current);
    if (exitTimeoutRef.current) clearTimeout(exitTimeoutRef.current);
    
    const monthName = viewDate.toLocaleString('default', { month: 'long' });
    const msg = hasEvent 
      ? `Showing events for ${monthName} ${day}, ${viewDate.getFullYear()}` 
      : `No events found for ${monthName} ${day}, ${viewDate.getFullYear()}`;
    
    setToastMessage(msg);
    setIsExiting(false);

    timeoutRef.current = setTimeout(() => {
      setIsExiting(true);
      exitTimeoutRef.current = setTimeout(() => {
        setToastMessage(null);
        setIsExiting(false);
      }, 500); // Match fade-out duration
    }, 3000);
  };

  const daysInMonth = new Date(viewYear, viewMonth + 1, 0).getDate();
  const startDayOfWeek = new Date(viewYear, viewMonth, 1).getDay();
  
  const daysArray = Array.from({ length: daysInMonth }, (_, i) => i + 1);
  const emptyDays = Array.from({ length: startDayOfWeek }, (_, i) => i);

  const eventsByDay = useMemo(() => {
    const map: Record<number, boolean> = {};
    events.forEach(event => {
      const d = new Date(event.timestamp);
      if (d.getMonth() === viewMonth && d.getFullYear() === viewYear) {
        map[d.getDate()] = true;
      }
    });
    return map;
  }, [events, viewMonth, viewYear]);

  const changeMonth = (offset: number) => {
    setViewDate(new Date(viewYear, viewMonth + offset, 1));
  };

  const isFuture = (day: number) => {
    const dateToCheck = new Date(viewYear, viewMonth, day);
    return dateToCheck > currentDate;
  };

  return (
    <MainLayout status="connected" breadcrumbs={[{ label: 'Dashboard' }, { label: 'Calendar View', active: true }]}>
      <div className="w-full max-w-[1200px] mx-auto flex gap-8 h-full relative">
        
        {/* Main Calendar Area */}
        <div className="flex-1 flex flex-col h-full relative">
          {/* Toast Notification */}
          {toastMessage && (
            <div className={`absolute bottom-4 right-4 z-50 ${isExiting ? 'animate-fade-out' : 'animate-slide-in'}`}>
              <div className="bg-zinc-900 border border-zinc-800 shadow-2xl rounded-xl px-6 py-4 flex items-center gap-3">
                <div className="w-2 h-2 bg-emerald-500 rounded-full animate-pulse"></div>
                <p className="text-zinc-200 text-sm font-medium">{toastMessage}</p>
              </div>
            </div>
          )}

          {/* Header */}
          <div className="flex justify-between items-center mb-8 bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-2xl animate-flash-event">
            <h1 className="text-2xl font-black tracking-tight text-white flex items-center gap-3">
              <CalendarIcon className="w-6 h-6 text-emerald-500" />
              {viewDate.toLocaleString('default', { month: 'long', year: 'numeric' })}
            </h1>
            <div className="flex gap-2">
              <button 
                onClick={() => changeMonth(-1)}
                className="p-2 bg-zinc-900 hover:bg-zinc-800 border border-zinc-800 rounded-lg text-zinc-400 transition-colors"
              >
                <ChevronLeft className="w-5 h-5" />
              </button>
              <button 
                onClick={() => changeMonth(1)}
                className="p-2 bg-zinc-900 hover:bg-zinc-800 border border-zinc-800 rounded-lg text-zinc-400 transition-colors"
              >
                <ChevronRight className="w-5 h-5" />
              </button>
            </div>
          </div>

          {/* Calendar Grid */}
          {isLoading ? (
            <div className="flex-1 flex items-center justify-center min-h-[400px]">
              <div className="flex flex-col items-center gap-4">
                <svg className="animate-spin h-8 w-8 text-emerald-500" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                  <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                  <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                </svg>
                <p className="text-sm text-zinc-400 font-mono uppercase tracking-widest">Loading Calendar...</p>
              </div>
            </div>
          ) : (
            <div className="grid grid-cols-7 gap-px bg-zinc-800 rounded-2xl overflow-hidden border border-zinc-800 animate-flash-event">
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
              const hasEvent = eventsByDay[day] && !isFuture(day);
              const future = isFuture(day);
              const isSelected = selectedDay === day;
              
              return (
                <button
                  key={day}
                  onClick={() => handleDayClick(day, hasEvent)}
                  aria-label={`${viewDate.toLocaleString('default', { month: 'long' })} ${day}, ${viewYear}`}
                  disabled={future}
                  className={`bg-zinc-950 min-h-[120px] p-4 flex flex-col items-start justify-start transition-colors group relative border-t border-transparent ${future ? 'opacity-40 cursor-not-allowed' : 'hover:bg-zinc-900 hover:border-zinc-800'} ${isSelected ? 'ring-2 ring-emerald-500 ring-inset bg-zinc-900' : ''}`}
                >
                  <span className={`text-lg font-bold ${isSelected ? 'text-emerald-500' : (hasEvent ? 'text-zinc-100' : 'text-zinc-500')}`}>
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
          )}
        </div>

        {/* Selected Day Side Panel */}
        {selectedDay && (
          <div className="w-80 bg-zinc-950 border border-zinc-800 rounded-3xl p-6 flex flex-col h-full animate-flash-event shrink-0 shadow-2xl">
            <div className="flex justify-between items-center mb-6">
              <h2 className="text-xl font-black text-white uppercase tracking-tight">
                {viewDate.toLocaleString('default', { month: 'short' })} {selectedDay}
              </h2>
              <button 
                onClick={() => setSelectedDay(null)}
                className="text-zinc-500 hover:text-white transition-colors"
              >
                ✕
              </button>
            </div>

            <div className="flex-1 overflow-y-auto space-y-4 pr-2 scrollbar-thin scrollbar-thumb-zinc-800 scrollbar-track-transparent">
              {selectedDayEvents.length > 0 ? (
                selectedDayEvents.map(event => (
                  <div key={event.id} className="bg-zinc-900/50 border border-zinc-800 p-4 rounded-xl hover:border-emerald-500/50 transition-colors group">
                    <div className="flex justify-between items-start mb-2">
                      <span className="text-emerald-500 text-[10px] font-black uppercase tracking-widest bg-emerald-500/10 px-2 py-0.5 rounded">
                        {event.eventType.replace('_', ' ')}
                      </span>
                      <span className="text-zinc-500 font-mono text-[10px]">
                        {new Date(event.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}
                      </span>
                    </div>
                    <p className="text-zinc-400 text-xs font-mono truncate">{event.imageKey}</p>
                  </div>
                ))
              ) : (
                <div className="flex flex-col items-center justify-center h-40 text-center">
                  <p className="text-zinc-600 text-sm font-bold uppercase tracking-widest">No Activity</p>
                </div>
              )}
            </div>
          </div>
        )}
      </div>
    </MainLayout>
  );
}
