'use client';

import { useEffect, useState, useMemo } from 'react';
import MainLayout, { ConnectionStatus } from '../../components/MainLayout';
import { Search, Download, ChevronLeft, ChevronRight, Eye, CalendarDays, X } from 'lucide-react';
import EventPreviewDrawer from '../../components/EventPreviewDrawer';

interface DoorbellEvent {
  id: number;
  timestamp: string;
  eventType: string;
  imageKey: string;
  audioKey?: string | null;
}

const API_BASE_URL = `/api/events`;

export default function EventLog() {
  const [events, setEvents] = useState<DoorbellEvent[]>([]);
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('connecting');
  const [latestLiveEventId, setLatestLiveEventId] = useState<number | null>(null);
  const [searchQuery, setSearchQuery] = useState('');
  const [filterDate, setFilterDate] = useState<string>('');
  const [selectedEvent, setSelectedEvent] = useState<DoorbellEvent | null>(null);
  const [isLoading, setIsLoading] = useState(true);

  useEffect(() => {
    fetch(`${API_BASE_URL}?size=1000`)
      .then((res) => res.json())
      .then((data) => {
        setEvents(data);
        setIsLoading(false);
      })
      .catch((err) => {
        console.error("Failed to fetch logs", err);
        setIsLoading(false);
      });

    const eventSource = new EventSource(`/stream`);
    
    eventSource.onopen = () => {
      setConnectionStatus('connected');
    };

    eventSource.onerror = () => {
      setConnectionStatus('connecting');
    };

    // Fallback: if we receive the init event, we are definitely connected
    eventSource.addEventListener('init', () => {
      setConnectionStatus('connected');
    });

    eventSource.addEventListener('doorbell-event', (e) => {
      setConnectionStatus('connected');
      try {
        const newEvent: DoorbellEvent = JSON.parse(e.data);
        setEvents((prev) => {
          if (prev.some((event) => event.id === newEvent.id)) {
            return prev;
          }
          return [newEvent, ...prev];
        });
        setLatestLiveEventId(newEvent.id);
      } catch (err) {
        console.error("Failed to parse event", err);
      }
    });

    return () => eventSource.close();
  }, []);

  const formatTitleCase = (str: string) => {
    return str.toLowerCase().split('_').map(word => word.charAt(0).toUpperCase() + word.slice(1)).join(' ');
  };

  const filteredEvents = useMemo(() => {
    return events.filter(e => {
      const matchesSearch = e.eventType.toLowerCase().includes(searchQuery.toLowerCase()) ||
                            e.imageKey.toLowerCase().includes(searchQuery.toLowerCase());
      
      if (!filterDate) return matchesSearch;
      
      const eventDate = new Date(e.timestamp);
      // 'en-CA' locale format is YYYY-MM-DD, matching the date input value exactly
      const eventDateStr = eventDate.toLocaleDateString('en-CA');
      
      return matchesSearch && eventDateStr === filterDate;
    });
  }, [events, searchQuery, filterDate]);

  return (
    <MainLayout status={connectionStatus} breadcrumbs={[{ label: 'Dashboard' }, { label: 'Event Log', active: true }]}>
      <div className="w-full max-w-[1800px] mx-auto flex flex-col h-full overflow-hidden">
        
        {/* Action Toolbar */}
        <div className="flex flex-col lg:flex-row justify-between items-start lg:items-center gap-4 mb-6 lg:mb-10 shrink-0 relative z-10 w-full">
          <div className="relative w-full lg:w-96">
            <Search className="absolute left-4 top-1/2 -translate-y-1/2 w-5 h-4 text-zinc-500" />
            <input 
              type="text" 
              placeholder="Search events..." 
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="w-full bg-zinc-950 border border-zinc-800 rounded-xl py-3 pl-12 pr-6 text-base text-zinc-200 focus:outline-none focus:ring-1 focus:ring-emerald-500 focus:border-emerald-500 transition-all shadow-inner"
            />
          </div>
          <div className="flex flex-col sm:flex-row gap-4 w-full lg:w-auto">
            <div className="relative w-full sm:w-auto min-w-[240px]">
              <input 
                type="date"
                value={filterDate}
                onChange={(e) => setFilterDate(e.target.value)}
                className="w-full bg-zinc-950 border border-zinc-800 text-zinc-200 px-10 py-3.5 rounded-xl text-sm font-bold uppercase tracking-widest hover:bg-zinc-800 transition-colors shadow-lg focus:outline-none focus:ring-1 focus:ring-emerald-500 [color-scheme:dark]"
              />
              <CalendarDays className="absolute left-4 top-1/2 -translate-y-1/2 w-4 h-4 text-emerald-500 pointer-events-none" />
              {filterDate && (
                <button 
                  onClick={(e) => { e.preventDefault(); setFilterDate(''); }} 
                  className="absolute right-12 top-1/2 -translate-y-1/2 text-zinc-500 hover:text-zinc-200 p-1 bg-zinc-950 rounded-full"
                  title="Clear filter"
                >
                  <X className="w-3 h-3" />
                </button>
              )}
            </div>
            <button className="w-full sm:w-auto flex justify-center items-center gap-3 bg-zinc-950 border border-zinc-800 text-zinc-200 px-6 py-3 rounded-xl text-sm font-bold uppercase tracking-widest hover:bg-zinc-800 transition-colors shadow-lg">
              <Download className="w-5 h-5 text-zinc-400" />
              Export CSV
            </button>
          </div>
        </div>

        {/* Data Table Container */}
        <div className={`w-full overflow-hidden rounded-3xl lg:border border-zinc-800 lg:bg-zinc-950/50 flex flex-col min-h-0 lg:shadow-2xl relative z-10 ${!isLoading ? 'animate-flash-event' : ''}`}>
          
          {isLoading ? (
            <div className="flex-1 flex items-center justify-center min-h-[400px]">
              <div className="flex flex-col items-center gap-4">
                <svg className="animate-spin h-8 w-8 text-emerald-500" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                  <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                  <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                </svg>
                <p className="text-sm text-zinc-400 font-mono uppercase tracking-widest">Loading Event Log...</p>
              </div>
            </div>
          ) : (
            <>
          {/* Desktop Table */}
          <div className="hidden lg:block overflow-y-auto flex-1 scrollbar-thin scrollbar-thumb-zinc-800 scrollbar-track-transparent">
            <table className="w-full border-collapse text-left">
              <thead className="bg-zinc-900/80 sticky top-0 z-20 backdrop-blur-md">
                <tr className="border-b border-zinc-800">
                  <th className="px-8 py-6 text-sm uppercase tracking-[0.2em] text-zinc-400 font-black">Timestamp</th>
                  <th className="px-8 py-6 text-sm uppercase tracking-[0.2em] text-zinc-400 font-black">Event Type</th>
                  <th className="px-8 py-6 text-sm uppercase tracking-[0.2em] text-zinc-400 font-black">Media Reference</th>
                  <th className="px-8 py-6 text-sm uppercase tracking-[0.2em] text-zinc-400 font-black">System Status</th>
                  <th className="px-8 py-6 text-sm uppercase tracking-[0.2em] text-zinc-400 font-black text-right">Actions</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-zinc-800/50">
                {filteredEvents.map((evt) => (
                  <tr key={evt.id} className={`transition-all cursor-pointer group ${latestLiveEventId === evt.id ? 'animate-slide-in bg-emerald-500/[0.06] shadow-[inset_3px_0_0_rgba(16,185,129,0.9)]' : 'hover:bg-zinc-800/30'}`}>
                    <td className="px-8 py-6 font-mono text-zinc-300 text-base whitespace-nowrap">
                      {new Date(evt.timestamp).toLocaleString(undefined, {
                        year: 'numeric', month: 'short', day: 'numeric',
                        hour: 'numeric', minute: '2-digit', second: '2-digit'
                      })}
                    </td>
                    <td className="px-8 py-6">
                      <span className="bg-emerald-500/10 text-emerald-500 px-4 py-1.5 rounded-full text-xs font-black uppercase tracking-widest border border-emerald-500/20 shadow-[0_0_10px_rgba(16,185,129,0.1)]">
                        {formatTitleCase(evt.eventType)}
                      </span>
                    </td>
                    <td className="px-8 py-6 text-zinc-400 text-base font-mono max-w-md truncate">
                      {evt.imageKey}
                    </td>
                    <td className="px-8 py-6">
                      <div className="flex items-center gap-3">
                        <div className="w-2 h-2 rounded-full bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.6)] animate-pulse"></div>
                        <span className="text-zinc-200 text-sm font-bold uppercase tracking-widest">Verified</span>
                      </div>
                    </td>
                    <td className="px-8 py-6 text-right">
                       <button 
                         type="button"
                         onClick={() => setSelectedEvent(evt)}
                         aria-label={`Preview ${formatTitleCase(evt.eventType)} from ${new Date(evt.timestamp).toLocaleString()}`}
                         className="opacity-100 lg:opacity-0 group-hover:opacity-100 transition-all p-3 hover:bg-zinc-700 bg-zinc-800/50 rounded-xl border border-zinc-700 shadow-lg"
                       >
                          <Eye className="w-5 h-5 text-emerald-500" />
                       </button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          {/* Mobile Cards */}
          <div className="lg:hidden flex flex-col gap-4 overflow-y-auto pb-4">
            {filteredEvents.map((evt) => (
              <div 
                key={evt.id} 
                className={`bg-zinc-950 border border-zinc-800 rounded-2xl p-5 flex flex-col gap-4 ${latestLiveEventId === evt.id ? 'border-emerald-500/50 shadow-[0_0_15px_rgba(16,185,129,0.1)]' : ''}`}
              >
                <div className="flex justify-between items-start">
                  <div>
                    <span className="bg-emerald-500/10 text-emerald-500 px-3 py-1 rounded-full text-[10px] font-black uppercase tracking-widest border border-emerald-500/20 mb-2 inline-block">
                      {formatTitleCase(evt.eventType)}
                    </span>
                    <div className="font-mono text-zinc-300 text-sm">
                      {new Date(evt.timestamp).toLocaleString(undefined, {
                        month: 'short', day: 'numeric',
                        hour: 'numeric', minute: '2-digit'
                      })}
                    </div>
                  </div>
                  <button 
                     type="button"
                     onClick={() => setSelectedEvent(evt)}
                     aria-label={`Preview event`}
                     className="p-3 bg-zinc-800/80 rounded-xl border border-zinc-700 active:scale-95 transition-transform"
                   >
                      <Eye className="w-5 h-5 text-emerald-500" />
                   </button>
                </div>
                <div className="flex justify-between items-center text-xs font-mono text-zinc-500">
                  <span className="truncate max-w-[150px]">{evt.imageKey}</span>
                  {evt.audioKey && <span className="bg-zinc-800 px-2 py-0.5 rounded text-zinc-300">Audio</span>}
                </div>
              </div>
            ))}
          </div>

          {/* Pagination Footer */}
          <div className="px-6 py-4 lg:border-t border-zinc-800 lg:bg-zinc-900 bg-transparent flex justify-between items-center shrink-0">
            <p className="text-xs font-mono text-zinc-500 uppercase tracking-widest">
              Showing <span className="text-zinc-300">1</span> to <span className="text-zinc-300">{filteredEvents.length}</span> of <span className="text-zinc-300">{filteredEvents.length}</span> events
            </p>
            <div className="flex gap-2">
              <button className="p-2 border border-zinc-800 rounded hover:bg-zinc-800 text-zinc-400 disabled:opacity-30" disabled>
                <ChevronLeft className="w-4 h-4" />
              </button>
              <button className="p-2 border border-zinc-800 rounded hover:bg-zinc-800 text-zinc-400 disabled:opacity-30" disabled>
                <ChevronRight className="w-4 h-4" />
              </button>
            </div>
          </div>
            </>
          )}
        </div>
      </div>
      <EventPreviewDrawer 
        event={selectedEvent} 
        onClose={() => setSelectedEvent(null)} 
      />
    </MainLayout>
  );
}
