#include <micro_ros_arduino.h>

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/int32.h>

rcl_subscription_t subscriber;
std_msgs__msg__Int32 msg;

rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

#define LED_PIN 2

void subscription_callback(const void * msgin)
{
  const std_msgs__msg__Int32 * msg =
    (const std_msgs__msg__Int32 *)msgin;

  int pwm_value = msg->data;

  if (pwm_value < 0) pwm_value = 0;
  if (pwm_value > 255) pwm_value = 255;

  analogWrite(LED_PIN, pwm_value);
}

void setup()
{
  Serial.begin(115200);
  delay(2000);

  set_microros_transports();

  pinMode(LED_PIN, OUTPUT);

  allocator = rcl_get_default_allocator();

  rclc_support_init(&support, 0, NULL, &allocator);

  rclc_node_init_default(
    &node,
    "esp32_pwm_node",
    "",
    &support);

  rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "pwm_value");

  rclc_executor_init(
    &executor,
    &support.context,
    1,
    &allocator);

  rclc_executor_add_subscription(
    &executor,
    &subscriber,
    &msg,
    &subscription_callback,
    ON_NEW_DATA);
}

void loop()
{
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
}
