//------------------------------------------
// HEXA NODE - ROS 2 Jazzy / micro-ROS
//------------------------------------------

#include <Arduino.h>
#include <micro_ros_platformio.h>

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

#include <sensor_msgs/msg/imu.h>
#include <std_msgs/msg/string.h>
#include <std_msgs/msg/u_int16_multi_array.h>
#include <HardwareSerial.h>

#include "secrets.h"

#define RX_PIN 44
#define TX_PIN 43

// ============================================================
// NETWORK
// ============================================================
IPAddress agent_ip(
    AGENT_IP_1,
    AGENT_IP_2,
    AGENT_IP_3,
    AGENT_IP_4
);

size_t agent_port = 8888;

char ssid[] = WIFI_SSID;
char psk[]  = WIFI_PASSWORD;

// ============================================================
// STATE
// ============================================================

bool letter_s = false;
bool letter_n = false;

bool bno_available = false;

// ============================================================
// ULTRASONIC SENSORS
// ============================================================

const int trig_frontult1 = 1;
const int echo_frontult1 = 2;

const int trig_leftult2 = 3;
const int echo_leftult2 = 4;

const int trig_rightult3 = 5;
const int echo_rightult3 = 6;

float duration_ult1 = 0;
float duration_ult2 = 0;
float duration_ult3 = 0;

float distance_ult1 = 0;
float distance_ult2 = 0;
float distance_ult3 = 0;

// ============================================================
// RGBW LEDs
// ============================================================

int r_red   = 36;
int r_green = 37;
int r_blue  = 38;
int r_white = 45;

int l_red   = 16;
int l_green = 33;
int l_blue  = 34;
int l_white = 35;

int right = 0;
int left  = 0;

// ============================================================
// LEGS / SERVO CHANNELS
// ============================================================
HardwareSerial ServoSerial(1);

int leg1_body   = 26;
int leg1_middle = 25;
int leg1_foot   = 24;

int leg2_body   = 22;
int leg2_middle = 21;
int leg2_foot   = 20;

int leg3_body   = 18;
int leg3_middle = 17;
int leg3_foot   = 16;

int leg4_body   = 2;
int leg4_middle = 1;
int leg4_foot   = 0;

int leg5_body   = 6;
int leg5_middle = 4;
int leg5_foot   = 5;

int leg6_body   = 10;
int leg6_middle = 9;
int leg6_foot   = 8;

// ============================================================
// BNO055
// ============================================================

Adafruit_BNO055 bno =
    Adafruit_BNO055(55, 0x28, &Wire);

// ============================================================
// micro-ROS
// ============================================================

rcl_allocator_t allocator;
rclc_support_t support;
rcl_node_t node;

rcl_publisher_t publisher_ult;
rcl_publisher_t publisher_imu;

rcl_subscription_t subscriber_rgbw;
rcl_subscription_t subscriber_move;

rcl_timer_t timer_ult;
rcl_timer_t timer_imu;

rclc_executor_t executor;

// ============================================================
// MESSAGES
// ============================================================

std_msgs__msg__String received_msg_rgbw;
std_msgs__msg__String received_msg_move;

char received_buffer_rgbw[50];
char received_buffer_move[50];

std_msgs__msg__UInt16MultiArray msg_ult;
sensor_msgs__msg__Imu msg_imu;

// ============================================================
// FORWARD DECLARATIONS
// ============================================================

void error_loop();

void SubscriptionCallback_RGBW(const void *msgin);
void SubscriptionCallback_Move(const void *msgin);

void timer_callback_ult(
    rcl_timer_t *timer,
    int64_t last_call_time);

void timer_callback_imu(
    rcl_timer_t *timer,
    int64_t last_call_time);

void moveMotorCommand(
    int ch,
    int pw,
    int spd,
    int tm);

void commitMoveMotorCommands();

void updateUltrasonicSensors();
void updateLEDs();
void executeMovement();

