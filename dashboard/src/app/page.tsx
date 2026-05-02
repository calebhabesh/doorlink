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

  if (!activeEvent) {
    return (
      <div className="min-h-screen bg-slate-950 text-white flex items-center justify-center">
        <div className="flex flex-col items-center gap-4">
          <div className="w-8 h-8 border-4 border-t-blue-500 border-white/10 rounded-full animate-spin"></div>
          <p className="text-lg text-slate-400 font-medium tracking-wide">Waiting for events...</p>
        </div>
      </div>
    );
  }

  return (
    <main className="min-h-screen bg-gradient-to-br from-slate-900 via-indigo-950 to-slate-900 animate-gradient-x text-white font-sans flex flex-col p-6 overflow-hidden relative">
      
      {/* Decorative Blur Orbs */}
      <div className="absolute top-[-10%] left-[-10%] w-96 h-96 bg-blue-600/30 rounded-full mix-blend-screen filter blur-[100px] animate-pulse"></div>
      <div className="absolute bottom-[-10%] right-[-10%] w-96 h-96 bg-purple-600/30 rounded-full mix-blend-screen filter blur-[100px] animate-pulse" style={{ animationDelay: '2s' }}></div>

      {/* Floating Header Pill */}
      <header className="relative z-10 mx-auto w-full max-w-5xl">
        <div className="bg-white/5 backdrop-blur-xl border border-white/10 rounded-full px-6 py-4 flex justify-between items-center shadow-[0_8px_32px_rgba(0,0,0,0.3)]">
          <h1 className="text-xl font-bold tracking-tight text-white/90">
            Smart Doorbell
          </h1>
          <div className="flex items-center gap-3 bg-black/20 px-3 py-1.5 rounded-full border border-white/5">
            <div className="relative flex h-3 w-3">
              {isConnected && <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>}
              <span className={`relative inline-flex rounded-full h-3 w-3 ${isConnected ? 'bg-emerald-500' : 'bg-rose-500'}`}></span>
            </div>
            <span className="text-sm font-medium text-slate-300">{isConnected ? 'System Live' : 'Disconnected'}</span>
          </div>
        </div>
      </header>

      {/* Main Focus Area */}
      <div className="relative z-10 flex-1 flex flex-col items-center justify-center w-full max-w-5xl mx-auto mt-8 mb-8">
        
        {/* Glass Card for Image */}
        <div className="w-full bg-white/5 backdrop-blur-2xl border border-white/10 rounded-3xl p-4 shadow-[0_16px_48px_rgba(0,0,0,0.5)]">
          <div className="relative w-full aspect-video bg-black/40 rounded-2xl overflow-hidden shadow-inner ring-1 ring-white/5">
            <Image
              src={`${MINIO_BASE_URL}/${activeEvent.imageKey}`}
              alt="Doorbell snapshot"
              fill
              className="object-contain"
              unoptimized
            />
          </div>
          
          <div className="mt-6 px-4 pb-2 flex justify-between items-end">
            <div>
              <p className="text-sm font-semibold text-indigo-300 uppercase tracking-wider mb-1">Latest Event</p>
              <h2 className="text-3xl font-bold text-white tracking-tight">{activeEvent.eventType.replace('_', ' ')}</h2>
            </div>
            <div className="text-right">
              <p className="text-xl font-medium text-slate-200">
                {new Date(activeEvent.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' })}
              </p>
              <p className="text-sm text-slate-400">
                {new Date(activeEvent.timestamp).toLocaleDateString(undefined, { weekday: 'long', month: 'short', day: 'numeric' })}
              </p>
            </div>
          </div>
        </div>
      </div>

      {/* History Carousel */}
      <div className="relative z-10 w-full max-w-5xl mx-auto">
        <h3 className="text-xs font-bold text-slate-400 uppercase tracking-widest mb-4 ml-2">Recent History</h3>
        <div className="flex gap-4 overflow-x-auto pb-4 pt-2 px-2 snap-x scrollbar-hide" style={{ scrollbarWidth: 'none', msOverflowStyle: 'none' }}>
          {events.map((evt) => (
            <button
              key={evt.id}
              onClick={() => setActiveEvent(evt)}
              className={`snap-start relative flex-shrink-0 w-56 aspect-video rounded-2xl overflow-hidden transition-all duration-300 ease-out focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2 focus:ring-offset-slate-900 ${
                activeEvent.id === evt.id 
                  ? 'ring-2 ring-indigo-400 shadow-[0_0_20px_rgba(99,102,241,0.5)] scale-100 opacity-100 z-10' 
                  : 'ring-1 ring-white/10 opacity-60 hover:opacity-100 hover:scale-[1.02] hover:shadow-lg'
              }`}
            >
              <Image
                src={`${MINIO_BASE_URL}/${evt.imageKey}`}
                alt="Thumbnail"
                fill
                className="object-cover"
                unoptimized
              />
              <div className="absolute inset-0 bg-gradient-to-t from-black/90 via-black/20 to-transparent"></div>
              <div className="absolute bottom-3 left-3 flex flex-col items-start">
                <span className="text-xs font-bold text-white">{evt.eventType.replace('_', ' ')}</span>
                <span className="text-[10px] font-medium text-slate-300">{new Date(evt.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}</span>
              </div>
            </button>
          ))}
        </div>
      </div>
    </main>
  );
}
