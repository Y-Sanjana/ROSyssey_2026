# micro-ROS PWM Control — ESP32 (Ubuntu)

Control PWM duty cycle on an ESP32 output pin over a ROS 2 topic using micro-ROS.

---

## Overview

The ESP32 subscribes to the `/pwm_value` topic. Publishing an integer between `0` and `255` sets the PWM duty cycle on the output pin accordingly (0 = fully off, 255 = fully on). Communication happens over a USB serial connection via the micro-ROS Agent running in Docker.

> **Note:** Arduino IDE, ESP32 board support, and the micro_ros_arduino library should already be set up. If not, follow the LED Blink README first.

---

---

## Step 1 — Flash the Sketch

1. Open the PWM sketch in Arduino IDE.
2. Go to **Tools → Board** and confirm your ESP32 variant is selected.
3. Go to **Tools → Port** and confirm the correct port is selected (e.g. `/dev/ttyUSB0`).
4. Click **Upload**.

---

## Step 2 — Start the micro-ROS Agent

### Terminal 1

```bash
# Confirm the device path
ls /dev/ttyUSB* /dev/ttyACM*

# Run the Agent (replace /dev/ttyUSB0 with your actual device)
docker run -it --rm \
  --privileged \
  -v /dev:/dev \
  --net=host \
  microros/micro-ros-agent:humble \
  serial --dev /dev/ttyUSB0 -v6
```

Keep this terminal open. You should see the Agent connect to the ESP32 node.

---

## Step 3 — Control the PWM

### Terminal 2

```bash
# Enter the ROS 2 container
docker exec -it ros2_humble bash
source /opt/ros/humble/setup.bash

# Verify the node and topic are visible
ros2 node list
# Expected: /esp32_node

ros2 topic list
# Expected: /pwm_value

# Set duty cycle to 50% (~128 out of 255)
ros2 topic pub --once /pwm_value std_msgs/msg/Int32 "{data: 128}"

# Full output
ros2 topic pub --once /pwm_value std_msgs/msg/Int32 "{data: 255}"

# Turn off
ros2 topic pub --once /pwm_value std_msgs/msg/Int32 "{data: 0}"
```

---

## If the Agent Stops Running

If Terminal 1 closes or the Agent crashes, restart it with the same command:

```bash
docker run -it --rm \
  --privileged \
  -v /dev:/dev \
  --net=host \
  microros/micro-ros-agent:humble \
  serial --dev /dev/ttyUSB0 -v6
```

Then **press the reset button on the ESP32** so it reconnects to the Agent. Once you see the node registration messages in the Agent terminal, go back to Terminal 2 and publish as normal.

> **Tip:** If the Agent restarts but the node still doesn't appear in `ros2 node list`, press the ESP32 reset button once more and wait a few seconds.

---

## Topic Reference

| Topic | Type | Range | Effect |
|-------|------|-------|--------|
| `/pwm_value` | `std_msgs/msg/Int32` | `0` | Output OFF (0% duty cycle) |
| `/pwm_value` | `std_msgs/msg/Int32` | `1–254` | Proportional output |
| `/pwm_value` | `std_msgs/msg/Int32` | `255` | Output fully ON (100% duty cycle) |

Values outside the 0–255 range are clamped in the sketch.

---

## Troubleshooting

| Issue | Fix |
|-------|-----|
| Node not visible in `ros2 node list` | Restart the Agent; press the ESP32 reset button and wait a few seconds |
| Wrong serial port | Run `ls /dev/ttyUSB* /dev/ttyACM*` and update `--dev` in the Agent command |
| No PWM output | Confirm GPIO 2 is correct for your board; check PWM channel and frequency in the sketch |
| Agent exits immediately | Ensure `--privileged` and `-v /dev:/dev` flags are included |
| Agent starts but ESP32 doesn't connect | Press the ESP32 reset button after the Agent is running |
| Output flickers | Avoid continuous `ros2 topic pub` without `--once`; use `--once` for stable duty cycles |
| Docker permission denied | Run `sudo usermod -aG docker $USER`, log out and back in |
