export const dynamic = 'force-dynamic';

export async function GET(request: Request) {
  const gatewayUrl = process.env.GATEWAY_URL || 'http://127.0.0.1:8080';
  const backendUrl = `${gatewayUrl}/api/events/stream`;

  try {
    const response = await fetch(backendUrl, {
      headers: {
        Accept: "text/event-stream",
        Cookie: request.headers.get('cookie') ?? '',
      },
      cache: "no-store",
    });

    if (!response.ok) {
      return new Response("Household device authentication required", {
        status: response.status,
      });
    }

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
