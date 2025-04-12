FROM node:18-alpine as build

# Create a app directory
WORKDIR /app

# Install app dependencies
COPY package*.json tsconfig*.json ./
COPY backend/ ./backend

# Bundle app source
COPY backend/ ./backend

# Run npm install
RUN npm install
RUN npm run build:backend

FROM node:18-alpine

WORKDIR /app

COPY --from=build /app/backend/dist ./backend/dist
COPY --from=build /app/node_modules ./node_modules
COPY frontend/index.html ./frontend/index.html

EXPOSE 8080

CMD ["node", "backend/dist/app.js"]