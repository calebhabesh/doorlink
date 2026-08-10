'use client';

import { useEffect, useState } from 'react';
import Image from 'next/image';
import { X } from 'lucide-react';
import { getSessionCoverImageKey, VisitorSession } from '../lib/visitorSessions';
import SessionTimeline from './SessionTimeline';

interface EventPreviewDrawerProps {
  session: VisitorSession | null;
  onClose: () => void;
}

const MEDIA_BASE_URL = '/api/events/media';

export default function EventPreviewDrawer({ session, onClose }: EventPreviewDrawerProps) {
  const [isImageLoaded, setIsImageLoaded] = useState(false);
  const coverImageKey = session ? getSessionCoverImageKey(session) : null;

  useEffect(() => setIsImageLoaded(false), [coverImageKey]);
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
            <p className="text-[10px] font-black tracking-[0.22em] text-emerald-400">Visitor Session</p>
            <h2 className="mt-2 text-2xl font-black text-zinc-100">{session.pressCount} {session.pressCount === 1 ? 'Press' : 'Presses'} · {session.recordingCount} {session.recordingCount === 1 ? 'Message' : 'Messages'}</h2>
            <p className="mt-2 font-mono text-xs uppercase text-zinc-500">{new Date(session.startedAt).toLocaleString()}</p>
          </div>
          <button type="button" onClick={onClose} aria-label="Close preview" className="rounded-xl border border-zinc-800 bg-zinc-900 p-3 text-zinc-400 hover:text-white"><X className="h-5 w-5" /></button>
        </div>

        {coverImageKey && (
          <div className="relative mb-6 aspect-[4/3] overflow-hidden rounded-2xl border border-zinc-800 bg-zinc-900">
            <Image src={`${MEDIA_BASE_URL}/${coverImageKey}`} alt="First visitor session snapshot" fill unoptimized onLoad={() => setIsImageLoaded(true)} className={`object-cover transition ${isImageLoaded ? 'opacity-100' : 'opacity-0'}`} />
          </div>
        )}

        <SessionTimeline session={session} />
      </aside>
    </div>
  );
}
