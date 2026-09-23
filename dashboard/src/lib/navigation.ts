import {
  LayoutDashboard,
  Database,
  CalendarDays,
  Activity,
  Settings as SettingsIcon,
  Users,
  type LucideIcon,
} from 'lucide-react';

export interface NavRoute {
  label: string;
  href: string;
  icon: LucideIcon;
}

export const NAV_ROUTES: NavRoute[] = [
  { label: 'Dashboard', href: '/', icon: LayoutDashboard },
  { label: 'Event Log', href: '/events', icon: Database },
  { label: 'Calendar View', href: '/calendar', icon: CalendarDays },
  { label: 'System Health', href: '/health', icon: Activity },
  { label: 'Settings', href: '/settings', icon: SettingsIcon },
  { label: 'Household', href: '/household', icon: Users },
];

export function getRouteByPathname(pathname: string): NavRoute {
  if (pathname === '/') return NAV_ROUTES[0];
  const matched = NAV_ROUTES.find((item) => item.href !== '/' && (pathname === item.href || pathname.startsWith(`${item.href}/`)));
  return matched ?? NAV_ROUTES[0];
}
