'use client';

import { useState } from 'react';
import Sidebar from './Sidebar';
import { Menu, X } from 'lucide-react';

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
  const [isMobileMenuOpen, setIsMobileMenuOpen] = useState(false);

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
    <div className="min-h-screen bg-zinc-950 text-zinc-100 lg:flex font-sans">
      
      {/* Mobile Header */}
      <div className="lg:hidden flex items-center justify-between px-4 h-16 border-b border-zinc-800 bg-zinc-950 sticky top-0 z-40">
        <div className="flex items-center gap-3">
          <button 
            onClick={() => setIsMobileMenuOpen(true)}
            className="p-2 -ml-2 text-zinc-400 hover:text-zinc-100 transition-colors"
          >
            <Menu className="w-6 h-6" />
          </button>
          <span className="font-black text-lg tracking-tight text-zinc-100">Smart Doorbell</span>
        </div>
        
        {/* Status Indicator (Compact) */}
        <div className="flex items-center gap-2 bg-zinc-900 border border-zinc-800 px-3 py-1.5 rounded-full shadow-inner">
          <div className="relative flex h-2 w-2">
            {status !== 'disconnected' && <span className={`animate-ping absolute inline-flex h-full w-full rounded-full opacity-75 ${getPingColor()}`}></span>}
            <span className={`relative inline-flex rounded-full h-2 w-2 ${getDotColor()}`}></span>
          </div>
        </div>
      </div>

      {/* Mobile Navigation Drawer */}
      {isMobileMenuOpen && (
        <div className="lg:hidden fixed inset-0 z-50 flex">
          <div 
            className="absolute inset-0 bg-black/60 backdrop-blur-sm"
            onClick={() => setIsMobileMenuOpen(false)}
          />
          <div className="relative w-64 h-full bg-zinc-950 border-r border-zinc-800 shadow-2xl flex flex-col">
            <div className="h-16 flex items-center justify-between px-4 border-b border-zinc-800 shrink-0">
              <span className="font-black text-lg tracking-tight text-zinc-100 pl-2">Menu</span>
              <button 
                onClick={() => setIsMobileMenuOpen(false)}
                className="p-2 text-zinc-400 hover:text-zinc-100"
              >
                <X className="w-6 h-6" />
              </button>
            </div>
            <div className="overflow-y-auto flex-1">
              <Sidebar onNavigate={() => setIsMobileMenuOpen(false)} />
            </div>
          </div>
        </div>
      )}

      {/* Desktop Sidebar wrapper */}
      <div className="hidden lg:flex w-64 shrink-0 h-screen sticky top-0">
        <Sidebar />
      </div>

      {/* Main Content Area */}
      <main className="flex-1 bg-zinc-900 min-h-[calc(100vh-4rem)] lg:min-h-screen lg:my-3 lg:mr-4 lg:rounded-l-[2rem] border-l border-t border-b border-zinc-800 shadow-2xl relative flex flex-col">
        <div className="absolute inset-0 bg-grid opacity-[0.35] pointer-events-none z-0"></div>
        
        {/* Desktop Header */}
        <header className="hidden lg:flex h-20 border-b border-zinc-800/50 justify-between items-center px-10 relative z-10 bg-zinc-900/50 backdrop-blur-sm shrink-0">
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

        {/* Content Box */}
        <div className="px-4 py-4 sm:px-6 lg:p-10 flex-1 relative z-10 flex flex-col">
          {children}
        </div>
      </main>
    </div>
  );
}