// ============================================================
// ROS ERROR HANDLING
// ============================================================

#define RCCHECK(fn)                                      \
{                                                        \
    rcl_ret_t temp_rc = fn;                              \
    if (temp_rc != RCL_RET_OK)                           \
    {                                                    \
        error_loop();                                    \
    }                                                    \
}

#define RCSOFTCHECK(fn)                                  \
{                                                        \
    rcl_ret_t temp_rc = fn;                              \
    (void)temp_rc;                                       \
}

void error_loop()
{
    while (1)
    {
        delay(100);
    }
}

// ============================================================
// TIMER: ULTRASONIC
// ============================================================

void timer_callback_ult(
    rcl_timer_t *timer,
    int64_t last_call_time)
{
    (void)last_call_time;

    if (timer == NULL)
        return;

    msg_ult.data.data[0] =
        (uint16_t)distance_ult1;

    msg_ult.data.data[1] =
        (uint16_t)distance_ult2;

    msg_ult.data.data[2] =
        (uint16_t)distance_ult3;

    RCSOFTCHECK(
        rcl_publish(
            &publisher_ult,
            &msg_ult,
            NULL
        )
    );
}

// ============================================================
// TIMER: IMU
// ============================================================

void timer_callback_imu(
    rcl_timer_t *timer,
    int64_t last_call_time)
{
    (void)last_call_time;

    if (timer == NULL)
        return;

    // If BNO055 is unavailable, do not block micro-ROS.
    if (!bno_available)
        return;

    imu::Quaternion quat = bno.getQuat();

    msg_imu.orientation.x = quat.x();
    msg_imu.orientation.y = quat.y();
    msg_imu.orientation.z = quat.z();
    msg_imu.orientation.w = quat.w();

    imu::Vector<3> accel =
        bno.getVector(
            Adafruit_BNO055::VECTOR_ACCELEROMETER
        );

    msg_imu.linear_acceleration.x = accel.x();
    msg_imu.linear_acceleration.y = accel.y();
    msg_imu.linear_acceleration.z = accel.z();

    imu::Vector<3> gyro =
        bno.getVector(
            Adafruit_BNO055::VECTOR_GYROSCOPE
        );

    msg_imu.angular_velocity.x = gyro.x();
    msg_imu.angular_velocity.y = gyro.y();
    msg_imu.angular_velocity.z = gyro.z();

    RCSOFTCHECK(
        rcl_publish(
            &publisher_imu,
            &msg_imu,
            NULL
        )
    );
}

// ============================================================
// RGBW CALLBACK
// ============================================================

void SubscriptionCallback_RGBW(const void *msgin)
{
    const std_msgs__msg__String *msg =
        (const std_msgs__msg__String *)msgin;

    if (msg->data.data == NULL)
        return;

    // Right LED
    if (strcmp(msg->data.data, "r-r") == 0)
        right = 1;

    else if (strcmp(msg->data.data, "r-g") == 0)
        right = 2;

    else if (strcmp(msg->data.data, "r-b") == 0)
        right = 3;

    else if (strcmp(msg->data.data, "r-w") == 0)
        right = 4;

    else if (strcmp(msg->data.data, "r-off") == 0)
        right = 0;

    // Left LED
    if (strcmp(msg->data.data, "l-r") == 0)
        left = 1;

    else if (strcmp(msg->data.data, "l-g") == 0)
        left = 2;

    else if (strcmp(msg->data.data, "l-b") == 0)
        left = 3;

    else if (strcmp(msg->data.data, "l-w") == 0)
        left = 4;

    else if (strcmp(msg->data.data, "l-off") == 0)
        left = 0;
}

// ============================================================
// MOVEMENT CALLBACK
// ============================================================

