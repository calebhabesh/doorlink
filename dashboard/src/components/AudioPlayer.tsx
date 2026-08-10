'use client';

import { useEffect, useRef, useState } from 'react';
import { Pause, Play, Volume2, VolumeX } from 'lucide-react';

interface AudioPlayerProps {
  src: string;
  label: string;
  durationMs?: number | null;
  tone?: 'visitor' | 'homeowner';
}

function formatTime(value: number) {
  if (!Number.isFinite(value) || value < 0) return '0:00';
  const minutes = Math.floor(value / 60);
  const seconds = Math.floor(value % 60);
  return `${minutes}:${seconds.toString().padStart(2, '0')}`;
}

export default function AudioPlayer({ src, label, durationMs, tone = 'visitor' }: AudioPlayerProps) {
  const audioRef = useRef<HTMLAudioElement>(null);
  const [isPlaying, setIsPlaying] = useState(false);
  const [currentTime, setCurrentTime] = useState(0);
  const fallbackDuration = (durationMs ?? 0) / 1000;
  const [duration, setDuration] = useState(fallbackDuration);
  const [isMuted, setIsMuted] = useState(false);

  useEffect(() => {
    setIsPlaying(false);
    setCurrentTime(0);
    setDuration(fallbackDuration);
  }, [src, fallbackDuration]);

  const togglePlayback = async () => {
    const audio = audioRef.current;
    if (!audio) return;
    if (audio.paused) {
      try {
        await audio.play();
      } catch (error) {
        console.error('Audio playback failed', error);
      }
    } else {
      audio.pause();
    }
  };

  const accent = tone === 'homeowner' ? 'text-sky-300' : 'text-emerald-300';
  const buttonAccent = tone === 'homeowner'
    ? 'border-sky-400/30 bg-sky-400/10 hover:bg-sky-400/20'
    : 'border-emerald-400/30 bg-emerald-400/10 hover:bg-emerald-400/20';

  return (
    <div className="flex min-w-0 items-center gap-3 rounded-xl border border-zinc-700/70 bg-zinc-950/80 p-2.5 shadow-inner">
      <audio
        ref={audioRef}
        src={src}
        preload="metadata"
        onLoadedMetadata={(event) => event.currentTarget.duration > 0 && setDuration(event.currentTarget.duration)}
        onDurationChange={(event) => event.currentTarget.duration > 0 && setDuration(event.currentTarget.duration)}
        onTimeUpdate={(event) => setCurrentTime(event.currentTarget.currentTime)}
        onPlay={() => setIsPlaying(true)}
        onPause={() => setIsPlaying(false)}
        onEnded={() => setIsPlaying(false)}
      />
      <button
        type="button"
        onClick={togglePlayback}
        aria-label={`${isPlaying ? 'Pause' : 'Play'} ${label}`}
        className={`grid h-9 w-9 shrink-0 place-items-center rounded-full border transition ${buttonAccent} ${accent}`}
      >
        {isPlaying ? <Pause className="h-4 w-4 fill-current" /> : <Play className="ml-0.5 h-4 w-4 fill-current" />}
      </button>
      <span className="w-9 shrink-0 font-mono text-[10px] tabular-nums text-zinc-400">{formatTime(currentTime)}</span>
      <input
        type="range"
        min={0}
        max={duration || 0}
        step="0.05"
        value={Math.min(currentTime, duration || 0)}
        onChange={(event) => {
          const nextTime = Number(event.target.value);
          if (audioRef.current) audioRef.current.currentTime = nextTime;
          setCurrentTime(nextTime);
        }}
        aria-label={`Seek ${label}`}
        className={`audio-player-range min-w-0 flex-1 ${tone === 'homeowner' ? 'audio-player-range-sky' : ''}`}
        style={{ '--audio-progress': `${duration > 0 ? (currentTime / duration) * 100 : 0}%` } as React.CSSProperties}
      />
      <span className="w-9 shrink-0 text-right font-mono text-[10px] tabular-nums text-zinc-500">{formatTime(duration)}</span>
      <button
        type="button"
        onClick={() => {
          if (!audioRef.current) return;
          audioRef.current.muted = !isMuted;
          setIsMuted(!isMuted);
        }}
        aria-label={`${isMuted ? 'Unmute' : 'Mute'} ${label}`}
        className="grid h-8 w-8 shrink-0 place-items-center rounded-lg text-zinc-500 transition hover:bg-zinc-800 hover:text-zinc-200"
      >
        {isMuted ? <VolumeX className="h-4 w-4" /> : <Volume2 className="h-4 w-4" />}
      </button>
    </div>
  );
}
