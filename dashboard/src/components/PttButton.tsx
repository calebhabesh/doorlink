'use client';

import { useEffect, useRef, useState } from 'react';
import { Loader2, Mic, Radio, Square } from 'lucide-react';

type PttState = 'idle' | 'arming' | 'recording' | 'sending' | 'queued' | 'error';

interface PttButtonProps {
  sessionId: string;
  eventId?: number | null;
}

const MAX_RECORDING_MS = 20_000;

function makeWav(samples: Int16Array, sampleRate: number): Blob {
  const buffer = new ArrayBuffer(44 + samples.byteLength);
  const view = new DataView(buffer);
  const writeAscii = (offset: number, value: string) => {
    for (let i = 0; i < value.length; i++) view.setUint8(offset + i, value.charCodeAt(i));
  };

  writeAscii(0, 'RIFF');
  view.setUint32(4, 36 + samples.byteLength, true);
  writeAscii(8, 'WAVE');
  writeAscii(12, 'fmt ');
  view.setUint32(16, 16, true);
  view.setUint16(20, 1, true);
  view.setUint16(22, 1, true);
  view.setUint32(24, sampleRate, true);
  view.setUint32(28, sampleRate * 2, true);
  view.setUint16(32, 2, true);
  view.setUint16(34, 16, true);
  writeAscii(36, 'data');
  view.setUint32(40, samples.byteLength, true);
  new Int16Array(buffer, 44).set(samples);
  return new Blob([buffer], { type: 'audio/wav' });
}

function resamplePcm(samples: Int16Array, sourceRate: number, targetRate = 16000): Int16Array {
  if (sourceRate === targetRate) return samples;
  const targetLength = Math.max(1, Math.round((samples.length * targetRate) / sourceRate));
  const output = new Int16Array(targetLength);
  const ratio = sourceRate / targetRate;
  for (let i = 0; i < targetLength; i++) {
    const position = i * ratio;
    const left = Math.floor(position);
    const right = Math.min(left + 1, samples.length - 1);
    const fraction = position - left;
    output[i] = Math.round(samples[left] * (1 - fraction) + samples[right] * fraction);
  }
  return output;
}

