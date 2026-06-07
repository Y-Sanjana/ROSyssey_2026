# ROS2 + micro-ROS Workshop — Day 1 Setup Guide

> **Goal:** By the end of this session, Docker is installed, ROS2 Humble container is running, and micro-ROS agent is built inside the container.

---

## Prerequisites
- Raspberry Pi (RPi 3A+) with Raspberry Pi OS 64-bit flashed and SSH enabled
- RPi and Laptop on the **same WiFi network**
- Laptop terminal (Windows: PowerShell or Windows Terminal)

---

## Legend

| Symbol | Where to run |
|--------|--------------|
| 🖥️ | RPi terminal (outside Docker) |
| 📦 | Inside Docker container |

---

## PART 1 — RPi OS Setup

> 🖥️ All commands: **RPi terminal (outside Docker)**

### Step 1: SSH into the Raspberry Pi
```bash
ssh username@hostname.local
# or
ssh username@ip_address
```
> First time: type `yes` when asked about authenticity.
> Nothing shows while typing password — that's normal.

---

### Step 2: Fix Locale Errors (skip if no error)
```bash
sudo dpkg-reconfigure locales
```
- Press `Space` to select **en_US.UTF-8 UTF-8** → Press `Tab` → OK
- Choose **en_US.UTF-8** as default locale

```bash
sudo reboot
```
> SSH back in after reboot (Step 1 again).

> ⚠️ Skip entirely if no locale error appears.

---

### Step 3: Update Packages
```bash
sudo apt update
```
> Refreshes the package list inside RPi OS.

---

### Step 4: Install Git
```bash
sudo apt install git -y
```

---

### Step 5: Clone the Docker Install Repo
```bash
cd ~
git clone https://github.com/aadi613/ROS2-on-rpi-with-dockers.git
cd ROS2-on-rpi-with-dockers
```

---

### Step 6: Make Scripts Executable
```bash
chmod +x install_ros2_docker.sh ros2_demo_test.sh
```

---

### Step 7: Run the Docker Install Script
```bash
./install_ros2_docker.sh
```
> Installs Docker on the RPi. Takes 3–5 min.

---

### Step 8: Exit & Reconnect
```
Ctrl + D
```
Then SSH back in:
```bash
ssh username@hostname.local
```
> Required to apply Docker group permission changes.

---

## PART 2 — Docker + ROS2 Container Setup

### Step 9: Pull the micro-ROS Agent Image
> 🖥️ **RPi terminal (outside Docker)**
```bash
docker pull microros/micro-ros-agent:humble
```
> Downloads the micro-ROS agent image. Do this early — depends on network speed.

---

### Step 10: Create the ROS2 Humble Container
> 🖥️ **RPi terminal (outside Docker)**
```bash
sudo docker run -it \
  --name ros2_humble \
  --network host \
  --privileged \
  --dns 8.8.8.8 \
  --dns 8.8.4.4 \
  ros:humble-ros-base
```
> This drops you **inside the container** automatically after running.

---

### Step 11: Verify ROS2 is Working
> 📦 **Inside Docker container**
```bash
ros2 --help
```
> Should print ROS2 command list. If it does — ROS2 is working correctly.

---

### Step 12: Exit the Container
> 📦 **Inside Docker container**
```
Ctrl + D
```
> Returns you to the RPi terminal.

---

## PART 3 — micro-ROS Setup

### Step 13: Clone micro-ROS Installation Repo
> 🖥️ **RPi terminal (outside Docker)**
```bash
cd ~
git clone https://github.com/aadi613/micro_ros_installation.git
```

---

### Step 14: Start the Container
> 🖥️ **RPi terminal (outside Docker)**
```bash
sudo docker start ros2_humble
```

---

### Step 15: Copy Scripts into the Container
> 🖥️ **RPi terminal (outside Docker)**
```bash
docker cp ~/micro_ros_installation/scripts ros2_humble:/root/scripts
```

---

### Step 16: Enter the Container
> 🖥️ **RPi terminal (outside Docker)**
```bash
sudo docker exec -it ros2_humble bash
```
> You are now inside the container.

---

### Step 17: Source ROS2 Environment
> 📦 **Inside Docker container**
```bash
source /opt/ros/humble/setup.bash
```
> Loads ROS2 environment variables into the current shell session.

---

### Step 18: Go to Scripts Folder & Make Executable
> 📦 **Inside Docker container**
```bash
cd /root/scripts
chmod +x *.sh
```

---

### Step 19: Run the micro-ROS Installation Script
> 📦 **Inside Docker container**
```bash
./install_microros_agent.sh
```
> Builds the micro-ROS agent inside the container.
> **Takes time Do not exit or interrupt.**

---

**What is ready:**
- Docker installed on RPi
- ROS2 Humble container (`ros2_humble`) created
- micro-ROS agent built inside the container
- micro-ROS agent image pulled (`microros/micro-ros-agent:humble`)

**Day 2 starts from:** ESP32 setup → flash rover sketch → run agent → teleop the bot.

---

## Troubleshooting

| Issue | Fix |
|-------|-----|
| `ssh: Could not resolve hostname` | Use IP address instead: `ssh username@ip_address` |
| `Connection refused` | SSH not enabled during OS flash — reflash with SSH enabled |
| `Connection timed out` | RPi and laptop not on same WiFi |
| Nothing prints while typing password | Normal — SSH hides password input |
| `docker: command not found` after Step 7 | Exit and reconnect (Step 8) not done yet |
| `docker pull` very slow | Network congestion — pre-pull the night before |
| Step 19 build fails mid-way | Re-run `./install_microros_agent.sh` — it will resume |
