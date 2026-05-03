'use client';

import { useState } from 'react';
import MainLayout from '../../components/MainLayout';
import { Settings as SettingsIcon, Bell, HardDrive, Camera } from 'lucide-react';

export default function Settings() {
  const [notifications, setNotifications] = useState(true);
  const [retention, setRetention] = useState('30');
  const [quality, setQuality] = useState('1080');
  const [toastMessage, setToastMessage] = useState<string | null>(null);

  const showSaveToast = () => {
    setToastMessage('Settings saved successfully');
    setTimeout(() => setToastMessage(null), 3000);
  };

  const handleToggle = () => {
    setNotifications(!notifications);
    showSaveToast();
  };

  const handleSelect = (setter: any, val: string) => {
    setter(val);
    showSaveToast();
  };

  return (
    <MainLayout status="connected" breadcrumbs={[{ label: 'Dashboard' }, { label: 'Settings', active: true }]}>
      <div className="w-full max-w-[800px] mx-auto flex flex-col h-full relative">
        
        {/* Toast Notification */}
        {toastMessage && (
          <div className="absolute -top-4 right-0 z-50 animate-slide-in">
            <div className="bg-zinc-900 border border-zinc-800 shadow-2xl rounded-xl px-6 py-4 flex items-center gap-3">
              <div className="w-2 h-2 bg-emerald-500 rounded-full"></div>
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

        <div className="space-y-6">
          
          {/* Notifications Section */}
          <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 rounded-3xl p-8 shadow-lg">
            <div className="flex items-center gap-3 mb-6">
              <Bell className="w-5 h-5 text-indigo-400" />
              <h2 className="text-lg font-bold text-white">Notifications</h2>
            </div>
            
            <div className="flex items-center justify-between">
              <div>
                <h3 className="font-bold text-zinc-200">Browser Push Notifications</h3>
                <p className="text-sm text-zinc-500 mt-1">Receive desktop alerts when the doorbell is pressed.</p>
              </div>
              <button 
                onClick={handleToggle}
                className={`w-14 h-7 rounded-full transition-colors relative flex items-center px-1 ${notifications ? 'bg-emerald-500' : 'bg-zinc-800 border border-zinc-700'}`}
              >
                <div className={`w-5 h-5 bg-white rounded-full transition-transform shadow-sm ${notifications ? 'translate-x-7' : 'translate-x-0'}`}></div>
              </button>
            </div>
          </div>

          {/* Storage Section */}
          <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 rounded-3xl p-8 shadow-lg">
            <div className="flex items-center gap-3 mb-6">
              <HardDrive className="w-5 h-5 text-purple-400" />
              <h2 className="text-lg font-bold text-white">Storage & Retention</h2>
            </div>
            
            <div className="flex items-center justify-between">
              <div>
                <h3 className="font-bold text-zinc-200">Event Retention Policy</h3>
                <p className="text-sm text-zinc-500 mt-1">How long to keep video/image events before auto-deleting.</p>
              </div>
              <select 
                value={retention}
                onChange={(e) => handleSelect(setRetention, e.target.value)}
                className="bg-zinc-900 border border-zinc-700 text-zinc-200 text-sm rounded-xl px-4 py-2.5 focus:outline-none focus:border-emerald-500 transition-colors"
              >
                <option value="7">7 Days</option>
                <option value="30">30 Days</option>
                <option value="90">90 Days</option>
                <option value="forever">Forever</option>
              </select>
            </div>
          </div>

          {/* Device Section */}
          <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 rounded-3xl p-8 shadow-lg">
            <div className="flex items-center gap-3 mb-6">
              <Camera className="w-5 h-5 text-emerald-400" />
              <h2 className="text-lg font-bold text-white">Device Preferences</h2>
            </div>
            
            <div className="flex items-center justify-between">
              <div>
                <h3 className="font-bold text-zinc-200">Camera Quality</h3>
                <p className="text-sm text-zinc-500 mt-1">Resolution setting for snapshot captures.</p>
              </div>
              <select 
                value={quality}
                onChange={(e) => handleSelect(setQuality, e.target.value)}
                className="bg-zinc-900 border border-zinc-700 text-zinc-200 text-sm rounded-xl px-4 py-2.5 focus:outline-none focus:border-emerald-500 transition-colors"
              >
                <option value="720">720p (HD)</option>
                <option value="1080">1080p (FHD)</option>
              </select>
            </div>
          </div>

        </div>
      </div>
    </MainLayout>
  );
}