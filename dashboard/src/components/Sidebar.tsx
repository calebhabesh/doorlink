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
      <div className="h-20 flex items-center px-6">
        <svg className="w-8 h-8 text-blue-500 mr-3" fill="currentColor" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 12.2 25.5">
          <path d="M6.938 6.586a.852.852 0 0 1-.852.851a.852.852 0 0 1-.852-.851a.852.852 0 0 1 .852-.852a.852.852 0 0 1 .851.852M9.08 16.54c0 2.673-3.231 4.011-5.121 2.121c-1.89-1.89-.552-5.121 2.121-5.121a3 3 0 0 1 3 3m-3-2.54c-2.227 0-3.343 2.733-1.768 4.308c1.575 1.575 4.268.46 4.268-1.768A2.5 2.5 0 0 0 6.08 14m0 .54c-1.782 0-2.674 2.154-1.414 3.414c1.26 1.26 3.414.368 3.414-1.414a2 2 0 0 0-2-2M-.008 25.381l.074.067l.098.058l1.976.004c-.119-5.397-.045-10.793-.068-16.19c-.66 0-1.406.023-2.066.023ZM12.203 9.314c-.683 0-1.39-.004-2.073-.004V25.5h1.61l.27.004l.08-.004l.07-.08l.047-.112c.024-5.304-.005-10.734-.004-15.994M2.14 25.51l8-.01v-.73l-8.02-.008M8.35 4.81v3.65h-4.6V4.81Zm-2.27.3c-1.336 0-2.006 1.616-1.06 2.56c.944.946 2.56.276 2.56-1.06a1.5 1.5 0 0 0-1.5-1.5m-2.33-.3a3 3 0 0 0-.63 1.8a3.56 3.56 0 0 0 .63 1.85Zm4.6 3.65A3.75 3.75 0 0 0 9 6.61a3.02 3.02 0 0 0-.65-1.8Zm-8.349.418l2.046-.01l.033-4.358l-2.072.003ZM12.204-4.37l-2.083-.007l.016 4.37l2.065.005zM0 4.08l2.08-.009V0L-.001.008ZM12.21-.003L10.08 0v4.07h2l.123-.004ZM2.08 4.07h8V0h-8zm-.008 6.687l8.07.058l-.012-1.505l-8.058.01ZM2.62 4.51h-.29l-.003 4.352h.282Zm7.22 0h-.29v4.36h.33z"/>
        </svg>
        <span className="font-bold text-xl tracking-wide text-zinc-100">Smart Doorbell</span>
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
