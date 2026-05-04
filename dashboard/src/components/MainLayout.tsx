'use client';

import Sidebar from './Sidebar';

export type ConnectionStatus = 'connected' | 'disconnected' | 'connecting';

export default function MainLayout({ 
  children, 
  breadcrumbs, 
  status = 'disconnected'
}: { 
  children: React.ReactNode;
  breadcrumbs: { label: string; active?: boolean }[];
  status?: ConnectionStatus;
}) {
  const getStatusColor = () => {
    switch (status) {
      case 'connected': return 'text-emerald-500';
      case 'connecting': return 'text-amber-500';
      case 'disconnected': return 'text-red-500';
      default: return 'text-red-500';
    }
  };

  const getDotColor = () => {
    switch (status) {
      case 'connected': return 'bg-emerald-500';
      case 'connecting': return 'bg-amber-500';
      case 'disconnected': return 'bg-red-500';
      default: return 'bg-red-500';
    }
  };

  const getPingColor = () => {
    switch (status) {
      case 'connected': return 'bg-emerald-400';
      case 'connecting': return 'bg-amber-400';
      default: return '';
    }
  };

  return (
    <div className="min-h-screen bg-zinc-950 text-zinc-100 flex font-sans overflow-hidden">
      <Sidebar />
      <main className="flex-1 bg-zinc-900 rounded-l-[2.5rem] border-l border-t border-b border-zinc-800 shadow-2xl relative overflow-hidden flex flex-col my-3 mr-4">
        <div className="absolute inset-0 bg-grid opacity-[0.35] pointer-events-none z-0"></div>
        
        <header className="h-20 border-b border-zinc-800/50 flex justify-between items-center px-10 relative z-10 bg-zinc-900/50 backdrop-blur-sm">
          <div className="flex items-center font-mono uppercase tracking-widest text-xl text-left">
            {breadcrumbs.map((crumb, idx) => (
              <span key={crumb.label} className="flex items-center">
                <span className={crumb.active ? 'text-zinc-100 font-black' : 'text-zinc-500'}>
                  {crumb.label}
                </span>
                {idx < breadcrumbs.length - 1 && <span className="mx-3 text-zinc-700">/</span>}
              </span>
            ))}
          </div>
          <div className="flex items-center gap-4 bg-zinc-950 border border-zinc-800 px-5 py-2.5 rounded-lg shadow-inner font-mono tracking-wider text-sm">
            <div className="relative flex h-3 w-3">
              {status !== 'disconnected' && <span className={`animate-ping absolute inline-flex h-full w-full rounded-full opacity-75 ${getPingColor()}`}></span>}
              <span className={`relative inline-flex rounded-full h-3 w-3 ${getDotColor()}`}></span>
            </div>
            <span className={`font-bold uppercase ${getStatusColor()}`}>
              {status === 'connected' ? 'SYSTEM LIVE' : status === 'connecting' ? 'CONNECTING...' : 'DISCONNECTED'}
            </span>
          </div>
        </header>

        <div className="p-10 flex-1 relative z-10 overflow-hidden flex flex-col">
          {children}
        </div>
      </main>
    </div>
  );
}
