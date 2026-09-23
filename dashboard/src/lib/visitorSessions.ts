export interface VisitorRecording {
  recordingId: string;
  audioKey: string;
  durationMs: number | null;
  createdAt: string;
}

export interface VisitorPress {
  id: number;
  pressId: string | null;
  pressNumber: number;
  pressedAt: string;
  durationMs: number | null;
  eventType: string;
  state: 'PHOTO_PENDING' | 'READY' | string;
  imageKey: string | null;
  recordings: VisitorRecording[];
}

export interface HomeownerReply {
  messageId: string;
  sender: string;
  audioKey: string;
  durationMs: number;
  createdAt: string;
  deliveredAt: string | null;
}

export interface VisitorSession {
  id: number | null;
  sessionId: string;
  startedAt: string;
  lastActivityAt: string;
  endsAt: string;
  closedAt: string | null;
  status: 'ACTIVE' | 'CLOSED' | string;
  latestImageKey: string | null;
  pressCount: number;
  recordingCount: number;
  presses: VisitorPress[];
  replies: HomeownerReply[];
}

export interface SessionImage {
  imageKey: string;
  pressId: number;
  pressNumber: number;
  capturedAt: string;
}

export function titleCase(value: string) {
  return value.toLowerCase().split('_')
    .map((word) => word.charAt(0).toUpperCase() + word.slice(1)).join(' ');
}

const loadedImageKeys = new Set<string>();

export function isImageKeyCached(key: string | null | undefined): boolean {
  return Boolean(key && loadedImageKeys.has(key));
}

export function markImageKeyCached(key: string | null | undefined): void {
  if (key) {
    loadedImageKeys.add(key);
  }
}

/**
 * Merge an incoming session record into an existing session record safely.
 * Key invariants:
 * 1. A closed session must NEVER revert to active in the UI.
 * 2. Newer photo keys and READY states replace pending states without dropping existing resolved images.
 * 3. Presses and homeowner replies are merged idempotently without duplication.
 */
export function mergeSession(existing: VisitorSession, incoming: VisitorSession): VisitorSession {
  const isExistingClosed = existing.status === 'CLOSED' || Boolean(existing.closedAt);
  const isIncomingClosed = incoming.status === 'CLOSED' || Boolean(incoming.closedAt);
  const status = (isExistingClosed || isIncomingClosed) ? 'CLOSED' : incoming.status;
  const closedAt = isExistingClosed
    ? (existing.closedAt ?? incoming.closedAt ?? new Date().toISOString())
    : incoming.closedAt;

  // Merge presses: deduplicate by pressId or id or pressNumber
  const pressMap = new Map<string | number, VisitorPress>();
  for (const p of existing.presses) {
    const key = p.pressId ?? (p.id ? `id-${p.id}` : `num-${p.pressNumber}`);
    pressMap.set(key, p);
  }
  for (const p of incoming.presses) {
    const key = p.pressId ?? (p.id ? `id-${p.id}` : `num-${p.pressNumber}`);
    const prev = pressMap.get(key);
    if (!prev) {
      pressMap.set(key, p);
    } else {
      const imageKey = p.imageKey ?? prev.imageKey;
      const state = (prev.state === 'READY' && (!p.state || p.state === 'PHOTO_PENDING') && imageKey)
        ? 'READY'
        : p.state;
      pressMap.set(key, {
        ...prev,
        ...p,
        imageKey,
        state,
        recordings: (p.recordings && p.recordings.length >= prev.recordings.length)
          ? p.recordings
          : prev.recordings,
      });
    }
  }

  const mergedPresses = Array.from(pressMap.values())
    .sort((a, b) => a.pressNumber - b.pressNumber || Date.parse(a.pressedAt) - Date.parse(b.pressedAt));

  // Merge replies: deduplicate by messageId
  const replyMap = new Map<string, HomeownerReply>();
  for (const r of existing.replies) replyMap.set(r.messageId, r);
  for (const r of incoming.replies) {
    const prev = replyMap.get(r.messageId);
    if (!prev) {
      replyMap.set(r.messageId, r);
    } else {
      replyMap.set(r.messageId, {
        ...prev,
        ...r,
        deliveredAt: r.deliveredAt ?? prev.deliveredAt,
      });
    }
  }
  const mergedReplies = Array.from(replyMap.values())
    .sort((a, b) => Date.parse(a.createdAt) - Date.parse(b.createdAt));

  const latestImageKey = incoming.latestImageKey ?? existing.latestImageKey;
  const startedAt = existing.startedAt || incoming.startedAt;
  const lastActivityAt = Date.parse(incoming.lastActivityAt) > Date.parse(existing.lastActivityAt)
    ? incoming.lastActivityAt
    : existing.lastActivityAt;
  const endsAt = Date.parse(incoming.endsAt) > Date.parse(existing.endsAt)
    ? incoming.endsAt
    : existing.endsAt;

  return {
    ...existing,
    ...incoming,
    status,
    closedAt,
    startedAt,
    lastActivityAt,
    endsAt,
    latestImageKey,
    pressCount: Math.max(existing.pressCount, incoming.pressCount, mergedPresses.length),
    recordingCount: Math.max(existing.recordingCount, incoming.recordingCount),
    presses: mergedPresses,
    replies: mergedReplies,
  };
}

export function upsertSession(sessions: VisitorSession[], incoming: VisitorSession): VisitorSession[] {
  const existingIndex = sessions.findIndex((session) => session.sessionId === incoming.sessionId);
  let nextSessions: VisitorSession[];
  if (existingIndex >= 0) {
    const merged = mergeSession(sessions[existingIndex], incoming);
    nextSessions = [...sessions];
    nextSessions[existingIndex] = merged;
  } else {
    nextSessions = [incoming, ...sessions];
  }
  return nextSessions.sort((left, right) => Date.parse(right.startedAt) - Date.parse(left.startedAt));
}

/**
 * Reconciles an authoritative fetched server snapshot with local session state
 * and stream updates buffered during the in-flight request.
 */
export function reconcileSnapshot(
  currentSessions: VisitorSession[],
  serverSnapshot: VisitorSession[],
  bufferedUpdates: VisitorSession[],
): VisitorSession[] {
  // Server snapshot is the reconciliation baseline
  let reconciled = serverSnapshot.map((serverSession) => {
    const existing = currentSessions.find((s) => s.sessionId === serverSession.sessionId);
    if (!existing) return serverSession;
    return mergeSession(existing, serverSession);
  });

  // Replay buffered updates received while the request was in flight
  for (const update of bufferedUpdates) {
    reconciled = upsertSession(reconciled, update);
  }

  return reconciled.sort((left, right) => Date.parse(right.startedAt) - Date.parse(left.startedAt));
}

export function getSessionImages(session: VisitorSession): SessionImage[] {
  const seen = new Set<string>();

  return [...session.presses]
    .sort((left, right) => Date.parse(left.pressedAt) - Date.parse(right.pressedAt))
    .flatMap((press) => {
      if (!press.imageKey || seen.has(press.imageKey)) return [];
      seen.add(press.imageKey);
      return [{
        imageKey: press.imageKey,
        pressId: press.id,
        pressNumber: press.pressNumber,
        capturedAt: press.pressedAt,
      }];
    });
}

export function getSessionCoverImageKey(session: VisitorSession) {
  return getSessionImages(session)[0]?.imageKey ?? session.latestImageKey;
}
