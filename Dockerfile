FROM node:18-alpine

WORKDIR /app

COPY package*.json ./
COPY tsconfig.base.json ./
RUN npm install

COPY backend/ ./backend/
RUN npm run build:backend

CMD ["node", "backend/dist/app.js"]
