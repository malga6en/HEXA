#!/bin/bash
set -e

source /opt/ros/jazzy/setup.bash
source /opt/microros_ws/install/setup.bash

export ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-0}

echo "Starting micro-ROS Agent"
echo "ROS_DOMAIN_ID=${ROS_DOMAIN_ID}"
echo "Transport: UDP"
echo "Port: 8888"

exec ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888 -v 4