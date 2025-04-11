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

FROM nginx:alpine

COPY --from=build /app/backend/dist /app/backend/dist
COPY --from=build /app/node_modules /app/node_modules

COPY frontend/ /usr/share/nginx/html/

# nginx conf
COPY nginx/nginx.conf /etc/nginx/nginx.conf
COPY nginx/default.conf /etc/nginx/conf.d/default.conf

# add node.js server
COPY backend/ /app/backend
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

CMD ["/entrypoint.sh"]