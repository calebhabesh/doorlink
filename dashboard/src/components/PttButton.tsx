'use client';

import { useState, useRef } from 'react';
import { Mic, Square } from 'lucide-react';

export default function PttButton() {
  const [isRecording, setIsRecording] = useState(false);
  const mediaRecorderRef = useRef<MediaRecorder | null>(null);
  const audioChunksRef = useRef<Blob[]>([]);

  const startRecording = async () => {
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      const mediaRecorder = new MediaRecorder(stream);
      mediaRecorderRef.current = mediaRecorder;
      audioChunksRef.current = [];

      mediaRecorder.ondataavailable = (event) => {
        if (event.data.size > 0) {
          audioChunksRef.current.push(event.data);
        }
      };

      mediaRecorder.onstop = async () => {
        const audioBlob = new Blob(audioChunksRef.current, { type: 'audio/wav' });
        const formData = new FormData();
        formData.append('audio', audioBlob);

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

        // Stop all tracks to release the microphone
        stream.getTracks().forEach(track => track.stop());
      };

      mediaRecorder.start();
      setIsRecording(true);
    } catch (error) {
      console.error('Error accessing microphone:', error);
      alert('Could not access microphone. Please ensure permissions are granted.');
    }
  };

  const stopRecording = () => {
    if (mediaRecorderRef.current && isRecording) {
      mediaRecorderRef.current.stop();
      setIsRecording(false);
    }
  };

  return (
    <button
      onMouseDown={startRecording}
      onMouseUp={stopRecording}
      onMouseLeave={stopRecording} // Stop if mouse leaves button while holding
      onTouchStart={startRecording}
      onTouchEnd={stopRecording}
      className={`flex items-center gap-2 px-6 py-3.5 rounded-xl text-sm font-bold uppercase tracking-widest transition-all shadow-xl border ${
        isRecording
          ? 'bg-rose-600 hover:bg-rose-500 text-white border-rose-500 shadow-rose-900/40 scale-95'
          : 'bg-emerald-600 hover:bg-emerald-500 text-white border-emerald-500 shadow-emerald-900/40'
      }`}
    >
      {isRecording ? (
        <>
          <Square className="w-5 h-5 animate-pulse" fill="currentColor" />
          Recording...
        </>
      ) : (
        <>
          <Mic className="w-5 h-5" />
          Push to Talk
        </>
      )}
    </button>
  );
}
