#!/bin/sh

# Start backend server in background
node /app/backend/dist/app.js &

# Start nginx
nginx -g "daemon off;"
