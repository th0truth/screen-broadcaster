# **Screen broadcaster**

**Screen Broadcaster** is a custom-built desktop application written in C / C++ and TypeScript. It designed to provide screen recording and live broadcasting capabilities on Windows 11 / 10. Inspired by tools like OBS, it offers a lot of intresting features e.g. screen recording / broadcasting quality, traffic monitoring and multiple displays support.

# Requirements

-   [Docker](https://docs.docker.com/)
-   [Node.js](https://nodejs.org/)

# Installation

```bash
git clone git@github.com:th0truth/screen-broadcaster.git
cd screen-broadcaster
```

# Usage

You must building some additional components.

```bash
docker compose build
npm run build:tsc
```

```bash
docker compose up
npm run broadcaster
```

After building and launching the application, it will automatically start a local server to provide access to the user interface.

```bash
http://localhost
```

This will load the page, where you can manage recordings, configure streams, settings.

# Deployments

If you need you can easily deploy this application on the cloud platform (e.g. [Render](https://render.com/), [Railway](https://railway.com/)).
