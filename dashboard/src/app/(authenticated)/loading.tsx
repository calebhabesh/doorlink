export default function Loading() {
  return (
    <div
      aria-label="Loading page content"
      className="flex-1 flex flex-col items-center justify-center min-h-[300px] w-full"
    >
      <div className="flex flex-col items-center gap-3">
        <div className="relative flex h-8 w-8 items-center justify-center">
          <div className="h-7 w-7 animate-spin motion-reduce:animate-none rounded-full border-2 border-emerald-500 border-t-transparent" />
        </div>
        <p className="font-mono text-xs uppercase tracking-widest text-zinc-500">
          Loading...
        </p>
      </div>
    </div>
  );
}
