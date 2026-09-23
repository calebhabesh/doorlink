'use client';

import Image from 'next/image';
import { Camera, Check, Clock3, Radio, Volume2 } from 'lucide-react';
import { getSessionImages, HomeownerReply, VisitorPress, VisitorSession } from '../lib/visitorSessions';
import AudioPlayer from './AudioPlayer';

const MEDIA_BASE_URL = '/api/events/media';

type PressGroupItem = { kind: 'press-group'; at: string; presses: VisitorPress[] };
type RecordingItem = { kind: 'recording'; at: string; press: VisitorPress };
type ReplyItem = { kind: 'reply'; at: string; reply: HomeownerReply };
type SnapshotItem = { kind: 'snapshot'; at: string; imageKey: string; pressNumber: number };
type TimelineItem = PressGroupItem | RecordingItem | ReplyItem | SnapshotItem;

function buildTimeline(session: VisitorSession): TimelineItem[] {
  const images = getSessionImages(session);
  const coverKey = images[0]?.imageKey;
  const raw: Array<RecordingItem | ReplyItem | SnapshotItem | { kind: 'press'; at: string; press: VisitorPress }> = [
    ...session.presses.map((press) => ({
      kind: press.recordings.length ? 'recording' as const : 'press' as const,
      at: press.pressedAt,
      press,
    })),
    ...session.replies.map((reply) => ({ kind: 'reply' as const, at: reply.createdAt, reply })),
    ...images.filter((image) => image.imageKey !== coverKey).map((image) => ({
      kind: 'snapshot' as const,
      at: image.capturedAt,
      imageKey: image.imageKey,
      pressNumber: image.pressNumber,
    })),
  ].sort((left, right) => {
    const timeDifference = Date.parse(left.at) - Date.parse(right.at);
    if (timeDifference !== 0) return timeDifference;
    const order = { press: 0, recording: 0, snapshot: 1, reply: 2 };
    return order[left.kind] - order[right.kind];
  });

  return raw.reduce<TimelineItem[]>((items, item) => {
    if (item.kind !== 'press') {
      items.push(item);
      return items;
    }
    const previous = items.at(-1);
    if (previous?.kind === 'press-group') previous.presses.push(item.press);
    else items.push({ kind: 'press-group', at: item.at, presses: [item.press] });
    return items;
  }, []);
}

function pressRangeLabel(presses: VisitorPress[]) {
  if (presses.length === 1) return `Press ${presses[0].pressNumber}`;
  return `Presses ${presses[0].pressNumber}–${presses.at(-1)!.pressNumber}`;
}

export default function SessionTimeline({ session }: { session: VisitorSession }) {
  const timeline = buildTimeline(session);

  return (
    <div className="space-y-3">
      {timeline.map((item) => {
        if (item.kind === 'press-group') {
          const pendingCount = item.presses.filter((press) => press.state === 'PHOTO_PENDING').length;
          return (
            <div key={`press-group-${item.presses[0].id}`} className="flex items-center gap-3 rounded-xl border border-zinc-800/80 bg-zinc-950/50 px-4 py-3">
              <span className="grid h-8 w-8 shrink-0 place-items-center rounded-full bg-emerald-500/10 text-xs font-black text-emerald-400">
                {item.presses.length}
              </span>
              <div className="min-w-0 flex-1">
                <p className="truncate text-sm font-bold text-zinc-200">{pressRangeLabel(item.presses)}</p>
                <p className="font-mono text-[10px] text-zinc-500">
                  {new Date(item.presses[0].pressedAt).toLocaleTimeString()}
                  {item.presses.length > 1 && ` – ${new Date(item.presses.at(-1)!.pressedAt).toLocaleTimeString()}`}
                </p>
              </div>
              <span className="shrink-0 rounded-full bg-zinc-800 px-3 py-1 text-[10px] font-bold text-zinc-400">
                {pendingCount ? `${pendingCount} Pending` : item.presses.length === 1 ? 'Short Press' : `${item.presses.length} Short Presses`}
              </span>
            </div>
          );
        }

        if (item.kind === 'snapshot') {
          return (
            <figure key={`snapshot-${item.imageKey}`} className="overflow-hidden rounded-2xl border border-zinc-800 bg-zinc-950/60">
              <div className="flex items-center justify-between gap-3 px-4 py-3">
                <figcaption className="flex items-center gap-2 text-sm font-bold text-zinc-200"><Camera className="h-4 w-4 text-emerald-400" /> Updated Snapshot</figcaption>
                <span className="font-mono text-[10px] text-zinc-500">After Press {item.pressNumber}</span>
              </div>
              <div className="relative aspect-video w-full border-t border-zinc-800 bg-zinc-900">
                <Image src={`${MEDIA_BASE_URL}/${item.imageKey}`} alt={`Visitor snapshot after press ${item.pressNumber}`} fill unoptimized className="object-cover" />
              </div>
            </figure>
          );
        }

        if (item.kind === 'recording') {
          return (
            <div key={`recording-${item.press.id}`} className="rounded-2xl border border-emerald-500/20 bg-emerald-500/[0.04] p-4">
              <div className="mb-3 flex flex-wrap items-center justify-between gap-2">
                <div className="flex items-center gap-3">
                  <span className="grid h-8 w-8 place-items-center rounded-full bg-emerald-500/10 text-xs font-black text-emerald-400">{item.press.pressNumber}</span>
                  <div>
                    <p className="flex items-center gap-2 text-sm font-bold text-zinc-200"><Volume2 className="h-4 w-4 text-emerald-400" /> Visitor Message</p>
                    <p className="font-mono text-[10px] text-zinc-500">{new Date(item.press.pressedAt).toLocaleTimeString()}</p>
                  </div>
                </div>
                <span className="rounded-full bg-emerald-500/10 px-3 py-1 text-[10px] font-bold text-emerald-300">Held {((item.press.durationMs ?? 0) / 1000).toFixed(1)}s</span>
              </div>
              <div className="space-y-2">
                {item.press.recordings.map((recording, recordingIndex) => (
                  <AudioPlayer key={recording.recordingId} src={`${MEDIA_BASE_URL}/${recording.audioKey}`} durationMs={recording.durationMs} label={`visitor message ${recordingIndex + 1} from press ${item.press.pressNumber}`} />
                ))}
              </div>
            </div>
          );
        }

        return (
          <div key={`reply-${item.reply.messageId}`} className="w-full rounded-2xl border border-sky-500/20 bg-sky-500/[0.06] p-4">
            <div className="mb-3 flex flex-wrap items-center justify-between gap-2">
              <div>
                <p className="flex items-center gap-2 text-sm font-bold text-sky-300"><Radio className="h-4 w-4" /> Homeowner Reply</p>
                <p className="mt-1 font-mono text-[10px] text-zinc-500">{new Date(item.reply.createdAt).toLocaleTimeString()}</p>
              </div>
              <span className="flex items-center gap-1.5 rounded-full bg-sky-500/10 px-3 py-1 text-[10px] font-bold text-sky-300">
                {item.reply.deliveredAt ? <Check className="h-3 w-3" /> : <Clock3 className="h-3 w-3" />}
                {item.reply.deliveredAt ? 'Delivered to Doorbell' : 'Playback Queued'}
              </span>
            </div>
            <AudioPlayer src={`${MEDIA_BASE_URL}/${item.reply.audioKey}`} durationMs={item.reply.durationMs} label="homeowner reply" tone="homeowner" />
          </div>
        );
      })}
    </div>
  );
}