void SubscriptionCallback_Move(const void *msgin)
{
    const std_msgs__msg__String *msg =
        (const std_msgs__msg__String *)msgin;

    if (msg->data.data == NULL)
        return;

    if (strcmp(msg->data.data, "S") == 0)
    {
        letter_s = true;
        letter_n = false;
    }
    else if (strcmp(msg->data.data, "N") == 0)
    {
        letter_n = true;
        letter_s = false;
    }
    else if (strcmp(msg->data.data, "T") == 0)
    {
        // Test only ONE servo
        moveMotorCommand(leg1_body, 1500, 0, 0);
        commitMoveMotorCommands();
    }
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
    // --------------------------------------------------------
    // Servo controller serial connection
    // --------------------------------------------------------

    Serial.begin(115200);

    ServoSerial.begin(
        9600,
        SERIAL_8N1,
        RX_PIN,
        TX_PIN
    );
    

    // --------------------------------------------------------
    // LEDs
    // --------------------------------------------------------

    pinMode(l_red, OUTPUT);
    pinMode(l_green, OUTPUT);
    pinMode(l_blue, OUTPUT);
    pinMode(l_white, OUTPUT);

    pinMode(r_red, OUTPUT);
    pinMode(r_green, OUTPUT);
    pinMode(r_blue, OUTPUT);
    pinMode(r_white, OUTPUT);

    // --------------------------------------------------------
    // Ultrasonic sensors
    // --------------------------------------------------------

    pinMode(trig_frontult1, OUTPUT);
    pinMode(echo_frontult1, INPUT);

    pinMode(trig_leftult2, OUTPUT);
    pinMode(echo_leftult2, INPUT);

    pinMode(trig_rightult3, OUTPUT);
    pinMode(echo_rightult3, INPUT);

    // --------------------------------------------------------
    // BNO055
    // --------------------------------------------------------

    bno_available = bno.begin();

    if (bno_available)
    {
        bno.setMode(OPERATION_MODE_CONFIG);
        delay(25);

        // Existing calibration values can be restored later.
        // For the first Jazzy test we keep this simple.

        bno.setMode(OPERATION_MODE_NDOF);
        delay(25);
    }

    // --------------------------------------------------------
    // micro-ROS WiFi transport
    // --------------------------------------------------------

    set_microros_wifi_transports(
        ssid,
        psk,
        agent_ip,
        agent_port
    );

    delay(2000);

    // --------------------------------------------------------
    // String message buffers
    // --------------------------------------------------------

    received_msg_rgbw.data.data =
        received_buffer_rgbw;

    received_msg_rgbw.data.capacity =
        sizeof(received_buffer_rgbw);

    received_msg_rgbw.data.size = 0;


    received_msg_move.data.data =
        received_buffer_move;

    received_msg_move.data.capacity =
        sizeof(received_buffer_move);

    received_msg_move.data.size = 0;

    // --------------------------------------------------------
    // Ultrasonic array
    // --------------------------------------------------------

    msg_ult.data.capacity = 3;
    msg_ult.data.size = 3;

    msg_ult.data.data =
        (uint16_t *)malloc(
            3 * sizeof(uint16_t)
        );

    if (msg_ult.data.data == NULL)
    {
        error_loop();
    }

    // --------------------------------------------------------
    // ROS allocator/support
    //
    // Domain 0 is used implicitly here.
    // This matches our working minimal Jazzy test.
    // --------------------------------------------------------

    allocator = rcl_get_default_allocator();

    RCCHECK(
        rclc_support_init(
            &support,
            0,
            NULL,
            &allocator
        )
    );

    // --------------------------------------------------------
    // Node
    // --------------------------------------------------------

    RCCHECK(
        rclc_node_init_default(
            &node,
            "HEXA_Node",
            "",
            &support
        )
    );

    // --------------------------------------------------------
    // Publishers
    // --------------------------------------------------------

    RCCHECK(
        rclc_publisher_init_default(
            &publisher_ult,
            &node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(
                std_msgs,
                msg,
                UInt16MultiArray
            ),
            "UltrasonicSensors_Publisher"
        )
    );

    RCCHECK(
        rclc_publisher_init_default(
            &publisher_imu,
            &node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(
                sensor_msgs,
                msg,
                Imu
            ),
            "IMU_Publisher"
        )
    );

    // --------------------------------------------------------
    // Subscribers
    // --------------------------------------------------------

    RCCHECK(
        rclc_subscription_init_default(
            &subscriber_rgbw,
            &node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(
                std_msgs,
                msg,
                String
            ),
            "RGBLEDs_Subscriber"
        )
    );

    RCCHECK(
        rclc_subscription_init_default(
            &subscriber_move,
            &node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(
                std_msgs,
                msg,
                String
            ),
            "Move_Subscriber"
        )
    );

    // --------------------------------------------------------
    // Timers
    //
    // IMPORTANT:
    // Initialize BEFORE adding to executor.
    // --------------------------------------------------------

    RCCHECK(
        rclc_timer_init_default2(
            &timer_ult,
            &support,
            RCL_MS_TO_NS(1000),
            timer_callback_ult,
            true
        )
    );

    RCCHECK(
        rclc_timer_init_default2(
            &timer_imu,
            &support,
            RCL_MS_TO_NS(1000),
            timer_callback_imu,
            true
        )
    );

    // --------------------------------------------------------
    // Executor
    //
    // 2 timers + 2 subscriptions = 4 handles
    // --------------------------------------------------------

    RCCHECK(
        rclc_executor_init(
            &executor,
            &support.context,
            4,
            &allocator
        )
    );

    RCCHECK(
        rclc_executor_add_timer(
            &executor,
            &timer_ult
        )
    );

    RCCHECK(
        rclc_executor_add_timer(
            &executor,
            &timer_imu
        )
    );

    RCCHECK(
        rclc_executor_add_subscription(
            &executor,
            &subscriber_rgbw,
            &received_msg_rgbw,
            &SubscriptionCallback_RGBW,
            ON_NEW_DATA
        )
    );

    RCCHECK(
        rclc_executor_add_subscription(
            &executor,
            &subscriber_move,
            &received_msg_move,
            &SubscriptionCallback_Move,
            ON_NEW_DATA
        )
    );
}

