# Node-RED dashboard starter

Import [main_controller_dashboard.json](/abs/c:/Users/jaanr/Documents/GitHub/EMT0330/M2_lahendus/node_red/main_controller_dashboard.json) into Node-RED to get a first dashboard for `Main_controller`.

Requirements:
- `node-red-dashboard`
- An MQTT broker config that can connect to `193.40.245.72:1883`
- MQTT credentials set on the broker node as `test` / `test`

What it does:
- Subscribes to `Main_controller/status`
- Parses the JSON published by the ESP32
- Shows a dark overview card with a colored status square
- Clicking the colored square switches to a detailed tab for that ESP32

Current status colors:
- `working` -> green / `Running`
- `idle` -> yellow / `Idle`
- `maintenance` -> red / `Maintenance`
- anything else -> gray / `Offline`

Next logical step:
- Add `reset_button` and `imu_controller` as additional overview tiles using the same pattern
