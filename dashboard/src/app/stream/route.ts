export const dynamic = 'force-dynamic';

export async function GET() {
  const backendUrl = "http://127.0.0.1:8080/api/events/stream";

  try {
    const response = await fetch(backendUrl, {
      headers: {
        Accept: "text/event-stream",
      },
      cache: "no-store",
    });

    return new Response(response.body, {
      headers: {
        "Content-Type": "text/event-stream",
        "Cache-Control": "no-cache, no-transform",
        "Connection": "keep-alive",
        "X-Accel-Buffering": "no",
      },
    });
  } catch {
    return new Response("Failed to connect to backend stream", { status: 502 });
  }
}
