'use client';

import MainLayout from '../../components/MainLayout';
import { Activity, Server, Cpu, Database, Wifi } from 'lucide-react';

export default function SystemHealth() {
  return (
    <MainLayout isConnected={true} breadcrumbs={[{ label: 'Dashboard' }, { label: 'System Health', active: true }]}>
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

        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
          
          {/* Gateway Status */}
          <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-3xl flex flex-col hover:border-zinc-700 transition-colors shadow-lg">
            <div className="flex justify-between items-start mb-6">
              <Server className="w-5 h-5 text-blue-400" />
              <span className="flex items-center gap-2 bg-emerald-500/10 text-emerald-500 px-3 py-1 rounded-full text-xs font-bold uppercase tracking-wider border border-emerald-500/20">
                <span className="w-1.5 h-1.5 bg-emerald-500 rounded-full animate-pulse"></span>
                Online
              </span>
            </div>
            <h3 className="text-zinc-500 text-sm font-bold uppercase tracking-widest mb-1">Gateway Server</h3>
            <p className="text-3xl font-black text-white">14d 12h</p>
            <p className="text-xs text-zinc-500 mt-2 font-mono">Uptime / Port 8080</p>
          </div>

          {/* Firmware Status */}
          <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-3xl flex flex-col hover:border-zinc-700 transition-colors shadow-lg">
            <div className="flex justify-between items-start mb-6">
              <Cpu className="w-5 h-5 text-indigo-400" />
              <span className="flex items-center gap-2 bg-emerald-500/10 text-emerald-500 px-3 py-1 rounded-full text-xs font-bold uppercase tracking-wider border border-emerald-500/20">
                <span className="w-1.5 h-1.5 bg-emerald-500 rounded-full"></span>
                Online
              </span>
            </div>
            <h3 className="text-zinc-500 text-sm font-bold uppercase tracking-widest mb-1">ESP32-S3 Node</h3>
            <p className="text-3xl font-black text-white">85%</p>
            <p className="text-xs text-zinc-500 mt-2 font-mono">Battery Level (Mocked)</p>
          </div>

          {/* Storage */}
          <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-3xl flex flex-col hover:border-zinc-700 transition-colors shadow-lg">
            <div className="flex justify-between items-start mb-6">
              <Database className="w-5 h-5 text-purple-400" />
              <span className="flex items-center gap-2 bg-blue-500/10 text-blue-400 px-3 py-1 rounded-full text-xs font-bold uppercase tracking-wider border border-blue-500/20">
                Healthy
              </span>
            </div>
            <h3 className="text-zinc-500 text-sm font-bold uppercase tracking-widest mb-1">MinIO Storage</h3>
            <p className="text-3xl font-black text-white">42%</p>
            <div className="w-full bg-zinc-900 rounded-full h-1.5 mt-3">
              <div className="bg-purple-500 h-1.5 rounded-full" style={{ width: '42%' }}></div>
            </div>
            <p className="text-xs text-zinc-500 mt-2 font-mono">12.6 GB / 30 GB Used</p>
          </div>

          {/* MQTT Broker */}
          <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-3xl flex flex-col hover:border-zinc-700 transition-colors shadow-lg">
            <div className="flex justify-between items-start mb-6">
              <Wifi className="w-5 h-5 text-rose-400" />
              <span className="flex items-center gap-2 bg-emerald-500/10 text-emerald-500 px-3 py-1 rounded-full text-xs font-bold uppercase tracking-wider border border-emerald-500/20">
                Connected
              </span>
            </div>
            <h3 className="text-zinc-500 text-sm font-bold uppercase tracking-widest mb-1">MQTT Broker</h3>
            <p className="text-3xl font-black text-white">12</p>
            <p className="text-xs text-zinc-500 mt-2 font-mono">Messages / sec</p>
          </div>

        </div>
      </div>
    </MainLayout>
  );
}