// ============================================================
// MOTOR COMMAND
// ============================================================

void moveMotorCommand(int ch, int pw, int spd, int tm)
{
    if ((spd != 0) && (tm != 0))
        return;

    if (pw < 500 || pw > 2500)
        return;

    ServoSerial.print("#");
    ServoSerial.print(ch);
    ServoSerial.print("P");
    ServoSerial.print(pw);

    if (spd != 0)
    {
        ServoSerial.print("S");
        ServoSerial.print(spd);
    }

    if (tm != 0)
    {
        ServoSerial.print("T");
        ServoSerial.print(tm);
    }
}

void commitMoveMotorCommands()
{
    ServoSerial.write('\r');   // ASCII 13
}


// ============================================================
// ULTRASONIC UPDATE
// ============================================================

void updateUltrasonicSensors()
{
    // Sensor 1

    digitalWrite(trig_frontult1, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_frontult1, LOW);

    duration_ult1 =
        pulseIn(
            echo_frontult1,
            HIGH,
            30000
        );

    distance_ult1 =
        duration_ult1 * 0.0343f / 2.0f;

    // Sensor 2

    digitalWrite(trig_leftult2, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_leftult2, LOW);

    duration_ult2 =
        pulseIn(
            echo_leftult2,
            HIGH,
            30000
        );

    distance_ult2 =
        duration_ult2 * 0.0343f / 2.0f;

    // Sensor 3

    digitalWrite(trig_rightult3, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_rightult3, LOW);

    duration_ult3 =
        pulseIn(
            echo_rightult3,
            HIGH,
            30000
        );

    distance_ult3 =
        duration_ult3 * 0.0343f / 2.0f;
}

