import Link from 'next/link';
import LogoIcon from '../../components/LogoIcon';
import { ShieldX } from 'lucide-react';

export default function AccessRequired() {
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
        <Link href="/" className="mt-7 inline-flex rounded-xl bg-zinc-800 px-5 py-3 text-sm font-bold text-zinc-200 hover:bg-zinc-700">
          Try again
        </Link>
      </section>
    </main>
  );
}
