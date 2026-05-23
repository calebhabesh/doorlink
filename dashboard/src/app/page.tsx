'use client';

import { useEffect, useState, useRef } from 'react';
import Image from 'next/image';
import MainLayout, { ConnectionStatus } from '../components/MainLayout';
import PttButton from '../components/PttButton';

interface DoorbellEvent {
  id: number;
  timestamp: string;
  eventType: string;
  imageKey: string;
  audioKey?: string | null;
}

const API_BASE_URL = `/api/events`;
const MINIO_BASE_URL = `${API_BASE_URL}/media`;

export default function Home() {
  const [events, setEvents] = useState<DoorbellEvent[]>([]);
  const [activeEvent, setActiveEvent] = useState<DoorbellEvent | null>(null);
  const [isImageLoaded, setIsImageLoaded] = useState(false);
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('connecting');
  const [latestLiveEventId, setLatestLiveEventId] = useState<number | null>(null);
  const [isAtBottom, setIsAtBottom] = useState(false);
  const scrollRef = useRef<HTMLDivElement>(null);

  const handleScroll = () => {
    if (scrollRef.current) {
      const { scrollTop, scrollHeight, clientHeight } = scrollRef.current;
      setIsAtBottom(scrollTop + clientHeight >= scrollHeight - 20);
    }
  };

  useEffect(() => {
    setIsImageLoaded(false);
  }, [activeEvent?.id]);

  useEffect(() => {
    handleScroll();
  }, [events]);

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
      setConnectionStatus('connected'); // Defensive: any data means we are connected
      try {
        const newEvent: DoorbellEvent = JSON.parse(e.data);
        setEvents((prev) => {
          if (prev.some((event) => event.id === newEvent.id)) {
            return prev;
          }
          return [newEvent, ...prev];
        });
        setActiveEvent(newEvent);
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

  const formatEventDate = (dateStr: string) => {
    const date = new Date(dateStr);
    const weekday = date.toLocaleDateString(undefined, { weekday: 'short' });
    const day = date.toLocaleDateString(undefined, { day: 'numeric' });
    const month = date.toLocaleDateString(undefined, { month: 'short' });
    const time = date.toLocaleTimeString(undefined, { hour: 'numeric', minute: '2-digit', second: '2-digit', hour12: true });
    return `${weekday}, ${day} ${month}, ${time}`;
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
              <div className="relative w-full aspect-[4/3] bg-zinc-900 flex items-center justify-center border-b border-zinc-800 group overflow-hidden">
                {!isImageLoaded && (
                  <div className="absolute inset-0 flex items-center justify-center">
                    <svg className="animate-spin h-8 w-8 text-zinc-700" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                      <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                      <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                    </svg>
                  </div>
                )}
                <Image 
                  src={`${MINIO_BASE_URL}/${activeEvent.imageKey}`} 
                  alt="Doorbell snapshot" 
                  fill 
                  className={`object-cover w-full transition-all duration-700 ease-out group-hover:scale-105 ${isImageLoaded ? 'opacity-100 blur-0' : 'opacity-0 blur-sm scale-105'}`} 
                  onLoad={() => setIsImageLoaded(true)}
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
              <div className="p-4 sm:p-8 flex flex-col sm:flex-row justify-between items-center gap-4 sm:gap-8 bg-zinc-900/40 shrink-0 text-center sm:text-left">
                <div className="flex-1 w-full">
                  <h2 className="text-xl sm:text-3xl font-black text-zinc-100 tracking-tight">{formatTitleCase(activeEvent.eventType)}</h2>
                  <p className="text-xs sm:text-lg font-mono text-zinc-400 mt-1 sm:mt-2 tracking-widest leading-relaxed">
                    {formatEventDate(activeEvent.timestamp)}
                  </p>
                </div>
                <div className="flex gap-3 sm:gap-4 shrink-0 justify-center w-full sm:w-auto">
                  <button onClick={playAudio} disabled={!activeEvent.audioKey} className={`flex items-center gap-2.5 px-6 py-4 sm:px-8 sm:py-4.5 rounded-2xl text-base sm:text-lg font-bold uppercase tracking-widest transition-all border ${activeEvent.audioKey ? 'bg-zinc-800 hover:bg-zinc-700 border-zinc-600 text-zinc-100 shadow-lg' : 'bg-zinc-900 border-zinc-800 text-zinc-600 cursor-not-allowed opacity-50'}`}>
                    <svg className="w-5 h-5 sm:w-6 sm:h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.664z" /><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M21 12a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
                    Play Audio
                  </button>
                  {isMostRecent && <PttButton />}
                </div>
              </div>
            </div>
          </div>

          {/* Right Wrapper (The Bounding Box) */}
          <div className="lg:col-span-4 lg:relative lg:h-full lg:self-stretch lg:min-h-[500px] flex flex-col h-[500px] lg:h-auto">
            {/* Opaque Header outside of scroll area */}
            <div className="flex items-center justify-between mb-4 pr-2 border-b border-zinc-800/50 py-4 bg-transparent shrink-0">
              <h3 className="text-xs font-mono text-zinc-500 uppercase tracking-[0.2em] flex items-center gap-3 font-black">
                <svg className="w-4 h-4 text-emerald-500" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={3} d="M4 6h16M4 10h16M4 14h16M4 18h16" /></svg>
                Recent Log
              </h3>
              <button 
                onClick={() => window.open(`${API_BASE_URL}/export`, '_blank')}
                className="text-[10px] font-black uppercase tracking-widest text-zinc-500 hover:text-emerald-400 transition-colors flex items-center gap-2 group"
              >
                <svg className="w-3.5 h-3.5 group-hover:scale-110 transition-transform" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 16v1a2 2 0 002 2h12a2 2 0 002-2v-1m-4-4l-4 4m0 0l-4-4m4 4V4" /></svg>
                Export CSV
              </button>
            </div>

            {/* The Scrolling List (The Inner Content) */}
            <div 
              ref={scrollRef}
              onScroll={handleScroll}
              className={`flex-1 lg:absolute lg:top-20 lg:inset-x-0 lg:bottom-0 overflow-y-auto pr-2 scrollbar-thin scrollbar-thumb-zinc-800 scrollbar-track-zinc-950/40 transition-[mask-image] duration-300 ${!isAtBottom ? '[mask-image:linear-gradient(to_bottom,transparent_0%,black_5%,black_90%,transparent_100%)] [-webkit-mask-image:linear-gradient(to_bottom,transparent_0%,black_5%,black_90%,transparent_100%)]' : '[mask-image:linear-gradient(to_bottom,transparent_0%,black_5%,black_100%)] [-webkit-mask-image:linear-gradient(to_bottom,transparent_0%,black_5%,black_100%)]'}`}
            >
              <div className="flex flex-col gap-4 pt-6">
                {events.map((evt) => (
                  <button key={evt.id} onClick={() => setActiveEvent(evt)} className={`text-left bg-zinc-950 border rounded-2xl p-6 transition-all duration-300 shrink-0 relative group overflow-hidden ${latestLiveEventId === evt.id ? 'animate-slide-in ' : ''}${activeEvent.id === evt.id ? 'border-emerald-500/50 shadow-[0_0_20px_rgba(16,185,129,0.15)] bg-emerald-500/[0.03]' : latestLiveEventId === evt.id ? 'border-emerald-400/70 shadow-[0_0_24px_rgba(52,211,153,0.22)] bg-emerald-500/[0.05]' : 'border-zinc-800 hover:border-zinc-600 hover:bg-zinc-900/50'}`}>
                    <div className="flex items-center justify-between mb-3 relative z-10 text-left">
                      <span className="text-lg font-black text-zinc-100 tracking-tight">{formatTitleCase(evt.eventType)}</span>
                      <span className="flex items-center gap-3">
                        {latestLiveEventId === evt.id && <span className="text-[10px] font-black uppercase tracking-[0.2em] text-emerald-400">New</span>}
                        {evt.id === events[0]?.id && <span className="flex h-2.5 w-2.5"><span className="animate-ping absolute inline-flex h-2.5 w-2.5 rounded-full bg-emerald-400 opacity-75"></span><span className="relative inline-flex rounded-full h-2.5 w-2.5 bg-emerald-500"></span></span>}
                      </span>
                    </div>
                    <div className="text-sm text-zinc-400 font-bold uppercase tracking-wider mb-1 relative z-10 text-left">{new Date(evt.timestamp).toLocaleDateString(undefined, { month: 'short', day: 'numeric', year: 'numeric' })}</div>
                    <div className="text-xs font-mono text-zinc-500 tracking-widest relative z-10 text-left">{new Date(evt.timestamp).toLocaleTimeString(undefined, { hour: 'numeric', minute: '2-digit', second: '2-digit', hour12: true })}</div>
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