// ============================================================
// LED UPDATE
// ============================================================

void updateLEDs()
{
    // RIGHT

    digitalWrite(r_red,   right == 1);
    digitalWrite(r_green, right == 2);
    digitalWrite(r_blue,  right == 3);
    digitalWrite(r_white, right == 4);

    // LEFT

    digitalWrite(l_red,   left == 1);
    digitalWrite(l_green, left == 2);
    digitalWrite(l_blue,  left == 3);
    digitalWrite(l_white, left == 4);
}

// ============================================================
// MOVEMENT
// ============================================================

void executeMovement()
{
    // --------------------------------------------------------
    // WALK SEQUENCE
    // --------------------------------------------------------

    if (letter_s)
    {
        // Group A up

        moveMotorCommand(
            leg1_middle, 1500, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg3_middle, 1500, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg5_middle, 1500, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        // Group A forward

        moveMotorCommand(
            leg1_body, 1650, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg3_body, 1650, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg5_body, 1350, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        // Group B backward

        moveMotorCommand(
            leg2_body, 1500, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg4_body, 1500, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg6_body, 1500, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        // Group A down

        moveMotorCommand(
            leg1_middle, 1700, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg3_middle, 1700, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg5_middle, 1700, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        // Group B up

        moveMotorCommand(
            leg2_middle, 1500, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg4_middle, 1500, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg6_middle, 1500, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        // Group B forward

        moveMotorCommand(
            leg2_body, 1650, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg4_body, 1350, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg6_body, 1350, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        // Group A backward

        moveMotorCommand(
            leg1_body, 1500, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg3_body, 1500, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg5_body, 1500, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        // Group B down

        moveMotorCommand(
            leg2_middle, 1700, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg4_middle, 1700, 1000, 0);
        commitMoveMotorCommands();
        delay(500);

        moveMotorCommand(
            leg6_middle, 1700, 1000, 0);
        commitMoveMotorCommands();

        letter_s = false;
    }

    // --------------------------------------------------------
    // NEUTRAL POSITION
    // --------------------------------------------------------

    if (letter_n)
    {
        moveMotorCommand(
            leg1_body, 1500, 1000, 0);
        moveMotorCommand(
            leg2_body, 1500, 1000, 0);
        moveMotorCommand(
            leg3_body, 1500, 1000, 0);
        moveMotorCommand(
            leg4_body, 1500, 1000, 0);
        moveMotorCommand(
            leg5_body, 1500, 1000, 0);
        moveMotorCommand(
            leg6_body, 1500, 1000, 0);

        moveMotorCommand(
            leg1_middle, 1700, 1000, 0);
        moveMotorCommand(
            leg2_middle, 1700, 1000, 0);
        moveMotorCommand(
            leg3_middle, 1700, 1000, 0);
        moveMotorCommand(
            leg4_middle, 1700, 1000, 0);
        moveMotorCommand(
            leg5_middle, 1700, 1000, 0);
        moveMotorCommand(
            leg6_middle, 1700, 1000, 0);

        moveMotorCommand(
            leg1_foot, 1500, 1000, 0);
        moveMotorCommand(
            leg2_foot, 1500, 1000, 0);
        moveMotorCommand(
            leg3_foot, 1500, 1000, 0);
        moveMotorCommand(
            leg4_foot, 1500, 1000, 0);
        moveMotorCommand(
            leg5_foot, 1500, 1000, 0);
        moveMotorCommand(
            leg6_foot, 1500, 1000, 0);

        commitMoveMotorCommands();

        letter_n = false;
    }
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
    // Keep executor responsive.
    RCSOFTCHECK(
        rclc_executor_spin_some(
            &executor,
            RCL_MS_TO_NS(20)
        )
    );

    updateUltrasonicSensors();

    updateLEDs();

    executeMovement();

    delay(10);
}