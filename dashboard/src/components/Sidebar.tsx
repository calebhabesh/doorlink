'use client';

import { usePathname } from 'next/navigation';
import Link from 'next/link';
import { LayoutDashboard, Database, CalendarDays, Activity, Settings } from 'lucide-react';

export default function Sidebar() {
  const pathname = usePathname();

  const menuItems = [
    { label: 'Dashboard', icon: LayoutDashboard, href: '/' },
    { label: 'Event Log', icon: Database, href: '/events' },
    { label: 'Calendar View', icon: CalendarDays, href: '#' },
    { label: 'System Health', icon: Activity, href: '#' },
    { label: 'Settings', icon: Settings, href: '#' },
  ];

  return (
    <aside className="w-64 bg-zinc-950 flex flex-col h-screen shrink-0 z-20">
      {/* Brand Header */}
      <div className="h-20 flex items-center px-6 border-b border-zinc-800">
        <div className="mr-3 p-2 bg-blue-600/10 rounded-lg border border-blue-500/20">
          <svg className="w-6 h-6 text-blue-400" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" xmlns="http://www.w3.org/2000/svg">
            <path d="M14.5 4h-5L7 7H4a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2V9a2 2 0 0 0-2-2h-3l-2.5-3z"/>
            <circle cx="12" cy="13" r="3" className="text-zinc-100" fill="currentColor" fillOpacity="0.2"/>
            <path d="M7 11h.01" className="text-zinc-100"/>
          </svg>
        </div>
        <span className="font-black text-xl tracking-tight text-zinc-100">Smart Doorbell</span>
      </div>
      <nav className="flex-1 py-6 flex flex-col gap-2">
        {menuItems.map((item) => {
          const Icon = item.icon;
          const isActive = pathname === item.href;
          return (
            <Link
              key={item.label}
              href={item.href}
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
