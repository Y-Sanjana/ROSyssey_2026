# ESP32 Teleop Node

micro-ROS node on ESP32. Subscribes to `/cmd_vel` over WiFi UDP and drives a differential-drive rover via DRV8833.

---

## Hardware

| Component | GPIO |
|---|---|
| Right Motor IN1 | 25 |
| Right Motor IN2 | 26 |
| Left Motor IN1 | 27 |
| Left Motor IN2 | 14 |
| Left Encoder A (ISR) | 27 |
| Left Encoder B (DIR) | 26 |
| Right Encoder A (ISR) | 25 |
| Right Encoder B (DIR) | 33 |
| MPU-6050 SDA | 13 |
| MPU-6050 SCL | 14 |

> ⚠️ **Pin conflicts exist in this version** — GPIO 25/26/27 are shared between motor driver and encoders, and GPIO 14 is shared between LEFT_IN2 and SCL. Fix before deploying.

---

## Step 0 — Edit credentials before flashing

Open `teleop_code.ino` and update these three lines:

```cpp
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/twist.h>
#include <Wire.h>

// ─────────────────────────────────────────
// PIN CONFIGURATION
// ─────────────────────────────────────────

// DRV8833: IN1,IN2 → Right motor
#define RIGHT_IN1   25    // D21
#define RIGHT_IN2   26    // D19

// DRV8833: IN3,IN4 → Left motor
#define LEFT_IN1    27    // D4
#define LEFT_IN2    14    // D2

// Left Encoder
#define LEFT_ENC_A  27    // D27 (C1) — interrupt
#define LEFT_ENC_B  26    // D26 (C2) — direction

// Right Encoder
#define RIGHT_ENC_A 25    // D25 (C1) — interrupt
#define RIGHT_ENC_B 33    // D33 (C2) — direction

// I2C MPU6050
#define SDA_PIN     13    // D13
#define SCL_PIN     14    // D14

// ─────────────────────────────────────────
// WiFi credentials for micro-ROS agent
// ─────────────────────────────────────────
#define WIFI_SSID   "astra8"      // ← CHANGE
#define WIFI_PASS   "12345678"  // ← CHANGE
#define AGENT_IP    ""       // ← CHANGE to your PC IP
#define AGENT_PORT  8888

// ─────────────────────────────────────────
// LEDC PWM  (v3 API — no ledcSetup)
// ─────────────────────────────────────────
#define LEFT_CH     0
#define RIGHT_CH    1
#define PWM_FREQ    1000
#define PWM_RES     10    // 10-bit → 0–1023

// MPU6050
#define MPU_ADDR    0x68

// Drive params
#define WHEELBASE   0.16
#define VEL_SCALE   800.0

// ─────────────────────────────────────────
// GLOBALS
// ─────────────────────────────────────────
volatile int32_t left_count  = 0;
volatile int32_t right_count = 0;

float linear_x_cmd  = 0.0;
float angular_z_cmd = 0.0;

rcl_node_t         node;
rcl_subscription_t sub;
rclc_executor_t    executor;
rclc_support_t     support;
rcl_allocator_t    allocator;
geometry_msgs__msg__Twist twist_msg;

int16_t AccX, AccY, AccZ;
int16_t GyroX, GyroY, GyroZ;
float   Ax, Ay, Az;
float   Gx, Gy, Gz;
float   angleX, angleY;

// ─────────────────────────────────────────
// ENCODER ISRs
// ─────────────────────────────────────────
void IRAM_ATTR leftEncoderISR() {
    if (digitalRead(LEFT_ENC_B))  left_count++;
    else                          left_count--;
}

void IRAM_ATTR rightEncoderISR() {
    if (digitalRead(RIGHT_ENC_B)) right_count++;
    else                          right_count--;
}

// ─────────────────────────────────────────
// MOTOR FUNCTIONS  (v3 API: ledcAttach with 3 args)
// ─────────────────────────────────────────
void Motor_Left(int speed) {
    speed = constrain(speed, -1023, 1023);
    if (speed > 0) {
        ledcAttach(LEFT_IN1, PWM_FREQ, PWM_RES);  // PWM on D4
        ledcWrite(LEFT_IN1, speed);
        digitalWrite(LEFT_IN2, LOW);               // D2 = LOW
    } else if (speed < 0) {
        ledcAttach(LEFT_IN2, PWM_FREQ, PWM_RES);  // PWM on D2
        ledcWrite(LEFT_IN2, -speed);
        digitalWrite(LEFT_IN1, LOW);               // D4 = LOW
    } else {
        ledcDetach(LEFT_IN1);
        ledcDetach(LEFT_IN2);
        digitalWrite(LEFT_IN1, LOW);
        digitalWrite(LEFT_IN2, LOW);               // coast
    }
}

void Motor_Right(int speed) {
    speed = constrain(speed, -1023, 1023);
    if (speed > 0) {
        ledcAttach(RIGHT_IN1, PWM_FREQ, PWM_RES); // PWM on D21
        ledcWrite(RIGHT_IN1, speed);
        digitalWrite(RIGHT_IN2, LOW);              // D19 = LOW
    } else if (speed < 0) {
        ledcAttach(RIGHT_IN2, PWM_FREQ, PWM_RES); // PWM on D19
        ledcWrite(RIGHT_IN2, -speed);
        digitalWrite(RIGHT_IN1, LOW);              // D21 = LOW
    } else {
        ledcDetach(RIGHT_IN1);
        ledcDetach(RIGHT_IN2);
        digitalWrite(RIGHT_IN1, LOW);
        digitalWrite(RIGHT_IN2, LOW);              // coast
    }
}

void Motor_Stop() {
    ledcDetach(LEFT_IN1);
    ledcDetach(LEFT_IN2);
    ledcDetach(RIGHT_IN1);
    ledcDetach(RIGHT_IN2);
    digitalWrite(LEFT_IN1,  HIGH);  // D4  — brake
    digitalWrite(LEFT_IN2,  HIGH);  // D2
    digitalWrite(RIGHT_IN1, HIGH);  // D21
    digitalWrite(RIGHT_IN2, HIGH);  // D19
}

// ─────────────────────────────────────────
// CMD_VEL CALLBACK
// ─────────────────────────────────────────
void twist_callback(const void* msg_in) {
    const geometry_msgs__msg__Twist* msg =
        (const geometry_msgs__msg__Twist*)msg_in;

    linear_x_cmd  = msg->linear.x;
    angular_z_cmd = msg->angular.z;

    float v_left  = linear_x_cmd - (angular_z_cmd * WHEELBASE / 2.0);
    float v_right = linear_x_cmd + (angular_z_cmd * WHEELBASE / 2.0);

    Motor_Left ((int)(v_left  * VEL_SCALE));
    Motor_Right((int)(v_right * VEL_SCALE));
}

// ─────────────────────────────────────────
// MPU6050
// ─────────────────────────────────────────
void MPU6050_Init() {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B);
    Wire.write(0x00);
    Wire.endTransmission();
}

void MPU6050_Read() {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 14, true);

    AccX  = Wire.read() << 8 | Wire.read();
    AccY  = Wire.read() << 8 | Wire.read();
    AccZ  = Wire.read() << 8 | Wire.read();
    Wire.read(); Wire.read();               // skip temp
    GyroX = Wire.read() << 8 | Wire.read();
    GyroY = Wire.read() << 8 | Wire.read();
    GyroZ = Wire.read() << 8 | Wire.read();
}

void MPU6050_Convert() {
    Ax = AccX  / 16384.0;
    Ay = AccY  / 16384.0;
    Az = AccZ  / 16384.0;
    Gx = GyroX / 131.0;
    Gy = GyroY / 131.0;
    Gz = GyroZ / 131.0;
}

void MPU6050_Angle() {
    angleX = atan2(Ay, Az) * 57.3;
    angleY = atan2(Ax, Az) * 57.3;
}

// ─────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    // micro-ROS over WiFi
    set_microros_wifi_transports(WIFI_SSID, WIFI_PASS, AGENT_IP, AGENT_PORT);
    delay(2000);

    // Motor pins
    pinMode(LEFT_IN1,  OUTPUT);  // D4
    pinMode(LEFT_IN2,  OUTPUT);  // D2
    pinMode(RIGHT_IN1, OUTPUT);  // D21
    pinMode(RIGHT_IN2, OUTPUT);  // D19
    Motor_Stop();

    // Encoder interrupts
    pinMode(LEFT_ENC_A,  INPUT_PULLUP);  // D27
    pinMode(LEFT_ENC_B,  INPUT_PULLUP);  // D26
    pinMode(RIGHT_ENC_A, INPUT_PULLUP);  // D25
    pinMode(RIGHT_ENC_B, INPUT_PULLUP);  // D33
    attachInterrupt(digitalPinToInterrupt(LEFT_ENC_A),  leftEncoderISR,  RISING);
    attachInterrupt(digitalPinToInterrupt(RIGHT_ENC_A), rightEncoderISR, RISING);

    // I2C + MPU6050
    Wire.begin(SDA_PIN, SCL_PIN);   // D13, D14
    MPU6050_Init();

    // micro-ROS init
    allocator = rcl_get_default_allocator();
    rclc_support_init(&support, 0, NULL, &allocator);
    rclc_node_init_default(&node, "esp32_teleop", "", &support);

    rclc_subscription_init_default(
        &sub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "/cmd_vel"
    );

    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_subscription(
        &executor, &sub, &twist_msg, &twist_callback, ON_NEW_DATA
    );
}

// ─────────────────────────────────────────
// LOOP
// ─────────────────────────────────────────
void loop() {
    MPU6050_Read();
    MPU6050_Convert();
    MPU6050_Angle();

    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
    delay(90);
}
```
**what the code does**
>ESP32 connects to WiFi and listens for movement commands from the Pi, then spins the left and right motors accordingly. It also counts wheel ticks and reads the tilt sensor in the background, but doesn't use that data for anything.
---


