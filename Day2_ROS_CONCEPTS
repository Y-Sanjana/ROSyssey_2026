# ROS 2 - Hands-On Session Guide
### Topics: Nodes · Topics · Messages · Turtlesim · Package Creation

---

## HOW TO USE THIS GUIDE
Run each command in order. Every command has a one-line explanation.  
Open a **new terminal** whenever the guide says so. Always source ROS 2 first in every new terminal.

---

## PART 1 — ENVIRONMENT SETUP 

> Do this in **every** new terminal you open before running any ROS 2 command.

```bash
source /opt/ros/humble/setup.bash
```
*Makes all ROS 2 commands available in the current terminal.*

---

## PART 2 — TURTLESIM: YOUR FIRST NODES & TOPICS 

### Step 1 — Launch the Turtlesim window

Open **Terminal 1** and run:

```bash
ros2 run turtlesim turtlesim_node
```
*Starts the turtlesim simulator. A window with a turtle appears.*  
`ros2 run <package_name> <executable_name>` — this is the standard way to launch any ROS 2 node.

---

### Step 2 — Control the turtle with your keyboard

Open **Terminal 2**:

```bash
source /opt/ros/humble/setup.bash
ros2 run turtlesim turtle_teleop_key
```
*Starts a keyboard controller. Use arrow keys to move the turtle.*

---

### Step 3 — See all running nodes

Open **Terminal 3**:

```bash
source /opt/ros/humble/setup.bash
ros2 node list
```
*Lists all active nodes. You should see `/turtlesim` and `/teleop_turtle`.*

---

### Step 4 — Inspect a specific node

```bash
ros2 node info /turtlesim
```
*Shows everything about the node: what topics it subscribes to, publishes, services it offers, etc.*

---

### Step 5 — See all active topics

```bash
ros2 topic list
```
*Lists all topics currently active in the system.*

```bash
ros2 topic list -t
```
*Same list, but also shows the message type for each topic (the `-t` flag = type).*

---

### Step 6 — See live data on a topic

```bash
ros2 topic echo /turtle1/cmd_vel
```
*Prints every message being published on this topic in real time.*  
**Now go to Terminal 2 and press an arrow key** — you'll see the data appear here.  
Press `Ctrl+C` to stop.

---

### Step 7 — Check who is publishing/subscribing to a topic

```bash
ros2 topic info /turtle1/cmd_vel
```
*Shows how many publishers and subscribers exist on this topic.*

```bash
ros2 topic info /turtle1/cmd_vel --verbose
```
*Full details: node names, QoS profiles (reliability, history depth, etc.).*

---

### Step 8 — Understand a message's structure

```bash
ros2 interface show geometry_msgs/msg/Twist
```
*Prints the exact fields inside the `Twist` message type (linear x/y/z + angular x/y/z).*

---

### Step 12 — Visualize the node graph (optional, if rqt is installed)

```bash
ros2 run rqt_graph rqt_graph
```
*Opens a GUI showing nodes as circles and topics as arrows between them.*

---

### Step 13 — Rename a node (Remapping)

Open a new terminal:
```bash
source /opt/ros/humble/setup.bash
ros2 run turtlesim turtlesim_node --ros-args --remap __node:=my_turtle
```
*Launches turtlesim with a custom node name. Now run `ros2 node list` — you'll see `/my_turtle`.*

---

### CLEANUP — Stop everything

Press `Ctrl+C` in each terminal to stop all running nodes.

---

## PART 3 — WORKSPACES & PACKAGES 

> A **workspace** is just a folder with a `src/` directory inside it.  
> A **package** is the unit that holds your ROS 2 code.

---

### Step 1 — Install colcon (build tool)

```bash
python3 -m pip install colcon-common-extensions
```
*`colcon` is the tool that builds ROS 2 packages.*

---

### Step 2 — Create a workspace

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws
```
*Creates the workspace directory with a `src/` folder inside.*

---

### Step 3 — Build the workspace

```bash
cd ~/ros2_ws
colcon build --symlink-install
```
*Builds all packages in `src/`. After build, you'll see three new folders: `build/`, `install/`, `log/`.*

---

### Step 4 — Source the workspace

```bash
source install/setup.bash
```
*Makes your built packages available in this terminal. Run this every time you open a new terminal inside `ros2_ws`.*

---

## PART 4 — CREATE YOUR OWN PUBLISHER & SUBSCRIBER PACKAGE 

> We'll build one package called `cpp_pubsub` that contains two nodes: a **talker** (publisher) and a **listener** (subscriber).

---

### Step 1 — Create the package

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_cmake --license Apache-2.0 cpp_pubsub
```
*Creates a new C++ package called `cpp_pubsub`. Auto-generates `CMakeLists.txt`, `package.xml`, and `src/`.*

---

### Step 2 — Download the publisher and subscriber source code

