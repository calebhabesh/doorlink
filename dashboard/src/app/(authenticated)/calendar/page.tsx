'use client';

import { useState, useRef, useMemo } from 'react';
import { Calendar as CalendarIcon, ChevronLeft, ChevronRight, Eye, ChevronDown } from 'lucide-react';
import EventPreviewDrawer from '../../../components/EventPreviewDrawer';
import { VisitorSession } from '../../../lib/visitorSessions';
import { useSessionContext } from '../../../context/SessionContext';

export default function CalendarView() {
  const { sessions, isLoadingSessions } = useSessionContext();
  const [toastMessage, setToastMessage] = useState<string | null>(null);
  const [isExiting, setIsExiting] = useState(false);
  const [currentDate] = useState(new Date()); 
  const [viewDate, setViewDate] = useState(new Date(new Date().getFullYear(), new Date().getMonth(), 1)); 
  const [selectedDay, setSelectedDay] = useState<number | null>(null);
  const [selectedSession, setSelectedSession] = useState<VisitorSession | null>(null);
  const timeoutRef = useRef<NodeJS.Timeout | null>(null);
  const exitTimeoutRef = useRef<NodeJS.Timeout | null>(null);

  const viewMonth = viewDate.getMonth();
  const viewYear = viewDate.getFullYear();

  const selectedDayEvents = useMemo(() => {
    if (selectedDay === null) return [];
    return sessions.filter((session) => {
      const d = new Date(session.startedAt);
      return d.getDate() === selectedDay && d.getMonth() === viewMonth && d.getFullYear() === viewYear;
    });
  }, [sessions, selectedDay, viewMonth, viewYear]);

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
      }, 500);
    }, 3000);
  };

  const daysInMonth = new Date(viewYear, viewMonth + 1, 0).getDate();
  const startDayOfWeek = new Date(viewYear, viewMonth, 1).getDay();
  
  const daysArray = Array.from({ length: daysInMonth }, (_, i) => i + 1);
  const emptyDays = Array.from({ length: startDayOfWeek }, (_, i) => i);

  const eventsByDay = useMemo(() => {
    const map: Record<number, boolean> = {};
    sessions.forEach((session) => {
      const d = new Date(session.startedAt);
      if (d.getMonth() === viewMonth && d.getFullYear() === viewYear) {
        map[d.getDate()] = true;
      }
    });
    return map;
  }, [sessions, viewMonth, viewYear]);

  const changeMonth = (offset: number) => {
    setViewDate(new Date(viewYear, viewMonth + offset, 1));
  };

  const isFuture = (day: number) => {
    const dateToCheck = new Date(viewYear, viewMonth, day);
    return dateToCheck > currentDate;
  };

  return (
    <div className="w-full max-w-[1200px] mx-auto flex gap-8 h-full relative flex-1">
      {/* Main Calendar Area */}
      <div className="flex-1 flex flex-col h-full relative min-w-0">
        {/* Toast Notification */}
        {toastMessage && (
          <div className={`absolute bottom-4 right-4 z-50 ${isExiting ? 'animate-fade-out' : 'animate-slide-in'}`}>
            <div className="bg-zinc-900 border border-zinc-800 shadow-2xl rounded-xl px-6 py-4 flex items-center gap-3">
              <div className="w-2 h-2 bg-emerald-500 rounded-full animate-pulse motion-reduce:animate-none" />
              <p className="text-zinc-200 text-sm font-medium">{toastMessage}</p>
            </div>
          </div>
        )}

        {/* Month Header */}
        <div className="flex flex-col sm:flex-row justify-between items-start sm:items-center gap-4 mb-8 bg-zinc-950 border border-zinc-800 p-6 rounded-2xl">
          <div className="flex flex-col sm:flex-row items-start sm:items-center gap-4 w-full lg:w-auto">
            <h2 className="text-xl sm:text-2xl font-black tracking-tight text-white flex items-center gap-3">
              <CalendarIcon className="w-6 h-6 text-emerald-500 shrink-0" />
              <span className="whitespace-nowrap">{viewDate.toLocaleString('default', { month: 'long', year: 'numeric' })}</span>
            </h2>
            <div className="relative w-full sm:w-auto">
              <input 
                type="date"
                title="Jump to date"
                onChange={(e) => {
                  if (e.target.value) {
                    const [y, m, d] = e.target.value.split('-');
                    setViewDate(new Date(parseInt(y), parseInt(m) - 1, 1));
                    const dayNum = parseInt(d);
                    const isFutureDay = isFuture(dayNum) && parseInt(m) - 1 === new Date().getMonth() && parseInt(y) === new Date().getFullYear();
                    if (!isFutureDay) {
                      setSelectedDay(dayNum);
                    }
                  }
                }}
                className="w-full min-w-[180px] sm:min-w-[200px] bg-zinc-900 border border-zinc-800 text-zinc-400 pl-12 pr-12 py-3 rounded-lg text-sm font-bold uppercase tracking-widest hover:text-zinc-200 transition-colors focus:outline-none focus-visible:ring-2 focus-visible:ring-emerald-500 [color-scheme:dark] appearance-none"
              />
              <CalendarIcon className="absolute left-4 top-1/2 -translate-y-1/2 w-5 h-5 text-emerald-500 pointer-events-none" />
              <ChevronDown className="absolute right-4 top-1/2 -translate-y-1/2 w-5 h-5 text-zinc-500 pointer-events-none" />
            </div>
          </div>
          <div className="flex gap-2 w-full lg:w-auto justify-end">
            <button 
              type="button"
              onClick={() => changeMonth(-1)}
              aria-label="Previous month"
              className="p-2 bg-zinc-900 hover:bg-zinc-800 border border-zinc-800 rounded-lg text-zinc-400 transition-colors focus-visible:ring-2 focus-visible:ring-emerald-500 focus-visible:outline-none"
            >
              <ChevronLeft className="w-5 h-5" />
            </button>
            <button 
              type="button"
              onClick={() => changeMonth(1)}
              aria-label="Next month"
              className="p-2 bg-zinc-900 hover:bg-zinc-800 border border-zinc-800 rounded-lg text-zinc-400 transition-colors focus-visible:ring-2 focus-visible:ring-emerald-500 focus-visible:outline-none"
            >
              <ChevronRight className="w-5 h-5" />
            </button>
          </div>
        </div>

        {/* Calendar Grid */}
        {isLoadingSessions ? (
          <div className="flex-1 flex items-center justify-center min-h-[400px]">
            <div className="flex flex-col items-center gap-4">
              <div className="h-8 w-8 rounded-full border-2 border-emerald-500 border-t-transparent animate-spin motion-reduce:animate-none" />
              <p className="text-sm text-zinc-400 font-mono uppercase tracking-widest">Loading Calendar…</p>
            </div>
          </div>
        ) : (
          <div className="grid grid-cols-7 gap-px bg-zinc-800 rounded-2xl overflow-hidden border border-zinc-800">
            {/* Days of week */}
            {['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'].map((day) => (
              <div key={day} className="bg-zinc-950 p-4 text-center text-xs font-bold uppercase tracking-widest text-zinc-500">
                {day}
              </div>
            ))}
            
            {/* Empty cells */}
            {emptyDays.map((i) => (
              <div key={`empty-${i}`} className="bg-zinc-950/50 min-h-[120px]" />
            ))}

            {/* Days cells */}
            {daysArray.map((day) => {
              const hasEvent = eventsByDay[day] && !isFuture(day);
              const future = isFuture(day);
              const isSelected = selectedDay === day;
              
              return (
                <button
                  key={day}
                  type="button"
                  onClick={() => handleDayClick(day, hasEvent)}
                  aria-label={`${viewDate.toLocaleString('default', { month: 'long' })} ${day}, ${viewYear}`}
                  disabled={future}
                  className={`bg-zinc-950 min-h-[120px] p-4 flex flex-col items-start justify-start transition-colors group relative border-t border-transparent focus-visible:ring-2 focus-visible:ring-emerald-500 focus-visible:outline-none ${
                    future ? 'opacity-40 cursor-not-allowed' : 'hover:bg-zinc-900 hover:border-zinc-800'
                  } ${isSelected ? 'ring-2 ring-emerald-500 ring-inset bg-zinc-900' : ''}`}
                >
                  <span className={`text-lg font-bold ${isSelected ? 'text-emerald-500' : (hasEvent ? 'text-zinc-100' : 'text-zinc-500')}`}>
                    {day}
                  </span>
                  
                  {hasEvent && (
                    <div className="mt-auto self-center flex items-center justify-center w-full">
                      <span className="relative flex h-3 w-3">
                        <span className="animate-ping motion-reduce:animate-none absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75" />
                        <span className="relative inline-flex rounded-full h-3 w-3 bg-emerald-500 shadow-[0_0_10px_rgba(16,185,129,0.8)]" />
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
        <div className="w-80 bg-zinc-950 border border-zinc-800 rounded-3xl p-6 flex flex-col h-full shrink-0 shadow-2xl">
          <div className="flex justify-between items-center mb-6">
            <h3 className="text-xl font-black text-white uppercase tracking-tight">
              {viewDate.toLocaleString('default', { month: 'short' })} {selectedDay}
            </h3>
            <button 
              type="button"
              onClick={() => setSelectedDay(null)}
              aria-label="Close day panel"
              className="text-zinc-500 hover:text-white transition-colors focus-visible:ring-2 focus-visible:ring-emerald-500 focus-visible:outline-none rounded-lg p-1"
            >
              ✕
            </button>
          </div>

          <div className="flex-1 overflow-y-auto space-y-4 pr-2">
            {selectedDayEvents.length > 0 ? (
              selectedDayEvents.map((session) => (
                <button 
                  key={session.sessionId}
                  type="button"
                  onClick={() => setSelectedSession(session)}
                  className="w-full text-left bg-zinc-900/50 border border-zinc-800 p-4 rounded-xl hover:border-emerald-500/50 transition-colors group flex flex-col focus-visible:ring-2 focus-visible:ring-emerald-500 focus-visible:outline-none"
                >
                  <div className="flex justify-between items-start mb-2 w-full">
                    <span className="text-emerald-500 text-[10px] font-black tracking-widest bg-emerald-500/10 px-2 py-0.5 rounded">
                      {session.pressCount} {session.pressCount === 1 ? 'Press' : 'Presses'}
                    </span>
                    <div className="flex items-center gap-2">
                      {session.recordingCount > 0 && (
                        <span className="bg-zinc-800 px-1.5 py-0.5 rounded text-[9px] text-zinc-300 font-mono tracking-widest">
                          {session.recordingCount} {session.recordingCount === 1 ? 'Message' : 'Messages'}
                        </span>
                      )}
                      <span className="text-zinc-500 font-mono text-[10px]">
                        {new Date(session.startedAt).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', hour12: true }).toUpperCase()}
                      </span>
                    </div>
                  </div>
                  <div className="flex justify-between items-center w-full">
                    <p className="text-zinc-400 text-xs font-mono truncate max-w-[180px]">{session.sessionId}</p>
                    <Eye className="w-4 h-4 text-emerald-500 opacity-0 group-hover:opacity-100 transition-opacity" />
                  </div>
                </button>
              ))
            ) : (
              <div className="flex flex-col items-center justify-center h-40 text-center">
                <p className="text-zinc-600 text-sm font-bold uppercase tracking-widest">No Activity</p>
              </div>
            )}
          </div>
        </div>
      )}

      <EventPreviewDrawer 
        session={selectedSession}
        onClose={() => setSelectedSession(null)}
      />
    </div>
  );
}
