# Doorlink dashboard

The Next.js dashboard displays doorbell events, stored media, and turn-based voice replies from the [Spring Boot gateway](../gateway/). See the [project overview](../README.md) for hardware and validation status.

For the complete local stack, install dependencies with `npm ci` in this directory, then run `./scripts/run-dev.sh` from the repository root. The dashboard opens at [localhost:3001](http://localhost:3001).

To run only the dashboard against an already running gateway:

```bash
GATEWAY_URL=http://127.0.0.1:8081 npm run dev -- -p 3001
```

The gateway handles authentication, media, and notifications. [Frontend development notes](../docs/frontend-development.md) cover remote gateway testing.
