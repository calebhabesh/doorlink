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
        <div className="mr-3 p-2 bg-blue-600/10 rounded-lg border border-blue-500/20 shadow-[0_0_15px_rgba(59,130,246,0.1)]">
          <svg className="w-8 h-8" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256">
            <defs>
              <linearGradient id="doorbell-gradient" x1="1" y1="0" x2="0" y2="1">
                <stop offset="50%" stopColor="#2563eb" /> {/* blue-600 */}
                <stop offset="50%" stopColor="#f4f4f5" /> {/* zinc-100 */}
              </linearGradient>
            </defs>
            <g>
              <path d="m221.66 85.66l-120 120a8 8 0 0 1-11.32 0L52.69 168L184 36.69l37.66 37.65a8 8 0 0 1 0 11.32" fill="url(#doorbell-gradient)"/>
              <path d="M248 136a8 8 0 0 0-8 8v16h-44.69L177 141.66l50.34-50.35a16 16 0 0 0 0-22.62l-56-56a16 16 0 0 0-22.63 0L2.92 158.94A10 10 0 0 0 10 176h39.37l35.32 35.31a16 16 0 0 0 22.62 0L165.66 153L184 171.31a15.86 15.86 0 0 0 11.31 4.69H240v16a8 8 0 0 0 16 0v-48a8 8 0 0 0-8-8M160 24l12.69 12.69L49.37 160H24.46ZM96 200l-32-32L184 48l32 32Z" className="fill-zinc-100/10 stroke-zinc-100/20" strokeWidth="1"/>
            </g>
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
