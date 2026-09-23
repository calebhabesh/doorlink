'use client';

import { FormEvent, useState } from 'react';
import { useRouter } from 'next/navigation';
import Link from 'next/link';
import LogoIcon from '../../components/LogoIcon';
import { ShieldX } from 'lucide-react';

export default function AccessRequired() {
  const router = useRouter();
  const [enrollmentLink, setEnrollmentLink] = useState('');
  const [error, setError] = useState<string | null>(null);

  function openEnrollment(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    try {
      const url = new URL(enrollmentLink.trim(), window.location.origin);
      const match = /^\/enroll\/([^/]+)\/?$/.exec(url.pathname);
      if (!match) throw new Error('Enter a Doorlink enrollment link.');
      router.push(`/enroll/${match[1]}`);
    } catch {
      setError('Enter a valid Doorlink enrollment link.');
    }
  }

  return (
    <main className="grid min-h-screen place-items-center bg-zinc-950 px-5 text-zinc-100">
      <section className="w-full max-w-md rounded-3xl border border-zinc-800 bg-zinc-900/70 p-8 text-center shadow-2xl">
        <LogoIcon className="mx-auto mb-6 h-16 w-16 drop-shadow-[0_0_18px_rgba(59,130,246,0.45)]" />
        <div className="mx-auto mb-4 grid h-12 w-12 place-items-center rounded-2xl bg-amber-500/10 text-amber-400">
          <ShieldX className="h-6 w-6" />
        </div>
        <h1 className="text-2xl font-black">This device needs an invitation</h1>
        <p className="mt-3 text-sm leading-6 text-zinc-400">
          Ask the household owner for a Doorlink enrollment link, then open it on this phone and browser. You only need to enroll once.
        </p>
        <form onSubmit={openEnrollment} className="mt-7 space-y-3 text-left">
          <label htmlFor="enrollment-link" className="block text-sm font-bold text-zinc-200">Enrollment link</label>
          <input
            id="enrollment-link"
            type="text"
            value={enrollmentLink}
            onChange={(event) => { setEnrollmentLink(event.target.value); setError(null); }}
            placeholder="Paste the link from Household"
            autoComplete="off"
            spellCheck={false}
            className="w-full rounded-xl border border-zinc-700 bg-zinc-950 px-4 py-3 text-sm text-zinc-100 outline-none focus-visible:ring-2 focus-visible:ring-emerald-500"
          />
          {error && <p role="alert" className="text-sm text-rose-300">{error}</p>}
          <button type="submit" disabled={!enrollmentLink.trim()} className="w-full rounded-xl bg-emerald-500 px-5 py-3 font-black text-zinc-950 hover:bg-emerald-400 disabled:opacity-50 focus-visible:ring-2 focus-visible:ring-emerald-500">
            Open enrollment here
          </button>
        </form>
        <p className="mt-4 text-xs leading-5 text-zinc-500">Links from the deployed site open on this local browser when pasted here.</p>
        <Link href="/" className="mt-4 inline-block text-sm font-semibold text-zinc-400 hover:text-zinc-100 focus-visible:rounded focus-visible:ring-2 focus-visible:ring-emerald-500">
          Try again
        </Link>
      </section>
    </main>
  );
}