## Step 2 — Terminal 1 — micro-ROS Agent

```bash
ssh astra8@raspi.local
sudo docker start ros2_humble
docker run -it --rm --network host --privileged \
  microros/micro-ros-agent:humble udp4 --port 8888
```

→ Press **EN** on ESP32  
→ Wait for `session_established` in the agent log

---

## Step 3 — Terminal 2 — Teleop keyboard

Open a **new terminal** (keep Terminal 1 running):

```bash
ssh astra8@raspi.local
sudo docker exec -it ros2_humble bash
source /opt/ros/humble/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```

Use `i` / `,` to go forward/back, `j` / `l` to turn, `k` to stop.

---

## One-shot test (inside ros2_humble container, optional)

If you want to verify the topic before running teleop:

```bash
# check ESP32 node is visible
ros2 node list

# check /cmd_vel exists
ros2 topic list

# send a single forward command
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.2, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}"

# stop
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.0, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}"
```

---

## Monitor serial (optional debug)

```bash
# Linux
screen /dev/ttyUSB0 115200

# or
python3 -m serial.tools.miniterm /dev/ttyUSB0 115200
```

---

## Known issues in this version

| Issue | Fix |
|---|---|
| GPIO 25/26/27 shared between motors and encoders | Remap encoder pins |
| GPIO 14 shared between LEFT_IN2 and MPU SCL | Remap SCL to GPIO 22 |
| `AGENT_IP` is blank by default | Fill in before flash |
| Encoder counts not published | Add `/odom` publisher |
| IMU data read but not published | Add `/imu` publisher |

