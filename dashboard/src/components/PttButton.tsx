'use client';

import { useState, useRef, TouchEvent } from 'react';
import { Mic, Square } from 'lucide-react';

export default function PttButton() {
  const [isRecording, setIsRecording] = useState(false);
  const isRecordingRef = useRef(false);
  const streamRef = useRef<MediaStream | null>(null);
  const audioContextRef = useRef<AudioContext | null>(null);
  const processorRef = useRef<ScriptProcessorNode | null>(null);
  const chunksRef = useRef<Int16Array[]>([]);

  const startRecording = async () => {
    try {
      isRecordingRef.current = true;
      setIsRecording(true);

      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      
      // Check if user released the button while stream acquisition was pending
      if (!isRecordingRef.current) {
        stream.getTracks().forEach(track => track.stop());
        return;
      }

      streamRef.current = stream;
      chunksRef.current = [];

      const audioContext = new (window.AudioContext || (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext)({
        sampleRate: 16000,
      });
      audioContextRef.current = audioContext;

      const source = audioContext.createMediaStreamSource(stream);
      // Create a ScriptProcessorNode to process audio in chunks of 4096 frames
      const processor = audioContext.createScriptProcessor(4096, 1, 1);
      processorRef.current = processor;

      processor.onaudioprocess = (e) => {
        const inputData = e.inputBuffer.getChannelData(0);
        // Convert Float32Array to Int16Array (16-bit signed PCM)
        const pttChunk = new Int16Array(inputData.length);
        for (let i = 0; i < inputData.length; i++) {
          // Clamp float sample to [-1.0, 1.0] range
          const s = Math.max(-1.0, Math.min(1.0, inputData[i]));
          pttChunk[i] = s < 0 ? s * 0x8000 : s * 0x7FFF;
        }
        chunksRef.current.push(pttChunk);
      };

      source.connect(processor);
      processor.connect(audioContext.destination);
    } catch (error) {
      console.error('Error accessing microphone:', error);
      alert('Could not access microphone. Please ensure permissions are granted.');
      isRecordingRef.current = false;
      setIsRecording(false);
    }
  };

  const stopRecording = async () => {
    // Immediately clear state and ref
    isRecordingRef.current = false;
    setIsRecording(false);

    if (processorRef.current && audioContextRef.current) {
      // Disconnect and close the AudioContext
      processorRef.current.disconnect();
      processorRef.current = null;
      if (audioContextRef.current.state !== 'closed') {
        await audioContextRef.current.close();
      }
      audioContextRef.current = null;
      if (streamRef.current) {
        streamRef.current.getTracks().forEach(track => track.stop());
        streamRef.current = null;
      }

      // Concatenate all Int16Array chunks into a single ArrayBuffer
      const totalSamples = chunksRef.current.reduce((acc, chunk) => acc + chunk.length, 0);
      if (totalSamples === 0) return;

      const pttBuffer = new Int16Array(totalSamples);
      let offset = 0;
      for (const chunk of chunksRef.current) {
        pttBuffer.set(chunk, offset);
        offset += chunk.length;
      }

      // Send raw 16-bit PCM bytes (audio/l16)
      const audioBlob = new Blob([pttBuffer.buffer], { type: 'audio/l16' });
      const formData = new FormData();
      formData.append('audio', audioBlob, 'ptt.raw');

      try {
        const response = await fetch('/api/system/ptt', {
          method: 'POST',
          body: formData,
        });
        if (response.ok) {
          console.log('PTT audio sent successfully');
        } else {
          console.error('Failed to send PTT audio');
        }
      } catch (error) {
        console.error('Error sending PTT audio:', error);
      }
    }
  };

  const handleTouchStart = (e: TouchEvent) => {
    e.preventDefault();
    startRecording();
  };

  const handleTouchEnd = (e: TouchEvent) => {
    e.preventDefault();
    stopRecording();
  };

  const handleTouchCancel = (e: TouchEvent) => {
    e.preventDefault();
    stopRecording();
  };

  return (
    <button
      onMouseDown={startRecording}
      onMouseUp={stopRecording}
      onMouseLeave={stopRecording} // Stop if mouse leaves button while holding
      onTouchStart={handleTouchStart}
      onTouchEnd={handleTouchEnd}
      onTouchCancel={handleTouchCancel}
      className={`select-none touch-none flex items-center gap-2 px-5 py-2.5 sm:px-6 sm:py-3.5 rounded-xl text-xs sm:text-sm font-bold uppercase tracking-widest transition-all shadow-xl border ${
        isRecording
          ? 'bg-rose-600 hover:bg-rose-500 text-white border-rose-500 shadow-rose-900/40 scale-95'
          : 'bg-emerald-600 hover:bg-emerald-500 text-white border-emerald-500 shadow-emerald-900/40'
      }`}
    >
      {isRecording ? (
        <>
          <Square className="w-4 h-4 sm:w-5 sm:h-5 animate-pulse" fill="currentColor" />
          Recording...
        </>
      ) : (
        <>
          <Mic className="w-4 h-4 sm:w-5 sm:h-5" />
          Push to Talk
        </>
      )}
    </button>
  );
}
