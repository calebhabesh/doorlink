'use client';

import { useEffect, useState } from 'react';
import Image from 'next/image';

interface DoorbellEvent {
  id: number;
  timestamp: string;
  eventType: string;
  imageKey: string;
}

export default function Home() {
  const [events, setEvents] = useState<DoorbellEvent[]>([]);
  const [activeEvent, setActiveEvent] = useState<DoorbellEvent | null>(null);
  const [isConnected, setIsConnected] = useState(false);

  const API_BASE_URL = 'http://localhost:8080/api/events';
  const MINIO_BASE_URL = 'http://localhost:9000/doorbell-images';

  useEffect(() => {
    fetch(API_BASE_URL)
      .then((res) => res.json())
      .then((data: DoorbellEvent[]) => {
        setEvents(data);
        if (data.length > 0) {
          setActiveEvent(data[0]);
        }
      })
      .catch((err) => console.error("Failed to fetch history", err));

    const eventSource = new EventSource(`${API_BASE_URL}/stream`);

    eventSource.onopen = () => setIsConnected(true);
    eventSource.onerror = () => setIsConnected(false);

    eventSource.addEventListener('doorbell-event', (e) => {
      try {
        const newEvent: DoorbellEvent = JSON.parse(e.data);
        setEvents((prev) => [newEvent, ...prev]);
        setActiveEvent(newEvent);
      } catch (err) {
        console.error("Failed to parse event", err);
      }
    });

    return () => {
      eventSource.close();
    };
  }, []);

  const formatTitleCase = (str: string) => {
    return str.toLowerCase().split('_').map(word => word.charAt(0).toUpperCase() + word.slice(1)).join(' ');
  };

  if (!activeEvent) {
    return (
      <div className="min-h-screen bg-zinc-950 text-zinc-100 flex items-center justify-center">
        <div className="flex flex-col items-center gap-4">
          <svg className="animate-spin h-8 w-8 text-emerald-500" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
            <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
            <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
          </svg>
          <p className="text-sm text-zinc-400 font-mono uppercase tracking-widest">Awaiting Signal...</p>
        </div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-zinc-950 text-zinc-100 flex font-sans">
      
      {/* Left Sidebar (Navigation) */}
      <aside className="w-64 bg-zinc-900 border-r border-zinc-800 flex flex-col h-screen sticky top-0">
        {/* Brand Header */}
        <div className="h-16 flex items-center px-6 border-b border-zinc-800">
          <svg className="w-6 h-6 text-blue-500 mr-3" fill="currentColor" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 12.2 25.5">
            <path d="M6.938 6.586a.852.852 0 0 1-.852.851a.852.852 0 0 1-.852-.851a.852.852 0 0 1 .852-.852a.852.852 0 0 1 .851.852M9.08 16.54c0 2.673-3.231 4.011-5.121 2.121c-1.89-1.89-.552-5.121 2.121-5.121a3 3 0 0 1 3 3m-3-2.54c-2.227 0-3.343 2.733-1.768 4.308c1.575 1.575 4.268.46 4.268-1.768A2.5 2.5 0 0 0 6.08 14m0 .54c-1.782 0-2.674 2.154-1.414 3.414c1.26 1.26 3.414.368 3.414-1.414a2 2 0 0 0-2-2M-.008 25.381l.074.067l.098.058l1.976.004c-.119-5.397-.045-10.793-.068-16.19c-.66 0-1.406.023-2.066.023ZM12.203 9.314c-.683 0-1.39-.004-2.073-.004V25.5h1.61l.27.004l.08-.004l.07-.08l.047-.112c.024-5.304-.005-10.734-.004-15.994M2.14 25.51l8-.01v-.73l-8.02-.008M8.35 4.81v3.65h-4.6V4.81Zm-2.27.3c-1.336 0-2.006 1.616-1.06 2.56c.944.946 2.56.276 2.56-1.06a1.5 1.5 0 0 0-1.5-1.5m-2.33-.3a3 3 0 0 0-.63 1.8a3.56 3.56 0 0 0 .63 1.85Zm4.6 3.65A3.75 3.75 0 0 0 9 6.61a3.02 3.02 0 0 0-.65-1.8Zm-8.349.418l2.046-.01l.033-4.358l-2.072.003Zm12.204-4.37l-2.083-.007l.016 4.37l2.065.005zM0 4.08l2.08-.009V0L-.001.008ZM12.21-.003L10.08 0v4.07h2l.123-.004ZM2.08 4.07h8V0h-8zm-.008 6.687l8.07.058l-.012-1.505l-8.058.01ZM2.62 4.51h-.29l-.003 4.352h.282Zm7.22 0h-.29v4.36h.33z"/>
          </svg>
          <span className="font-bold text-lg tracking-wide text-zinc-100">Smart Doorbell</span>
        </div>

        {/* Menu Items */}
        <nav className="flex-1 py-6 flex flex-col gap-2">
          <a href="#" className="flex items-center px-6 py-3 bg-emerald-500/10 border-l-2 border-emerald-500 text-emerald-500">
            <svg className="w-5 h-5 mr-3" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6" /></svg>
            <span className="text-sm font-medium">Dashboard</span>
          </a>
          <a href="#" className="flex items-center px-6 py-3 text-zinc-400 hover:text-zinc-100 hover:bg-zinc-800/50 border-l-2 border-transparent transition-colors">
            <svg className="w-5 h-5 mr-3" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
            <span className="text-sm font-medium">Recent History</span>
          </a>
          <a href="#" className="flex items-center px-6 py-3 text-zinc-400 hover:text-zinc-100 hover:bg-zinc-800/50 border-l-2 border-transparent transition-colors">
            <svg className="w-5 h-5 mr-3" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M8 7V3m8 4V3m-9 8h10M5 21h14a2 2 0 002-2V7a2 2 0 00-2-2H5a2 2 0 00-2 2v12a2 2 0 002 2z" /></svg>
            <span className="text-sm font-medium">Deliveries</span>
          </a>
          <a href="#" className="flex items-center px-6 py-3 text-zinc-400 hover:text-zinc-100 hover:bg-zinc-800/50 border-l-2 border-transparent transition-colors">
            <svg className="w-5 h-5 mr-3" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 10V3L4 14h7v7l9-11h-7z" /></svg>
            <span className="text-sm font-medium">System Health</span>
          </a>
          <a href="#" className="flex items-center px-6 py-3 text-zinc-400 hover:text-zinc-100 hover:bg-zinc-800/50 border-l-2 border-transparent transition-colors">
            <svg className="w-5 h-5 mr-3" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z" /><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" /></svg>
            <span className="text-sm font-medium">Settings</span>
          </a>
        </nav>
      </aside>

      {/* Main Content Area */}
      <main className="flex-1 flex flex-col min-w-0">
        
        {/* Top Bar */}
        <header className="h-16 bg-zinc-950 border-b border-zinc-800 flex justify-between items-center px-8 sticky top-0 z-10">
          <div className="flex items-center text-sm font-mono uppercase tracking-widest">
            <span className="text-zinc-500">Dashboard</span>
            <span className="mx-2 text-zinc-700">/</span>
            <span className="text-zinc-100">Active Event</span>
          </div>
          <div className="flex items-center gap-2 bg-zinc-900 border border-zinc-800 px-3 py-1.5 rounded text-xs font-mono tracking-wider">
            <div className="relative flex h-2 w-2">
              {isConnected && <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>}
              <span className={`relative inline-flex rounded-full h-2 w-2 ${isConnected ? 'bg-emerald-500' : 'bg-red-500'}`}></span>
            </div>
            <span className={isConnected ? 'text-emerald-500' : 'text-red-500'}>{isConnected ? 'SYSTEM LIVE' : 'DISCONNECTED'}</span>
          </div>
        </header>

        {/* Dashboard Content */}
        <div className="p-8 flex flex-col items-center flex-1 overflow-y-auto">
          
          {/* Main Media Card */}
          <div className="w-full max-w-4xl bg-zinc-900 border border-zinc-800 rounded-xl overflow-hidden shadow-2xl flex flex-col">
            
            {/* Image Container */}
            <div className="relative w-full aspect-video bg-black flex items-center justify-center border-b border-zinc-800">
              <Image
                src={`${MINIO_BASE_URL}/${activeEvent.imageKey}`}
                alt="Doorbell snapshot"
                fill
                className="object-contain"
                unoptimized
              />
            </div>
            
            {/* Event Metadata & Audio Interface */}
            <div className="p-6 flex flex-col md:flex-row justify-between items-start md:items-center gap-6">
              
              {/* Metadata */}
              <div>
                <h2 className="text-2xl font-bold text-zinc-100">{formatTitleCase(activeEvent.eventType)}</h2>
                <p className="text-sm font-mono text-zinc-400 mt-1 uppercase tracking-widest">
                  {new Date(activeEvent.timestamp).toLocaleString()}
                </p>
              </div>

              {/* Audio Controls */}
              <div className="flex gap-3">
                <button className="flex items-center gap-2 bg-zinc-800 hover:bg-zinc-700 border border-zinc-700 text-zinc-100 px-4 py-2 rounded transition-colors text-sm font-medium">
                  <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.664z" />
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
                  </svg>
                  Play Audio
                </button>
                <button className="flex items-center gap-2 bg-emerald-600 hover:bg-emerald-500 text-white px-4 py-2 rounded transition-colors text-sm font-medium shadow-lg shadow-emerald-900/50">
                  <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 11a7 7 0 01-7 7m0 0a7 7 0 01-7-7m7 7v4m0 0H8m4 0h4m-4-8a3 3 0 01-3-3V5a3 3 0 116 0v6a3 3 0 01-3 3z" />
                  </svg>
                  Push to Talk
                </button>
              </div>
            </div>
          </div>

          {/* Minimal History Row */}
          <div className="w-full max-w-4xl mt-8">
             <h3 className="text-xs font-mono text-zinc-500 uppercase tracking-widest mb-4">Recent Log</h3>
             <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
                {events.slice(0, 4).map((evt) => (
                  <button
                    key={evt.id}
                    onClick={() => setActiveEvent(evt)}
                    className={`text-left bg-zinc-900 border rounded p-3 transition-colors ${
                      activeEvent.id === evt.id ? 'border-emerald-500 bg-emerald-500/5' : 'border-zinc-800 hover:border-zinc-600'
                    }`}
                  >
                    <div className="text-sm font-bold text-zinc-200 truncate">{formatTitleCase(evt.eventType)}</div>
                    <div className="text-xs font-mono text-zinc-500 mt-1 truncate">{new Date(evt.timestamp).toLocaleTimeString()}</div>
                  </button>
                ))}
             </div>
          </div>

        </div>
      </main>
    </div>
  );
}
