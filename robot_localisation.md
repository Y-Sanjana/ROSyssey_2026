````md
# ROS2 + micro-ROS Differential Drive Robot Setup

## System Architecture

ESP32 <---- USB Serial ----> Raspberry Pi 3

- ESP32:
  - Motor control
  - Encoder odometry
  - IMU publishing
  - micro-ROS client

- Raspberry Pi 3:
  - ROS2 Humble
  - micro-ROS Agent
  - robot_localization EKF
  - RViz visualization
  - Navigation node

---

# Workspace Build

## Build ROS2 workspace
```bash
git clone https://github.com/Y-Sanjana/Robot_localisation.git nav_Ws
```

```bash
cd ~/nav_ws

colcon build --symlink-install
````

## Source workspace

```bash
source /opt/ros/humble/setup.bash

source ~/nav_ws/install/setup.bash
```

---

# Cleaning Workspace

```bash
cd ~/nav_ws

rm -rf build install log
```

---

# Launch Navigation Stack

```bash
ros2 launch nav_pkg nav.launch.py
```

---

# micro-ROS Agent (Serial)

## Check ESP32 serial device

```bash
ls /dev/ttyUSB*

ls /dev/ttyACM*
```

## Run micro-ROS agent

```bash
docker run -it --rm --privileged \
-v /dev:/dev \
microros/micro-ros-agent:humble \
serial --dev /dev/ttyUSB0 -b 115200 -v6
```

Replace `/dev/ttyUSB0` with `/dev/ttyACM0` if needed.

---

# ROS2 Debugging Commands

## List ROS2 topics

```bash
ros2 topic list
```

## List ROS2 nodes

```bash
ros2 node list
```

---

# Topic Debugging

## Check wheel odometry

```bash
ros2 topic echo /wheel/odom
```

## Check IMU data

```bash
ros2 topic echo /imu/data
```

## Check filtered odometry

```bash
ros2 topic echo /odometry/filtered
```

## Check velocity commands

```bash
ros2 topic echo /cmd_vel
```

## Check RViz goal pose

```bash
ros2 topic echo /goal_pose
```

---

# RViz

## Start RViz manually

```bash
rviz2
```

## Fixed Frame

```text
odom
```

## Add Displays

* TF
* RobotModel
* Odometry

## Odometry Topic

```text
/odometry/filtered
```

---

# Navigation

## RViz Goal Navigation

Use:

```text
2D Goal Pose
```

in RViz toolbar.

Workflow:

```text
RViz Goal
   ↓
/goal_pose
   ↓
simple_nav_node
   ↓
/cmd_vel
   ↓
ESP32
   ↓
Robot Motion
```

---

# Useful ROS2 Commands

## Check package location

```bash
ros2 pkg prefix nav_pkg
```

## Find installed launch files

```bash
find install/nav_pkg -name "*.launch.py"
```

## Check package structure

```bash
tree ~/nav_ws/src/nav_pkg
```

---

# Docker Commands

## Verify Docker

```bash
docker --version
```

## Test Docker access

```bash
docker ps
```

---

# Serial Port Debugging

## List all serial devices

```bash
ls /dev/tty*
```

---

# ESP32 micro-ROS Setup

## Correct transport initialization

```cpp
set_microros_transports();
```

---

# EKF Configuration

## Main Topics

```text
/wheel/odom
/odometry/filtered
/cmd_vel
/goal_pose
```

---

# Final Working Pipeline

```text
RViz Goal
   ↓
simple_nav_node
   ↓
/cmd_vel
   ↓
ESP32 PID Control
   ↓
Wheel Motion
   ↓
/wheel/odom
   ↓
robot_localization EKF
   ↓
/odometry/filtered
   ↓
RViz Visualization
```

```
```
