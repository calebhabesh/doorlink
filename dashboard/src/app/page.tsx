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
  const MINIO_BASE_URL = 'http://localhost:9000';

  useEffect(() => {
    // Fetch initial history
    fetch(API_BASE_URL)
      .then((res) => res.json())
      .then((data: DoorbellEvent[]) => {
        setEvents(data);
        if (data.length > 0) {
          setActiveEvent(data[0]);
        }
      })
      .catch((err) => console.error("Failed to fetch history", err));

    // Connect to SSE Stream
    const eventSource = new EventSource(`${API_BASE_URL}/stream`);

    eventSource.onopen = () => setIsConnected(true);
    eventSource.onerror = () => setIsConnected(false);

    eventSource.addEventListener('doorbell-event', (e) => {
      try {
        const newEvent: DoorbellEvent = JSON.parse(e.data);
        setEvents((prev) => [newEvent, ...prev]);
        setActiveEvent(newEvent); // Auto-focus the new event
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
      <div className="min-h-screen bg-neutral-900 text-white flex items-center justify-center">
        <p className="text-xl text-neutral-400">Waiting for events...</p>
      </div>
    );
  }

  return (
    <main className="min-h-screen bg-neutral-900 text-white flex flex-col">
      {/* Header */}
      <header className="p-4 border-b border-neutral-800 flex justify-between items-center bg-neutral-950">
        <h1 className="text-xl font-semibold">Smart Doorbell Dashboard</h1>
        <div className="flex items-center gap-2">
          <div className={`w-3 h-3 rounded-full ${isConnected ? 'bg-green-500' : 'bg-red-500'}`}></div>
          <span className="text-sm text-neutral-400">{isConnected ? 'Live' : 'Disconnected'}</span>
        </div>
      </header>

      {/* Main Focus Area */}
      <div className="flex-1 flex flex-col items-center justify-center p-8">
        <div className="relative w-full max-w-4xl aspect-video bg-black rounded-lg overflow-hidden border border-neutral-800 shadow-2xl">
          <Image
            src={`${MINIO_BASE_URL}/${activeEvent.imageKey}`}
            alt="Doorbell snapshot"
            fill
            className="object-contain"
            unoptimized // Bypass next/image optimization for local dev to avoid setup issues
          />
        </div>
        <div className="mt-6 text-center">
          <h2 className="text-3xl font-bold text-neutral-100">{activeEvent.eventType}</h2>
          <p className="text-lg text-neutral-400 mt-2">
            {new Date(activeEvent.timestamp).toLocaleString()}
          </p>
        </div>
      </div>

      {/* Recent History Footer */}
      <div className="h-48 border-t border-neutral-800 bg-neutral-950 p-4 overflow-x-auto">
        <h3 className="text-sm font-semibold text-neutral-500 uppercase mb-3">Recent Events</h3>
        <div className="flex gap-4">
          {events.map((evt) => (
            <button
              key={evt.id}
              onClick={() => setActiveEvent(evt)}
              className={`relative flex-shrink-0 w-48 aspect-video rounded-md overflow-hidden border-2 transition-all ${
                activeEvent.id === evt.id ? 'border-blue-500 scale-105' : 'border-transparent opacity-60 hover:opacity-100'
              }`}
            >
              <Image
                src={`${MINIO_BASE_URL}/${evt.imageKey}`}
                alt="Thumbnail"
                fill
                className="object-cover"
                unoptimized
              />
              <div className="absolute bottom-0 inset-x-0 bg-gradient-to-t from-black/80 to-transparent p-2 text-xs text-left">
                {new Date(evt.timestamp).toLocaleTimeString()}
              </div>
            </button>
          ))}
        </div>
      </div>
    </main>
  );
}
