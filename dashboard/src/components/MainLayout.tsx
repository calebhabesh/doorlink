'use client';

import Sidebar from './Sidebar';

export default function MainLayout({ 
  children, 
  breadcrumbs, 
  isConnected 
}: { 
  children: React.ReactNode;
  breadcrumbs: { label: string; active?: boolean }[];
  isConnected: boolean;
}) {
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
              {isConnected && <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>}
              <span className={`relative inline-flex rounded-full h-3 w-3 ${isConnected ? 'bg-emerald-500' : 'bg-red-500'}`}></span>
            </div>
            <span className={`font-bold uppercase ${isConnected ? 'text-emerald-500' : 'text-red-500'}`}>
              {isConnected ? 'SYSTEM LIVE' : 'DISCONNECTED'}
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
