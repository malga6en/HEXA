FROM osrf/ros:jazzy-desktop

ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DOMAIN_ID=0

# ------------------------------------------------------------
# Basic development tools
# ------------------------------------------------------------

RUN apt-get update && apt-get install -y \
    git \
    curl \
    wget \
    ca-certificates \
    build-essential \
    cmake \
    python3 \
    python3-pip \
    python3-venv \
    python3-colcon-common-extensions \
    && rm -rf /var/lib/apt/lists/*

# ------------------------------------------------------------
# PlatformIO
# Use the default PlatformIO location because
# micro_ros_platformio expects ~/.platformio/penv
# ------------------------------------------------------------

RUN curl -fsSL \
    https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py \
    -o /tmp/get-platformio.py \
    && python3 /tmp/get-platformio.py \
    && rm /tmp/get-platformio.py \
    && ln -s /root/.platformio/penv/bin/pio /usr/local/bin/pio \
    && ln -s /root/.platformio/penv/bin/platformio /usr/local/bin/platformio

# ------------------------------------------------------------
# micro-ROS Agent for ROS 2 Jazzy
#
# Build ONLY the agent and its required sources.
# We deliberately avoid the complete micro_ros_setup rosdep
# installation that caused clang-tidy/flex/usbutils problems.
# ------------------------------------------------------------

RUN mkdir -p /opt/microros_ws/src \
    && cd /opt/microros_ws/src \
    && git clone -b jazzy https://github.com/micro-ROS/micro-ROS-Agent.git micro_ros_agent \
    && git clone -b jazzy https://github.com/micro-ROS/micro_ros_msgs.git micro_ros_msgs

RUN /bin/bash -c " \
    source /opt/ros/jazzy/setup.bash && \
    cd /opt/microros_ws && \
    colcon build \
        --merge-install \
        --cmake-args -DCMAKE_BUILD_TYPE=Release \
    "

# ------------------------------------------------------------
# Shell environment
# ------------------------------------------------------------

RUN echo 'source /opt/ros/jazzy/setup.bash' >> /root/.bashrc \
    && echo 'source /opt/microros_ws/install/setup.bash' >> /root/.bashrc \
    && echo 'export ROS_DOMAIN_ID=0' >> /root/.bashrc

WORKDIR /workspaces/HEXA