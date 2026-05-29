'use client';

import { useState } from 'react';
import { Volume2, CheckCircle } from 'lucide-react';

interface Preset {
  id: string;
  label: string;
  icon: string;
  speech: string;
}

export default function QuickResponsesCard() {
  const [activePreset, setActivePreset] = useState<string | null>(null);
  const [showStatus, setShowStatus] = useState<string | null>(null);

  const presets: Preset[] = [
    {
      id: 'leave_package',
      label: 'Leave Package',
      icon: '📦',
      speech: 'Please leave the package at the front door. Thank you!'
    },
    {
      id: 'one_moment',
      label: 'One Moment',
      icon: '⏳',
      speech: 'One moment, I am coming to the door.'
    },
    {
      id: 'no_solicitors',
      label: 'No Solicitors',
      icon: '❌',
      speech: 'No solicitation, thank you.'
    },
    {
      id: 'recording',
      label: 'Security Alert',
      icon: '🚷',
      speech: 'You are being recorded. Please leave the property.'
    }
  ];

  const handleTriggerPreset = (preset: Preset) => {
    setActivePreset(preset.id);
    
    // Simulate MQTT publish
    console.log(`Publishing preset text to MQTT topic: "${preset.speech}"`);
    
    setTimeout(() => {
      setActivePreset(null);
      setShowStatus(preset.label);
      setTimeout(() => setShowStatus(null), 3000);
    }, 1500);
  };

  return (
    <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-3xl flex flex-col hover:border-zinc-700 transition-all duration-300 shadow-lg group relative overflow-hidden animate-flash-event h-full">
      <div className="absolute inset-0 bg-gradient-to-br from-emerald-500/[0.01] via-transparent to-transparent pointer-events-none" />

      {/* Header */}
      <div className="flex justify-between items-center mb-4">
        <h3 className="text-zinc-500 text-xs font-bold uppercase tracking-widest flex items-center gap-1.5 text-left">
          <Volume2 className="w-3.5 h-3.5 text-zinc-400" />
          Quick Responses (Intercom)
        </h3>
        {showStatus && (
          <span className="text-[10px] text-emerald-500 font-bold uppercase tracking-widest flex items-center gap-1 animate-slide-in">
            <CheckCircle className="w-3.5 h-3.5" />
            Sent: {showStatus}
          </span>
        )}
      </div>

      <p className="text-xs text-zinc-400 mb-4 text-left leading-normal">
        Broadcast preset voice responses to the speaker on the doorbell over MQTT.
      </p>

      {/* Grid of presets */}
      <div className="grid grid-cols-2 gap-3">
        {presets.map((preset) => (
          <button
            key={preset.id}
            onClick={() => handleTriggerPreset(preset)}
            disabled={activePreset !== null}
            className={`p-4 rounded-2xl border transition-all duration-200 flex flex-col items-center justify-center text-center relative overflow-hidden group ${
              activePreset === preset.id
                ? 'bg-emerald-500/10 border-emerald-500/50 text-emerald-500 shadow-[0_0_15px_rgba(16,185,129,0.15)]'
                : 'bg-zinc-900/30 border-zinc-800 hover:border-zinc-700 text-zinc-300 hover:bg-zinc-900/50'
            }`}
          >
            {activePreset === preset.id ? (
              <svg className="animate-spin h-5 w-5 text-emerald-500 mb-2" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
              </svg>
            ) : (
              <span className="text-xl mb-1.5 group-hover:scale-110 transition-transform">{preset.icon}</span>
            )}
            <span className="text-xs font-black tracking-tight">{preset.label}</span>
          </button>
        ))}
      </div>
    </div>
  );
}
