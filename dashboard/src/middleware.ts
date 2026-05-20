import { NextResponse } from 'next/server';
import type { NextRequest } from 'next/server';

export function middleware(request: NextRequest) {
  const path = request.nextUrl.pathname;

  if (path.startsWith('/api')) {
    const backendUrl = process.env.GATEWAY_URL || 'http://127.0.0.1:8080';
    const url = new URL(request.nextUrl.pathname + request.nextUrl.search, backendUrl);

    const requestHeaders = new Headers(request.headers);
    const apiKey = process.env.GATEWAY_API_KEY || 'default-dev-api-key';
    requestHeaders.set('X-API-Key', apiKey);

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
