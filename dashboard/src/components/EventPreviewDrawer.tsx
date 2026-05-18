'use client';

import { useEffect, useState } from 'react';
import Image from 'next/image';
import { X } from 'lucide-react';

interface DoorbellEvent {
  id: number;
  timestamp: string;
  eventType: string;
  imageKey: string;
  audioKey?: string | null;
}

interface EventPreviewDrawerProps {
  event: DoorbellEvent | null;
  onClose: () => void;
}

const MINIO_BASE_URL = `/api/events/media`;

export default function EventPreviewDrawer({ event, onClose }: EventPreviewDrawerProps) {
  const [isImageLoaded, setIsImageLoaded] = useState(false);

  useEffect(() => {
    setIsImageLoaded(false);
  }, [event?.id]);

  useEffect(() => {
    const handleEscape = (e: KeyboardEvent) => {
      if (e.key === 'Escape') onClose();
    };
    if (event) {
      document.body.style.overflow = 'hidden';
      window.addEventListener('keydown', handleEscape);
    } else {
      document.body.style.overflow = '';
    }
    return () => {
      document.body.style.overflow = '';
      window.removeEventListener('keydown', handleEscape);
    };
  }, [event, onClose]);

  if (!event) return null;

  const formatTitleCase = (str: string) => {
    return str.toLowerCase().split('_').map(word => word.charAt(0).toUpperCase() + word.slice(1)).join(' ');
  };

  return (
    <div 
      className="fixed inset-0 z-50 flex justify-end" 
      role="dialog" 
      aria-modal="true"
    >
      {/* Backdrop */}
      <div 
        className="absolute inset-0 bg-black/60 backdrop-blur-sm transition-opacity"
        onClick={onClose}
      />

      {/* Drawer */}
      <div className="relative w-full max-w-xl h-full bg-zinc-950 border-l border-zinc-800 shadow-2xl flex flex-col overflow-y-auto transform transition-transform">
        
        {/* Header */}
        <div className="p-4 sm:p-6 flex items-center justify-between border-b border-zinc-800 bg-zinc-900/50 sticky top-0 z-10 backdrop-blur-md">
          <h2 className="text-xl font-black text-zinc-100 tracking-tight">Event Preview</h2>
          <button 
            onClick={onClose}
            className="p-2 bg-zinc-800 hover:bg-zinc-700 rounded-full text-zinc-400 hover:text-white transition-colors"
            aria-label="Close preview"
          >
            <X className="w-5 h-5" />
          </button>
        </div>

        {/* Content */}
        <div className="flex-1 flex flex-col p-4 sm:p-6 gap-6">
          
          {/* Image */}
          <div className="w-full aspect-[4/3] bg-zinc-900 rounded-2xl border border-zinc-800 relative overflow-hidden flex items-center justify-center">
            {!isImageLoaded && (
              <div className="absolute inset-0 flex items-center justify-center">
                <svg className="animate-spin h-8 w-8 text-zinc-700" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                  <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                  <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                </svg>
              </div>
            )}
            <Image 
              src={`${MINIO_BASE_URL}/${event.imageKey}`} 
              alt="Doorbell snapshot" 
              fill 
              className={`object-cover w-full transition-all duration-700 ease-out ${isImageLoaded ? 'opacity-100 blur-0' : 'opacity-0 blur-sm scale-105'}`} 
              onLoad={() => setIsImageLoaded(true)}
              unoptimized 
            />
          </div>

          {/* Metadata */}
          <div className="bg-zinc-900/50 rounded-2xl border border-zinc-800 p-5 space-y-4">
            <div>
              <p className="text-xs text-zinc-500 font-mono uppercase tracking-widest mb-1">Event Type</p>
              <p className="text-lg font-bold text-emerald-400">{formatTitleCase(event.eventType)}</p>
            </div>
            <div>
              <p className="text-xs text-zinc-500 font-mono uppercase tracking-widest mb-1">Timestamp</p>
              <p className="text-base text-zinc-300">
                {new Date(event.timestamp).toLocaleString(undefined, {
                  weekday: 'short', year: 'numeric', month: 'short', day: 'numeric',
                  hour: 'numeric', minute: '2-digit', second: '2-digit'
                })}
              </p>
            </div>
            <div>
              <p className="text-xs text-zinc-500 font-mono uppercase tracking-widest mb-1">Event ID</p>
              <p className="text-sm font-mono text-zinc-400">{event.id}</p>
            </div>
            <div>
              <p className="text-xs text-zinc-500 font-mono uppercase tracking-widest mb-1">Image Key</p>
              <p className="text-sm font-mono text-zinc-400 truncate" title={event.imageKey}>{event.imageKey}</p>
            </div>
            {event.audioKey && (
              <div>
                <p className="text-xs text-zinc-500 font-mono uppercase tracking-widest mb-1">Audio Key</p>
                <p className="text-sm font-mono text-zinc-400 truncate" title={event.audioKey}>{event.audioKey}</p>
              </div>
            )}
          </div>

          {/* Audio Playback */}
          {event.audioKey ? (
            <div className="bg-zinc-900/50 rounded-2xl border border-zinc-800 p-5">
              <p className="text-xs text-zinc-500 font-mono uppercase tracking-widest mb-3">Recorded Audio</p>
              <audio 
                controls 
                src={`${MINIO_BASE_URL}/${event.audioKey}`} 
                className="w-full h-10 rounded-lg outline-none" 
              />
            </div>
          ) : (
            <div className="bg-zinc-900/30 rounded-2xl border border-zinc-800/50 p-5 flex items-center justify-center">
              <p className="text-sm text-zinc-500 font-medium">No audio recorded for this event</p>
            </div>
          )}

          {/* Actions */}
          <div className="pt-4 flex gap-3 pb-8">
            <a 
              href={`${MINIO_BASE_URL}/${event.imageKey}`} 
              target="_blank" 
              rel="noopener noreferrer"
              className="flex-1 text-center bg-zinc-800 hover:bg-zinc-700 text-zinc-200 px-4 py-3 rounded-xl text-sm font-bold uppercase tracking-widest transition-colors border border-zinc-700"
            >
              Open Image Raw
            </a>
          </div>

        </div>
      </div>
    </div>
  );
}
