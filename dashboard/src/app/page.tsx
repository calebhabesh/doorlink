'use client';

import { useEffect, useState } from 'react';
import Image from 'next/image';

interface DoorbellEvent {
  id: number;
  timestamp: string;
  eventType: string;
  imageKey: string;
  hasAudio?: boolean; // For future implementation
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

  const isMostRecent = activeEvent && events.length > 0 && activeEvent.id === events[0].id;

  if (!activeEvent) {
    return (
      <div className="min-h-screen bg-zinc-950 text-zinc-100 flex items-center justify-center relative">
        <div className="absolute inset-0 bg-grid opacity-20 pointer-events-none"></div>
        <div className="flex flex-col items-center gap-4 relative z-10">
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
    <div className="min-h-screen bg-zinc-950 text-zinc-100 flex font-sans overflow-hidden">
      
      {/* Left Sidebar (Navigation) */}
      <aside className="w-64 bg-zinc-950 flex flex-col h-screen shrink-0 z-20">
        <div className="h-20 flex items-center px-6">
          <svg className="w-8 h-8 text-blue-500 mr-3" fill="currentColor" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 12.2 25.5">
            <path d="M6.938 6.586a.852.852 0 0 1-.852.851a.852.852 0 0 1-.852-.851a.852.852 0 0 1 .852-.852a.852.852 0 0 1 .851.852M9.08 16.54c0 2.673-3.231 4.011-5.121 2.121c-1.89-1.89-.552-5.121 2.121-5.121a3 3 0 0 1 3 3m-3-2.54c-2.227 0-3.343 2.733-1.768 4.308c1.575 1.575 4.268.46 4.268-1.768A2.5 2.5 0 0 0 6.08 14m0 .54c-1.782 0-2.674 2.154-1.414 3.414c1.26 1.26 3.414.368 3.414-1.414a2 2 0 0 0-2-2M-.008 25.381l.074.067l.098.058l1.976.004c-.119-5.397-.045-10.793-.068-16.19c-.66 0-1.406.023-2.066.023ZM12.203 9.314c-.683 0-1.39-.004-2.073-.004V25.5h1.61l.27.004l.08-.004l.07-.08l.047-.112c.024-5.304-.005-10.734-.004-15.994M2.14 25.51l8-.01v-.73l-8.02-.008M8.35 4.81v3.65h-4.6V4.81Zm-2.27.3c-1.336 0-2.006 1.616-1.06 2.56c.944.946 2.56.276 2.56-1.06a1.5 1.5 0 0 0-1.5-1.5m-2.33-.3a3 3 0 0 0-.63 1.8a3.56 3.56 0 0 0 .63 1.85Zm4.6 3.65A3.75 3.75 0 0 0 9 6.61a3.02 3.02 0 0 0-.65-1.8Zm-8.349.418l2.046-.01l.033-4.358l-2.072.003Zm12.204-4.37l-2.083-.007l.016 4.37l2.065.005zM0 4.08l2.08-.009V0L-.001.008ZM12.21-.003L10.08 0v4.07h2l.123-.004ZM2.08 4.07h8V0h-8zm-.008 6.687l8.07.058l-.012-1.505l-8.058.01ZM2.62 4.51h-.29l-.003 4.352h.282Zm7.22 0h-.29v4.36h.33z"/>
          </svg>
          <span className="font-bold text-xl tracking-wide text-zinc-100">Smart Doorbell</span>
        </div>

        <nav className="flex-1 py-6 flex flex-col gap-2">
          <a href="#" className="flex items-center px-6 py-4 bg-emerald-500/10 border-l-4 border-emerald-500 text-emerald-500">
            <svg className="w-5 h-5 mr-4" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6" /></svg>
            <span className="text-sm font-medium uppercase tracking-wider">Dashboard</span>
          </a>
          <a href="#" className="flex items-center px-6 py-4 text-zinc-400 hover:text-zinc-100 hover:bg-zinc-900 transition-colors border-l-4 border-transparent">
            <svg className="w-5 h-5 mr-4" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
            <span className="text-sm font-medium uppercase tracking-wider">History</span>
          </a>
          <a href="#" className="flex items-center px-6 py-4 text-zinc-400 hover:text-zinc-100 hover:bg-zinc-900 transition-colors border-l-4 border-transparent">
            <svg className="w-5 h-5 mr-4" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M8 7V3m8 4V3m-9 8h10M5 21h14a2 2 0 002-2V7a2 2 0 00-2-2H5a2 2 0 00-2 2v12a2 2 0 002 2z" /></svg>
            <span className="text-sm font-medium uppercase tracking-wider">Deliveries</span>
          </a>
          <a href="#" className="flex items-center px-6 py-4 text-zinc-400 hover:text-zinc-100 hover:bg-zinc-900 transition-colors border-l-4 border-transparent">
            <svg className="w-5 h-5 mr-4" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 10V3L4 14h7v7l9-11h-7z" /></svg>
            <span className="text-sm font-medium uppercase tracking-wider">System Health</span>
          </a>
        </nav>
      </aside>

      {/* Main Content Area Container */}
      <main className="flex-1 bg-zinc-900 rounded-l-[2.5rem] border-l border-t border-b border-zinc-800 shadow-2xl relative overflow-hidden flex flex-col my-3 mr-4">
        {/* Subtle Grid Background */}
        <div className="absolute inset-0 bg-grid opacity-[0.35] pointer-events-none z-0"></div>
        
        {/* Top Bar */}
        <header className="h-20 border-b border-zinc-800/50 flex justify-between items-center px-10 relative z-10 bg-zinc-900/50 backdrop-blur-sm">
          <div className="flex items-center font-mono uppercase tracking-widest text-xl">
            <span className="text-zinc-500">Dashboard</span>
            <span className="mx-3 text-zinc-700">/</span>
            <span className="text-zinc-100 font-black">Active Event</span>
          </div>
          <div className="flex items-center gap-4 bg-zinc-950 border border-zinc-800 px-5 py-2.5 rounded-lg shadow-inner font-mono tracking-wider text-sm">
            <div className="relative flex h-3 w-3">
              {isConnected && <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>}
              <span className={`relative inline-flex rounded-full h-3 w-3 ${isConnected ? 'bg-emerald-500' : 'bg-red-500'}`}></span>
            </div>
            <span className={`font-bold uppercase ${isConnected ? 'text-emerald-500' : 'text-red-500'}`}>{isConnected ? 'SYSTEM LIVE' : 'DISCONNECTED'}</span>
          </div>
        </header>

        {/* Dashboard Content */}
        <div className="p-10 flex-1 relative z-10 overflow-hidden">
          
          <div className="grid grid-cols-1 lg:grid-cols-12 gap-10 w-full max-w-[1600px] mx-auto h-fit items-start">
            
            {/* Left Column: Media Card (Scaled Up) */}
            <div className="lg:col-span-8 flex flex-col min-h-0 h-fit">
              {/* Media Card - key prop forces re-render/animation on change */}
              <div key={activeEvent.id} className="w-full bg-zinc-950 border border-zinc-800 rounded-3xl overflow-hidden shadow-2xl flex flex-col animate-flash-event h-fit">
                
                {/* Image Container - Strict 4:3 Aspect Ratio */}
                <div className="relative w-full aspect-[4/3] bg-black flex items-center justify-center border-b border-zinc-800 group overflow-hidden">
                  <Image
                    src={`${MINIO_BASE_URL}/${activeEvent.imageKey}`}
                    alt="Doorbell snapshot"
                    fill
                    className="object-cover transition-transform duration-700 group-hover:scale-105"
                    unoptimized
                  />
                  {isMostRecent && (
                    <div className="absolute top-6 right-6 bg-emerald-500/20 border border-emerald-500/50 text-emerald-500 text-xs font-black px-4 py-1.5 rounded-full shadow-lg backdrop-blur-md flex items-center gap-2 tracking-widest ring-1 ring-emerald-500/30">
                       <span className="w-2 h-2 rounded-full bg-emerald-500 animate-pulse"></span>
                       MOST RECENT
                    </div>
                  )}
                </div>
                
                {/* Event Metadata & Audio Interface */}
                <div className="p-8 flex flex-col sm:flex-row justify-between items-start sm:items-center gap-8 bg-zinc-900/40 shrink-0">
                  
                  {/* Metadata */}
                  <div className="flex-1">
                    <h2 className="text-3xl font-black text-zinc-100 tracking-tight">{formatTitleCase(activeEvent.eventType)}</h2>
                    <p className="text-lg font-mono text-zinc-400 mt-2 tracking-widest leading-relaxed">
                      {new Date(activeEvent.timestamp).toLocaleString(undefined, {
                        weekday: 'short', month: 'short', day: 'numeric',
                        hour: 'numeric', minute: '2-digit', second: '2-digit'
                      })}
                    </p>
                  </div>

                  {/* Audio Controls */}
                  <div className="flex gap-4 shrink-0">
                    <button 
                      disabled={!activeEvent.hasAudio} 
                      className={`flex items-center gap-2 px-6 py-3.5 rounded-xl text-sm font-bold uppercase tracking-widest transition-all border ${
                        activeEvent.hasAudio 
                          ? 'bg-zinc-800 hover:bg-zinc-700 border-zinc-600 text-zinc-100 shadow-lg' 
                          : 'bg-zinc-900 border-zinc-800 text-zinc-600 cursor-not-allowed opacity-50'
                      }`}
                    >
                      <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.664z" />
                        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
                      </svg>
                      Play Audio
                    </button>
                    
                    {isMostRecent && (
                      <button className="flex items-center gap-2 bg-emerald-600 hover:bg-emerald-500 active:scale-95 text-white px-6 py-3.5 rounded-xl text-sm font-bold uppercase tracking-widest transition-all shadow-xl shadow-emerald-900/40 border border-emerald-500/50">
                        <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 11a7 7 0 01-7 7m0 0a7 7 0 01-7-7m7 7v4m0 0H8m4 0h4m-4-8a3 3 0 01-3-3V5a3 3 0 116 0v6a3 3 0 01-3 3z" />
                        </svg>
                        Push to Talk
                      </button>
                    )}
                  </div>
                </div>
              </div>
            </div>

            {/* Right Column: History Feed (Absolute Inset Trick) */}
            <div className="lg:col-span-4 relative h-full min-h-[500px]">
              <div className="absolute inset-0 flex flex-col pr-2 overflow-hidden">
                <h3 className="text-xs font-mono text-zinc-500 uppercase tracking-[0.2em] mb-5 flex items-center gap-3 shrink-0 px-2 font-black">
                  <svg className="w-4 h-4 text-emerald-500" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={3} d="M4 6h16M4 10h16M4 14h16M4 18h16" /></svg>
                  Recent Log
                </h3>
                
                <div className="flex flex-col gap-5 overflow-y-auto pr-3 flex-1 scrollbar-thin scrollbar-thumb-zinc-800 scrollbar-track-transparent">
                  {events.map((evt) => (
                    <button
                      key={evt.id}
                      onClick={() => setActiveEvent(evt)}
                      className={`text-left bg-zinc-950 border rounded-2xl p-6 transition-all duration-300 animate-slide-in shrink-0 relative group overflow-hidden ${
                        activeEvent.id === evt.id 
                          ? 'border-emerald-500/50 shadow-[0_0_20px_rgba(16,185,129,0.15)] bg-emerald-500/[0.03]' 
                          : 'border-zinc-800 hover:border-zinc-600 hover:bg-zinc-900/50'
                      }`}
                    >
                      <div className="flex items-center justify-between mb-3 relative z-10">
                        <span className="text-lg font-black text-zinc-100 tracking-tight">{formatTitleCase(evt.eventType)}</span>
                        {evt.id === events[0]?.id && (
                          <span className="flex h-2.5 w-2.5">
                             <span className="animate-ping absolute inline-flex h-2.5 w-2.5 rounded-full bg-emerald-400 opacity-75"></span>
                             <span className="relative inline-flex rounded-full h-2.5 w-2.5 bg-emerald-500"></span>
                          </span>
                        )}
                      </div>
                      <div className="text-sm text-zinc-400 font-bold uppercase tracking-wider mb-1 relative z-10">
                        {new Date(evt.timestamp).toLocaleDateString(undefined, { month: 'short', day: 'numeric', year: 'numeric' })}
                      </div>
                      <div className="text-xs font-mono text-zinc-500 tracking-widest relative z-10">
                        {new Date(evt.timestamp).toLocaleTimeString(undefined, { hour: 'numeric', minute: '2-digit', second: '2-digit' })}
                      </div>
                      {activeEvent.id === evt.id && (
                        <div className="absolute left-0 top-0 bottom-0 w-1 bg-emerald-500 shadow-[2px_0_10px_rgba(16,185,129,0.5)]"></div>
                      )}
                    </button>
                  ))}
                </div>
              </div>
            </div>
            
          </div>
        </div>
      </main>
    </div>
  );
}