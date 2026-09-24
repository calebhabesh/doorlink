#!/bin/bash
# Smart Doorbell Single-Pane Local Dev Runner
# Runs infra, backend, and frontend concurrently in the current pane.
# Cleanly shuts down all processes and containers on Ctrl+C.

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENV_FILE="$PROJECT_DIR/.env.dev"
DEV_BOOTSTRAP_TOKEN="${HOUSEHOLD_BOOTSTRAP_TOKEN:-$(openssl rand -hex 24)}"

# Clean shutdown handler
cleanup() {
  echo -e "\n\n🛑 Shutting down local development stack..."
  
  # Kill background Spring Boot and Next.js processes
  if [ -n "$BACKEND_PID" ]; then
    echo "Stopping Spring Boot backend (PID: $BACKEND_PID)..."
    kill $BACKEND_PID 2>/dev/null
  fi
  
  if [ -n "$FRONTEND_PID" ]; then
    echo "Stopping Next.js dashboard (PID: $FRONTEND_PID)..."
    kill $FRONTEND_PID 2>/dev/null
  fi
  
  # Shut down docker compose
  echo "Bringing down Docker infrastructure..."
  docker compose --env-file "$ENV_FILE" down
  
  echo "✅ Local dev stack shut down cleanly."
  exit 0
}

# Trap Ctrl+C (SIGINT) and termination (SIGTERM) signals
trap cleanup SIGINT SIGTERM

echo "🐳 Starting local Docker infrastructure (Postgres, Mosquitto, MinIO)..."
docker compose --env-file "$ENV_FILE" up -d

echo "☕ Starting Spring Boot Gateway on port 8081..."
cd "$PROJECT_DIR/gateway"
HOUSEHOLD_BOOTSTRAP_TOKEN="$DEV_BOOTSTRAP_TOKEN" SPRING_PROFILES_ACTIVE=dev ./mvnw spring-boot:run &
BACKEND_PID=$!

echo "⚡ Starting Next.js Dashboard on port 3001..."
cd "$PROJECT_DIR/dashboard"
GATEWAY_URL=http://127.0.0.1:8081 npm run dev -- -p 3001 &
FRONTEND_PID=$!

echo -e "\n🚀 All services running concurrently in this pane!"
echo "👉 First-time dashboard setup token: $DEV_BOOTSTRAP_TOKEN"
echo "👉 Press Ctrl+C to stop everything and clean up."
echo "------------------------------------------------------------"

# Wait for background jobs to finish
wait
