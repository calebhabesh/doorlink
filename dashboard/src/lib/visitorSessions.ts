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

export function titleCase(value: string) {
  return value.toLowerCase().split('_')
    .map((word) => word.charAt(0).toUpperCase() + word.slice(1)).join(' ');
}

export function upsertSession(sessions: VisitorSession[], next: VisitorSession) {
  return [next, ...sessions.filter((session) => session.sessionId !== next.sessionId)]
    .sort((left, right) => Date.parse(right.startedAt) - Date.parse(left.startedAt));
}
