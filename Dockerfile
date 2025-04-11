FROM node:18-alpine as build

# Create a app directory
WORKDIR /app

# Install app dependencies
COPY package*.json ./
COPY tsconfig.base.json ./

# Bundle app source
COPY backend/ ./backend

# Run npm install
RUN npm install
RUN npm run build:backend

EXPOSE 8080

CMD ["node", "app/backend/dist/app.js"]