'use client';

import { useState, useRef, useEffect } from 'react';
import MainLayout from '../../components/MainLayout';
import { Settings as SettingsIcon, Bell, HardDrive, Camera, ChevronDown } from 'lucide-react';

interface SystemSettings {
  id?: number;
  retentionDays: number;
  cameraQuality: string;
  notificationsEnabled: boolean;
}

export default function Settings() {
  const [settings, setSettings] = useState<SystemSettings | null>(null);
  const [loading, setLoading] = useState(true);
  const [toastMessage, setToastMessage] = useState<string | null>(null);
  const timeoutRef = useRef<NodeJS.Timeout | null>(null);

  useEffect(() => {
    fetch('/api/system/settings')
      .then((res) => res.json())
      .then((data: SystemSettings) => {
        setSettings(data);
        setLoading(false);
      })
      .catch((err) => {
        console.error("Failed to fetch settings", err);
        setLoading(false);
      });
  }, []);

  const showSaveToast = (message: string = 'Settings saved successfully') => {
    setToastMessage(message);
    if (timeoutRef.current) clearTimeout(timeoutRef.current);
    timeoutRef.current = setTimeout(() => setToastMessage(null), 3000);
  };

  const saveSettings = (updated: SystemSettings) => {
    fetch('/api/system/settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(updated),
    })
      .then((res) => res.json())
      .then((data: SystemSettings) => {
        setSettings(data);
        showSaveToast();
      })
      .catch((err) => {
        console.error("Failed to save settings", err);
        showSaveToast('Error saving settings');
      });
  };

  const handleToggle = () => {
    if (!settings) return;
    const updated = { ...settings, notificationsEnabled: !settings.notificationsEnabled };
    setSettings(updated);
    saveSettings(updated);
  };

  const handleSelect = (field: keyof SystemSettings, val: string | number) => {
    if (!settings) return;
    const updated = { ...settings, [field]: val };
    setSettings(updated);
    saveSettings(updated);
  };

  return (
    <MainLayout status="connected" breadcrumbs={[{ label: 'Dashboard' }, { label: 'Settings', active: true }]}>
      <div className="w-full max-w-[800px] mx-auto flex flex-col h-full relative">
        
        {/* Toast Notification */}
        {toastMessage && (
          <div className="absolute -top-4 right-0 z-50 animate-slide-in">
            <div className="bg-zinc-900 border border-zinc-800 shadow-2xl rounded-xl px-6 py-4 flex items-center gap-3">
              <div className={`w-2 h-2 rounded-full ${toastMessage.includes('Error') ? 'bg-rose-500' : 'bg-emerald-500'}`}></div>
              <p className="text-zinc-200 text-sm font-medium">{toastMessage}</p>
            </div>
          </div>
        )}

        <div className="mb-8 flex items-center gap-3">
          <div className="p-3 bg-zinc-900 rounded-xl border border-zinc-800">
            <SettingsIcon className="w-6 h-6 text-zinc-400" />
          </div>
          <div>
            <h1 className="text-2xl font-black tracking-tight text-white">System Settings</h1>
            <p className="text-sm text-zinc-400 font-medium">Configure dashboard and device preferences</p>
          </div>
        </div>

        {loading || !settings ? (
          <div className="flex-1 flex items-center justify-center">
             <p className="text-zinc-500 animate-pulse font-mono uppercase tracking-widest">Loading Preferences...</p>
          </div>
        ) : (
          <div className="space-y-6">
            
            {/* Notifications Section */}
            <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 rounded-3xl p-8 shadow-lg animate-flash-event">
              <div className="flex items-center gap-3 mb-6">
                <Bell className="w-5 h-5 text-indigo-400" />
                <h2 className="text-lg font-bold text-white">Notifications</h2>
              </div>
              
              <div className="flex items-center justify-between gap-8">
                <div className="flex-1">
                  <h3 className="font-bold text-zinc-200">Browser Push Notifications</h3>
                  <p className="text-sm text-zinc-500 mt-1">Receive desktop alerts when the doorbell is pressed.</p>
                </div>
                <button 
                  onClick={handleToggle}
                  className={`w-11 h-6 rounded-full transition-colors relative flex items-center shrink-0 shadow-inner ${settings.notificationsEnabled ? 'bg-emerald-500' : 'bg-zinc-800 border border-zinc-700'}`}
                >
                  <div className={`w-5 h-5 bg-white rounded-full transition-transform shadow-md transform ${settings.notificationsEnabled ? 'translate-x-[22px]' : 'translate-x-[2px]'}`}></div>
                </button>
              </div>
            </div>

            {/* Storage Section */}
            <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 rounded-3xl p-8 shadow-lg animate-flash-event">
              <div className="flex items-center gap-3 mb-6">
                <HardDrive className="w-5 h-5 text-purple-400" />
                <h2 className="text-lg font-bold text-white">Storage & Retention</h2>
              </div>
              
              <div className="flex items-center justify-between">
                <div>
                  <h3 className="font-bold text-zinc-200">Event Retention Policy</h3>
                  <p className="text-sm text-zinc-500 mt-1">How long to keep video/image events before auto-deleting.</p>
                </div>
                <div className="relative">
                  <select 
                    value={settings.retentionDays}
                    onChange={(e) => handleSelect('retentionDays', parseInt(e.target.value))}
                    className="appearance-none bg-zinc-900 border border-zinc-800 text-zinc-200 text-sm rounded-xl pl-5 pr-10 py-3 focus:outline-none focus:border-emerald-500 transition-colors shadow-inner cursor-pointer"
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

            {/* Device Section */}
            <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 rounded-3xl p-8 shadow-lg animate-flash-event">
              <div className="flex items-center gap-3 mb-6">
                <Camera className="w-5 h-5 text-emerald-400" />
                <h2 className="text-lg font-bold text-white">Device Preferences</h2>
              </div>
              
              <div className="flex items-center justify-between">
                <div>
                  <h3 className="font-bold text-zinc-200">Camera Quality</h3>
                  <p className="text-sm text-zinc-500 mt-1">Resolution setting for snapshot captures.</p>
                </div>
                <div className="relative">
                  <select 
                    value={settings.cameraQuality}
                    onChange={(e) => handleSelect('cameraQuality', e.target.value)}
                    className="appearance-none bg-zinc-900 border border-zinc-800 text-zinc-200 text-sm rounded-xl pl-5 pr-10 py-3 focus:outline-none focus:border-emerald-500 transition-colors shadow-inner cursor-pointer"
                  >
                    <option value="720p">720p (HD)</option>
                    <option value="1080p">1080p (FHD)</option>
                    <option value="UXGA">UXGA (2MP)</option>
                  </select>
                  <ChevronDown className="absolute right-3 top-1/2 -translate-y-1/2 w-4 h-4 text-zinc-500 pointer-events-none" />
                </div>
              </div>
            </div>

          </div>
        )}
      </div>
    </MainLayout>
  );
}
