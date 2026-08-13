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
- Verified on hardware: SNTP time synchronization and Global Core `day_window` evaluation using the proven Ukraine timezone settings.
- Verified on hardware: the main four-wire fan uses GPIO13 switched +12 V master power and GPIO25 25 kHz PWM; GPIO34 tachometer feedback is temporarily disabled. The humidifier local fan is a separate binary output; its mist output uses 800 Hz PWM at 50/60/70% duty.
- Verified end to end on device #2: provisioning, HiveMQ Cloud transport, retained V3 MQTT state and commands, PWA control, and OTA update through the deployed Worker. Device #2 is ready for field trials in the live cabinet.