---

## Extended Version — `teleop_full.ino`

Adds **watchdog safety stop**, **PID closed-loop velocity control**, **`/odom` publisher**, and **`/joint_states` publisher** on top of the basic teleop.

### Flow (runs at 20 Hz)

```
       /cmd_vel ──► callback ──► target_left_w, target_right_w
                                          │
   ┌──────────────────────────────────────┼──────────────────────────┐
   │                          (every loop tick)                       │
   │   Watchdog: no cmd for 500ms? → targets = 0                     │
   │      ▼                                                           │
   │   Read encoder counts → measured wheel rad/s                    │
   │      ▼                                                           │
   │   PID: error = target - measured → PWM                          │
   │      ▼                                                           │
   │   Motor_Left() / Motor_Right()                                  │
   │      ▼                                                           │
   │   update_odom() → publish /odom                                 │
   │      ▼                                                           │
   │   publish /joint_states                                         │
   └──────────────────────────────────────────────────────────────────┘
```

### Tune before first run

| Param | Where | How |
|---|---|---|
| `TICKS_PER_REV` | top of file | Spin wheel one full rotation by hand, read `left_count` diff |
| `KP / KI / KD` | top of file | Start KP only → increase till oscillation → back off 50% → add KI / KD |
| `AGENT_IP` | WiFi block | RPi IP |
| Encoder pins | remapped to 32–35 to avoid motor pin clash | verify wiring |

### Verify after flash

```bash
ros2 topic list           # /cmd_vel /odom /joint_states
ros2 topic echo /odom     # pose updates when you push the rover
ros2 topic hz /odom       # ~20 Hz
```

