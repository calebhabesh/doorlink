'use client';

import { useEffect, useState, useMemo } from 'react';
import MainLayout from '../../components/MainLayout';

type ConnectionStatus = 'connecting' | 'connected' | 'error';
import { Search, Calendar, Download, ChevronLeft, ChevronRight, Play } from 'lucide-react';

interface DoorbellEvent {
  id: number;
  timestamp: string;
  eventType: string;
  imageKey: string;
}

export default function EventLog() {
  const [events, setEvents] = useState<DoorbellEvent[]>([]);
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('connecting');
  const [searchQuery, setSearchQuery] = useState('');
  const [filterDate, setFilterDate] = useState<string>('');

  const API_BASE_URL = 'http://localhost:8080/api/events';

  useEffect(() => {
    fetch(API_BASE_URL)
      .then((res) => res.json())
      .then((data) => setEvents(data))
      .catch((err) => console.error("Failed to fetch logs", err));

    const eventSource = new EventSource(`${API_BASE_URL}/stream`);
    
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
      const eventDateStr = `${eventDate.getFullYear()}-${String(eventDate.getMonth() + 1).padStart(2, '0')}-${String(eventDate.getDate()).padStart(2, '0')}`;
      
      return matchesSearch && eventDateStr === filterDate;
    });
  }, [events, searchQuery, filterDate]);

  return (
    <MainLayout status={connectionStatus} breadcrumbs={[{ label: 'Dashboard' }, { label: 'Event Log', active: true }]}>
      <div className="w-full max-w-[1800px] mx-auto flex flex-col h-full overflow-hidden">
        
        {/* Action Toolbar */}
        <div className="flex justify-between items-center mb-10 shrink-0 relative z-10">
          <div className="relative w-96">
            <Search className="absolute left-4 top-1/2 -translate-y-1/2 w-5 h-4 text-zinc-500" />
            <input 
              type="text" 
              placeholder="Search events..." 
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="w-full bg-zinc-950 border border-zinc-800 rounded-xl py-3 pl-12 pr-6 text-base text-zinc-200 focus:outline-none focus:ring-1 focus:ring-emerald-500 focus:border-emerald-500 transition-all shadow-inner"
            />
          </div>
          <div className="flex gap-4">
            <div className="relative group">
              <input 
                type="date"
                value={filterDate}
                onChange={(e) => setFilterDate(e.target.value)}
                className="flex items-center gap-3 bg-zinc-950 border border-zinc-800 text-zinc-200 px-4 py-3 rounded-xl text-sm font-bold uppercase tracking-widest hover:bg-zinc-800 transition-colors shadow-lg cursor-pointer focus:outline-none focus:ring-1 focus:ring-emerald-500 [color-scheme:dark]"
              />
              {!filterDate && (
                 <div className="absolute inset-0 pointer-events-none flex items-center justify-center gap-2 bg-zinc-950 rounded-xl group-hover:bg-zinc-800 transition-colors">
                   <Calendar className="w-5 h-5 text-zinc-400" />
                   <span className="text-sm font-bold uppercase tracking-widest text-zinc-200">Filter by Date</span>
                 </div>
              )}
              {filterDate && (
                <button 
                  onClick={() => setFilterDate('')} 
                  className="absolute -right-2 -top-2 bg-zinc-800 border border-zinc-700 text-zinc-400 hover:text-white rounded-full w-6 h-6 flex items-center justify-center text-xs"
                >
                  ✕
                </button>
              )}
            </div>
            <button className="flex items-center gap-3 bg-zinc-950 border border-zinc-800 text-zinc-200 px-6 py-3 rounded-xl text-sm font-bold uppercase tracking-widest hover:bg-zinc-800 transition-colors shadow-lg">
              <Download className="w-5 h-5 text-zinc-400" />
              Export CSV
            </button>
          </div>
        </div>

        {/* Data Table Container */}
        <div className="w-full overflow-hidden rounded-3xl border border-zinc-800 bg-zinc-950/50 flex flex-col min-h-0 shadow-2xl relative z-10 animate-flash-event">
          <div className="overflow-y-auto flex-1 scrollbar-thin scrollbar-thumb-zinc-800 scrollbar-track-transparent">
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
                  <tr key={evt.id} className="hover:bg-zinc-800/30 transition-all cursor-pointer group">
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
                       <button className="opacity-0 group-hover:opacity-100 transition-all p-3 hover:bg-zinc-700 bg-zinc-800/50 rounded-xl border border-zinc-700 shadow-lg">
                          <Play className="w-5 h-5 text-emerald-500 fill-emerald-500/20" />
                       </button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          {/* Pagination Footer */}
          <div className="px-6 py-4 border-t border-zinc-800 bg-zinc-900 flex justify-between items-center shrink-0">
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
        </div>
      </div>
    </MainLayout>
  );
}
