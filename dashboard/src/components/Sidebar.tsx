'use client';

import { usePathname, useRouter } from 'next/navigation';
import Link from 'next/link';
import { LogOut } from 'lucide-react';
import LogoIcon from './LogoIcon';
import { NAV_ROUTES } from '../lib/navigation';
import { useSessionContext } from '../context/SessionContext';

interface SidebarProps {
  onNavigate?: () => void;
}

export default function Sidebar({ onNavigate }: SidebarProps) {
  const pathname = usePathname();
  const router = useRouter();
  const { clearPrivateData } = useSessionContext();

  async function logout() {
    clearPrivateData();
    try {
      const response = await fetch('/api/household/session', { method: 'DELETE' });
      if (!response.ok) {
        const body = await response.json().catch(() => ({ error: 'Could not sign out this device' }));
        window.alert(body.error ?? 'Could not sign out this device');
        return;
      }
    } catch {
      // Continue cleanup and redirect
    }
    router.replace('/access');
    router.refresh();
  }

  return (
    <aside className="w-full lg:w-64 bg-zinc-950 flex flex-col h-full lg:h-screen shrink-0 z-20">
      {/* Brand Header */}
      <div className="hidden lg:flex h-20 items-center px-6 border-b border-zinc-800 shrink-0">
        <div className="relative mr-3 w-12 h-12 flex items-center justify-center shrink-0">
          {/* Radial gradient glow behind the logo */}
          <div className="absolute inset-0 bg-blue-500/20 blur-lg rounded-full pointer-events-none scale-110 motion-reduce:animate-none animate-pulse" />
          {/* Logo with drop-shadow glow */}
          <LogoIcon className="w-11 h-11 relative z-10 filter drop-shadow-[0_0_12px_rgba(59,130,246,0.7)] hover:scale-105 transition-transform duration-300" />
        </div>
        <span className="font-grotesk font-semibold text-3xl tracking-tight text-zinc-100">Doorlink</span>
      </div>

      <nav className="flex-1 py-6 flex flex-col gap-2">
        {NAV_ROUTES.map((item) => {
          const Icon = item.icon;
          const isActive = item.href === '/'
            ? pathname === '/'
            : (pathname === item.href || pathname.startsWith(`${item.href}/`));

          return (
            <Link
              key={item.label}
              href={item.href}
              onClick={onNavigate}
              className={`flex items-center px-6 py-4 transition-all border-l-4 rounded-r-lg focus-visible:ring-2 focus-visible:ring-emerald-500 focus-visible:outline-none ${
                isActive
                  ? 'bg-emerald-500/10 border-emerald-500 text-emerald-400 font-bold'
                  : 'text-zinc-400 border-transparent hover:text-zinc-100 hover:bg-zinc-900/60'
              }`}
            >
              <Icon className="w-5 h-5 mr-4 shrink-0" />
              <span className="text-sm font-medium uppercase tracking-wider">{item.label}</span>
            </Link>
          );
        })}
      </nav>

      <button
        type="button"
        onClick={logout}
        className="flex items-center border-t border-zinc-800 px-6 py-5 text-sm font-medium uppercase tracking-wider text-zinc-500 hover:bg-zinc-900 hover:text-zinc-200 transition-colors focus-visible:ring-2 focus-visible:ring-rose-500 focus-visible:outline-none"
      >
        <LogOut className="mr-4 h-5 w-5 shrink-0" /> Sign out this device
      </button>
    </aside>
  );
}
