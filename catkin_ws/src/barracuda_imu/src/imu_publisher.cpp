#include "ros/ros.h"
#include <ros/console.h>
#include "sensor_msgs/Imu.h"
#include "serialib.h"
#include <string>
#include "Osc99.h"
#include "NgimuReceive.h"
#include <thread>

// #define PORT "/dev/ngimu"
#define PORT "/dev/ttyACM0"
#define BAUDRATE 115200

ros::Publisher imu_pub;

float quat[4];
float gyro[3];
float accel[3];
float mag[3];
float baro = 0;
float earth[3];

void serial_imu_callback() {
    // Get Relevant ROS Params
    // const char* port;
    // int baudrate;
    // ROS_WARN_COND(!n.getParam("/port", port), "\'port\' wasn't defined as a param!");
    // ROS_WARN_COND(!n.getParam("/baudrate", baudrate), "\'baudrate\' wasn't defined as a param!");

    // Process each received byte
    serialib serial;
    serial.openDevice(PORT, BAUDRATE);
    ROS_INFO("Opened PORT for IMU");
    while(true) {
        while (serial.available() > 0) {
            try
            {
                char* buff = new char[4];
                serial.readBytes(buff, 1, 500, 0);
                printf("Received: %c\n", *buff);
                NgimuReceiveProcessSerialByte(*buff);
                delete[] buff;
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
                
        }
    }
    serial.closeDevice();
}

// This function is called each time there is a receive error
extern void ngimuReceiveErrorCallback(const char* const errorMessage) {
    ROS_ERROR(errorMessage);
}

// This function is called each time a "/sensors" message is received
void ngimuSensorsCallback(const NgimuSensors ngimuSensors) {

    gyro[0] = ngimuSensors.gyroscopeX;
    gyro[1] = ngimuSensors.gyroscopeY;
    gyro[2] = ngimuSensors.gyroscopeZ;

    accel[0] = ngimuSensors.accelerometerX;
    accel[1] = ngimuSensors.accelerometerY;
    accel[2] = ngimuSensors.accelerometerZ;

    mag[0] = ngimuSensors.magnetometerX;
    mag[1] = ngimuSensors.magnetometerY;
    mag[2] = ngimuSensors.magnetometerZ;

    baro = ngimuSensors.barometer;
}

// This function is called each time a "/quaternion" message is received
void ngimuQuaternionCallback(const NgimuQuaternion ngimuQuaternion) {
    quat[0] = ngimuQuaternion.w;
    quat[1] = ngimuQuaternion.x;
    quat[2] = ngimuQuaternion.y;
    quat[3] = ngimuQuaternion.z;

}

// This function is called each time a "/euler" message is received.
void ngimuEulerCallback(const NgimuEuler ngimuEuler) {
    
}

void ngimuEarthAccelerationCallback(const NgimuEarthAcceleration ngimuEarthAcceleration) {
    earth[0] = ngimuEarthAcceleration.x;
    earth[1] = ngimuEarthAcceleration.y;
    earth[2] = ngimuEarthAcceleration.z;
}

void simCallback(const sensor_msgs::Imu::ConstPtr& msg) {
    ROS_DEBUG("Inside simCallback");
    quat[0] = msg->orientation.w;
    quat[1] = msg->orientation.x;
    quat[2] = msg->orientation.y;
    quat[3] = msg->orientation.z;
    ROS_DEBUG("Created quats");

    gyro[0] = msg->angular_velocity.x;
    gyro[1] = msg->angular_velocity.y;
    gyro[2] = msg->angular_velocity.z;

    ROS_DEBUG("Created gyro");
    earth[0] = msg->linear_acceleration.x;
    earth[1] = msg->linear_acceleration.y;
    earth[2] = msg->linear_acceleration.z;
    ROS_DEBUG("Inside accel");
}

int main(int argc, char **argv) {

    // Connection to serial port

    // Initialise NGIMU receive module
        
    NgimuReceiveInitialise();
    // Assign NGIMU receive callback functions
    NgimuReceiveSetReceiveErrorCallback(ngimuReceiveErrorCallback);
    NgimuReceiveSetSensorsCallback(ngimuSensorsCallback);
    NgimuReceiveSetQuaternionCallback(ngimuQuaternionCallback);
    NgimuReceiveSetEulerCallback(ngimuEulerCallback);
    NgimuReceiveSetEarthAccelerationCallback(ngimuEarthAccelerationCallback);
    
    ros::init(argc, argv, "imu_publisher");
    
    ros::NodeHandle n;
    ROS_DEBUG("Inside of IMU_publisher");

    // Get Relevant ROS Params
    bool simulation = false;

    // Retrieve the current namespace
    // std::string namespace_str = n.getNamespace()

    ROS_WARN_COND(!n.getParam("/simulation", simulation), "\'simulation\' wasn't defined as a param!");

    imu_pub = n.advertise<sensor_msgs::Imu>("imu", 1);

    ros::Rate loop_rate(50);
    ros::Subscriber sub; // For some reason sub needs to be declared out here or else the conditional statement
    // won't work
    std::thread t1 (serial_imu_callback);
    if (simulation) {
        ROS_INFO("Simulation IMU - Subscribing to /sim_imu");
        sub = n.subscribe("/sim_imu", 10, simCallback);
    }
    
    else {
        ROS_INFO("Using NGIMU IMU");
    }

    while(ros::ok()) {
        
        sensor_msgs::Imu msg;
        msg.header.stamp = ros::Time::now();
        msg.header.frame_id = "mantaray/base_link";

        msg.orientation_covariance[0] = 0.1;
        msg.orientation_covariance[4] = 0.1;
        msg.orientation_covariance[8] = 0.1;
        msg.angular_velocity_covariance[0] = 0.1;
        msg.angular_velocity_covariance[4] = 0.1;
        msg.angular_velocity_covariance[8] = 0.1;
        msg.linear_acceleration_covariance[0] = 0.1;
        msg.linear_acceleration_covariance[4] = 0.1;
        msg.linear_acceleration_covariance[8] = 0.1;


        msg.orientation.w = quat[0];
        msg.orientation.x = quat[1];
        msg.orientation.y = quat[2];
        msg.orientation.z = quat[3];

        msg.angular_velocity.x = gyro[0];
        msg.angular_velocity.y = gyro[1];
        msg.angular_velocity.z = gyro[2];

        msg.linear_acceleration.x = earth[0];
        msg.linear_acceleration.y = earth[1];
        msg.linear_acceleration.z = earth[2];
        
        imu_pub.publish(msg);
        
        ros::spinOnce();

        loop_rate.sleep();
    }
}
