import { NextResponse } from 'next/server';
import type { NextRequest } from 'next/server';

function hasSameOrigin(request: NextRequest, origin: string): boolean {
  try {
    const originUrl = new URL(origin);
    const requestHost = request.headers.get('x-forwarded-host') ?? request.headers.get('host');
    const forwardedProtocol = request.headers.get('x-forwarded-proto')?.split(',')[0].trim();
    const requestProtocol = forwardedProtocol ? `${forwardedProtocol}:` : request.nextUrl.protocol;
    return originUrl.host === requestHost && originUrl.protocol === requestProtocol;
  } catch {
    return false;
  }
}

export function middleware(request: NextRequest) {
  const path = request.nextUrl.pathname;

  if (path.startsWith('/api')) {
    const gatewayReadOnly = process.env.GATEWAY_READ_ONLY === 'true';
    const methodIsReadOnly = request.method === 'GET'
      || request.method === 'HEAD'
      || request.method === 'OPTIONS';

    if (gatewayReadOnly && !methodIsReadOnly) {
      return NextResponse.json(
        { error: 'The development gateway proxy is read-only.' },
        {
          status: 405,
          headers: { Allow: 'GET, HEAD, OPTIONS' },
        },
      );
    }

    const origin = request.headers.get('origin');
    if (origin && !hasSameOrigin(request, origin)) {
      return new NextResponse('Invalid CORS request', { status: 403 });
    }

    const backendUrl = process.env.GATEWAY_URL || 'http://127.0.0.1:8080';
    const url = new URL(request.nextUrl.pathname + request.nextUrl.search, backendUrl);

    const requestHeaders = new Headers(request.headers);
    if (gatewayReadOnly) {
      requestHeaders.delete('X-API-Key');
    } else {
      const apiKey = process.env.GATEWAY_API_KEY || 'default-dev-api-key';
      requestHeaders.set('X-API-Key', apiKey);
    }
    // The browser talks to this same-origin proxy, not directly to Spring.
    // Do not make Spring apply browser CORS policy to the internal rewrite.
    requestHeaders.delete('origin');

    return NextResponse.rewrite(url, {
      request: {
        headers: requestHeaders,
      },
    });
  }

  return NextResponse.next();
}

export const config = {
  matcher: '/api/:path*',
};
