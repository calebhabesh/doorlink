'use client';

import React, { useState } from 'react';
import { usePathname } from 'next/navigation';
import Sidebar from './Sidebar';
import { Menu, X, Battery, Bell, ArrowRight } from 'lucide-react';
import LogoIcon from './LogoIcon';
import { getRouteByPathname } from '../lib/navigation';
import { useSessionContext } from '../context/SessionContext';

export default function AuthenticatedShell({
  children,
}: {
  children: React.ReactNode;
}) {
  const [isMobileMenuOpen, setIsMobileMenuOpen] = useState(false);
  const pathname = usePathname();
  const currentRoute = getRouteByPathname(pathname);
  const {
    connectionStatus,
    batteryPercentage,
    newVisitorNotice,
    clearNewVisitorNotice,
    dismissNoticeAndSelect,
  } = useSessionContext();

  const getStatusColor = () => {
    switch (connectionStatus) {
      case 'connected': return 'text-emerald-400';
      case 'connecting': return 'text-amber-400';
      case 'disconnected': return 'text-rose-400';
      default: return 'text-rose-400';
    }
  };

  const getDotColor = () => {
    switch (connectionStatus) {
      case 'connected': return 'bg-emerald-400';
      case 'connecting': return 'bg-amber-400';
      case 'disconnected': return 'bg-rose-400';
      default: return 'bg-rose-400';
    }
  };

  const getPingColor = () => {
    switch (connectionStatus) {
      case 'connected': return 'bg-emerald-400';
      case 'connecting': return 'bg-amber-400';
      default: return '';
    }
  };

  const getBatteryColor = (pct: number) => {
    if (pct > 50) return 'text-emerald-400';
    if (pct > 20) return 'text-amber-400';
    return 'text-rose-400';
  };

  const statusLabel = connectionStatus === 'connected'
    ? 'Updates Live'
    : connectionStatus === 'connecting'
      ? 'Connecting...'
      : 'Disconnected';

  return (
    <div className="min-h-screen bg-zinc-950 text-zinc-100 lg:flex font-sans">
      {/* Mobile Header */}
      <div className="lg:hidden flex items-center justify-between px-4 h-16 border-b border-zinc-800 bg-zinc-950 sticky top-0 z-40">
        <div className="flex items-center gap-3">
          <button
            type="button"
            onClick={() => setIsMobileMenuOpen(true)}
            aria-label="Open navigation menu"
            className="p-2 -ml-2 text-zinc-400 hover:text-zinc-100 transition-colors focus-visible:ring-2 focus-visible:ring-emerald-500 focus-visible:outline-none rounded-lg"
          >
            <Menu className="w-6 h-6" />
          </button>
          <div className="flex items-center gap-2">
            <div className="relative w-8 h-8 flex items-center justify-center shrink-0">
              <div className="absolute inset-0 bg-blue-500/25 blur-md rounded-full pointer-events-none scale-110 motion-reduce:animate-none animate-pulse" />
              <LogoIcon className="w-7 h-7 relative z-10 filter drop-shadow-[0_0_8px_rgba(59,130,246,0.65)] hover:scale-110 transition-transform duration-300" />
            </div>
            {/* Semantic h1 on mobile that matches current route label */}
            <h1 className="font-grotesk font-semibold text-lg tracking-tight text-zinc-100">
              {currentRoute.label}
            </h1>
          </div>
        </div>

        {/* Mobile Status & Battery Indicators */}
        <div className="flex items-center gap-2">
          {batteryPercentage !== undefined && (
            <div
              className="flex items-center justify-center gap-1.5 bg-zinc-900 border border-zinc-800 h-7 px-2.5 rounded-full shadow-inner text-[10px] font-bold tracking-wider select-none text-zinc-400"
              title="Estimated from the doorbell's last reported battery voltage"
            >
              <Battery className={`w-3.5 h-3.5 ${getBatteryColor(batteryPercentage)}`} />
              <span className="text-zinc-100 font-mono leading-none">~{batteryPercentage}%</span>
            </div>
          )}

          <div className="flex items-center justify-center gap-2 bg-zinc-900 border border-zinc-800 h-7 px-3 rounded-full shadow-inner">
            <div className="relative flex h-2 w-2 shrink-0">
              {connectionStatus !== 'disconnected' && (
                <span className={`motion-reduce:animate-none animate-ping absolute inline-flex h-full w-full rounded-full opacity-75 ${getPingColor()}`} />
              )}
              <span className={`relative inline-flex rounded-full h-2 w-2 ${getDotColor()}`} />
            </div>
            <span className={`text-[10px] font-bold uppercase tracking-widest leading-none whitespace-nowrap ${getStatusColor()}`}>
              {statusLabel}
            </span>
          </div>
        </div>
      </div>

      {/* Mobile Navigation Drawer */}
      {isMobileMenuOpen && (
        <div className="lg:hidden fixed inset-0 z-50 flex">
          <div
            className="absolute inset-0 bg-black/60 backdrop-blur-sm animate-fade-in"
            onClick={() => setIsMobileMenuOpen(false)}
          />
          <div className="relative w-64 h-full bg-zinc-950 border-r border-zinc-800 shadow-2xl flex flex-col animate-slide-in-left">
            <div className="h-16 flex items-center justify-between px-4 border-b border-zinc-800 shrink-0">
              <span className="font-bold text-lg tracking-tight text-zinc-100 pl-2">Menu</span>
              <button
                type="button"
                onClick={() => setIsMobileMenuOpen(false)}
                aria-label="Close navigation menu"
                className="p-2 text-zinc-400 hover:text-zinc-100 focus-visible:ring-2 focus-visible:ring-emerald-500 focus-visible:outline-none rounded-lg"
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
      <main className="flex-1 bg-zinc-900 min-h-[calc(100vh-4rem)] lg:min-h-screen lg:rounded-tl-[2.5rem] border-l border-zinc-800 shadow-2xl relative flex flex-col overflow-hidden">
        <div className="absolute inset-0 bg-grid opacity-[0.35] pointer-events-none z-0" />

        {/* Desktop Header */}
        <header className="hidden lg:flex h-20 border-b border-zinc-800/50 justify-between items-center px-10 relative z-10 bg-zinc-900/50 backdrop-blur-sm shrink-0">
          <div className="flex items-center text-left">
            {/* Single semantic h1 matching sidebar label */}
            <h1 className="font-mono uppercase tracking-widest text-xl font-black text-zinc-100">
              {currentRoute.label}
            </h1>
          </div>

          <div className="flex items-center justify-center gap-3 bg-zinc-950 border border-zinc-800 h-10 px-5 rounded-lg shadow-inner font-mono tracking-wider text-sm">
            {batteryPercentage !== undefined && (
              <div
                className="flex items-center gap-2 border-r border-zinc-800 pr-4"
                title="Estimated from the doorbell's last reported battery voltage"
              >
                <Battery className={`h-4 w-4 ${getBatteryColor(batteryPercentage)}`} />
                <span className="font-bold text-zinc-100">~{batteryPercentage}%</span>
              </div>
            )}
            <div className="relative flex h-3 w-3 shrink-0">
              {connectionStatus !== 'disconnected' && (
                <span className={`motion-reduce:animate-none animate-ping absolute inline-flex h-full w-full rounded-full opacity-75 ${getPingColor()}`} />
              )}
              <span className={`relative inline-flex rounded-full h-3 w-3 ${getDotColor()}`} />
            </div>
            <span className={`font-bold uppercase leading-none whitespace-nowrap ${getStatusColor()}`}>
              {statusLabel}
            </span>
          </div>
        </header>

        {/* Global Quiet New Visitor Notice (clickable, no sound) */}
        {newVisitorNotice && (
          <aside
            aria-live="polite"
            className="relative z-20 mx-4 sm:mx-6 lg:mx-10 mt-4 rounded-2xl border border-emerald-500/30 bg-emerald-950/40 p-4 shadow-lg flex items-center justify-between gap-4 animate-slide-in"
          >
            <div className="flex items-center gap-3 min-w-0">
              <span className="grid h-8 w-8 shrink-0 place-items-center rounded-xl bg-emerald-500/20 text-emerald-400">
                <Bell className="h-4 w-4" />
              </span>
              <div className="min-w-0">
                <p className="text-sm font-bold text-zinc-100 truncate">
                  New visitor session started
                </p>
                <p className="font-mono text-xs text-zinc-400">
                  {newVisitorNotice.pressCount} {newVisitorNotice.pressCount === 1 ? 'press' : 'presses'} · {new Date(newVisitorNotice.startedAt).toLocaleTimeString([], { hour: 'numeric', minute: '2-digit' })}
                </p>
              </div>
            </div>
            <div className="flex items-center gap-2 shrink-0">
              <button
                type="button"
                onClick={() => dismissNoticeAndSelect(newVisitorNotice.sessionId)}
                className="inline-flex items-center gap-1.5 px-3.5 py-1.5 rounded-xl bg-emerald-500 text-zinc-950 font-bold text-xs hover:bg-emerald-400 transition-colors focus-visible:ring-2 focus-visible:ring-emerald-400 focus-visible:outline-none"
              >
                <span>View</span>
                <ArrowRight className="h-3.5 w-3.5" />
              </button>
              <button
                type="button"
                onClick={clearNewVisitorNotice}
                aria-label="Dismiss new visitor notice"
                className="p-1.5 rounded-lg text-zinc-400 hover:text-zinc-200 transition-colors focus-visible:ring-2 focus-visible:ring-zinc-400 focus-visible:outline-none"
              >
                <X className="h-4 w-4" />
              </button>
            </div>
          </aside>
        )}

        {/* Route Content Box */}
        <div className="px-4 py-4 sm:px-6 lg:p-10 flex-1 relative z-10 flex flex-col min-h-0">
          {children}
        </div>
      </main>
    </div>
  );
}
