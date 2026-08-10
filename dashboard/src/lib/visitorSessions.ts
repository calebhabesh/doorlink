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

export function upsertSession(sessions: VisitorSession[], next: VisitorSession) {
  return [next, ...sessions.filter((session) => session.sessionId !== next.sessionId)]
    .sort((left, right) => Date.parse(right.startedAt) - Date.parse(left.startedAt));
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
