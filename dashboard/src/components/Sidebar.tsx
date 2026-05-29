'use client';

import { usePathname } from 'next/navigation';
import Link from 'next/link';
import { LayoutDashboard, Database, CalendarDays, Activity, Settings } from 'lucide-react';
import LogoIcon from './LogoIcon';

interface SidebarProps {
  onNavigate?: () => void;
}

export default function Sidebar({ onNavigate }: SidebarProps) {
  const pathname = usePathname();

  const menuItems = [
    { label: 'Dashboard', icon: LayoutDashboard, href: '/' },
    { label: 'Event Log', icon: Database, href: '/events' },
    { label: 'Calendar View', icon: CalendarDays, href: '/calendar' },
    { label: 'System Health', icon: Activity, href: '/health' },
    { label: 'Settings', icon: Settings, href: '/settings' },
  ];

  return (
    <aside className="w-full lg:w-64 bg-zinc-950 flex flex-col h-full lg:h-screen shrink-0 z-20">
      {/* Brand Header */}
      <div className="hidden lg:flex h-20 items-center px-6 border-b border-zinc-800 shrink-0">
        <div className="relative mr-3 w-12 h-12 flex items-center justify-center shrink-0">
          {/* Radial gradient glow behind the logo */}
          <div className="absolute inset-0 bg-blue-500/20 blur-lg rounded-full pointer-events-none scale-110 animate-pulse" />
          {/* Logo with drop-shadow glow */}
          <LogoIcon className="w-11 h-11 relative z-10 filter drop-shadow-[0_0_12px_rgba(59,130,246,0.7)] hover:scale-105 transition-transform duration-300" />
        </div>
        <span className="font-sora font-extrabold text-xl tracking-tight text-zinc-100">Smart Doorbell</span>
      </div>
      <nav className="flex-1 py-6 flex flex-col gap-2">
        {menuItems.map((item) => {
          const Icon = item.icon;
          const isActive = pathname === item.href;
          return (
            <Link
              key={item.label}
              href={item.href}
              onClick={onNavigate}
              className={`flex items-center px-6 py-4 transition-all border-l-4 ${
                isActive
                  ? 'bg-emerald-500/10 border-emerald-500 text-emerald-500'
                  : 'text-zinc-400 border-transparent hover:text-zinc-100 hover:bg-zinc-900'
              }`}
            >
              <Icon className="w-5 h-5 mr-4" />
              <span className="text-sm font-medium uppercase tracking-wider">{item.label}</span>
            </Link>
          );
        })}
      </nav>
    </aside>
  );
}
