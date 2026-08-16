'use client';

import { useEffect, useMemo, useState } from 'react';
import { CalendarDays, ChevronDown, Download, Eye, Search, X } from 'lucide-react';
import MainLayout, { ConnectionStatus } from '../../components/MainLayout';
import EventPreviewDrawer from '../../components/EventPreviewDrawer';
import { upsertSession, VisitorSession } from '../../lib/visitorSessions';

const API_BASE_URL = '/api/events';

function csvCell(value: string | number) {
  const text = String(value);
  return /[",\n]/.test(text) ? `"${text.replaceAll('"', '""')}"` : text;
}

export default function EventLog() {
  const [sessions, setSessions] = useState<VisitorSession[]>([]);
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('connecting');
  const [latestLiveSessionId, setLatestLiveSessionId] = useState<string | null>(null);
  const [searchQuery, setSearchQuery] = useState('');
  const [filterDate, setFilterDate] = useState('');
  const [selectedSession, setSelectedSession] = useState<VisitorSession | null>(null);
  const [isLoading, setIsLoading] = useState(true);

  useEffect(() => {
    let cancelled = false;
    let historyLoadInFlight = false;
    const loadSessions = async () => {
      if (historyLoadInFlight) return;
      historyLoadInFlight = true;
      try {
        const response = await fetch(`${API_BASE_URL}/sessions?size=1000`, {
          cache: 'no-store',
        });
        if (!response.ok) throw new Error(`Session history returned ${response.status}`);
        const data: VisitorSession[] = await response.json();
        if (!cancelled) {
          setSessions((current) => current.reduce(
            (merged, session) => upsertSession(merged, session),
            data,
          ));
        }
      } catch (error) {
        console.error('Failed to fetch session log', error);
      } finally {
        historyLoadInFlight = false;
        if (!cancelled) setIsLoading(false);
      }
    };

    void loadSessions();

    const eventSource = new EventSource('/stream');
    eventSource.onopen = () => setConnectionStatus('connected');
    eventSource.onerror = () => {
      setConnectionStatus('connecting');
      void loadSessions();
    };
    eventSource.addEventListener('init', () => {
      setConnectionStatus('connected');
      void loadSessions();
    });
    eventSource.addEventListener('session-update', (event) => {
      setConnectionStatus('connected');
      try {
        const session: VisitorSession = JSON.parse(event.data);
        setSessions((current) => upsertSession(current, session));
        setLatestLiveSessionId(session.sessionId);
        setSelectedSession((current) => current?.sessionId === session.sessionId ? session : current);
      } catch (error) {
        console.error('Failed to parse session update', error);
      }
    });
    const reconcileWhenVisible = () => {
      if (document.visibilityState === 'visible') void loadSessions();
    };
    window.addEventListener('focus', reconcileWhenVisible);
    document.addEventListener('visibilitychange', reconcileWhenVisible);
    return () => {
      cancelled = true;
      eventSource.close();
      window.removeEventListener('focus', reconcileWhenVisible);
      document.removeEventListener('visibilitychange', reconcileWhenVisible);
    };
  }, []);

  const filteredSessions = useMemo(() => sessions.filter((session) => {
    const query = searchQuery.trim().toLowerCase();
    const matchesSearch = !query || session.sessionId.toLowerCase().includes(query)
      || session.presses.some((press) => press.eventType.toLowerCase().includes(query));
    if (!filterDate) return matchesSearch;
    return matchesSearch && new Date(session.startedAt).toLocaleDateString('en-CA') === filterDate;
  }), [sessions, searchQuery, filterDate]);

  const exportToCsv = () => {
    const rows = filteredSessions.map((session) => [
      session.sessionId, session.startedAt, session.status, session.pressCount,
      session.recordingCount, session.replies.length, session.latestImageKey ?? '',
    ]);
    const csv = [
      ['Session ID', 'Started At', 'Status', 'Presses', 'Visitor Recordings', 'Homeowner Replies', 'Latest Image'],
      ...rows,
    ].map((row) => row.map(csvCell).join(',')).join('\n');
    const url = URL.createObjectURL(new Blob([csv], { type: 'text/csv;charset=utf-8' }));
    const link = document.createElement('a');
    link.href = url;
    link.download = `doorbell_sessions_${filterDate || 'all'}.csv`;
    link.click();
    URL.revokeObjectURL(url);
  };

  return (
    <MainLayout status={connectionStatus} breadcrumbs={[{ label: 'Dashboard' }, { label: 'Session Log', active: true }]}>
      <div className="mx-auto flex h-full w-full max-w-[1800px] flex-col overflow-hidden">
        <div className="mb-6 flex shrink-0 flex-col items-start justify-between gap-4 lg:mb-10 lg:flex-row lg:items-center">
          <div className="relative w-full lg:w-96">
            <Search className="absolute left-4 top-1/2 h-4 w-5 -translate-y-1/2 text-zinc-500" />
            <input value={searchQuery} onChange={(event) => setSearchQuery(event.target.value)} placeholder="Search sessions…" className="w-full rounded-xl border border-zinc-800 bg-zinc-950 py-3 pl-12 pr-6 text-zinc-200 outline-none focus:border-emerald-500" />
          </div>
          <div className="flex w-full flex-col gap-4 sm:flex-row lg:w-auto">
            <div className="relative">
              <CalendarDays className="pointer-events-none absolute left-4 top-1/2 h-5 w-5 -translate-y-1/2 text-emerald-500" />
              <input type="date" value={filterDate} onChange={(event) => setFilterDate(event.target.value)} className="w-full min-w-[230px] appearance-none rounded-xl border border-zinc-800 bg-zinc-950 py-3.5 pl-12 pr-12 text-sm font-bold text-zinc-200 [color-scheme:dark]" />
              {!filterDate && <ChevronDown className="pointer-events-none absolute right-4 top-1/2 h-4 w-5 -translate-y-1/2 text-zinc-500" />}
              {filterDate && <button onClick={() => setFilterDate('')} aria-label="Clear date filter" className="absolute right-3 top-1/2 -translate-y-1/2 rounded-full bg-zinc-900 p-1.5 text-zinc-500"><X className="h-4 w-4" /></button>}
            </div>
            <button onClick={exportToCsv} className="flex items-center justify-center gap-3 rounded-xl border border-zinc-800 bg-zinc-950 px-6 py-3 text-sm font-bold uppercase tracking-widest text-zinc-200 hover:bg-zinc-900"><Download className="h-5 w-5 text-emerald-400" /> Export CSV</button>
          </div>
        </div>

        <div className="min-h-0 flex-1 overflow-hidden rounded-3xl border border-zinc-800 bg-zinc-950/50 shadow-2xl">
          {isLoading ? (
            <div className="grid h-full min-h-[400px] place-items-center font-mono text-sm uppercase tracking-widest text-zinc-500">Loading session log…</div>
          ) : (
            <div className="h-full overflow-y-auto">
              <table className="hidden w-full border-collapse text-left lg:table">
                <thead className="sticky top-0 z-10 border-b border-zinc-800 bg-zinc-900/95">
                  <tr>{['Started', 'Session', 'Presses', 'Conversation', 'Status', ''].map((heading) => <th key={heading} className="px-8 py-5 text-xs font-black uppercase tracking-[0.2em] text-zinc-400">{heading}</th>)}</tr>
                </thead>
                <tbody className="divide-y divide-zinc-800/60">
                  {filteredSessions.map((session) => (
                    <tr key={session.sessionId} className={latestLiveSessionId === session.sessionId ? 'bg-emerald-500/[0.05]' : 'hover:bg-zinc-900/60'}>
                      <td className="whitespace-nowrap px-8 py-6 font-mono text-sm text-zinc-300">{new Date(session.startedAt).toLocaleString()}</td>
                      <td className="max-w-52 truncate px-8 py-6 font-mono text-xs text-zinc-500" title={session.sessionId}>{session.sessionId}</td>
                      <td className="px-8 py-6 font-bold text-zinc-200">{session.pressCount}</td>
                      <td className="px-8 py-6 text-sm text-zinc-400">{session.recordingCount} visitor · {session.replies.length} homeowner</td>
                      <td className="px-8 py-6"><span className={`rounded-full border px-3 py-1 text-[10px] font-black uppercase tracking-wider ${session.status === 'ACTIVE' ? 'border-emerald-500/30 bg-emerald-500/10 text-emerald-400' : 'border-zinc-700 bg-zinc-800 text-zinc-400'}`}>{session.status}</span></td>
                      <td className="px-8 py-6 text-right"><button onClick={() => setSelectedSession(session)} aria-label="Preview visitor session" className="rounded-xl border border-zinc-700 bg-zinc-800 p-3 text-emerald-400"><Eye className="h-5 w-5" /></button></td>
                    </tr>
                  ))}
                </tbody>
              </table>
              <div className="space-y-4 p-4 lg:hidden">
                {filteredSessions.map((session) => (
                  <button key={session.sessionId} onClick={() => setSelectedSession(session)} className="w-full rounded-2xl border border-zinc-800 bg-zinc-950 p-5 text-left">
                    <div className="flex items-start justify-between gap-3"><span className="font-bold text-zinc-200">{session.pressCount} {session.pressCount === 1 ? 'Press' : 'Presses'}</span><span className="text-xs text-emerald-400">{session.recordingCount} {session.recordingCount === 1 ? 'Message' : 'Messages'}</span></div>
                    <p className="mt-3 font-mono text-xs text-zinc-500">{new Date(session.startedAt).toLocaleString()}</p>
                  </button>
                ))}
              </div>
            </div>
          )}
        </div>
        <p className="mt-3 shrink-0 font-mono text-[10px] uppercase tracking-widest text-zinc-600">Showing {filteredSessions.length} grouped visitor sessions</p>
      </div>
      <EventPreviewDrawer session={selectedSession} onClose={() => setSelectedSession(null)} />
    </MainLayout>
  );
}