Push the rover by hand (motors off) → `/odom` x/y should change. That's your encoder sanity check.

### Full code

```cpp
// ═══════════════════════════════════════════════════════════════
//  ESP32 micro-ROS Differential Drive Rover — FULL VERSION
//  Subscribes: /cmd_vel
//  Publishes : /odom, /joint_states
//  Features  : Watchdog safety stop, PID closed-loop velocity
// ═══════════════════════════════════════════════════════════════

#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/twist.h>
#include <nav_msgs/msg/odometry.h>
#include <sensor_msgs/msg/joint_state.h>
#include <rosidl_runtime_c/string_functions.h>

// ─────────────────────────────────────────
// PIN CONFIG  (⚠️ fix conflicts before deploying)
// ─────────────────────────────────────────
#define RIGHT_IN1   25
#define RIGHT_IN2   26
#define LEFT_IN1    27
#define LEFT_IN2    14

#define LEFT_ENC_A  34   // ← remapped to avoid clash with motor pins
#define LEFT_ENC_B  35
#define RIGHT_ENC_A 32
#define RIGHT_ENC_B 33

// ─────────────────────────────────────────
// WiFi
// ─────────────────────────────────────────
#define WIFI_SSID   "astra8"
#define WIFI_PASS   "12345678"
#define AGENT_IP    "192.168.1.100"    // ← RPi IP
#define AGENT_PORT  8888

// ─────────────────────────────────────────
// PWM (LEDC v3 API)
// ─────────────────────────────────────────
#define PWM_FREQ    1000
#define PWM_RES     10        // 0–1023

// ─────────────────────────────────────────
// ROBOT PARAMS
// ─────────────────────────────────────────
#define WHEEL_RADIUS   0.0325f    // metres  (65 mm wheel)
#define WHEELBASE      0.16f      // metres
#define TICKS_PER_REV  840.0f     // N20 298:1 with quadrature (verify yours)

// PID gains (tune these)
#define KP   200.0f
#define KI    50.0f
#define KD     5.0f

// Watchdog — stop if no /cmd_vel for this long (ms)
#define CMD_TIMEOUT_MS 500

// Loop period (ms)
#define LOOP_DT_MS 50            // 20 Hz control loop

// ─────────────────────────────────────────
// GLOBALS
// ─────────────────────────────────────────
volatile int32_t left_count  = 0;
volatile int32_t right_count = 0;
int32_t  left_count_prev  = 0;
int32_t  right_count_prev = 0;

// Velocity command from /cmd_vel (target wheel velocities in rad/s)
float target_left_w  = 0.0f;
float target_right_w = 0.0f;

// PID state
float left_integral  = 0.0f, left_prev_err  = 0.0f;
float right_integral = 0.0f, right_prev_err = 0.0f;

// Odometry pose
float pose_x = 0.0f, pose_y = 0.0f, pose_th = 0.0f;

// Timing
unsigned long last_cmd_ms = 0;
unsigned long last_loop_ms = 0;

// micro-ROS handles
rcl_node_t          node;
rcl_subscription_t  cmd_sub;
rcl_publisher_t     odom_pub;
rcl_publisher_t     joint_pub;
rclc_executor_t     executor;
rclc_support_t      support;
rcl_allocator_t     allocator;

geometry_msgs__msg__Twist     twist_msg;
nav_msgs__msg__Odometry       odom_msg;
sensor_msgs__msg__JointState  joint_msg;

// JointState needs string arrays + double arrays — allocate statically
double joint_pos[2] = {0.0, 0.0};
double joint_vel[2] = {0.0, 0.0};
rosidl_runtime_c__String joint_names[2];

// ─────────────────────────────────────────
// ENCODER ISRs
// ─────────────────────────────────────────
void IRAM_ATTR leftEncoderISR()  {
    if (digitalRead(LEFT_ENC_B))  left_count++;  else left_count--;
}
void IRAM_ATTR rightEncoderISR() {
    if (digitalRead(RIGHT_ENC_B)) right_count++; else right_count--;
}

// ─────────────────────────────────────────
// MOTOR DRIVE
// ─────────────────────────────────────────
void Motor_Left(int pwm) {
    pwm = constrain(pwm, -1023, 1023);
    if (pwm > 0)      { ledcAttach(LEFT_IN1, PWM_FREQ, PWM_RES); ledcWrite(LEFT_IN1, pwm);  digitalWrite(LEFT_IN2, LOW); }
    else if (pwm < 0) { ledcAttach(LEFT_IN2, PWM_FREQ, PWM_RES); ledcWrite(LEFT_IN2, -pwm); digitalWrite(LEFT_IN1, LOW); }
    else              { ledcDetach(LEFT_IN1); ledcDetach(LEFT_IN2);
                        digitalWrite(LEFT_IN1, LOW); digitalWrite(LEFT_IN2, LOW); }
}
void Motor_Right(int pwm) {
    pwm = constrain(pwm, -1023, 1023);
    if (pwm > 0)      { ledcAttach(RIGHT_IN1, PWM_FREQ, PWM_RES); ledcWrite(RIGHT_IN1, pwm);  digitalWrite(RIGHT_IN2, LOW); }
    else if (pwm < 0) { ledcAttach(RIGHT_IN2, PWM_FREQ, PWM_RES); ledcWrite(RIGHT_IN2, -pwm); digitalWrite(RIGHT_IN1, LOW); }
    else              { ledcDetach(RIGHT_IN1); ledcDetach(RIGHT_IN2);
                        digitalWrite(RIGHT_IN1, LOW); digitalWrite(RIGHT_IN2, LOW); }
}
void Motor_Stop() {
    Motor_Left(0); Motor_Right(0);
}

// ─────────────────────────────────────────
// /cmd_vel CALLBACK
//   Converts (linear, angular) → target wheel angular velocities
// ─────────────────────────────────────────
void twist_callback(const void* msg_in) {
    const geometry_msgs__msg__Twist* msg = (const geometry_msgs__msg__Twist*)msg_in;

    float v = msg->linear.x;       // m/s
    float w = msg->angular.z;      // rad/s

    // Differential drive kinematics
    float v_left  = v - (w * WHEELBASE / 2.0f);
    float v_right = v + (w * WHEELBASE / 2.0f);

    // Convert m/s → wheel rad/s
    target_left_w  = v_left  / WHEEL_RADIUS;
    target_right_w = v_right / WHEEL_RADIUS;

    last_cmd_ms = millis();        // pet the watchdog
}

// ─────────────────────────────────────────
// PID — returns PWM for one wheel
// ─────────────────────────────────────────
int pid_step(float target_w, float measured_w,
             float &integral, float &prev_err, float dt) {
    float err = target_w - measured_w;
    integral += err * dt;
    integral = constrain(integral, -5.0f, 5.0f);   // anti-windup
    float deriv = (err - prev_err) / dt;
    prev_err = err;

    float out = KP * err + KI * integral + KD * deriv;
    return (int)constrain(out, -1023.0f, 1023.0f);
}

// ─────────────────────────────────────────
// ODOMETRY UPDATE
// ─────────────────────────────────────────
void update_odom(float left_w, float right_w, float dt) {
    float v_left  = left_w  * WHEEL_RADIUS;
    float v_right = right_w * WHEEL_RADIUS;
    float v = (v_left + v_right) / 2.0f;
    float w = (v_right - v_left) / WHEELBASE;

    pose_th += w * dt;
    pose_x  += v * cosf(pose_th) * dt;
    pose_y  += v * sinf(pose_th) * dt;

    // Fill odom message
    odom_msg.pose.pose.position.x = pose_x;
    odom_msg.pose.pose.position.y = pose_y;
    odom_msg.pose.pose.position.z = 0.0;
    // Quaternion from yaw
    odom_msg.pose.pose.orientation.z = sinf(pose_th / 2.0f);
    odom_msg.pose.pose.orientation.w = cosf(pose_th / 2.0f);
    odom_msg.twist.twist.linear.x  = v;
    odom_msg.twist.twist.angular.z = w;
}

// ─────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    set_microros_wifi_transports(WIFI_SSID, WIFI_PASS, AGENT_IP, AGENT_PORT);
    delay(2000);

    // Motor pins
    pinMode(LEFT_IN1,  OUTPUT); pinMode(LEFT_IN2,  OUTPUT);
    pinMode(RIGHT_IN1, OUTPUT); pinMode(RIGHT_IN2, OUTPUT);
    Motor_Stop();

    // Encoder pins + ISRs
    pinMode(LEFT_ENC_A,  INPUT_PULLUP); pinMode(LEFT_ENC_B,  INPUT_PULLUP);
    pinMode(RIGHT_ENC_A, INPUT_PULLUP); pinMode(RIGHT_ENC_B, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(LEFT_ENC_A),  leftEncoderISR,  RISING);
    attachInterrupt(digitalPinToInterrupt(RIGHT_ENC_A), rightEncoderISR, RISING);

    // micro-ROS init
    allocator = rcl_get_default_allocator();
    rclc_support_init(&support, 0, NULL, &allocator);
    rclc_node_init_default(&node, "esp32_rover", "", &support);

    // Subscriber
    rclc_subscription_init_default(&cmd_sub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), "/cmd_vel");

    // Publishers
    rclc_publisher_init_default(&odom_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(nav_msgs, msg, Odometry), "/odom");
    rclc_publisher_init_default(&joint_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, JointState), "/joint_states");

    // Init joint_state arrays
    rosidl_runtime_c__String__init(&joint_names[0]);
    rosidl_runtime_c__String__init(&joint_names[1]);
    rosidl_runtime_c__String__assign(&joint_names[0], "left_wheel_joint");
    rosidl_runtime_c__String__assign(&joint_names[1], "right_wheel_joint");
    joint_msg.name.data     = joint_names; joint_msg.name.size     = 2; joint_msg.name.capacity     = 2;
    joint_msg.position.data = joint_pos;   joint_msg.position.size = 2; joint_msg.position.capacity = 2;
    joint_msg.velocity.data = joint_vel;   joint_msg.velocity.size = 2; joint_msg.velocity.capacity = 2;

    // Odom frame
    odom_msg.header.frame_id.data = (char*)"odom";
    odom_msg.header.frame_id.size = 4;
    odom_msg.child_frame_id.data  = (char*)"base_link";
    odom_msg.child_frame_id.size  = 9;

    // Executor
    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_subscription(&executor, &cmd_sub, &twist_msg,
                                   &twist_callback, ON_NEW_DATA);

    last_cmd_ms  = millis();
    last_loop_ms = millis();
}

// ─────────────────────────────────────────
// LOOP — runs at 20 Hz
// ─────────────────────────────────────────
void loop() {
    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(5));

    unsigned long now = millis();
    if (now - last_loop_ms < LOOP_DT_MS) return;
    float dt = (now - last_loop_ms) / 1000.0f;
    last_loop_ms = now;

    // ── WATCHDOG ─────────────────────────
    if (now - last_cmd_ms > CMD_TIMEOUT_MS) {
        target_left_w = 0.0f;
        target_right_w = 0.0f;
    }

    // ── READ ENCODERS ────────────────────
    noInterrupts();
    int32_t lc = left_count;
    int32_t rc = right_count;
    interrupts();

    int32_t d_left  = lc - left_count_prev;
    int32_t d_right = rc - right_count_prev;
    left_count_prev  = lc;
    right_count_prev = rc;

    // Measured wheel angular velocity (rad/s)
    float left_w_meas  = (d_left  / TICKS_PER_REV) * 2.0f * PI / dt;
    float right_w_meas = (d_right / TICKS_PER_REV) * 2.0f * PI / dt;

    // ── PID CONTROL ──────────────────────
    int pwm_left  = pid_step(target_left_w,  left_w_meas,
                             left_integral,  left_prev_err,  dt);
    int pwm_right = pid_step(target_right_w, right_w_meas,
                             right_integral, right_prev_err, dt);

    Motor_Left(pwm_left);
    Motor_Right(pwm_right);

    // ── ODOM ─────────────────────────────
    update_odom(left_w_meas, right_w_meas, dt);
    rcl_publish(&odom_pub, &odom_msg, NULL);

    // ── JOINT STATES ─────────────────────
    joint_pos[0] += left_w_meas  * dt;   // accumulated wheel angle
    joint_pos[1] += right_w_meas * dt;
    joint_vel[0] = left_w_meas;
    joint_vel[1] = right_w_meas;
    rcl_publish(&joint_pub, &joint_msg, NULL);
}
```
**what the code does**
>ESP32 listens for movement commands and uses wheel-tick feedback to actually hit the requested speed accurately, while constantly reporting back where it has moved. It also stops itself automatically if commands stop arriving.

### Note on header timestamps

The code above doesn't fill `odom_msg.header.stamp` — RViz will warn but topic still publishes fine. For production, sync time with the agent and set the stamp. Fine for workshop / bench testing.
