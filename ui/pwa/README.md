# Vazon V3 PWA Skeleton

Document status: draft
Code status: static UI skeleton
Scope: PWA UI layout and mock state rendering

## Purpose

```text
Provide a minimal UI skeleton before MQTT integration and before final firmware state is available.
```

## Current Scope

```text
Overview screen
Subsystem tiles
Climate numbers
Subsystem detail screen
Service/debug screen
Mock V3 state
PWA manifest
Service worker stub
```

## Current Non-Scope

```text
Real MQTT connection
Real command sending
Authentication
Persistent settings editing
OTA UI implementation
Production icon set
Build system
```

## Local Run

Use any static HTTP server from this folder.

Example:

```bash
cd ui/pwa
python -m http.server 8080
```

Open:

```text
http://localhost:8080
```

Direct `file://` opening is not recommended because service worker and ES modules require HTTP context.

## File Layout

```text
index.html
manifest.webmanifest
sw.js
src/app.js
src/ui-state.js
src/mock-state.js
src/styles.css
```

## Boundaries

```text
src/mock-state.js
    temporary mock state matching docs/v3_data_model.md

src/ui-state.js
    UI-only mapping from device data to screen labels and cards

src/app.js
    rendering and screen navigation only

src/styles.css
    visual layout only
```

## Next UI Step

```text
Add MQTT adapter boundary without changing screen rendering.
```

Expected future files:

```text
src/mqtt/mqtt-client.js
src/mqtt/topic-registry.js
src/state/device-store.js
```
