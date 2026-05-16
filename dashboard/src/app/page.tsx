'use client';

import { useEffect, useState, useRef } from 'react';
import Image from 'next/image';
import MainLayout, { ConnectionStatus } from '../components/MainLayout';

interface DoorbellEvent {
  id: number;
  timestamp: string;
  eventType: string;
  imageKey: string;
  audioKey?: string | null;
}

export default function Home() {
  const [events, setEvents] = useState<DoorbellEvent[]>([]);
  const [activeEvent, setActiveEvent] = useState<DoorbellEvent | null>(null);
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('connecting');
  const [isAtBottom, setIsAtBottom] = useState(false);
  const scrollRef = useRef<HTMLDivElement>(null);

  const handleScroll = () => {
    if (scrollRef.current) {
      const { scrollTop, scrollHeight, clientHeight } = scrollRef.current;
      setIsAtBottom(scrollTop + clientHeight >= scrollHeight - 20);
    }
  };

  useEffect(() => {
    handleScroll();
  }, [events]);

  const API_BASE_URL = `/api/events`;
  const MINIO_BASE_URL = `${API_BASE_URL}/media`;

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
      setConnectionStatus('connected'); // Defensive: any data means we are connected
      try {
        const newEvent: DoorbellEvent = JSON.parse(e.data);
        setEvents((prev) => [newEvent, ...prev]);
        setActiveEvent(newEvent);
      } catch (err) {
        console.error("Failed to parse event", err);
      }
    });

    return () => eventSource.close();
  }, []);

  const formatTitleCase = (str: string) => {
    return str.toLowerCase().split('_').map(word => word.charAt(0).toUpperCase() + word.slice(1)).join(' ');
  };

  const isMostRecent = activeEvent && events.length > 0 && activeEvent.id === events[0].id;

  const playAudio = () => {
    if (activeEvent?.audioKey) {
      const audio = new Audio(`${MINIO_BASE_URL}/${activeEvent.audioKey}`);
      audio.play().catch(err => console.error("Audio play failed:", err));
    }
  };

  return (
    <MainLayout status={connectionStatus} breadcrumbs={[{ label: 'Dashboard' }, { label: 'Active Event', active: true }]}>
      {!activeEvent ? (
        <div className="flex-1 flex items-center justify-center relative">
          <div className="flex flex-col items-center gap-4 relative z-10">
            <svg className="animate-spin h-8 w-8 text-emerald-500" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
              <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
              <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
            </svg>
            <p className="text-sm text-zinc-400 font-mono uppercase tracking-widest">Awaiting Signal...</p>
          </div>
        </div>
      ) : (
        <div className="grid grid-cols-1 lg:grid-cols-12 gap-6 items-start w-full max-w-[1350px] mx-auto">
          
          {/* Left Column: Media Card (The Anchor) */}
          <div className="lg:col-span-8 h-fit flex flex-col">
            <div key={activeEvent.id} className="w-full bg-zinc-950 border border-zinc-800 rounded-3xl overflow-hidden shadow-2xl flex flex-col animate-flash-event h-fit">
              
              {/* Image Container - Strict 4:3 Aspect Ratio */}
              <div className="relative w-full aspect-[4/3] bg-black flex items-center justify-center border-b border-zinc-800 group overflow-hidden">
                <Image 
                  src={`${MINIO_BASE_URL}/${activeEvent.imageKey}`} 
                  alt="Doorbell snapshot" 
                  fill 
                  className="object-cover w-full transition-transform duration-700 group-hover:scale-105" 
                  unoptimized 
                />
                {isMostRecent && (
                  <div className="absolute top-6 right-6 bg-emerald-500/20 border border-emerald-500/50 text-emerald-500 text-xs font-black px-4 py-1.5 rounded-full shadow-lg backdrop-blur-md flex items-center gap-2 tracking-widest ring-1 ring-emerald-500/30 z-20">
                    <span className="w-2 h-2 rounded-full bg-emerald-500 animate-pulse"></span>
                    MOST RECENT
                  </div>
                )}
              </div>

              {/* Event Metadata & Audio Interface */}
              <div className="p-8 flex flex-col sm:flex-row justify-between items-start sm:items-center gap-8 bg-zinc-900/40 shrink-0 text-left">
                <div className="flex-1">
                  <h2 className="text-3xl font-black text-zinc-100 tracking-tight">{formatTitleCase(activeEvent.eventType)}</h2>
                  <p className="text-lg font-mono text-zinc-400 mt-2 tracking-widest leading-relaxed">
                    {new Date(activeEvent.timestamp).toLocaleString(undefined, { weekday: 'short', month: 'short', day: 'numeric', hour: 'numeric', minute: '2-digit', second: '2-digit' })}
                  </p>
                </div>
                <div className="flex gap-4 shrink-0">
                  <button onClick={playAudio} disabled={!activeEvent.audioKey} className={`flex items-center gap-2 px-6 py-3.5 rounded-xl text-sm font-bold uppercase tracking-widest transition-all border ${activeEvent.audioKey ? 'bg-zinc-800 hover:bg-zinc-700 border-zinc-600 text-zinc-100 shadow-lg' : 'bg-zinc-900 border-zinc-800 text-zinc-600 cursor-not-allowed opacity-50'}`}>
                    <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.664z" /><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M21 12a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
                    Play Audio
                  </button>
                  {isMostRecent && (
                    <button className="flex items-center gap-2 bg-emerald-600 hover:bg-emerald-500 active:scale-95 text-white px-6 py-3.5 rounded-xl text-sm font-bold uppercase tracking-widest transition-all shadow-xl shadow-emerald-900/40 border border-emerald-500/50">
                      <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 11a7 7 0 01-7 7m0 0a7 7 0 01-7-7m7 7v4m0 0H8m4 0h4m-4-8a3 3 0 01-3-3V5a3 3 0 116 0v6a3 3 0 01-3 3z" /></svg>
                      Push to Talk
                    </button>
                  )}
                </div>
              </div>
            </div>
          </div>

          {/* Right Wrapper (The Bounding Box) */}
          <div className="lg:col-span-4 relative h-full self-stretch min-h-[500px]">
            {/* The Scrolling List (The Inner Content) */}
            <div 
              ref={scrollRef}
              onScroll={handleScroll}
              className={`absolute inset-0 overflow-y-auto pr-2 scrollbar-thin scrollbar-thumb-zinc-800 scrollbar-track-transparent transition-[mask-image] duration-300 ${!isAtBottom ? '[mask-image:linear-gradient(to_bottom,black_85%,transparent_100%)] [-webkit-mask-image:linear-gradient(to_bottom,black_85%,transparent_100%)]' : ''}`}
            >
              <h3 className="text-xs font-mono text-zinc-500 uppercase tracking-[0.2em] mb-5 flex items-center gap-3 shrink-0 px-2 font-black sticky top-0 bg-zinc-900/80 backdrop-blur-md py-2 z-10">
                <svg className="w-4 h-4 text-emerald-500" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={3} d="M4 6h16M4 10h16M4 14h16M4 18h16" /></svg>
                Recent Log
              </h3>
              <div className="flex flex-col gap-4">
                {events.map((evt) => (
                  <button key={evt.id} onClick={() => setActiveEvent(evt)} className={`text-left bg-zinc-950 border rounded-2xl p-6 transition-all duration-300 animate-slide-in shrink-0 relative group overflow-hidden ${activeEvent.id === evt.id ? 'border-emerald-500/50 shadow-[0_0_20px_rgba(16,185,129,0.15)] bg-emerald-500/[0.03]' : 'border-zinc-800 hover:border-zinc-600 hover:bg-zinc-900/50'}`}>
                    <div className="flex items-center justify-between mb-3 relative z-10 text-left">
                      <span className="text-lg font-black text-zinc-100 tracking-tight">{formatTitleCase(evt.eventType)}</span>
                      {evt.id === events[0]?.id && <span className="flex h-2.5 w-2.5"><span className="animate-ping absolute inline-flex h-2.5 w-2.5 rounded-full bg-emerald-400 opacity-75"></span><span className="relative inline-flex rounded-full h-2.5 w-2.5 bg-emerald-500"></span></span>}
                    </div>
                    <div className="text-sm text-zinc-400 font-bold uppercase tracking-wider mb-1 relative z-10 text-left">{new Date(evt.timestamp).toLocaleDateString(undefined, { month: 'short', day: 'numeric', year: 'numeric' })}</div>
                    <div className="text-xs font-mono text-zinc-500 tracking-widest relative z-10 text-left">{new Date(evt.timestamp).toLocaleTimeString(undefined, { hour: 'numeric', minute: '2-digit', second: '2-digit' })}</div>
                    {activeEvent.id === evt.id && <div className="absolute left-0 top-0 bottom-0 w-1 bg-emerald-500 shadow-[2px_0_10px_rgba(16,185,129,0.5)]"></div>}
                  </button>
                ))}
              </div>
            </div>
          </div>
        </div>
      )}
    </MainLayout>
  );
}
