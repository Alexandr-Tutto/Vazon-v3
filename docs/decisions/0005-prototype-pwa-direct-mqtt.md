# Decision 0005: Prototype PWA direct MQTT access

Status: accepted
Date: 2026-08-01

## Decision

During the private home-DIY prototype stage, Vazon V3 PWA connects directly to
the client HiveMQ Cloud cluster over secure MQTT WebSockets.

The prototype keeps dedicated PWA credentials in a separate source
configuration file. It does not reuse controller credentials. This is an
explicit temporary tradeoff while production authentication remains
undefined.

The PWA subscribes to the Vazon V3 namespace, discovers the configured serial
number from its canonical device ID, maps retained state into its
device store, and publishes commands using the active MQTT command envelope.

## Consequences

The credential is visible to anyone who can read the deployed browser assets,
even when the source repository is private. The prototype credential must be
revoked or replaced before public or customer deployment.

Production authentication, credential delivery, user identity, and access
revocation remain unresolved and require a later accepted decision.
