# Vazon V3

Clean Vazon V3 workspace seeded from active documentation only.

Active source of truth:

```text
docs/project_overview.md
docs/software_architecture.md
docs/ui_architecture.md
docs/v3_data_model.md
docs/mqtt_topic_registry.md
docs/migration_rules.md
docs/hardware/v3_board_pinout.md
docs/v3_contracts/
components/board_config/include/vazon_board_config.h
```

Old Vazon repository is a reference source only. Do not copy old implementation code without adapting it to V3 contracts.

## Current status

- Target: Vazon V3 PCB v1.0, device instance #2, ESP32-WROVER with 16 MB flash.
- Verified on hardware: safe-OFF boot, status LEDs, GPIO inputs/outputs, both SHT31 sensors, both soil-moisture ADC channels, and both DS18B20 probes.
- Implemented: normal/provisioning boot paths, Global Core, local climate/door/pot/light/fan/humidifier modules, system status, MQTT command routing, and retained MQTT state/status/settings publishing.
- Implemented, pending build and device verification: SNTP time synchronization and Global Core `day_window` evaluation using the proven Ukraine timezone settings.
- Deferred: main fan is not installed; a future four-wire PWM interface needs a dedicated GPIO and driver. The humidifier local fan is a separate binary output; its mist output uses 800 Hz PWM at 40/50/60% duty.
- External integration pending: provision device #2 for the current network, verify live MQTT topics, and connect the new UI.
