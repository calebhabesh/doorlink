'use client';

import { useMemo, useState } from 'react';
import Image from 'next/image';
import { Camera, CalendarDays, ChevronDown, Download, Eye, Search, X } from 'lucide-react';
import EventPreviewDrawer from '../../../components/EventPreviewDrawer';
import { getSessionCoverImageKey, markImageKeyCached, VisitorSession } from '../../../lib/visitorSessions';
import { useSessionContext } from '../../../context/SessionContext';

const API_BASE_URL = '/api/events';
const MEDIA_BASE_URL = `${API_BASE_URL}/media`;

function csvCell(value: string | number) {
  const text = String(value);
  return /[",\n]/.test(text) ? `"${text.replaceAll('"', '""')}"` : text;
}

export default function EventLog() {
  const {
    sessions,
    isLoadingSessions,
    eventLogSearch,
    setEventLogSearch,
    eventLogDate,
    setEventLogDate,
  } = useSessionContext();

  const [selectedSession, setSelectedSession] = useState<VisitorSession | null>(null);

  const filteredSessions = useMemo(() => sessions.filter((session) => {
    const query = eventLogSearch.trim().toLowerCase();
    const matchesSearch = !query
      || session.sessionId.toLowerCase().includes(query)
      || session.presses.some((press) => press.eventType.toLowerCase().includes(query));
    if (!eventLogDate) return matchesSearch;
    return matchesSearch && new Date(session.startedAt).toLocaleDateString('en-CA') === eventLogDate;
  }), [sessions, eventLogSearch, eventLogDate]);

  const exportToCsv = () => {
    const rows = filteredSessions.map((session) => [
      session.sessionId,
      session.startedAt,
      session.status,
      session.pressCount,
      session.recordingCount,
      session.replies.length,
      session.latestImageKey ?? '',
    ]);
    const csv = [
      ['Session ID', 'Started At', 'Status', 'Presses', 'Visitor Recordings', 'Homeowner Replies', 'Latest Image'],
      ...rows,
    ].map((row) => row.map(csvCell).join(',')).join('\n');
    const url = URL.createObjectURL(new Blob([csv], { type: 'text/csv;charset=utf-8' }));
    const link = document.createElement('a');
    link.href = url;
    link.download = `doorbell_sessions_${eventLogDate || 'all'}.csv`;
    link.click();
    URL.revokeObjectURL(url);
  };

  return (
    <div className="mx-auto flex h-full w-full max-w-[1800px] flex-col overflow-hidden flex-1">
      {/* Search, Filter, and Export Header */}
      <div className="mb-6 flex shrink-0 flex-col items-start justify-between gap-4 lg:mb-8 lg:flex-row lg:items-center">
        <div className="relative w-full lg:w-96">
          <Search className="absolute left-4 top-1/2 h-4 w-5 -translate-y-1/2 text-zinc-500" />
          <input
            value={eventLogSearch}
            onChange={(event) => setEventLogSearch(event.target.value)}
            placeholder="Search sessions…"
            className="w-full rounded-xl border border-zinc-800 bg-zinc-950 py-3 pl-12 pr-6 text-zinc-200 outline-none transition-colors focus:border-emerald-500 focus-visible:ring-2 focus-visible:ring-emerald-500"
          />
        </div>
        <div className="flex w-full flex-col gap-4 sm:flex-row lg:w-auto">
          <div className="relative">
            <CalendarDays className="pointer-events-none absolute left-4 top-1/2 h-5 w-5 -translate-y-1/2 text-emerald-500" />
            <input
              type="date"
              value={eventLogDate}
              onChange={(event) => setEventLogDate(event.target.value)}
              className="w-full min-w-[230px] appearance-none rounded-xl border border-zinc-800 bg-zinc-950 py-3 pl-12 pr-12 text-sm font-bold text-zinc-200 [color-scheme:dark] transition-colors focus:border-emerald-500 focus-visible:ring-2 focus-visible:ring-emerald-500 outline-none"
            />
            {!eventLogDate && (
              <ChevronDown className="pointer-events-none absolute right-4 top-1/2 h-4 w-5 -translate-y-1/2 text-zinc-500" />
            )}
            {eventLogDate && (
              <button
                type="button"
                onClick={() => setEventLogDate('')}
                aria-label="Clear date filter"
                className="absolute right-3 top-1/2 -translate-y-1/2 rounded-full bg-zinc-900 p-1.5 text-zinc-400 hover:text-zinc-200 focus-visible:ring-2 focus-visible:ring-emerald-500 focus-visible:outline-none"
              >
                <X className="h-4 w-4" />
              </button>
            )}
          </div>
          <button
            type="button"
            onClick={exportToCsv}
            className="flex items-center justify-center gap-3 rounded-xl border border-zinc-800 bg-zinc-950 px-6 py-3 text-sm font-bold uppercase tracking-widest text-zinc-200 hover:bg-zinc-900 transition-colors focus-visible:ring-2 focus-visible:ring-emerald-500 focus-visible:outline-none"
          >
            <Download className="h-4 w-4 text-emerald-400" /> Export CSV
          </button>
        </div>
      </div>

      {/* Solid Opaque Dark Table Surface */}
      <div className="min-h-0 flex-1 overflow-hidden rounded-3xl border border-zinc-800 bg-zinc-950 shadow-2xl flex flex-col">
        {isLoadingSessions ? (
          <div className="grid h-full min-h-[400px] place-items-center font-mono text-sm uppercase tracking-widest text-zinc-500">
            Loading session log…
          </div>
        ) : (
          <div className="h-full overflow-y-auto">
            {/* Desktop Denser Table */}
            <table className="hidden w-full border-collapse text-left lg:table">
              <thead className="sticky top-0 z-10 border-b border-zinc-800 bg-zinc-900">
                <tr>
                  <th className="px-6 py-3.5 text-xs font-black uppercase tracking-[0.2em] text-zinc-400">Snapshot</th>
                  <th className="px-6 py-3.5 text-xs font-black uppercase tracking-[0.2em] text-zinc-400">Started</th>
                  <th className="px-6 py-3.5 text-xs font-black uppercase tracking-[0.2em] text-zinc-400">Session</th>
                  <th className="px-6 py-3.5 text-xs font-black uppercase tracking-[0.2em] text-zinc-400">Presses</th>
                  <th className="px-6 py-3.5 text-xs font-black uppercase tracking-[0.2em] text-zinc-400">Conversation</th>
                  <th className="px-6 py-3.5 text-xs font-black uppercase tracking-[0.2em] text-zinc-400">Status</th>
                  <th className="px-6 py-3.5 text-xs font-black uppercase tracking-[0.2em] text-zinc-400 text-right"><span className="sr-only">Actions</span></th>
                </tr>
              </thead>
              <tbody className="divide-y divide-zinc-800">
                {filteredSessions.length > 0 ? (
                  filteredSessions.map((session) => {
                    const coverImageKey = getSessionCoverImageKey(session);
                    return (
                      <tr
                        key={session.sessionId}
                        onClick={() => setSelectedSession(session)}
                        onKeyDown={(e) => {
                          if (e.key === 'Enter' || e.key === ' ') {
                            e.preventDefault();
                            setSelectedSession(session);
                          }
                        }}
                        tabIndex={0}
                        role="button"
                        aria-label={`View visitor session from ${new Date(session.startedAt).toLocaleString()}`}
                        className="group cursor-pointer hover:bg-zinc-900/60 transition-colors focus-visible:outline-none focus-visible:bg-zinc-900/80"
                      >
                        {/* Event Thumbnail Snapshot */}
                        <td className="whitespace-nowrap px-6 py-3">
                          <div className="relative h-12 w-16 shrink-0 overflow-hidden rounded-xl border border-zinc-800 bg-zinc-900 shadow-inner">
                            {coverImageKey ? (
                              <Image
                                src={`${MEDIA_BASE_URL}/${coverImageKey}`}
                                alt="Event snapshot"
                                fill
                                unoptimized
                                onLoad={() => markImageKeyCached(coverImageKey)}
                                className="object-cover transition duration-300 group-hover:scale-105"
                              />
                            ) : (
                              <div className="grid h-full place-items-center text-zinc-500 bg-zinc-900">
                                <Camera className="h-5 w-5" />
                              </div>
                            )}
                            {session.status === 'ACTIVE' && (
                              <span className="absolute right-1.5 top-1.5 h-2 w-2 rounded-full bg-emerald-400 shadow-[0_0_0_2px_rgba(16,185,129,0.3)]" />
                            )}
                          </div>
                        </td>

                        {/* Emphasized Date */}
                        <td className="whitespace-nowrap px-6 py-3.5 font-mono text-sm font-semibold text-zinc-200">
                          {new Date(session.startedAt).toLocaleString()}
                        </td>

                        {/* De-emphasized Session ID with accessible tooltip */}
                        <td className="px-6 py-3.5 font-mono text-[11px] text-zinc-500" title={session.sessionId}>
                          <span className="block max-w-[130px] truncate" aria-label={`Session ID ${session.sessionId}`}>
                            {session.sessionId}
                          </span>
                        </td>

                        {/* Emphasized Press Count */}
                        <td className="px-6 py-3.5">
                          <span className="font-black text-emerald-400 text-sm">
                            {session.pressCount}
                          </span>
                        </td>

                        {/* Conversation Summary */}
                        <td className="px-6 py-3.5 text-xs text-zinc-400">
                          <span className="text-zinc-300 font-medium">{session.recordingCount}</span> visitor ·{' '}
                          <span className="text-sky-300 font-medium">{session.replies.length}</span> homeowner
                        </td>

                        {/* Status */}
                        <td className="px-6 py-3.5">
                          <span
                            className={`rounded-full border px-2.5 py-0.5 text-[10px] font-black uppercase tracking-wider ${
                              session.status === 'ACTIVE'
                                ? 'border-emerald-500/30 bg-emerald-500/10 text-emerald-400'
                                : 'border-zinc-700 bg-zinc-800 text-zinc-400'
                            }`}
                          >
                            {session.status}
                          </span>
                        </td>

                        {/* Preview Action */}
                        <td className="px-6 py-3.5 text-right">
                          <button
                            type="button"
                            onClick={(e) => {
                              e.stopPropagation();
                              setSelectedSession(session);
                            }}
                            aria-label={`Preview visitor session from ${new Date(session.startedAt).toLocaleString()}`}
                            className="rounded-lg border border-zinc-700 bg-zinc-800 p-2 text-emerald-400 hover:border-emerald-500 hover:text-white transition-colors focus-visible:ring-2 focus-visible:ring-emerald-500 focus-visible:outline-none"
                          >
                            <Eye className="h-4 w-4" />
                          </button>
                        </td>
                      </tr>
                    );
                  })
                ) : (
                  <tr>
                    <td colSpan={7} className="px-6 py-12 text-center text-sm font-mono uppercase tracking-wider text-zinc-500">
                      No sessions match the current filter
                    </td>
                  </tr>
                )}
              </tbody>
            </table>

            {/* Mobile Cards */}
            <div className="space-y-3 p-4 lg:hidden">
              {filteredSessions.length > 0 ? (
                filteredSessions.map((session) => {
                  const coverImageKey = getSessionCoverImageKey(session);
                  return (
                    <button
                      key={session.sessionId}
                      type="button"
                      onClick={() => setSelectedSession(session)}
                      className="w-full rounded-2xl border border-zinc-800 bg-zinc-900/60 p-4 text-left transition-colors hover:border-zinc-700 hover:bg-zinc-900 focus-visible:ring-2 focus-visible:ring-emerald-500 focus-visible:outline-none flex items-center gap-4"
                    >
                      <div className="relative h-14 w-16 shrink-0 overflow-hidden rounded-xl border border-zinc-800 bg-zinc-900">
                        {coverImageKey ? (
                          <Image
                            src={`${MEDIA_BASE_URL}/${coverImageKey}`}
                            alt="Event snapshot"
                            fill
                            unoptimized
                            onLoad={() => markImageKeyCached(coverImageKey)}
                            className="object-cover"
                          />
                        ) : (
                          <div className="grid h-full place-items-center text-zinc-500">
                            <Camera className="h-5 w-5" />
                          </div>
                        )}
                        {session.status === 'ACTIVE' && (
                          <span className="absolute right-1.5 top-1.5 h-2 w-2 rounded-full bg-emerald-400 shadow-[0_0_0_2px_rgba(16,185,129,0.3)]" />
                        )}
                      </div>
                      <div className="flex-1 min-w-0">
                        <div className="flex items-start justify-between gap-3">
                          <span className="font-bold text-zinc-200">
                            {session.pressCount} {session.pressCount === 1 ? 'Press' : 'Presses'}
                          </span>
                          <span className="text-xs text-emerald-400">
                            {session.recordingCount} {session.recordingCount === 1 ? 'Message' : 'Messages'}
                          </span>
                        </div>
                        <p className="mt-1 font-mono text-xs text-zinc-400">
                          {new Date(session.startedAt).toLocaleString()}
                        </p>
                        <div className="mt-2 flex items-center justify-between">
                          <span className="font-mono text-[10px] text-zinc-500 truncate max-w-[150px]">
                            {session.sessionId}
                          </span>
                          <span
                            className={`rounded-full border px-2 py-0.5 text-[9px] font-black uppercase tracking-wider ${
                              session.status === 'ACTIVE'
                                ? 'border-emerald-500/30 bg-emerald-500/10 text-emerald-400'
                                : 'border-zinc-700 bg-zinc-800 text-zinc-400'
                            }`}
                          >
                            {session.status}
                          </span>
                        </div>
                      </div>
                    </button>
                  );
                })
              ) : (
                <div className="py-12 text-center text-sm font-mono uppercase tracking-wider text-zinc-500">
                  No sessions match the current filter
                </div>
              )}
            </div>
          </div>
        )}
      </div>

      <p className="mt-3 shrink-0 font-mono text-[10px] uppercase tracking-widest text-zinc-500">
        Showing {filteredSessions.length} grouped visitor sessions
      </p>

      <EventPreviewDrawer session={selectedSession} onClose={() => setSelectedSession(null)} />
    </div>
  );
}
