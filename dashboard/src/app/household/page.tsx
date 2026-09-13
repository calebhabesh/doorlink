'use client';

import { FormEvent, useCallback, useEffect, useState } from 'react';
import MainLayout from '../../components/MainLayout';
import { Check, Copy, Link2, Pencil, ShieldCheck, Smartphone, UserPlus, Users, X } from 'lucide-react';

type Device = {
  id: number;
  name: string;
  createdAt: string;
  lastSeenAt: string;
  expiresAt: string;
  revokedAt: string | null;
  userAgent: string | null;
};

type Member = {
  id: number;
  name: string;
  email: string;
  role: 'OWNER' | 'MEMBER';
  createdAt: string;
  disabledAt: string | null;
  devices: Device[];
};

type Invitation = {
  memberId: number;
  name: string;
  email: string;
  enrollmentUrl: string;
  expiresAt: string;
};

type Session = {
  role: 'OWNER' | 'MEMBER';
};

export default function HouseholdPage() {
  const [members, setMembers] = useState<Member[]>([]);
  const [isOwner, setIsOwner] = useState(false);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [invitation, setInvitation] = useState<Invitation | null>(null);
  const [copied, setCopied] = useState(false);
  const [editingDeviceId, setEditingDeviceId] = useState<number | null>(null);
  const [editingName, setEditingName] = useState('');
  const [savingDeviceId, setSavingDeviceId] = useState<number | null>(null);

  const load = useCallback(async () => {
    const [membersResponse, sessionResponse] = await Promise.all([
      fetch('/api/household/members', { cache: 'no-store' }),
      fetch('/api/household/session', { cache: 'no-store' }),
    ]);
    if (!membersResponse.ok || !sessionResponse.ok) {
      const failedResponse = !membersResponse.ok ? membersResponse : sessionResponse;
      const body = await failedResponse.json().catch(() => ({ error: 'Could not load household' }));
      throw new Error(body.error ?? 'Could not load household');
    }
    const [loadedMembers, session]: [Member[], Session] = await Promise.all([
      membersResponse.json(),
      sessionResponse.json(),
    ]);
    setMembers(loadedMembers);
    setIsOwner(session.role === 'OWNER');
  }, []);

  useEffect(() => {
    load().catch(reason => setError(reason.message)).finally(() => setLoading(false));
  }, [load]);

  async function addMember(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    setError(null);
    const form = event.currentTarget;
    const data = new FormData(form);
    const response = await fetch('/api/household/members', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ name: data.get('name'), email: data.get('email') }),
    });
    const body = await response.json().catch(() => ({}));
    if (!response.ok) {
      setError(body.error ?? 'Could not add household member');
      return;
    }
    form.reset();
    setInvitation(body);
    await load();
  }

  async function newDeviceLink(memberId: number) {
    setError(null);
    const response = await fetch(`/api/household/members/${memberId}/invitations`, { method: 'POST' });
    const body = await response.json().catch(() => ({}));
    if (!response.ok) return setError(body.error ?? 'Could not create enrollment link');
    setInvitation(body);
    setCopied(false);
  }

  function startEditingDevice(device: Device) {
    setEditingDeviceId(device.id);
    setEditingName(device.name);
    setError(null);
  }

  function cancelEditingDevice() {
    setEditingDeviceId(null);
    setEditingName('');
  }

  async function handleRenameDevice(event: FormEvent<HTMLFormElement>, deviceId: number) {
    event.preventDefault();
    const trimmed = editingName.trim();
    if (!trimmed) return;
    setSavingDeviceId(deviceId);
    setError(null);
    try {
      const response = await fetch(`/api/household/devices/${deviceId}`, {
        method: 'PATCH',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ name: trimmed }),
      });
      const body = await response.json().catch(() => ({}));
      if (!response.ok) {
        setError(body.error ?? 'Could not rename device');
        return;
      }
      setEditingDeviceId(null);
      setEditingName('');
      await load();
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Could not rename device');
    } finally {
      setSavingDeviceId(null);
    }
  }

  async function revokeDevice(deviceId: number) {
    if (!window.confirm('Revoke this device now? It will need a new enrollment link to regain access.')) return;
    const response = await fetch(`/api/household/devices/${deviceId}`, { method: 'DELETE' });
    if (!response.ok) {
      const body = await response.json().catch(() => ({ error: 'Could not revoke device' }));
      return setError(body.error ?? 'Could not revoke device');
    }
    await load();
  }

  async function disableMember(member: Member) {
    if (!window.confirm(`Remove ${member.name} and revoke every device?`)) return;
    const response = await fetch(`/api/household/members/${member.id}`, { method: 'DELETE' });
    if (!response.ok) return setError('Could not remove household member');
    await load();
  }

  async function copyInvitation() {
    if (!invitation) return;
    await navigator.clipboard.writeText(invitation.enrollmentUrl);
    setCopied(true);
  }

  return (
    <MainLayout status="connected" breadcrumbs={[{ label: 'Dashboard' }, { label: 'Household', active: true }]}>
      <div className="mx-auto w-full max-w-5xl space-y-6">
        <header className="flex items-start gap-4">
          <div className="rounded-2xl border border-zinc-800 bg-zinc-950 p-3 text-emerald-400"><Users className="h-6 w-6" /></div>
          <div><h1 className="text-2xl font-black text-white">Household Access</h1><p className="mt-1 text-sm text-zinc-400">{isOwner ? 'Enroll browsers once, then revoke individual devices whenever needed.' : 'People who have access to this doorbell.'}</p></div>
        </header>

        {error && <div className="flex items-center justify-between rounded-2xl border border-rose-500/30 bg-rose-500/10 p-4 text-sm text-rose-300"><span>{error}</span><button onClick={() => setError(null)}><X className="h-4 w-4" /></button></div>}

        {isOwner && <form onSubmit={addMember} className="grid gap-4 rounded-3xl border border-zinc-800 bg-zinc-950/60 p-6 sm:grid-cols-[1fr_1.4fr_auto] sm:items-end">
          <label className="text-sm font-bold text-zinc-300">Name<input name="name" required maxLength={100} className="mt-2 w-full rounded-xl border border-zinc-700 bg-zinc-900 px-4 py-3 outline-none focus:border-emerald-500" /></label>
          <label className="text-sm font-bold text-zinc-300">Email<input name="email" type="email" required className="mt-2 w-full rounded-xl border border-zinc-700 bg-zinc-900 px-4 py-3 outline-none focus:border-emerald-500" /></label>
          <button className="inline-flex items-center justify-center gap-2 rounded-xl bg-emerald-500 px-5 py-3 font-black text-zinc-950 hover:bg-emerald-400"><UserPlus className="h-4 w-4" /> Add member</button>
        </form>}

        {loading ? <p className="py-12 text-center font-mono text-xs uppercase tracking-widest text-zinc-500">Loading household…</p> : (
          <div className="space-y-4">
            {members.map(member => <article key={member.id} className={`rounded-3xl border border-zinc-800 bg-zinc-950/60 p-6 ${member.disabledAt ? 'opacity-55' : ''}`}>
              <div className="flex flex-col justify-between gap-4 sm:flex-row sm:items-start">
                <div>
                  <div className="flex items-center gap-2"><h2 className="text-lg font-black text-white">{member.name}</h2>{member.role === 'OWNER' && <span className="inline-flex items-center gap-1 rounded-full bg-blue-500/10 px-2.5 py-1 text-[10px] font-black uppercase tracking-wider text-blue-300"><ShieldCheck className="h-3 w-3" /> Owner</span>}</div>
                  <p className="mt-1 text-sm text-zinc-500">{member.email}</p>
                </div>
                {isOwner && !member.disabledAt && <div className="flex flex-wrap gap-2">
                  <button onClick={() => newDeviceLink(member.id)} className="inline-flex items-center gap-2 rounded-xl border border-zinc-700 bg-zinc-900 px-4 py-2.5 text-xs font-bold text-zinc-200 hover:border-blue-500"><Link2 className="h-4 w-4" /> New device link</button>
                  {member.role !== 'OWNER' && <button onClick={() => disableMember(member)} className="rounded-xl border border-rose-500/30 px-4 py-2.5 text-xs font-bold text-rose-300 hover:bg-rose-500/10">Remove member</button>}
                </div>}
              </div>

              {isOwner && <div className="mt-5 space-y-2 border-t border-zinc-800 pt-4">
                {member.devices.length === 0 && <p className="text-sm text-zinc-600">No enrolled devices yet.</p>}
                {member.devices.map(device => (
                  <div key={device.id} className="flex flex-col justify-between gap-3 rounded-2xl bg-zinc-900/70 p-4 sm:flex-row sm:items-center">
                    {editingDeviceId === device.id ? (
                      <form onSubmit={(e) => handleRenameDevice(e, device.id)} className="flex w-full flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
                        <div className="flex flex-1 items-center gap-3">
                          <Smartphone className="h-5 w-5 shrink-0 text-emerald-400" />
                          <input
                            type="text"
                            value={editingName}
                            onChange={e => setEditingName(e.target.value)}
                            onKeyDown={e => { if (e.key === 'Escape') cancelEditingDevice(); }}
                            placeholder="Device name"
                            maxLength={100}
                            autoFocus
                            required
                            className="w-full max-w-md rounded-xl border border-zinc-700 bg-zinc-950 px-3.5 py-2 text-sm text-zinc-100 outline-none focus:border-emerald-500"
                          />
                        </div>
                        <div className="flex items-center gap-2 self-end sm:self-auto">
                          <button
                            type="submit"
                            disabled={savingDeviceId === device.id || !editingName.trim()}
                            className="inline-flex items-center gap-1.5 rounded-xl bg-emerald-500 px-3.5 py-2 text-xs font-bold text-zinc-950 hover:bg-emerald-400 disabled:opacity-50"
                          >
                            <Check className="h-3.5 w-3.5" />
                            {savingDeviceId === device.id ? 'Saving…' : 'Save'}
                          </button>
                          <button
                            type="button"
                            disabled={savingDeviceId === device.id}
                            onClick={cancelEditingDevice}
                            className="inline-flex items-center gap-1.5 rounded-xl border border-zinc-700 bg-zinc-900 px-3.5 py-2 text-xs font-bold text-zinc-300 hover:bg-zinc-800"
                          >
                            <X className="h-3.5 w-3.5" /> Cancel
                          </button>
                        </div>
                      </form>
                    ) : (
                      <>
                        <div className="flex min-w-0 items-start gap-3">
                          <Smartphone className="mt-0.5 h-5 w-5 shrink-0 text-zinc-500" />
                          <div className="min-w-0">
                            <p className="font-bold text-zinc-200">
                              {device.name}
                              {device.revokedAt && <span className="ml-2 rounded-full bg-rose-500/10 px-2 py-0.5 text-xs font-medium text-rose-400">Revoked</span>}
                            </p>
                            <p className="mt-1 truncate text-xs text-zinc-600">Last used {new Date(device.lastSeenAt).toLocaleString()}</p>
                          </div>
                        </div>
                        {!device.revokedAt && (
                          <div className="flex items-center gap-2 self-start sm:self-auto">
                            <button
                              onClick={() => startEditingDevice(device)}
                              className="inline-flex items-center gap-1.5 rounded-xl border border-zinc-700 bg-zinc-900 px-3 py-2 text-xs font-bold text-zinc-200 hover:border-emerald-500 hover:text-white transition-colors"
                            >
                              <Pencil className="h-3.5 w-3.5 text-zinc-400" /> Rename
                            </button>
                            <button
                              onClick={() => revokeDevice(device.id)}
                              className="rounded-xl border border-rose-500/30 px-3 py-2 text-xs font-bold text-rose-300 hover:bg-rose-500/10 transition-colors"
                            >
                              Revoke
                            </button>
                          </div>
                        )}
                      </>
                    )}
                  </div>
                ))}
              </div>}
            </article>)}
          </div>
        )}
      </div>

      {isOwner && invitation && <div className="fixed inset-0 z-50 grid place-items-center bg-black/70 px-5 backdrop-blur-sm">
        <section className="w-full max-w-lg rounded-3xl border border-zinc-700 bg-zinc-900 p-7 shadow-2xl">
          <div className="flex items-start justify-between"><div><p className="text-xs font-black uppercase tracking-[0.2em] text-emerald-400">Single-use enrollment link</p><h2 className="mt-2 text-xl font-black">Send this to {invitation.name}</h2></div><button onClick={() => setInvitation(null)} className="text-zinc-500 hover:text-white"><X /></button></div>
          <p className="mt-3 text-sm leading-6 text-zinc-400">They must open it on the phone and browser they want to authorize. The link expires {new Date(invitation.expiresAt).toLocaleString()}.</p>
          <div className="mt-5 break-all rounded-xl border border-zinc-700 bg-zinc-950 p-4 font-mono text-xs text-zinc-300">{invitation.enrollmentUrl}</div>
          <button onClick={copyInvitation} className="mt-4 inline-flex w-full items-center justify-center gap-2 rounded-xl bg-emerald-500 px-5 py-3.5 font-black text-zinc-950 hover:bg-emerald-400">{copied ? <Check className="h-4 w-4" /> : <Copy className="h-4 w-4" />}{copied ? 'Copied' : 'Copy enrollment link'}</button>
        </section>
      </div>}
    </MainLayout>
  );
}
