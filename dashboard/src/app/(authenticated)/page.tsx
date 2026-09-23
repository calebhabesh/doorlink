'use client';

import { useEffect, useRef, useState } from 'react';
import Image from 'next/image';
import { Camera, Clock3, ImageIcon, MessageSquareText, Mic2, Radio } from 'lucide-react';
import PttButton from '../../components/PttButton';
import ClockGlobeCard from '../../components/ClockGlobeCard';
import SessionTimeline from '../../components/SessionTimeline';
import {
  getSessionCoverImageKey,
  getSessionImages,
  isImageKeyCached,
  markImageKeyCached,
  VisitorSession,
} from '../../lib/visitorSessions';
import { useSessionContext } from '../../context/SessionContext';

const API_BASE_URL = '/api/events';
const MEDIA_BASE_URL = `${API_BASE_URL}/media`;

function sessionIsActive(session: VisitorSession, now: number) {
  return session.status === 'ACTIVE' && Date.parse(session.endsAt) > now;
}

function remainingLabel(session: VisitorSession, now: number) {
  const seconds = Math.max(0, Math.ceil((Date.parse(session.endsAt) - now) / 1000));
  return seconds > 0 ? `${seconds}s reply window` : 'Session ended';
}

export default function Home() {
  const {
    sessions,
    activeSessionId,
    setActiveSessionId,
    isLoadingSessions,
    sessionError,
  } = useSessionContext();

  const activeSession = sessions.find((session) => session.sessionId === activeSessionId)
    ?? sessions[0] ?? null;
  const activeCoverImageKey = activeSession ? getSessionCoverImageKey(activeSession) : null;

  const [isImageLoaded, setIsImageLoaded] = useState(() => isImageKeyCached(activeCoverImageKey));
  const [now, setNow] = useState(Date.now());
  const [leftCardHeight, setLeftCardHeight] = useState<number | undefined>();
  const visitorCardRef = useRef<HTMLElement | null>(null);

  useEffect(() => {
    const timer = window.setInterval(() => setNow(Date.now()), 1000);
    return () => window.clearInterval(timer);
  }, []);

  useEffect(() => {
    setIsImageLoaded(isImageKeyCached(activeCoverImageKey));
  }, [activeCoverImageKey]);

  // Lock the desktop right-hand column height strictly to the left visitor container height
  useEffect(() => {
    const updateHeight = () => {
      if (visitorCardRef.current && window.innerWidth >= 1024) {
        setLeftCardHeight(visitorCardRef.current.offsetHeight);
      } else {
        setLeftCardHeight(undefined);
      }
    };

    updateHeight();
    const observer = new ResizeObserver(updateHeight);
    if (visitorCardRef.current) observer.observe(visitorCardRef.current);
    window.addEventListener('resize', updateHeight);

    return () => {
      observer.disconnect();
      window.removeEventListener('resize', updateHeight);
    };
  }, [activeSession, isImageLoaded]);

  const isMostRecent = activeSession?.sessionId === sessions[0]?.sessionId;
  const canReply = Boolean(activeSession && isMostRecent && sessionIsActive(activeSession, now));
  const latestPressId = activeSession?.presses.at(-1)?.id ?? null;

  return (
    <div className="mx-auto grid w-full max-w-[1450px] grid-cols-1 items-start gap-6 lg:grid-cols-12 flex-1">
      {/* Mobile Clock & Globe */}
      <div className="block lg:hidden shrink-0">
        <ClockGlobeCard />
      </div>

      {/* Main Visitor Card (Left Column) */}
      <section ref={visitorCardRef} className="flex min-w-0 flex-col gap-6 lg:col-span-8">
        {activeSession ? (
          <article className="overflow-hidden rounded-3xl border border-zinc-800 bg-zinc-950 shadow-2xl flex flex-col">
            <div className="relative aspect-[4/3] w-full overflow-hidden border-b border-zinc-800 bg-zinc-900 shrink-0">
              {activeCoverImageKey ? (
                <>
                  {!isImageLoaded && (
                    <div className="absolute inset-0 grid place-items-center text-zinc-600">
                      <Camera className="h-9 w-9 animate-pulse motion-reduce:animate-none" />
                    </div>
                  )}
                  <Image
                    src={`${MEDIA_BASE_URL}/${activeCoverImageKey}`}
                    alt="First snapshot from this visitor session"
                    fill
                    unoptimized
                    priority
                    onLoad={() => {
                      markImageKeyCached(activeCoverImageKey);
                      setIsImageLoaded(true);
                    }}
                    className={`object-cover transition-opacity duration-300 ${isImageLoaded ? 'opacity-100' : 'opacity-0'}`}
                  />
                </>
              ) : (
                <div className="absolute inset-0 grid place-items-center text-center text-zinc-500">
                  <div>
                    <Camera className="mx-auto mb-3 h-9 w-9 text-zinc-500 animate-pulse motion-reduce:animate-none" />
                    <p className="font-mono text-xs uppercase tracking-widest text-zinc-400">Snapshot pending</p>
                  </div>
                </div>
              )}
              <div
                className={`absolute right-5 top-5 flex items-center gap-2 rounded-full border px-4 py-2 text-[10px] font-black uppercase tracking-[0.18em] backdrop-blur ${
                  sessionIsActive(activeSession, now)
                    ? 'border-emerald-500/40 bg-emerald-500/20 text-emerald-300'
                    : 'border-zinc-600 bg-zinc-900/70 text-zinc-300'
                }`}
              >
                {sessionIsActive(activeSession, now) && (
                  <span className="h-2 w-2 animate-pulse motion-reduce:animate-none rounded-full bg-emerald-400" />
                )}
                {remainingLabel(activeSession, now)}
              </div>
            </div>

            <div className="flex flex-col gap-5 bg-zinc-900/40 p-5 sm:p-8 flex-1">
              <div className="flex flex-col justify-between gap-4 sm:flex-row sm:items-center">
                <div>
                  <p className="mb-2 text-[10px] font-black tracking-[0.22em] text-emerald-400 uppercase">
                    Visitor Session
                  </p>
                  {/* Semantic h2 for session summary, matching hierarchy decision */}
                  <h2 className="text-2xl font-black tracking-tight text-zinc-100 sm:text-3xl">
                    {activeSession.pressCount} {activeSession.pressCount === 1 ? 'Press' : 'Presses'} · {activeSession.recordingCount} {activeSession.recordingCount === 1 ? 'Message' : 'Messages'}
                  </h2>
                  <p className="mt-2 font-mono text-xs uppercase tracking-wider text-zinc-500">
                    {new Date(activeSession.startedAt).toLocaleString()}
                  </p>
                </div>
                {canReply && <PttButton sessionId={activeSession.sessionId} eventId={latestPressId} />}
              </div>

              <div className="border-t border-zinc-800 pt-5">
                <div className="mb-4 flex items-center gap-2 text-xs font-black uppercase tracking-[0.2em] text-zinc-400">
                  <MessageSquareText className="h-4 w-4 text-emerald-400" /> Conversation Timeline
                </div>
                <SessionTimeline session={activeSession} />
              </div>
            </div>
          </article>
        ) : (
          <article className="overflow-hidden rounded-3xl border border-zinc-800 bg-zinc-950 shadow-2xl flex flex-col">
            <div className="relative aspect-[4/3] w-full overflow-hidden border-b border-zinc-800 bg-zinc-900 shrink-0">
              <div className="absolute inset-0 grid place-items-center px-6 text-center">
                <div>
                  <div className="mx-auto mb-5 grid h-16 w-16 place-items-center rounded-2xl border border-zinc-800 bg-zinc-950/70 text-zinc-500 shadow-lg">
                    <Camera className="h-8 w-8" />
                  </div>
                  <p className="text-lg font-bold text-zinc-200">
                    {isLoadingSessions
                      ? 'Loading recent sessions'
                      : sessionError
                        ? 'Session history unavailable'
                        : 'No active event'}
                  </p>
                  <p className="mx-auto mt-2 max-w-md font-mono text-xs uppercase leading-6 tracking-wider text-zinc-500">
                    {sessionError
                      ? 'Unable to reach the gateway. Retrying automatically.'
                      : 'The next doorbell press will appear here automatically.'}
                  </p>
                </div>
              </div>
              <div className="absolute right-5 top-5 rounded-full border border-zinc-700 bg-zinc-950/70 px-4 py-2 text-[10px] font-black uppercase tracking-[0.18em] text-zinc-400 backdrop-blur">
                Standby
              </div>
            </div>

            <div className="bg-zinc-900/40 p-5 sm:p-8 flex-1">
              <p className="mb-2 text-[10px] font-black tracking-[0.22em] text-emerald-400 uppercase">
                Visitor Session
              </p>
              <h2 className="text-2xl font-black tracking-tight text-zinc-100 sm:text-3xl">
                {sessionError ? 'Reconnecting to Doorlink' : 'Waiting for a visitor'}
              </h2>
              <p className="mt-3 max-w-xl text-sm leading-6 text-zinc-500">
                {!isLoadingSessions && !sessionError
                  ? 'There are no saved sessions yet. The dashboard will update when a new event arrives.'
                  : 'Saved sessions will appear here as soon as the gateway connection is restored.'}
              </p>
            </div>
          </article>
        )}
      </section>

      {/* Right Column: Clock & Recent Sessions locked to left card height */}
      <aside
        style={leftCardHeight ? { height: `${leftCardHeight}px`, maxHeight: `${leftCardHeight}px` } : undefined}
        className="flex min-w-0 flex-col gap-6 lg:col-span-4 h-auto lg:h-full overflow-hidden"
      >
        {/* Desktop Clock */}
        <div className="hidden lg:block shrink-0">
          <ClockGlobeCard />
        </div>

        {/* Recent Sessions Card: Strictly consumes space until left card bottom edge, scrolls internally */}
        <div className="rounded-3xl border border-zinc-800 bg-zinc-950 p-6 shadow-lg flex flex-col flex-1 min-h-0 overflow-hidden">
          {/* Unified uppercase section heading */}
          <div className="mb-4 flex items-center gap-2 border-b border-zinc-800 pb-4 text-xs font-black uppercase tracking-[0.2em] text-zinc-400 shrink-0">
            <Clock3 className="h-4 w-4 text-emerald-400" /> Recent Sessions
          </div>

          {/* Internally scrolling sessions list */}
          <div className="flex-1 min-h-0 overflow-y-auto space-y-3 pr-1">
            {sessions.length > 0 ? (
              sessions.map((session) => {
                const coverImageKey = getSessionCoverImageKey(session);
                const imageCount = getSessionImages(session).length;
                const isSelected = activeSession?.sessionId === session.sessionId;

                return (
                  <button
                    key={session.sessionId}
                    type="button"
                    onClick={() => setActiveSessionId(session.sessionId)}
                    className={`group relative w-full overflow-hidden rounded-2xl border p-3 text-left transition focus-visible:ring-2 focus-visible:ring-emerald-500 focus-visible:outline-none ${
                      isSelected
                        ? 'border-emerald-500/50 bg-gradient-to-r from-emerald-500/[0.09] to-zinc-950 shadow-[0_0_24px_rgba(16,185,129,0.07)]'
                        : 'border-zinc-800 bg-zinc-950 hover:border-zinc-700 hover:bg-zinc-900/70'
                    }`}
                  >
                    {isSelected && (
                      <span className="absolute inset-y-3 left-0 w-0.5 rounded-full bg-emerald-400" />
                    )}
                    <div className="flex items-center gap-3">
                      <div className="relative h-14 w-16 shrink-0 overflow-hidden rounded-xl border border-zinc-800 bg-zinc-900">
                        {coverImageKey ? (
                          <Image
                            src={`${MEDIA_BASE_URL}/${coverImageKey}`}
                            alt="Visitor session cover"
                            fill
                            unoptimized
                            onLoad={() => markImageKeyCached(coverImageKey)}
                            className="object-cover transition duration-300 group-hover:scale-105"
                          />
                        ) : (
                          <div className="grid h-full place-items-center">
                            <Camera className="h-5 w-5 text-zinc-600" />
                          </div>
                        )}
                        {session.status === 'ACTIVE' && (
                          <span className="absolute right-1.5 top-1.5 h-2 w-2 rounded-full bg-emerald-400 shadow-[0_0_0_3px_rgba(16,185,129,0.2)]" />
                        )}
                      </div>
                      <div className="min-w-0 flex-1">
                        <div className="flex items-center justify-between gap-2">
                          <span className="truncate font-bold text-zinc-100">
                            {session.pressCount} {session.pressCount === 1 ? 'Press' : 'Presses'}
                          </span>
                          <span className="shrink-0 font-mono text-[9px] text-zinc-500">
                            {new Date(session.startedAt).toLocaleTimeString([], { hour: 'numeric', minute: '2-digit' })}
                          </span>
                        </div>
                        <p className="mt-1 truncate font-mono text-[9px] text-zinc-600">
                          {new Date(session.startedAt).toLocaleDateString()}
                        </p>
                        <div className="mt-2 flex items-center gap-3 text-[10px] font-semibold text-zinc-400">
                          <span className="flex items-center gap-1">
                            <Mic2 className="h-3 w-3 text-emerald-400" /> {session.recordingCount} {session.recordingCount === 1 ? 'Message' : 'Messages'}
                          </span>
                          <span className="flex items-center gap-1">
                            <Radio className="h-3 w-3 text-sky-400" /> {session.replies.length} {session.replies.length === 1 ? 'Reply' : 'Replies'}
                          </span>
                          {imageCount > 1 && (
                            <span className="flex items-center gap-1 text-zinc-400">
                              <ImageIcon className="h-3 w-3" /> {imageCount}
                            </span>
                          )}
                        </div>
                      </div>
                    </div>
                  </button>
                );
              })
            ) : (
              <div className="rounded-2xl border border-dashed border-zinc-800 bg-zinc-950/50 px-5 py-8 text-center">
                <Clock3 className="mx-auto h-6 w-6 text-zinc-600" />
                <p className="mt-3 text-sm font-bold text-zinc-300">
                  {!isLoadingSessions && !sessionError ? 'No session history' : 'Session history unavailable'}
                </p>
                <p className="mt-2 text-xs leading-5 text-zinc-600">
                  {!isLoadingSessions && !sessionError
                    ? 'Saved visitor sessions will be listed here.'
                    : 'Waiting for the gateway connection.'}
                </p>
              </div>
            )}
          </div>
        </div>
      </aside>
    </div>
  );
}
