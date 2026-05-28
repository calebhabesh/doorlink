#!/bin/bash

# Configuration
DEFAULT_API_URL="http://localhost:8080/api/events"
API_URL="${GATEWAY_URL:-$DEFAULT_API_URL}"

# Support overriding via command line argument (either a port number or full URL)
if [ ! -z "$1" ]; then
  if [[ "$1" =~ ^[0-9]+$ ]]; then
    API_URL="http://localhost:$1/api/events"
  else
    API_URL="$1"
  fi
fi

IMAGE_FILE="$(dirname "$0")/test-assets/test-image.jpg"
AUDIO_FILE="$(dirname "$0")/test-assets/test-audio.mp3"
EVENT_TYPE="DOORBELL_PRESS"

# Load API Key from environment or .env file
API_KEY="${GATEWAY_API_KEY}"
if [ -z "$API_KEY" ] && [ -f "$(dirname "$0")/../.env" ]; then
  # Sourcing .env can be risky if there are spaces or exports, so we grep the key specifically
  API_KEY=$(grep -E "^GATEWAY_API_KEY=" "$(dirname "$0")/../.env" | cut -d'=' -f2-)
fi
if [ -z "$API_KEY" ]; then
  API_KEY="default-dev-api-key"
fi

# Safe masking of the key for log output
if [ ${#API_KEY} -gt 8 ]; then
  MASKED_KEY="${API_KEY:0:4}...${API_KEY: -4}"
else
  MASKED_KEY="***"
fi

echo "📡 Virtual ESP32 Initialized"
echo "----------------------------------------"
echo "Target API: $API_URL"
echo "API Key   : $MASKED_KEY"
echo "Event Type: $EVENT_TYPE"
echo "Image: $IMAGE_FILE"
echo "Audio: $AUDIO_FILE"
echo "----------------------------------------"
echo "📤 Sending payload..."

# We capture the HTTP status code and response body
response=$(curl -s -w "\nHTTP_STATUS:%{http_code}" -X POST "$API_URL" \
  -H "X-API-Key: $API_KEY" \
  -F "eventType=$EVENT_TYPE" \
  -F "image=@$IMAGE_FILE;type=image/jpeg" \
  -F "audio=@$AUDIO_FILE;type=audio/mpeg")

status_code=$(echo "$response" | grep "HTTP_STATUS" | awk -F":" '{print $2}')
body=$(echo "$response" | sed -e 's/HTTP_STATUS\:.*//g')

echo -e "\n----------------------------------------"
if [ "$status_code" -eq 200 ]; then
  echo "✅ Success! (HTTP 200)"
  echo "Gateway Response: $body"
else
  echo "❌ Failed! (HTTP $status_code)"
  echo "Gateway Error: $body"
fi
