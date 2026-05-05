#!/bin/bash

# Configuration
API_URL="http://localhost:8080/api/events"
IMAGE_FILE="$(dirname "$0")/test-assets/test-image.jpg"
AUDIO_FILE="$(dirname "$0")/test-assets/test-audio.mp3"
EVENT_TYPE="DOORBELL_PRESS"

echo "📡 Virtual ESP32 Initialized"
echo "----------------------------------------"
echo "Target API: $API_URL"
echo "Event Type: $EVENT_TYPE"
echo "Image: $IMAGE_FILE"
echo "Audio: $AUDIO_FILE"
echo "----------------------------------------"
echo "📤 Sending payload..."

# We capture the HTTP status code and response body
response=$(curl -s -w "\nHTTP_STATUS:%{http_code}" -X POST "$API_URL" \
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
