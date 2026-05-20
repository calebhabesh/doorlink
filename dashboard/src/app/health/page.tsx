'use client';

import { useEffect, useState } from 'react';
import MainLayout from '../../components/MainLayout';
import { Activity, Server, Database, Wifi } from 'lucide-react';

interface SystemHealthData {
  database: 'UP' | 'DOWN';
  storage: 'UP' | 'DOWN';
  mqtt: 'UP' | 'DOWN';
  gateway: 'UP' | 'DOWN';
}

export default function SystemHealth() {
  const [health, setHealth] = useState<SystemHealthData | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const fetchHealth = () => {
      fetch('/api/system/health')
        .then((res) => res.json())
        .then((data: SystemHealthData) => {
          setHealth(data);
          setLoading(false);
        })
        .catch((err) => {
          console.error("Failed to fetch health data", err);
          setLoading(false);
        });
    };

    fetchHealth();
    const interval = setInterval(fetchHealth, 30000); // Poll every 30 seconds
    return () => clearInterval(interval);
  }, []);

  const getStatusColor = (status: 'UP' | 'DOWN' | undefined) => {
    if (!status) return 'text-zinc-500';
    return status === 'UP' ? 'text-emerald-500' : 'text-rose-500';
  };

  const getStatusBg = (status: 'UP' | 'DOWN' | undefined) => {
    if (!status) return 'bg-zinc-500/10 border-zinc-500/20';
    return status === 'UP' ? 'bg-emerald-500/10 border-emerald-500/20' : 'bg-rose-500/10 border-rose-500/20';
  };

  const getIndicatorColor = (status: 'UP' | 'DOWN' | undefined) => {
    if (!status) return 'bg-zinc-500';
    return status === 'UP' ? 'bg-emerald-500' : 'bg-rose-500';
  };

  return (
    <MainLayout status={health?.gateway === 'UP' ? 'connected' : 'connecting'} breadcrumbs={[{ label: 'Dashboard' }, { label: 'System Health', active: true }]}>
      <div className="w-full max-w-[1200px] mx-auto flex flex-col h-full">
        
        <div className="mb-8 flex items-center gap-3">
          <div className="p-3 bg-emerald-500/10 rounded-xl border border-emerald-500/20">
            <Activity className="w-6 h-6 text-emerald-500" />
          </div>
          <div>
            <h1 className="text-2xl font-black tracking-tight text-white">System Health</h1>
            <p className="text-sm text-zinc-400 font-medium">Real-time infrastructure monitoring</p>
          </div>
        </div>

        {loading ? (
          <div className="flex-1 flex items-center justify-center">
             <p className="text-zinc-500 animate-pulse font-mono uppercase tracking-widest">Diagnosing Systems...</p>
          </div>
        ) : (
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
            
            {/* Gateway Status */}
            <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-3xl flex flex-col hover:border-zinc-700 transition-colors shadow-lg animate-flash-event">
              <div className="flex justify-between items-start mb-6">
                <Server className="w-5 h-5 text-blue-400" />
                <span className={`flex items-center gap-2 px-3 py-1 rounded-full text-xs font-bold uppercase tracking-wider border ${getStatusBg(health?.gateway)} ${getStatusColor(health?.gateway)}`}>
                  <span className={`w-1.5 h-1.5 rounded-full ${getIndicatorColor(health?.gateway)} ${health?.gateway === 'UP' ? 'animate-pulse' : ''}`}></span>
                  {health?.gateway || 'UNKNOWN'}
                </span>
              </div>
              <h3 className="text-zinc-500 text-sm font-bold uppercase tracking-widest mb-1">Gateway Server</h3>
              <p className="text-3xl font-black text-white">{health?.gateway === 'UP' ? 'Online' : 'Offline'}</p>
              <p className="text-xs text-zinc-500 mt-2 font-mono">Port 8080 / Spring Boot</p>
            </div>

            {/* Database Status */}
            <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-3xl flex flex-col hover:border-zinc-700 transition-colors shadow-lg animate-flash-event">
              <div className="flex justify-between items-start mb-6">
                <Database className="w-5 h-5 text-indigo-400" />
                <span className={`flex items-center gap-2 px-3 py-1 rounded-full text-xs font-bold uppercase tracking-wider border ${getStatusBg(health?.database)} ${getStatusColor(health?.database)}`}>
                  <span className={`w-1.5 h-1.5 rounded-full ${getIndicatorColor(health?.database)}`}></span>
                  {health?.database || 'UNKNOWN'}
                </span>
              </div>
              <h3 className="text-zinc-500 text-sm font-bold uppercase tracking-widest mb-1">PostgreSQL</h3>
              <p className="text-3xl font-black text-white">{health?.database === 'UP' ? 'Connected' : 'Error'}</p>
              <p className="text-xs text-zinc-500 mt-2 font-mono">Event Persistence</p>
            </div>

            {/* Storage Status */}
            <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-3xl flex flex-col hover:border-zinc-700 transition-colors shadow-lg animate-flash-event">
              <div className="flex justify-between items-start mb-6">
                <HardDrive className="w-5 h-5 text-purple-400" />
                <span className={`flex items-center gap-2 px-3 py-1 rounded-full text-xs font-bold uppercase tracking-wider border ${getStatusBg(health?.storage)} ${getStatusColor(health?.storage)}`}>
                  <span className={`w-1.5 h-1.5 rounded-full ${getIndicatorColor(health?.storage)}`}></span>
                  {health?.storage || 'UNKNOWN'}
                </span>
              </div>
              <h3 className="text-zinc-500 text-sm font-bold uppercase tracking-widest mb-1">MinIO Storage</h3>
              <p className="text-3xl font-black text-white">{health?.storage === 'UP' ? 'Available' : 'Offline'}</p>
              <p className="text-xs text-zinc-500 mt-2 font-mono">Object Store / S3 API</p>
            </div>

            {/* MQTT Broker Status */}
            <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-3xl flex flex-col hover:border-zinc-700 transition-colors shadow-lg animate-flash-event">
              <div className="flex justify-between items-start mb-6">
                <Wifi className="w-5 h-5 text-rose-400" />
                <span className={`flex items-center gap-2 px-3 py-1 rounded-full text-xs font-bold uppercase tracking-wider border ${getStatusBg(health?.mqtt)} ${getStatusColor(health?.mqtt)}`}>
                  <span className={`w-1.5 h-1.5 rounded-full ${getIndicatorColor(health?.mqtt)}`}></span>
                  {health?.mqtt || 'UNKNOWN'}
                </span>
              </div>
              <h3 className="text-zinc-500 text-sm font-bold uppercase tracking-widest mb-1">MQTT Broker</h3>
              <p className="text-3xl font-black text-white">{health?.mqtt === 'UP' ? 'Active' : 'Offline'}</p>
              <p className="text-xs text-zinc-500 mt-2 font-mono">Mosquitto / Port 1883</p>
            </div>

          </div>
        )}
      </div>
    </MainLayout>
  );
}

const HardDrive = ({ className }: { className?: string }) => (
  <svg className={className} xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><line x1="22" y1="12" x2="2" y2="12"></line><path d="M5.45 5.11L2 12v6a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2v-6l-3.45-6.89A2 2 0 0 0 16.76 4H7.24a2 2 0 0 0-1.79 1.11z"></path><line x1="6" y1="16" x2="6.01" y2="16"></line><line x1="10" y1="16" x2="10.01" y2="16"></line></svg>
);