```bash
cd ~/ros2_ws/src/cpp_pubsub/src

wget -O publisher_lambda_function.cpp https://raw.githubusercontent.com/ros2/examples/jazzy/rclcpp/topics/minimal_publisher/lambda.cpp

wget -O subscriber_lambda_function.cpp https://raw.githubusercontent.com/ros2/examples/jazzy/rclcpp/topics/minimal_subscriber/lambda.cpp
```
*Downloads both source files into the `src/` folder of the package.*

Verify both files are there:
```bash
ls
```
*You should see: `publisher_lambda_function.cpp` and `subscriber_lambda_function.cpp`*

---

### Step 3 — Understand the Publisher code

```bash
cat publisher_lambda_function.cpp
```

**What the publisher does — line by line:**

```cpp
#include "rclcpp/rclcpp.hpp"         // Core ROS 2 C++ library
#include "std_msgs/msg/string.hpp"   // The String message type we'll publish
```

```cpp
class MinimalPublisher : public rclcpp::Node   // Our node inherits from rclcpp::Node
```

```cpp
publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);
// Creates a publisher on topic named "topic", queue size 10
```

```cpp
timer_ = this->create_wall_timer(500ms, timer_callback);
// Fires timer_callback every 500ms (twice per second)
```

```cpp
message.data = "Hello, world! " + std::to_string(this->count_++);
this->publisher_->publish(message);
// Builds the message string and publishes it
```

```cpp
rclcpp::spin(std::make_shared<MinimalPublisher>());
// Keeps the node alive and running until Ctrl+C
```

---

### Step 4 — Understand the Subscriber code

```bash
cat subscriber_lambda_function.cpp
```

**What the subscriber does:**

```cpp
subscription_ = this->create_subscription<std_msgs::msg::String>("topic", 10, topic_callback);
// Subscribes to the same "topic" with queue size 10
// topic_callback is triggered every time a message arrives
```

```cpp
RCLCPP_INFO(this->get_logger(), "I heard: '%s'", msg->data.c_str());
// Prints the received message to the terminal
```

> **Key rule:** The topic name (`"topic"`) and message type (`std_msgs/msg/String`) must **match exactly** between publisher and subscriber for them to communicate.

---

### Step 5 — Edit package.xml to declare dependencies

Open `~/ros2_ws/src/cpp_pubsub/package.xml` in a text editor and add these two lines after the `<buildtool_depend>` line:

```xml
<depend>rclcpp</depend>
<depend>std_msgs</depend>
```
*Tells ROS 2 this package needs the `rclcpp` and `std_msgs` libraries to build and run.*

---

### Step 6 — Edit CMakeLists.txt to register both nodes

Open `~/ros2_ws/src/cpp_pubsub/CMakeLists.txt` and replace its contents with this clean version:

```cmake
cmake_minimum_required(VERSION 3.5)
project(cpp_pubsub)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(talker src/publisher_lambda_function.cpp)
ament_target_dependencies(talker rclcpp std_msgs)

add_executable(listener src/subscriber_lambda_function.cpp)
ament_target_dependencies(listener rclcpp std_msgs)

install(TARGETS talker listener
  DESTINATION lib/${PROJECT_NAME})

ament_package()
```

*`add_executable` tells colcon what to compile. `install(TARGETS ...)` makes `ros2 run` able to find them.*

---

### Step 7 — Build the package

```bash
cd ~/ros2_ws
colcon build --packages-select cpp_pubsub
```
*Builds only `cpp_pubsub`. Watch for any errors — usually means a typo in CMakeLists.txt or package.xml.*

---

### Step 8 — Run the talker (publisher)

Open **Terminal A**:
```bash
source /opt/ros/humble/setup.bash
source ~/ros2_ws/install/setup.bash
ros2 run cpp_pubsub talker
```

Expected output (every 0.5 seconds):
```
[INFO] [minimal_publisher]: Publishing: "Hello, world! 0"
[INFO] [minimal_publisher]: Publishing: "Hello, world! 1"
[INFO] [minimal_publisher]: Publishing: "Hello, world! 2"
```

---

### Step 9 — Run the listener (subscriber)

Open **Terminal B**:
```bash
source /opt/ros/humble/setup.bash
source ~/ros2_ws/install/setup.bash
ros2 run cpp_pubsub listener
```

Expected output:
```
[INFO] [minimal_subscriber]: I heard: "Hello, world! 4"
[INFO] [minimal_subscriber]: I heard: "Hello, world! 5"
[INFO] [minimal_subscriber]: I heard: "Hello, world! 6"
```

*The listener picks up from wherever the talker currently is — you may not start at 0.*

---

### Step 10 — Verify with topic tools (connecting back to Part 2)

Open **Terminal C**:
```bash
source /opt/ros/humble/setup.bash
ros2 topic list
```
*You'll see `/topic` — the channel our two nodes are using.*

```bash
ros2 topic echo /topic
```
*Watch the messages flow live.*

```bash
ros2 topic info /topic
```
*Should show: Publisher count: 1, Subscription count: 1*

Press `Ctrl+C` in all terminals when done.

