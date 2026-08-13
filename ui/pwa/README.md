# Vazon V3 PWA

Document status: active field-trial interface
Code status: deployed direct-MQTT prototype PWA UI
Scope: PWA UI layout, menu navigation, actions, MQTT state rendering, and commands

## Purpose

```text
Provide a PWA UI connected directly to the prototype HiveMQ Cloud cluster.
```

## Current Scope

```text
Overview screen
Subsystem tiles
Climate numbers
Subsystem detail screen
Subsystem settings screen
Service/debug screen
MQTT-backed V3 state store
Secure MQTT WebSocket connection
MQTT command publishing
OTA check/install commands and controller status display
Persistent controller-settings editing through MQTT commands
UI action registry
Menu rendering helpers
PWA manifest
Service worker offline cache
```

## Current Non-Scope

```text
Authentication
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
src/ui-actions.js
src/ui-components.js
src/ui-menu-levels.js
src/ui-panels.js
src/ui-state.js
src/mock-state.js
src/styles.css
```

## Boundaries

```text
src/mock-state.js
    retained reference fixture matching docs/v3_data_model.md

src/mqtt-config.js
    prototype HiveMQ WebSocket endpoint and dedicated PWA credentials

src/mqtt/mqtt-client.js
    MQTT.js connection, subscription, reconnect, and command publishing

src/mqtt/topic-registry.js
    V3 topic parsing, device matching, and command topic construction

src/state/device-store.js
    retained MQTT state merged into the UI's raw device state

src/ui-state.js
    UI-only mapping from device data to screen labels and cards

src/ui-actions.js
    UI action registry for command buttons

src/ui-components.js
    shared DOM rendering helpers

src/ui-menu-levels.js
    menu level constants and menu view construction

src/ui-panels.js
    subsystem status, settings, and service panel definitions

src/app.js
    rendering and screen navigation only

src/styles.css
    visual layout only
```

## MQTT Prototype

```text
The browser connects directly to HiveMQ Cloud over WSS and discovers serial
number 02 from topics matching `Vazon_02_yyyyyy`.
```

The browser dependency is pinned to MQTT.js 5.15.2 from unpkg. The prototype
credential is intentionally visible in browser assets under accepted Decision
0005 and must be replaced before public or customer deployment.

## Next UI Step

```text
Collect field-trial feedback from controller 02. Replace the prototype
credential model before public or customer deployment.
```
