/** @type {import('next').NextConfig} */
const nextConfig = {
    output: 'standalone',
    // Private household dashboard; the middleware sets the same header on the routes it handles.
    async headers() {
        return [{ source: '/:path*', headers: [{ key: 'X-Robots-Tag', value: 'noindex, nofollow, noarchive' }] }];
    },
    images: {
        remotePatterns: [
            {
                protocol: 'http',
                hostname: 'localhost',
                port: '9000',
                pathname: '/doorbell-images/**',
            },
        ],
    },
};

export default nextConfig;
