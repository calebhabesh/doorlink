import React from 'react';

export default function AuthenticatedTemplate({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <div className="animate-page-enter flex-1 flex flex-col min-h-0 w-full">
      {children}
    </div>
  );
}
