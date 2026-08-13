'use client';

import { FormEvent, useEffect, useState } from 'react';
import { useParams, useRouter } from 'next/navigation';
import LogoIcon from '../../../components/LogoIcon';
import { Smartphone } from 'lucide-react';

type Invitation = { name: string; email: string; expiresAt: string };

export default function EnrollmentPage() {
  const params = useParams<{ token: string }>();
  const router = useRouter();
  const [invitation, setInvitation] = useState<Invitation | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [submitting, setSubmitting] = useState(false);

  useEffect(() => {
    fetch(`/api/household/enroll/${encodeURIComponent(params.token)}`)
      .then(async response => {
        if (!response.ok) throw new Error((await response.json()).error ?? 'Invalid invitation');
        return response.json();
      })
      .then(setInvitation)
      .catch(reason => setError(reason.message));
  }, [params.token]);

  async function enroll(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    setSubmitting(true);
    setError(null);
    const data = new FormData(event.currentTarget);
    const response = await fetch(`/api/household/enroll/${encodeURIComponent(params.token)}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ deviceName: data.get('deviceName') }),
    });
    if (!response.ok) {
      const body = await response.json().catch(() => ({ error: 'Enrollment failed' }));
      setError(body.error ?? 'Enrollment failed');
      setSubmitting(false);
      return;
    }
    router.replace('/');
    router.refresh();
  }

  return (
    <main className="grid min-h-screen place-items-center bg-zinc-950 px-5 text-zinc-100">
      <section className="w-full max-w-md rounded-3xl border border-zinc-800 bg-zinc-900/70 p-8 shadow-2xl">
        <LogoIcon className="mb-6 h-14 w-14" />
        <div className="mb-3 flex items-center gap-3 text-blue-400"><Smartphone className="h-5 w-5" /><span className="text-xs font-black uppercase tracking-[0.2em]">Device enrollment</span></div>
        {invitation ? <>
          <h1 className="text-2xl font-black">Welcome, {invitation.name}</h1>
          <p className="mt-3 text-sm leading-6 text-zinc-400">Name this phone or browser. After this one step, ntfy links open Doorlink directly until this device is revoked.</p>
          <form onSubmit={enroll} className="mt-7 space-y-4">
            <label className="block text-sm font-bold text-zinc-300">Device name<input name="deviceName" placeholder="My iPhone" required maxLength={100} className="mt-2 w-full rounded-xl border border-zinc-700 bg-zinc-950 px-4 py-3 outline-none focus:border-blue-500" /></label>
            {error && <p className="text-sm text-rose-300">{error}</p>}
            <button disabled={submitting} className="w-full rounded-xl bg-blue-500 px-5 py-3.5 font-black text-white hover:bg-blue-400 disabled:opacity-50">{submitting ? 'Enrolling…' : 'Enroll this device'}</button>
          </form>
        </> : <p className={`text-sm ${error ? 'text-rose-300' : 'animate-pulse text-zinc-400'}`}>{error ?? 'Checking invitation…'}</p>}
      </section>
    </main>
  );
}
