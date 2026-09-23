import React from 'react';
import { SessionProvider } from '../../context/SessionContext';
import AuthenticatedShell from '../../components/AuthenticatedShell';

export default function AuthenticatedLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <SessionProvider>
      <AuthenticatedShell>{children}</AuthenticatedShell>
    </SessionProvider>
  );
}
