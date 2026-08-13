import { NextResponse } from 'next/server';
import type { NextRequest } from 'next/server';

const DEVICE_COOKIE = 'doorlink_device';

function gatewayUrl(path: string): URL {
  return new URL(path, process.env.GATEWAY_URL || 'http://127.0.0.1:8080');
}

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

async function bootstrapRequired(): Promise<boolean> {
  try {
    const response = await fetch(gatewayUrl('/api/household/bootstrap/status'), {
      cache: 'no-store',
    });
    if (!response.ok) return false;
    const status: { needsBootstrap: boolean } = await response.json();
    return status.needsBootstrap;
  } catch {
    return false;
  }
}

async function checkDeviceSession(request: NextRequest): Promise<{ authorized: boolean; error?: boolean }> {
  const cookie = request.headers.get('cookie');
  if (!cookie || !request.cookies.has(DEVICE_COOKIE)) return { authorized: false, error: false };
  try {
    const response = await fetch(gatewayUrl('/api/household/session'), {
      headers: { cookie },
      cache: 'no-store',
    });
    if (response.ok) return { authorized: true, error: false };
    if (response.status === 401 || response.status === 403) {
      return { authorized: false, error: false };
    }
    return { authorized: false, error: true };
  } catch {
    return { authorized: false, error: true };
  }
}

function redirectWithCookieCleanup(request: NextRequest, destination: string): NextResponse {
  const response = NextResponse.redirect(new URL(destination, request.url));
  if (request.cookies.has(DEVICE_COOKIE)) {
    response.cookies.delete(DEVICE_COOKIE);
  }
  return response;
}

export async function middleware(request: NextRequest) {
  const path = request.nextUrl.pathname;

  if (path.startsWith('/api/')) {
    const gatewayReadOnly = process.env.GATEWAY_READ_ONLY === 'true';
    const methodIsReadOnly = request.method === 'GET'
      || request.method === 'HEAD'
      || request.method === 'OPTIONS';

    if (gatewayReadOnly && !methodIsReadOnly) {
      return NextResponse.json(
        { error: 'The development gateway proxy is read-only.' },
        { status: 405, headers: { Allow: 'GET, HEAD, OPTIONS' } },
      );
    }

    const origin = request.headers.get('origin');
    if (origin && !hasSameOrigin(request, origin)) {
      return new NextResponse('Invalid CORS request', { status: 403 });
    }

    const url = gatewayUrl(request.nextUrl.pathname + request.nextUrl.search);
    const requestHeaders = new Headers(request.headers);
    // Never turn an Internet-facing browser request into a hardware request.
    // The ESP32 talks to the gateway directly on the LAN with this header.
    requestHeaders.delete('X-API-Key');
    requestHeaders.delete('origin');

    return NextResponse.rewrite(url, { request: { headers: requestHeaders } });
  }

  // Enrollment links are single-use invitation tokens and always accessible
  if (path.startsWith('/enroll/')) {
    return NextResponse.next();
  }

  const session = await checkDeviceSession(request);

  // Authenticated / Enrolled browsers
  if (session.authorized) {
    if (path === '/setup' || path === '/access') {
      return NextResponse.redirect(new URL('/', request.url));
    }
    return NextResponse.next();
  }

  // Preserve cookie during transient gateway outages/restarts
  if (session.error) {
    return NextResponse.next();
  }

  // Unauthenticated / Revoked browsers: check whether first-time household setup is required
  const needsBootstrap = await bootstrapRequired();

  if (path === '/setup') {
    if (needsBootstrap) {
      return NextResponse.next();
    }
    return redirectWithCookieCleanup(request, '/access');
  }

  if (path === '/access') {
    if (needsBootstrap) {
      return redirectWithCookieCleanup(request, '/setup');
    }
    const response = NextResponse.next();
    if (request.cookies.has(DEVICE_COOKIE)) {
      response.cookies.delete(DEVICE_COOKIE);
    }
    return response;
  }

  // Any other protected route (e.g. /, /events, /calendar, /health, /settings, /household)
  const destination = needsBootstrap ? '/setup' : '/access';
  return redirectWithCookieCleanup(request, destination);
}

export const config = {
  matcher: ['/((?!_next/static|_next/image|icon.svg|favicon.ico).*)'],
};