export default function PttButton({ sessionId, eventId }: PttButtonProps) {
  const [state, setState] = useState<PttState>('idle');
  const [error, setError] = useState<string | null>(null);
  const heldRef = useRef(false);
  const streamRef = useRef<MediaStream | null>(null);
  const audioContextRef = useRef<AudioContext | null>(null);
  const sourceRef = useRef<MediaStreamAudioSourceNode | null>(null);
  const processorRef = useRef<ScriptProcessorNode | null>(null);
  const silentGainRef = useRef<GainNode | null>(null);
  const chunksRef = useRef<Int16Array[]>([]);
  const startedAtRef = useRef(0);
  const timeoutRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const stoppingRef = useRef(false);

  const releaseAudioResources = async () => {
    if (timeoutRef.current) clearTimeout(timeoutRef.current);
    timeoutRef.current = null;
    processorRef.current?.disconnect();
    sourceRef.current?.disconnect();
    silentGainRef.current?.disconnect();
    processorRef.current = null;
    sourceRef.current = null;
    silentGainRef.current = null;
    streamRef.current?.getTracks().forEach((track) => track.stop());
    streamRef.current = null;
    const context = audioContextRef.current;
    audioContextRef.current = null;
    if (context && context.state !== 'closed') await context.close();
  };

  const cancelDevicePtt = async () => {
    try {
      await fetch('/api/system/ptt/cancel', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ sessionId, eventId }),
      });
    } catch {
      // The device also leaves the armed state on its own session timeout.
    }
  };

  const stopRecording = async (send = true) => {
    if (stoppingRef.current) return;
    if (!heldRef.current && state !== 'recording' && state !== 'arming') return;
    stoppingRef.current = true;
    heldRef.current = false;
    const sampleRate = audioContextRef.current?.sampleRate ?? 16000;
    const durationMs = Math.max(0, Date.now() - startedAtRef.current);
    await releaseAudioResources();

    const totalSamples = chunksRef.current.reduce((total, chunk) => total + chunk.length, 0);
    if (!send || totalSamples === 0 || durationMs < 100) {
      await cancelDevicePtt();
      setState('idle');
      stoppingRef.current = false;
      return;
    }

    setState('sending');
    const samples = new Int16Array(totalSamples);
    let offset = 0;
    for (const chunk of chunksRef.current) {
      samples.set(chunk, offset);
      offset += chunk.length;
    }

    const doorbellSamples = resamplePcm(samples, sampleRate);
    const formData = new FormData();
    formData.append('sessionId', sessionId);
    if (eventId != null) formData.append('eventId', String(eventId));
    formData.append('durationMs', String(Math.round((doorbellSamples.length * 1000) / 16000)));
    formData.append('audio', makeWav(doorbellSamples, 16000), 'homeowner-reply.wav');

    try {
      const response = await fetch('/api/system/ptt', { method: 'POST', body: formData });
      if (!response.ok) throw new Error(await response.text());
      setState('queued');
      setTimeout(() => setState('idle'), 1500);
    } catch (sendError) {
      await cancelDevicePtt();
      setError(sendError instanceof Error ? sendError.message : 'Reply could not be sent');
      setState('error');
    }
    stoppingRef.current = false;
  };

  const startRecording = async () => {
    if (heldRef.current || state === 'sending') return;
    stoppingRef.current = false;
    heldRef.current = true;
    setError(null);
    setState('arming');

    try {
      const stream = await navigator.mediaDevices.getUserMedia({
        audio: { channelCount: 1, echoCancellation: true, noiseSuppression: true },
      });
      if (!heldRef.current) {
        stream.getTracks().forEach((track) => track.stop());
        return;
      }

      const armResponse = await fetch('/api/system/ptt/start', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ sessionId, eventId }),
      });
      if (!armResponse.ok) throw new Error(await armResponse.text());
      if (!heldRef.current) {
        stream.getTracks().forEach((track) => track.stop());
        await cancelDevicePtt();
        return;
      }

      const AudioContextClass = window.AudioContext ||
        (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
      const context = new AudioContextClass({ sampleRate: 16000 });
      const source = context.createMediaStreamSource(stream);
      const processor = context.createScriptProcessor(2048, 1, 1);
      const silentGain = context.createGain();
      silentGain.gain.value = 0;

      streamRef.current = stream;
      audioContextRef.current = context;
      sourceRef.current = source;
      processorRef.current = processor;
      silentGainRef.current = silentGain;
      chunksRef.current = [];
      startedAtRef.current = Date.now();

      processor.onaudioprocess = (audioEvent) => {
        const input = audioEvent.inputBuffer.getChannelData(0);
        const chunk = new Int16Array(input.length);
        for (let i = 0; i < input.length; i++) {
          const sample = Math.max(-1, Math.min(1, input[i]));
          chunk[i] = sample < 0 ? sample * 0x8000 : sample * 0x7fff;
        }
        chunksRef.current.push(chunk);
      };

      source.connect(processor);
      processor.connect(silentGain);
      silentGain.connect(context.destination);
      setState('recording');
      timeoutRef.current = setTimeout(() => void stopRecording(true), MAX_RECORDING_MS);
    } catch (startError) {
      heldRef.current = false;
      await releaseAudioResources();
      await cancelDevicePtt();
      setError(startError instanceof Error ? startError.message : 'Microphone unavailable');
      setState('error');
    }
  };

  useEffect(() => {
    return () => {
      const wasHeld = heldRef.current;
      heldRef.current = false;
      void releaseAudioResources();
      if (wasHeld) {
        void fetch('/api/system/ptt/cancel', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ sessionId, eventId }),
        });
      }
    };
  }, [sessionId, eventId]);

  const busy = state === 'arming' || state === 'sending';
  const label = state === 'arming' ? 'Arming…'
    : state === 'recording' ? 'Release to Send'
    : state === 'sending' ? 'Sending…'
    : state === 'queued' ? 'Reply Queued'
    : state === 'error' ? 'Try Again'
    : 'Push to Talk';

  return (
    <div className="relative flex-1 sm:flex-initial">
      <button
        type="button"
        disabled={busy}
        aria-label={label}
        title={error ?? 'Hold to record; release to send'}
        onPointerDown={(event) => {
          event.preventDefault();
          event.currentTarget.setPointerCapture(event.pointerId);
          void startRecording();
        }}
        onPointerUp={(event) => {
          event.preventDefault();
          void stopRecording(true);
        }}
        onPointerCancel={() => void stopRecording(false)}
        onLostPointerCapture={() => {
          if (heldRef.current) void stopRecording(true);
        }}
        className={`select-none touch-none flex items-center justify-center gap-2 w-full h-24 sm:h-auto sm:px-6 sm:py-3.5 rounded-2xl sm:rounded-xl text-sm font-bold uppercase tracking-widest transition-all shadow-xl border ${
          state === 'recording'
            ? 'bg-rose-600 text-white border-rose-500 shadow-rose-900/40 scale-95'
            : state === 'queued'
              ? 'bg-sky-600 text-white border-sky-500'
              : 'bg-emerald-600 hover:bg-emerald-500 text-white border-emerald-500 shadow-emerald-900/40 disabled:opacity-70'
        }`}
      >
        {busy ? <Loader2 className="w-5 h-5 animate-spin" />
          : state === 'recording' ? <Square className="w-5 h-5 animate-pulse" fill="currentColor" />
            : state === 'queued' ? <Radio className="w-5 h-5" />
              : <Mic className="w-5 h-5" />}
        <span>{label}</span>
      </button>
      {error && <p className="absolute top-full mt-2 right-0 w-64 text-right text-xs text-rose-400">{error}</p>}
    </div>
  );
}
