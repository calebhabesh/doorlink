'use client';

import React, { createContext, useContext, useEffect, useRef, useState, useCallback } from 'react';
import { usePathname, useRouter } from 'next/navigation';
import {
  VisitorSession,
  upsertSession,
  reconcileSnapshot,
} from '../lib/visitorSessions';

export type StreamConnectionStatus = 'connecting' | 'connected' | 'disconnected';

export interface NewVisitorNotice {
  sessionId: string;
  startedAt: string;
  pressCount: number;
}

interface SessionContextType {
  sessions: VisitorSession[];
  activeSessionId: string | null;
  setActiveSessionId: (id: string | null) => void;
  connectionStatus: StreamConnectionStatus;
  batteryPercentage: number | undefined;
  isLoadingSessions: boolean;
  sessionError: string | null;
  refreshSessions: () => Promise<void>;
  newVisitorNotice: NewVisitorNotice | null;
  clearNewVisitorNotice: () => void;
  dismissNoticeAndSelect: (sessionId: string) => void;
  // Preserved filter state for Event Log
  eventLogSearch: string;
  setEventLogSearch: (search: string) => void;
  eventLogDate: string;
  setEventLogDate: (date: string) => void;
  // Sign-out cleanup
  clearPrivateData: () => void;
}

const SessionContext = createContext<SessionContextType | null>(null);

const API_BASE_URL = '/api/events';
const SESSIONS_INITIAL_LIMIT = 100;
const HISTORY_RETRY_DELAY_MS = 5000;

