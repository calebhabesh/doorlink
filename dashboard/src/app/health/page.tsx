'use client';

import { useEffect, useState } from 'react';
import MainLayout from '../../components/MainLayout';
import { Activity, Cpu, Database, HardDrive, Radio, Server } from 'lucide-react';

type ComponentStatus = 'UP' | 'DOWN' | 'UNKNOWN';

interface SystemHealthData {
  checkedAt: string;
  gateway: { status: ComponentStatus; uptimeSeconds: number };
  database: { status: ComponentStatus; detail: string };
  storage: { status: ComponentStatus; detail: string };
  mqtt: { status: ComponentStatus; lastChangedAt?: string | null };
  device: {
    status: 'KNOWN' | 'UNKNOWN';
    deviceId?: string | null;
    lastSeen?: string | null;
    firmwareVersion?: string | null;
    lastEventType?: string | null;
    lastEventId?: string | null;
    wifiRssiDbm?: number | null;
    batteryStatus: 'NOT_REPORTED';
  };
}

function statusClasses(status?: string) {
  if (status === 'UP' || status === 'KNOWN') {
    return 'bg-emerald-500/10 border-emerald-500/20 text-emerald-500';
  }
  if (status === 'DOWN') {
    return 'bg-rose-500/10 border-rose-500/20 text-rose-500';
  }
  return 'bg-zinc-500/10 border-zinc-500/20 text-zinc-500';
}

function formatDuration(seconds?: number) {
  if (seconds === undefined) return 'Unknown';
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  return days > 0 ? `${days}d ${hours}h` : `${hours}h ${minutes}m`;
}

function StatusBadge({ status }: { status?: string }) {
  return (
    <span className={`px-3 py-1 rounded-full text-xs font-bold border ${statusClasses(status)}`}>
      {status ?? 'UNKNOWN'}
    </span>
  );
}

export default function SystemHealth() {
  const [health, setHealth] = useState<SystemHealthData | null>(null);
  const [error, setError] = useState(false);

  useEffect(() => {
    const fetchHealth = async () => {
      try {
        const response = await fetch('/api/system/health', { cache: 'no-store' });
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        setHealth(await response.json() as SystemHealthData);
        setError(false);
      } catch (fetchError) {
        console.error('Failed to fetch health data', fetchError);
        setError(true);
      }
    };

    void fetchHealth();
    const interval = setInterval(fetchHealth, 30000);
    return () => clearInterval(interval);
  }, []);

  const gatewayStatus = error ? 'DOWN' : health?.gateway.status;

  return (
    <MainLayout status={gatewayStatus === 'UP' ? 'connected' : gatewayStatus === 'DOWN' ? 'disconnected' : 'connecting'} breadcrumbs={[{ label: 'Dashboard' }, { label: 'System Health', active: true }]}>
      <div className="w-full max-w-[1200px] mx-auto flex flex-col h-full">
        <div className="mb-8 flex items-center justify-between gap-3">
          <div className="flex items-center gap-3">
            <div className="p-3 bg-emerald-500/10 rounded-xl border border-emerald-500/20">
              <Activity className="w-6 h-6 text-emerald-500" />
            </div>
            <div>
              <h1 className="text-2xl font-black tracking-tight text-white">System Health</h1>
              <p className="text-sm text-zinc-400 font-medium">Measured infrastructure and last reported device state</p>
            </div>
          </div>
          <p className="text-xs text-zinc-500 font-mono">
            {health?.checkedAt ? `Checked ${new Date(health.checkedAt).toLocaleTimeString()}` : 'Awaiting probe'}
          </p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
          <HealthCard icon={<Server className="w-5 h-5 text-blue-400" />} title="Gateway" status={gatewayStatus} value={gatewayStatus === 'UP' ? 'Online' : error ? 'Unreachable' : 'Checking'} detail={`Process uptime: ${formatDuration(health?.gateway.uptimeSeconds)}`} />
          <HealthCard icon={<Database className="w-5 h-5 text-indigo-400" />} title="PostgreSQL" status={health?.database.status} value={health?.database.status === 'UP' ? 'Connected' : 'Unavailable'} detail={health?.database.detail ?? 'Probe pending'} />
          <HealthCard icon={<HardDrive className="w-5 h-5 text-purple-400" />} title="MinIO" status={health?.storage.status} value={health?.storage.status === 'UP' ? 'Available' : 'Unavailable'} detail={health?.storage.detail ?? 'Probe pending'} />
          <HealthCard icon={<Radio className="w-5 h-5 text-rose-400" />} title="MQTT" status={health?.mqtt.status} value={health?.mqtt.status === 'UP' ? 'Subscribed' : health?.mqtt.status === 'DOWN' ? 'Disconnected' : 'Unknown'} detail={health?.mqtt.lastChangedAt ? `Last state change: ${new Date(health.mqtt.lastChangedAt).toLocaleString()}` : 'No connection event observed yet'} />

          <div className="md:col-span-2 lg:col-span-4 bg-zinc-950/50 border border-zinc-800 p-6 rounded-3xl shadow-lg">
            <div className="flex justify-between items-start mb-5">
              <Cpu className="w-5 h-5 text-emerald-400" />
              <StatusBadge status={health?.device.status} />
            </div>
            <h2 className="text-zinc-500 text-sm font-bold uppercase tracking-widest mb-3">Doorbell Device</h2>
            {health?.device.status === 'KNOWN' ? (
              <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 text-sm">
                <Metric label="Device" value={health.device.deviceId ?? 'Not reported'} />
                <Metric label="Last contact" value={health.device.lastSeen ? new Date(health.device.lastSeen).toLocaleString() : 'Not reported'} />
                <Metric label="Firmware" value={health.device.firmwareVersion ?? 'Not reported'} />
                <Metric label="Last event" value={health.device.lastEventType?.replaceAll('_', ' ') ?? 'Not reported'} />
                <Metric label="Wi-Fi RSSI at contact" value={health.device.wifiRssiDbm !== null && health.device.wifiRssiDbm !== undefined ? `${health.device.wifiRssiDbm} dBm` : 'Not reported'} />
                <Metric label="Battery" value="Not reported" />
              </div>
            ) : (
              <p className="text-zinc-400">No production firmware telemetry has reached this gateway yet.</p>
            )}
            <p className="text-xs text-zinc-500 mt-5 font-mono">Device contact is event-driven, not a claim that the sleeping doorbell is continuously online. Battery remains unknown until trustworthy telemetry is available.</p>
          </div>
        </div>
      </div>
    </MainLayout>
  );
}

function HealthCard({ icon, title, status, value, detail }: { icon: React.ReactNode; title: string; status?: string; value: string; detail: string }) {
  return (
    <div className="bg-zinc-950/50 border border-zinc-800 p-6 rounded-3xl shadow-lg">
      <div className="flex justify-between items-start mb-6">{icon}<StatusBadge status={status} /></div>
      <h2 className="text-zinc-500 text-sm font-bold uppercase tracking-widest mb-1">{title}</h2>
      <p className="text-2xl font-black text-white">{value}</p>
      <p className="text-xs text-zinc-500 mt-2 font-mono">{detail}</p>
    </div>
  );
}

function Metric({ label, value }: { label: string; value: string }) {
  return <div className="bg-zinc-900/40 border border-zinc-800 rounded-xl p-3"><span className="text-zinc-500 block text-xs uppercase tracking-wider mb-1">{label}</span><span className="text-zinc-200 font-mono">{value}</span></div>;
}
