'use client';

import { useEffect, useState } from 'react';
import Image from 'next/image';
import { X } from 'lucide-react';
import { VisitorSession } from '../lib/visitorSessions';

interface EventPreviewDrawerProps {
  session: VisitorSession | null;
  onClose: () => void;
}

const MEDIA_BASE_URL = '/api/events/media';

export default function EventPreviewDrawer({ session, onClose }: EventPreviewDrawerProps) {
  const [isImageLoaded, setIsImageLoaded] = useState(false);

  useEffect(() => setIsImageLoaded(false), [session?.latestImageKey]);
  useEffect(() => {
    if (!session) return;
    const handleEscape = (event: KeyboardEvent) => event.key === 'Escape' && onClose();
    document.body.style.overflow = 'hidden';
    window.addEventListener('keydown', handleEscape);
    return () => {
      document.body.style.overflow = '';
      window.removeEventListener('keydown', handleEscape);
    };
  }, [session, onClose]);

  if (!session) return null;

  return (
    <div className="fixed inset-0 z-50 flex justify-end bg-black/70 backdrop-blur-sm" onMouseDown={(event) => event.target === event.currentTarget && onClose()}>
      <aside className="h-full w-full max-w-xl overflow-y-auto border-l border-zinc-800 bg-zinc-950 p-6 shadow-2xl sm:p-8">
        <div className="mb-7 flex items-start justify-between gap-4">
          <div>
            <p className="text-[10px] font-black uppercase tracking-[0.22em] text-emerald-400">Visitor session</p>
            <h2 className="mt-2 text-2xl font-black text-zinc-100">{session.pressCount} presses · {session.recordingCount} messages</h2>
            <p className="mt-2 font-mono text-xs uppercase text-zinc-500">{new Date(session.startedAt).toLocaleString()}</p>
          </div>
          <button type="button" onClick={onClose} aria-label="Close preview" className="rounded-xl border border-zinc-800 bg-zinc-900 p-3 text-zinc-400 hover:text-white"><X className="h-5 w-5" /></button>
        </div>

        {session.latestImageKey && (
          <div className="relative mb-6 aspect-[4/3] overflow-hidden rounded-2xl border border-zinc-800 bg-zinc-900">
            <Image src={`${MEDIA_BASE_URL}/${session.latestImageKey}`} alt="Visitor session snapshot" fill unoptimized onLoad={() => setIsImageLoaded(true)} className={`object-cover transition ${isImageLoaded ? 'opacity-100' : 'opacity-0'}`} />
          </div>
        )}

        <div className="space-y-3">
          {session.presses.map((press) => (
            <div key={press.id} className="rounded-2xl border border-zinc-800 bg-zinc-900/40 p-5">
              <div className="flex items-center justify-between gap-3">
                <span className="font-bold text-zinc-200">Press {press.pressNumber}</span>
                <span className="font-mono text-[10px] uppercase tracking-wider text-zinc-500">{new Date(press.pressedAt).toLocaleTimeString()}</span>
              </div>
              <p className="mt-2 text-xs text-zinc-500">{press.recordings.length ? `Held ${(press.durationMs! / 1000).toFixed(1)} seconds` : press.state === 'PHOTO_PENDING' ? 'Media upload pending' : 'Short press — no message'}</p>
              {press.recordings.map((recording) => <audio key={recording.recordingId} controls preload="none" src={`${MEDIA_BASE_URL}/${recording.audioKey}`} className="mt-4 h-9 w-full" />)}
            </div>
          ))}
          {session.replies.map((reply) => (
            <div key={reply.messageId} className="ml-6 rounded-2xl border border-sky-500/20 bg-sky-500/[0.06] p-5">
              <div className="flex items-center justify-between gap-3"><span className="font-bold text-sky-300">Homeowner reply</span><span className="text-[10px] uppercase text-zinc-500">{reply.deliveredAt ? 'Delivered' : 'Queued'}</span></div>
              <audio controls preload="none" src={`${MEDIA_BASE_URL}/${reply.audioKey}`} className="mt-4 h-9 w-full" />
            </div>
          ))}
        </div>
      </aside>
    </div>
  );
}
