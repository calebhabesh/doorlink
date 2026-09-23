'use client';

import { useState, useRef, useEffect } from 'react';
import { Settings as SettingsIcon, HardDrive, ChevronDown } from 'lucide-react';

interface SystemSettings {
  id?: number;
  retentionDays: number;
  cameraQuality?: string;
  notificationsEnabled?: boolean;
}

export default function Settings() {
  const [settings, setSettings] = useState<SystemSettings | null>(null);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [toast, setToast] = useState<{ message: string; isError?: boolean } | null>(null);
  const timeoutRef = useRef<NodeJS.Timeout | null>(null);

  useEffect(() => {
    fetch('/api/system/settings')
      .then((res) => {
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        return res.json();
      })
      .then((data: SystemSettings) => {
        setSettings(data);
        setLoading(false);
      })
      .catch((err) => {
        console.error('Failed to fetch settings', err);
        setLoading(false);
      });
  }, []);

  const showToast = (message: string, isError = false) => {
    setToast({ message, isError });
    if (timeoutRef.current) clearTimeout(timeoutRef.current);
    timeoutRef.current = setTimeout(() => setToast(null), 3000);
  };

  const handleRetentionChange = async (days: number) => {
    if (!settings || saving) return;
    const previous = settings;
    const payload = { ...settings, retentionDays: days };
    setSaving(true);

    try {
      const response = await fetch('/api/system/settings', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
      });

      if (!response.ok) {
        throw new Error(`Server returned HTTP ${response.status}`);
      }

      const savedData: SystemSettings = await response.json();
      setSettings(savedData);
      showToast('Retention policy updated successfully');
    } catch (err) {
      console.error('Failed to save retention policy', err);
      setSettings(previous);
      showToast('Could not save retention policy', true);
    } finally {
      setSaving(false);
    }
  };

  return (
    <div className="w-full max-w-[800px] mx-auto flex flex-col h-full relative flex-1">
      {/* Toast Notification */}
      {toast && (
        <div className="absolute -top-4 right-0 z-50 animate-slide-in">
          <div className="bg-zinc-900 border border-zinc-800 shadow-2xl rounded-xl px-6 py-4 flex items-center gap-3">
            <div className={`w-2 h-2 rounded-full ${toast.isError ? 'bg-rose-500' : 'bg-emerald-500'}`} />
            <p className="text-zinc-200 text-sm font-medium">{toast.message}</p>
          </div>
        </div>
      )}

      <div className="mb-8 flex items-center gap-3">
        <div className="p-3 bg-zinc-900 rounded-xl border border-zinc-800">
          <SettingsIcon className="w-6 h-6 text-zinc-400" />
        </div>
        <div>
          <h2 className="text-2xl font-black tracking-tight text-white">System Settings</h2>
          <p className="text-sm text-zinc-400 font-medium">Configure media retention and system preferences</p>
        </div>
      </div>

      {loading || !settings ? (
        <div className="flex-1 flex items-center justify-center">
          <p className="text-zinc-500 animate-pulse font-mono uppercase tracking-widest text-xs">Loading Preferences…</p>
        </div>
      ) : (
        <div className="space-y-6">
          {/* Storage & Event Retention Policy (backed by gateway retention service) */}
          <div className="bg-zinc-950 border border-zinc-800 rounded-3xl p-8 shadow-lg">
            <div className="flex items-center gap-3 mb-6">
              <HardDrive className="w-5 h-5 text-purple-400" />
              <h3 className="text-lg font-bold text-white">Storage & Retention</h3>
            </div>
            
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
              <div>
                <h4 className="font-bold text-zinc-200">Event Retention Policy</h4>
                <p className="text-sm text-zinc-400 mt-1">
                  How long to keep visitor recordings and snapshot images before cleanup by the gateway retention job.
                </p>
              </div>
              <div className="relative shrink-0">
                <select 
                  value={settings.retentionDays}
                  disabled={saving}
                  onChange={(e) => void handleRetentionChange(parseInt(e.target.value, 10))}
                  aria-label="Event Retention Policy"
                  className="appearance-none bg-zinc-900 border border-zinc-800 text-zinc-200 text-sm rounded-xl pl-5 pr-10 py-3 focus:outline-none focus:border-emerald-500 focus-visible:ring-2 focus-visible:ring-emerald-500 transition-colors shadow-inner cursor-pointer disabled:opacity-50"
                >
                  <option value="7">7 Days</option>
                  <option value="30">30 Days</option>
                  <option value="90">90 Days</option>
                  <option value="0">Forever</option>
                </select>
                <ChevronDown className="absolute right-3 top-1/2 -translate-y-1/2 w-4 h-4 text-zinc-500 pointer-events-none" />
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
