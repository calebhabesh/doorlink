'use client';

import { useEffect, useState } from 'react';
import MainLayout from '../../components/MainLayout';
import { Search, Calendar, Download, ChevronLeft, ChevronRight, Play } from 'lucide-react';

interface DoorbellEvent {
  id: number;
  timestamp: string;
  eventType: string;
  imageKey: string;
}

export default function EventLog() {
  const [events, setEvents] = useState<DoorbellEvent[]>([]);
  const [isConnected, setIsConnected] = useState(false);
  const [searchQuery, setSearchQuery] = useState('');

  const API_BASE_URL = 'http://localhost:8080/api/events';

  useEffect(() => {
    fetch(API_BASE_URL)
      .then((res) => res.json())
      .then((data) => setEvents(data))
      .catch((err) => console.error("Failed to fetch logs", err));

    const eventSource = new EventSource(`${API_BASE_URL}/stream`);
    eventSource.onopen = () => setIsConnected(true);
    eventSource.onerror = () => setIsConnected(false);
    return () => eventSource.close();
  }, []);

  const formatTitleCase = (str: string) => {
    return str.toLowerCase().split('_').map(word => word.charAt(0).toUpperCase() + word.slice(1)).join(' ');
  };

  const filteredEvents = events.filter(e => 
    e.eventType.toLowerCase().includes(searchQuery.toLowerCase()) ||
    e.imageKey.toLowerCase().includes(searchQuery.toLowerCase())
  );

  return (
    <MainLayout isConnected={isConnected} breadcrumbs={[{ label: 'Dashboard' }, { label: 'Event Log', active: true }]}>
      <div className="w-full max-w-7xl mx-auto flex flex-col h-full overflow-hidden">
        
        {/* Action Toolbar */}
        <div className="flex justify-between items-center mb-6 shrink-0 relative z-10">
          <div className="relative w-80">
            <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-zinc-500" />
            <input 
              type="text" 
              placeholder="Search events..." 
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="w-full bg-zinc-950 border border-zinc-800 rounded-lg py-2 pl-10 pr-4 text-sm text-zinc-200 focus:outline-none focus:ring-1 focus:ring-emerald-500 focus:border-emerald-500 transition-all"
            />
          </div>
          <div className="flex gap-3">
            <button className="flex items-center gap-2 bg-zinc-950 border border-zinc-800 text-zinc-200 px-4 py-2 rounded-lg text-sm font-medium hover:bg-zinc-800 transition-colors">
              <Calendar className="w-4 h-4 text-zinc-400" />
              Filter by Date
            </button>
            <button className="flex items-center gap-2 bg-zinc-950 border border-zinc-800 text-zinc-200 px-4 py-2 rounded-lg text-sm font-medium hover:bg-zinc-800 transition-colors">
              <Download className="w-4 h-4 text-zinc-400" />
              Export CSV
            </button>
          </div>
        </div>

        {/* Data Table Container */}
        <div className="w-full overflow-hidden rounded-xl border border-zinc-800 bg-zinc-950/50 flex flex-col min-h-0 shadow-2xl relative z-10">
          <div className="overflow-y-auto flex-1 scrollbar-thin scrollbar-thumb-zinc-800 scrollbar-track-transparent">
            <table className="w-full border-collapse text-left">
              <thead className="bg-zinc-900 sticky top-0 z-10">
                <tr className="border-b border-zinc-800">
                  <th className="px-6 py-4 text-xs uppercase tracking-widest text-zinc-400 font-black">Timestamp</th>
                  <th className="px-6 py-4 text-xs uppercase tracking-widest text-zinc-400 font-black">Event Type</th>
                  <th className="px-6 py-4 text-xs uppercase tracking-widest text-zinc-400 font-black">Media</th>
                  <th className="px-6 py-4 text-xs uppercase tracking-widest text-zinc-400 font-black">Status</th>
                  <th className="px-6 py-4 text-xs uppercase tracking-widest text-zinc-400 font-black text-right">Actions</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-zinc-800/50">
                {filteredEvents.map((evt) => (
                  <tr key={evt.id} className="hover:bg-zinc-800/30 transition-colors cursor-pointer group">
                    <td className="px-6 py-4 font-mono text-zinc-300 text-sm whitespace-nowrap">
                      {new Date(evt.timestamp).toLocaleString()}
                    </td>
                    <td className="px-6 py-4">
                      <span className="bg-emerald-500/10 text-emerald-500 px-3 py-1 rounded-full text-[10px] font-black uppercase tracking-wider border border-emerald-500/20">
                        {formatTitleCase(evt.eventType)}
                      </span>
                    </td>
                    <td className="px-6 py-4 text-zinc-400 text-sm max-w-xs truncate">
                      {evt.imageKey}
                    </td>
                    <td className="px-6 py-4">
                      <div className="flex items-center gap-2">
                        <div className="w-1.5 h-1.5 rounded-full bg-emerald-500 shadow-[0_0_5px_rgba(16,185,129,0.5)]"></div>
                        <span className="text-zinc-300 text-sm">Processed</span>
                      </div>
                    </td>
                    <td className="px-6 py-4 text-right">
                       <button className="opacity-0 group-hover:opacity-100 transition-opacity p-2 hover:bg-zinc-700 rounded-lg">
                          <Play className="w-4 h-4 text-emerald-500 fill-emerald-500/20" />
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
