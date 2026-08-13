'use client';

import { FormEvent, useState } from 'react';
import { useRouter } from 'next/navigation';
import LogoIcon from '../../components/LogoIcon';
import { KeyRound } from 'lucide-react';

export default function SetupPage() {
  const router = useRouter();
  const [error, setError] = useState<string | null>(null);
  const [submitting, setSubmitting] = useState(false);

  async function submit(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    setSubmitting(true);
    setError(null);
    const data = new FormData(event.currentTarget);
    const response = await fetch('/api/household/bootstrap', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        setupToken: data.get('setupToken'),
        name: data.get('name'),
        email: data.get('email'),
        deviceName: data.get('deviceName'),
      }),
    });
    if (!response.ok) {
      const body = await response.json().catch(() => ({ error: 'Setup failed' }));
      setError(body.error ?? 'Setup failed');
      setSubmitting(false);
      return;
    }
    router.replace('/');
    router.refresh();
  }

  return (
    <main className="grid min-h-screen place-items-center bg-zinc-950 px-5 py-10 text-zinc-100">
      <section className="w-full max-w-lg rounded-3xl border border-zinc-800 bg-zinc-900/70 p-8 shadow-2xl">
        <LogoIcon className="mb-6 h-14 w-14" />
        <div className="mb-3 flex items-center gap-3 text-emerald-400"><KeyRound className="h-5 w-5" /><span className="text-xs font-black uppercase tracking-[0.2em]">One-time setup</span></div>
        <h1 className="text-3xl font-black">Create the household owner</h1>
        <p className="mt-3 text-sm leading-6 text-zinc-400">Enter the bootstrap token configured on the gateway. This browser becomes your first revocable device.</p>
        <form onSubmit={submit} className="mt-7 space-y-4">
          <Field name="name" label="Your name" autoComplete="name" required />
          <Field name="email" label="Email" type="email" autoComplete="email" required />
          <Field name="deviceName" label="Device name" placeholder="Caleb’s iPhone" required />
          <Field name="setupToken" label="Bootstrap token" type="password" autoComplete="off" required />
          {error && <p className="rounded-xl border border-rose-500/30 bg-rose-500/10 p-3 text-sm text-rose-300">{error}</p>}
          <button disabled={submitting} className="w-full rounded-xl bg-emerald-500 px-5 py-3.5 font-black text-zinc-950 hover:bg-emerald-400 disabled:opacity-50">
            {submitting ? 'Setting up…' : 'Set up Doorlink'}
          </button>
        </form>
      </section>
    </main>
  );
}

function Field(props: React.InputHTMLAttributes<HTMLInputElement> & { label: string }) {
  const { label, ...input } = props;
  return <label className="block text-sm font-bold text-zinc-300">{label}<input {...input} className="mt-2 w-full rounded-xl border border-zinc-700 bg-zinc-950 px-4 py-3 text-zinc-100 outline-none focus:border-emerald-500" /></label>;
}
