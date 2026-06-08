# micro-ROS LED Blink — ESP32 (Ubuntu)

Control an ESP32's onboard LED over a ROS 2 topic using micro-ROS on Ubuntu.


## Step 1 — Install Arduino IDE

### 1.1 — Download the AppImage

```bash
# Download Arduino IDE 2.x (check https://www.arduino.cc/en/software for the latest version)
wget https://downloads.arduino.cc/arduino-ide/arduino-ide_2.3.2_Linux_64bit.AppImage -O arduino-ide.AppImage
```

### 1.2 — Make It Executable and Run

```bash
chmod +x arduino-ide.AppImage
./arduino-ide.AppImage
```


### 1.3 — Fix Serial Port Permission

By default Ubuntu restricts access to serial ports. Add your user to the `dialout` group:

```bash
sudo usermod -a -G dialout $USER
```

**Log out and log back in** for the change to take effect. Without this, Arduino IDE won't be able to upload to the ESP32.

---

## Step 2 — Add ESP32 Board Support

### 2.1 — Add the Board Manager URL

1. Go to **File → Preferences**.
2. In the **Additional Boards Manager URLs** field, paste:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Click **OK**.

### 2.2 — Install the ESP32 Package

1. Go to **Tools → Board → Boards Manager**.
2. Search for `esp32`.
3. Select **esp32 by Espressif Systems** and click **Install**.
4. Close the Boards Manager once done.

### 2.3 — Select Your Board and Port

1. Go to **Tools → Board → esp32** and select **ESP32 Dev Module** (or your specific variant).
2. Go to **Tools → Port** and select the port your ESP32 is connected to (e.g. `/dev/ttyUSB0` or `/dev/ttyACM0`).

> **Tip:** If the port doesn't appear, install the USB-to-serial driver for your board's USB chip:
> ```bash
> # For CH340-based boards
> sudo apt install ch341
>
> # For CP210x-based boards — driver is built into the Linux kernel,
> # confirm it loaded with:
> dmesg | grep cp210x
> ```

---

## Step 3 — Install the micro_ros_arduino Library

Clone the prebuilt library directly into your Arduino libraries folder 

```bash
# Navigate to your Arduino libraries directory
cd ~/Arduino/libraries

# Clone the micro-ROS Arduino library (Humble branch)
git clone --branch humble https://github.com/micro-ROS/micro_ros_arduino.git
```

After cloning:

1. Restart Arduino IDE so it picks up the new library.
2. Go to **File → Examples → micro_ros_arduino** to verify the library appears.
3. If the examples are missing, confirm the folder name is exactly `micro_ros_arduino` (no `-` characters) — rename it if needed.

> **Note:** The `humble` branch matches the ROS 2 Humble Agent used in this project. If you are using a different ROS 2 distro, replace `humble` with the matching branch name (e.g. `iron`, `jazzy`).

---

## Step 4 — Flash the Sketch

1. Open the sketch in Arduino IDE.
2. Confirm the correct board and port are selected under **Tools**.
3. Click **Upload** (right arrow button).

---

## Step 5 — Install and Run the micro-ROS Agent


### Terminal 1: Start the Agent

```bash
# Confirm the device path
ls /dev/ttyUSB* /dev/ttyACM*

# Pull the micro-ROS Agent image
docker pull microros/micro-ros-agent:humble

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

## Step 6 — Control the LED

### Terminal 2 — ROS 2 Commands

```bash
# Enter the ROS 2 container
docker exec -it ros2_humble bash
source /opt/ros/humble/setup.bash

# Verify the node and topic are visible
ros2 node list
# Expected: /esp32_node

ros2 topic list
# Expected: /led_state

# Turn LED ON
ros2 topic pub --once /led_state std_msgs/msg/Int32 "{data: 1}"

# Turn LED OFF
ros2 topic pub --once /led_state std_msgs/msg/Int32 "{data: 0}"
```

---

## Topic Reference

| Topic | Type | Value | Effect |
|-------|------|-------|--------|
| `/led_state` | `std_msgs/msg/Int32` | `1` | LED ON |
| `/led_state` | `std_msgs/msg/Int32` | `0` | LED OFF |

---

## Troubleshooting

| Issue | Fix |
|-------|-----|
| Node not visible in `ros2 node list` | Check Agent terminal for connection errors; try pressing the ESP32 reset button |
| Wrong serial port | Run `ls /dev/ttyUSB* /dev/ttyACM*` and update `--dev` in the Agent command |
| LED doesn't respond | Confirm the sketch uploaded successfully; verify GPIO 2 is the correct LED pin for your board variant |
| Agent exits immediately | Ensure `--privileged` and `-v /dev:/dev` flags are included |
