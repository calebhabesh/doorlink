'use client';

import { useEffect, useMemo, useState } from 'react';
import Image from 'next/image';
import { Camera, Clock3, MessageSquareText, Radio, Volume2 } from 'lucide-react';
import MainLayout, { ConnectionStatus } from '../components/MainLayout';
import PttButton from '../components/PttButton';
import ClockGlobeCard from '../components/ClockGlobeCard';
import ShipmentsCard from '../components/ShipmentsCard';
import QuickResponsesCard from '../components/QuickResponsesCard';
import { upsertSession, VisitorSession } from '../lib/visitorSessions';

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
  const [sessions, setSessions] = useState<VisitorSession[]>([]);
  const [activeSessionId, setActiveSessionId] = useState<string | null>(null);
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('connecting');
  const [latestLiveSessionId, setLatestLiveSessionId] = useState<string | null>(null);
  const [isImageLoaded, setIsImageLoaded] = useState(false);
  const [now, setNow] = useState(Date.now());

  const activeSession = sessions.find((session) => session.sessionId === activeSessionId)
    ?? sessions[0] ?? null;

  useEffect(() => {
    const timer = window.setInterval(() => setNow(Date.now()), 1000);
    return () => window.clearInterval(timer);
  }, []);

  useEffect(() => setIsImageLoaded(false), [activeSession?.latestImageKey]);

  useEffect(() => {
    fetch(`${API_BASE_URL}/sessions?size=100`)
      .then((response) => {
        if (!response.ok) throw new Error(`Session history returned ${response.status}`);
        return response.json();
      })
      .then((data: VisitorSession[]) => {
        setSessions(data);
        setActiveSessionId((current) => current ?? data[0]?.sessionId ?? null);
      })
      .catch((error) => console.error('Failed to fetch visitor sessions', error));

    const eventSource = new EventSource('/stream');
    eventSource.onopen = () => setConnectionStatus('connected');
    eventSource.onerror = () => setConnectionStatus('connecting');
    eventSource.addEventListener('init', () => setConnectionStatus('connected'));
    eventSource.addEventListener('session-update', (event) => {
      setConnectionStatus('connected');
      try {
        const session: VisitorSession = JSON.parse(event.data);
        setSessions((current) => upsertSession(current, session));
        setActiveSessionId(session.sessionId);
        setLatestLiveSessionId(session.sessionId);
      } catch (error) {
        console.error('Failed to parse session update', error);
      }
    });
    return () => eventSource.close();
  }, []);

  const timeline = useMemo(() => {
    if (!activeSession) return [];
    return [
      ...activeSession.presses.map((press) => ({ kind: 'press' as const, at: press.pressedAt, press })),
      ...activeSession.replies.map((reply) => ({ kind: 'reply' as const, at: reply.createdAt, reply })),
    ].sort((left, right) => Date.parse(left.at) - Date.parse(right.at));
  }, [activeSession]);

  const isMostRecent = activeSession?.sessionId === sessions[0]?.sessionId;
  const canReply = Boolean(activeSession && isMostRecent && sessionIsActive(activeSession, now));
  const latestPressId = activeSession?.presses.at(-1)?.id ?? null;

  return (
    <MainLayout status={connectionStatus} breadcrumbs={[{ label: 'Dashboard' }, { label: 'Active Event', active: true }]}>
      {!activeSession ? (
        <div className="flex flex-1 items-center justify-center">
          <p className="font-mono text-sm uppercase tracking-widest text-zinc-500">Awaiting first visitor session…</p>
        </div>
      ) : (
        <div className="mx-auto grid w-full max-w-[1450px] grid-cols-1 items-start gap-6 lg:grid-cols-12">
          <div className="block lg:hidden"><ClockGlobeCard /></div>

          <section className="flex min-w-0 flex-col gap-6 lg:col-span-8">
            <article className="overflow-hidden rounded-3xl border border-zinc-800 bg-zinc-950 shadow-2xl">
              <div className="relative aspect-[4/3] w-full overflow-hidden border-b border-zinc-800 bg-zinc-900">
                {activeSession.latestImageKey ? (
                  <>
                    {!isImageLoaded && <div className="absolute inset-0 grid place-items-center text-zinc-600"><Camera className="h-9 w-9 animate-pulse" /></div>}
                    <Image
                      src={`${MEDIA_BASE_URL}/${activeSession.latestImageKey}`}
                      alt="Latest snapshot from this visitor session"
                      fill
                      unoptimized
                      onLoad={() => setIsImageLoaded(true)}
                      className={`object-cover transition duration-500 ${isImageLoaded ? 'opacity-100' : 'opacity-0'}`}
                    />
                  </>
                ) : (
                  <div className="absolute inset-0 grid place-items-center text-center text-zinc-500">
                    <div><Camera className="mx-auto mb-3 h-9 w-9" /><p className="font-mono text-xs uppercase tracking-widest">Snapshot pending</p></div>
                  </div>
                )}
                <div className={`absolute right-5 top-5 flex items-center gap-2 rounded-full border px-4 py-2 text-[10px] font-black uppercase tracking-[0.18em] backdrop-blur ${sessionIsActive(activeSession, now) ? 'border-emerald-500/40 bg-emerald-500/20 text-emerald-300' : 'border-zinc-600 bg-zinc-900/70 text-zinc-300'}`}>
                  {sessionIsActive(activeSession, now) && <span className="h-2 w-2 animate-pulse rounded-full bg-emerald-400" />}
                  {remainingLabel(activeSession, now)}
                </div>
              </div>

              <div className="flex flex-col gap-5 bg-zinc-900/40 p-5 sm:p-8">
                <div className="flex flex-col justify-between gap-4 sm:flex-row sm:items-center">
                  <div>
                    <p className="mb-2 text-[10px] font-black uppercase tracking-[0.22em] text-emerald-400">Visitor session</p>
                    <h1 className="text-2xl font-black tracking-tight text-zinc-100 sm:text-3xl">
                      {activeSession.pressCount} {activeSession.pressCount === 1 ? 'press' : 'presses'} · {activeSession.recordingCount} {activeSession.recordingCount === 1 ? 'message' : 'messages'}
                    </h1>
                    <p className="mt-2 font-mono text-xs uppercase tracking-wider text-zinc-500">
                      {new Date(activeSession.startedAt).toLocaleString()}
                    </p>
                  </div>
                  {canReply && <PttButton sessionId={activeSession.sessionId} eventId={latestPressId} />}
                </div>

                <div className="border-t border-zinc-800 pt-5">
                  <div className="mb-4 flex items-center gap-2 text-xs font-black uppercase tracking-[0.2em] text-zinc-400">
                    <MessageSquareText className="h-4 w-4 text-emerald-400" /> Conversation timeline
                  </div>
                  <div className="space-y-3">
                    {timeline.map((item) => item.kind === 'press' ? (
                      <div key={`press-${item.press.id}`} className="rounded-2xl border border-zinc-800 bg-zinc-950/70 p-4">
                        <div className="flex flex-wrap items-center justify-between gap-2">
                          <div className="flex items-center gap-3">
                            <span className="grid h-8 w-8 place-items-center rounded-full bg-emerald-500/10 text-xs font-black text-emerald-400">{item.press.pressNumber}</span>
                            <div>
                              <p className="text-sm font-bold text-zinc-200">Doorbell press</p>
                              <p className="font-mono text-[10px] uppercase tracking-wider text-zinc-500">{new Date(item.press.pressedAt).toLocaleTimeString()}</p>
                            </div>
                          </div>
                          <span className="rounded-full bg-zinc-800 px-3 py-1 text-[10px] font-bold uppercase tracking-wider text-zinc-400">
                            {item.press.recordings.length ? `Held ${(item.press.durationMs! / 1000).toFixed(1)}s` : item.press.state === 'PHOTO_PENDING' ? 'Media pending' : 'Short press'}
                          </span>
                        </div>
                        {item.press.recordings.map((recording) => (
                          <div key={recording.recordingId} className="mt-4 flex items-center gap-3 border-t border-zinc-800 pt-4">
                            <Volume2 className="h-4 w-4 shrink-0 text-emerald-400" />
                            <audio controls preload="none" src={`${MEDIA_BASE_URL}/${recording.audioKey}`} className="h-9 w-full" />
                          </div>
                        ))}
                      </div>
                    ) : (
                      <div key={`reply-${item.reply.messageId}`} className="ml-6 rounded-2xl border border-sky-500/20 bg-sky-500/[0.06] p-4">
                        <div className="mb-3 flex items-center justify-between gap-2">
                          <span className="flex items-center gap-2 text-sm font-bold text-sky-300"><Radio className="h-4 w-4" /> Homeowner reply</span>
                          <span className="text-[10px] uppercase tracking-wider text-zinc-500">{item.reply.deliveredAt ? 'Played at door' : 'Playback queued'}</span>
                        </div>
                        <audio controls preload="none" src={`${MEDIA_BASE_URL}/${item.reply.audioKey}`} className="h-9 w-full" />
                      </div>
                    ))}
                  </div>
                </div>
              </div>
            </article>
            {canReply && <QuickResponsesCard />}
          </section>

          <aside className="flex min-w-0 flex-col gap-6 lg:col-span-4">
            <div className="hidden lg:block"><ClockGlobeCard /></div>
            <div className="rounded-3xl border border-zinc-800 bg-zinc-950/60 p-6 shadow-lg">
              <div className="mb-4 flex items-center gap-3 border-b border-zinc-800 pb-4 text-xs font-black uppercase tracking-[0.2em] text-zinc-500"><Clock3 className="h-4 w-4 text-emerald-400" /> Recent sessions</div>
              <div className="max-h-[420px] space-y-3 overflow-y-auto pr-1">
                {sessions.map((session) => (
                  <button key={session.sessionId} onClick={() => setActiveSessionId(session.sessionId)} className={`w-full rounded-2xl border p-4 text-left transition ${activeSession.sessionId === session.sessionId ? 'border-emerald-500/50 bg-emerald-500/[0.05]' : 'border-zinc-800 bg-zinc-950 hover:border-zinc-700'} ${latestLiveSessionId === session.sessionId ? 'animate-slide-in' : ''}`}>
                    <div className="flex items-center justify-between gap-3">
                      <span className="font-bold text-zinc-200">{session.pressCount} {session.pressCount === 1 ? 'press' : 'presses'}</span>
                      <span className="text-[10px] font-bold uppercase tracking-widest text-zinc-500">{session.recordingCount} audio</span>
                    </div>
                    <p className="mt-2 font-mono text-[10px] uppercase tracking-wider text-zinc-500">{new Date(session.startedAt).toLocaleString()}</p>
                  </button>
                ))}
              </div>
            </div>
            <div className="hidden md:block"><ShipmentsCard /></div>
          </aside>
        </div>
      )}
    </MainLayout>
  );
}