export function SessionProvider({ children }: { children: React.ReactNode }) {
  const [sessions, setSessions] = useState<VisitorSession[]>([]);
  const [activeSessionId, setActiveSessionId] = useState<string | null>(null);
  const [connectionStatus, setConnectionStatus] = useState<StreamConnectionStatus>('connecting');
  const [batteryPercentage, setBatteryPercentage] = useState<number | undefined>();
  const [isLoadingSessions, setIsLoadingSessions] = useState(true);
  const [sessionError, setSessionError] = useState<string | null>(null);
  const [newVisitorNotice, setNewVisitorNotice] = useState<NewVisitorNotice | null>(null);

  // Preserved filters for Event Log across navigation
  const [eventLogSearch, setEventLogSearch] = useState('');
  const [eventLogDate, setEventLogDate] = useState('');

  const pathname = usePathname();
  const router = useRouter();

  const pathnameRef = useRef(pathname);
  pathnameRef.current = pathname;

  const activeSessionIdRef = useRef(activeSessionId);
  activeSessionIdRef.current = activeSessionId;

  const historyLoadInFlightRef = useRef(false);
  const inFlightBufferRef = useRef<VisitorSession[]>([]);
  const eventSourceRef = useRef<EventSource | null>(null);
  const retryTimerRef = useRef<number | undefined>();
  const unmountedRef = useRef(false);

  // Clear all in-memory private session data on sign-out
  const clearPrivateData = useCallback(() => {
    if (eventSourceRef.current) {
      eventSourceRef.current.close();
      eventSourceRef.current = null;
    }
    if (retryTimerRef.current !== undefined) {
      window.clearTimeout(retryTimerRef.current);
    }
    setSessions([]);
    setActiveSessionId(null);
    setConnectionStatus('disconnected');
    setBatteryPercentage(undefined);
    setNewVisitorNotice(null);
    setEventLogSearch('');
    setEventLogDate('');
  }, []);

  const loadSessionHistory = useCallback(async () => {
    if (historyLoadInFlightRef.current) return;
    historyLoadInFlightRef.current = true;
    inFlightBufferRef.current = [];

    try {
      const response = await fetch(`${API_BASE_URL}/sessions?size=${SESSIONS_INITIAL_LIMIT}`, {
        cache: 'no-store',
      });
      if (!response.ok) throw new Error(`Session history returned ${response.status}`);
      const data: VisitorSession[] = await response.json();
      if (unmountedRef.current) return;

      setSessions((current) => {
        const buffered = inFlightBufferRef.current;
        const reconciled = reconcileSnapshot(current, data, buffered);
        inFlightBufferRef.current = [];
        return reconciled;
      });

      // Default activeSessionId to the latest session if not already selected
      setActiveSessionId((current) => current ?? data[0]?.sessionId ?? null);
      setSessionError(null);
      setIsLoadingSessions(false);
    } catch (err) {
      if (unmountedRef.current) return;
      console.error('Failed to fetch visitor sessions; retrying', err);
      setSessionError(err instanceof Error ? err.message : 'Session history unavailable');
      setIsLoadingSessions(false);
      if (retryTimerRef.current !== undefined) window.clearTimeout(retryTimerRef.current);
      retryTimerRef.current = window.setTimeout(loadSessionHistory, HISTORY_RETRY_DELAY_MS);
    } finally {
      historyLoadInFlightRef.current = false;
    }
  }, []);

  // Poll device battery telemetry
  useEffect(() => {
    let cancelled = false;
    const loadBattery = async () => {
      try {
        const response = await fetch('/api/system/health', { cache: 'no-store' });
        if (!response.ok) return;
        const data = await response.json() as {
          device?: { batteryPercentageEstimate?: number | null };
        };
        if (!cancelled) {
          setBatteryPercentage(data.device?.batteryPercentageEstimate ?? undefined);
        }
      } catch {
        if (!cancelled) setBatteryPercentage(undefined);
      }
    };

    void loadBattery();
    const batteryTimer = window.setInterval(loadBattery, 30000);
    return () => {
      cancelled = true;
      window.clearInterval(batteryTimer);
    };
  }, []);

  // Main EventSource connection and lifecycle management
  useEffect(() => {
    unmountedRef.current = false;
    void loadSessionHistory();

    const eventSource = new EventSource('/stream');
    eventSourceRef.current = eventSource;

    // Notice: HTTP open does not mark "connected" yet.
    // The brief requires: "Model stream state as connecting, connected, or disconnected.
    // Mark Updates Live only after the application-level init event."
    eventSource.onopen = () => {
      // Stay in 'connecting' until the application-level 'init' event confirms backend readiness
    };

    eventSource.onerror = () => {
      setConnectionStatus('connecting');
      void loadSessionHistory();
    };

    eventSource.addEventListener('init', () => {
      setConnectionStatus('connected');
      // Reconcile immediately after initial connect or reconnect
      void loadSessionHistory();
    });

    eventSource.addEventListener('session-update', (event) => {
      setConnectionStatus('connected');
      try {
        const incoming: VisitorSession = JSON.parse(event.data);

        // If history request is in flight, buffer this update so it gets replayed after the snapshot
        if (historyLoadInFlightRef.current) {
          inFlightBufferRef.current.push(incoming);
        }

        setSessions((current) => {
          const wasEmpty = current.length === 0;
          const previousLatest = current[0]?.sessionId ?? null;
          const isNewSession = !current.some((s) => s.sessionId === incoming.sessionId);
          const nextSessions = upsertSession(current, incoming);

          // Handle auto-selection & new visitor notice
          const currentActive = activeSessionIdRef.current;
          const isViewingLatest = wasEmpty || currentActive === null || currentActive === previousLatest;
          const isDashboard = pathnameRef.current === '/';

          if (isNewSession) {
            if (isDashboard && isViewingLatest) {
              setActiveSessionId(incoming.sessionId);
              setNewVisitorNotice(null);
            } else {
              // User was viewing older history on Dashboard, or is on another route
              setNewVisitorNotice({
                sessionId: incoming.sessionId,
                startedAt: incoming.startedAt,
                pressCount: incoming.pressCount,
              });
            }
          }

          return nextSessions;
        });
      } catch (err) {
        console.error('Failed to parse session-update', err);
      }
    });

    const reconcileWhenVisible = () => {
      if (document.visibilityState === 'visible') {
        void loadSessionHistory();
      }
    };
    window.addEventListener('focus', reconcileWhenVisible);
    document.addEventListener('visibilitychange', reconcileWhenVisible);

    return () => {
      unmountedRef.current = true;
      if (retryTimerRef.current !== undefined) window.clearTimeout(retryTimerRef.current);
      eventSource.close();
      eventSourceRef.current = null;
      window.removeEventListener('focus', reconcileWhenVisible);
      document.removeEventListener('visibilitychange', reconcileWhenVisible);
    };
  }, [loadSessionHistory]);

  const clearNewVisitorNotice = useCallback(() => {
    setNewVisitorNotice(null);
  }, []);

  const dismissNoticeAndSelect = useCallback((sessionId: string) => {
    setActiveSessionId(sessionId);
    setNewVisitorNotice(null);
    if (pathnameRef.current !== '/') {
      router.push('/');
    }
  }, [router]);

  return (
    <SessionContext.Provider
      value={{
        sessions,
        activeSessionId,
        setActiveSessionId,
        connectionStatus,
        batteryPercentage,
        isLoadingSessions,
        sessionError,
        refreshSessions: loadSessionHistory,
        newVisitorNotice,
        clearNewVisitorNotice,
        dismissNoticeAndSelect,
        eventLogSearch,
        setEventLogSearch,
        eventLogDate,
        setEventLogDate,
        clearPrivateData,
      }}
    >
      {children}
    </SessionContext.Provider>
  );
}

export function useSessionContext() {
  const context = useContext(SessionContext);
  if (!context) {
    throw new Error('useSessionContext must be used within a SessionProvider');
  }
  return context;
}
