# HEXA – ROS 2 Jazzy / micro-ROS Development Environment

HEXA is a six-legged robot based on an **ESP32-S2 Saola-1** running micro-ROS.

The development environment is provided as a **VS Code Dev Container** and contains all Linux-side dependencies required for development:

- Ubuntu 24.04
- ROS 2 Jazzy
- PlatformIO
- ESP32-S2 toolchain
- micro-ROS for PlatformIO
- micro-ROS Agent
- C/C++ development tools

The goal is to provide a reproducible development environment without requiring students to manually install ROS 2, PlatformIO or micro-ROS on their host system.

---

# 1. System Architecture

The development setup consists of three parts:

```text
Windows Host
│
├── VS Code
├── Docker Desktop
├── Git
│
├── USB
│    └── ESP32-S2
│
└── Dev Container
     │
     ├── Ubuntu 24.04
     ├── ROS 2 Jazzy
     ├── PlatformIO
     ├── micro-ROS Agent
     │
     └── HEXA source code
              │
              └── WiFi / UDP
                    │
                    ▼
                 ESP32-S2
                    │
                    ▼
                 HEXA
```

Firmware development and ROS 2 development take place inside the container.

The firmware is currently flashed from Windows using `esptool`.

---

# 2. Requirements

Install the following software on the Windows host:

## Required

- Git
- Docker Desktop
- Visual Studio Code
- VS Code extension: **Dev Containers**

Docker Desktop must be running before opening the project in the Dev Container.

Check Docker with:

```powershell
docker --version
```

Check Git with:

```powershell
git --version
```

---

# 3. Clone the Repository

Clone the repository:

```powershell
git clone https://github.com/malga6en/HEXA.git
cd HEXA
code .
```

For a private repository, GitHub authentication is required.

---

# 4. Open the Development Container

In VS Code press:

```text
Ctrl + Shift + P
```

Select:

```text
Dev Containers: Reopen in Container
```

The first build can take several minutes because Docker has to download and build the development environment.

Subsequent starts are normally much faster.

No manual ROS 2, PlatformIO or micro-ROS installation is required.

---

# 5. Verify the Environment

Open a terminal inside VS Code.

Check ROS 2:

```bash
echo $ROS_DISTRO
```

Expected:

```text
jazzy
```

Check the ROS domain:

```bash
echo $ROS_DOMAIN_ID
```

Default:

```text
0
```

Check PlatformIO:

```bash
pio --version
```

Check the micro-ROS Agent:

```bash
ros2 pkg prefix micro_ros_agent
```

Expected:

```text
/opt/microros_ws/install
```

If all commands work, the development environment is ready.

---

# 6. Local Network Configuration

Network credentials are **not stored in Git**.

Create the local configuration from the provided example:

```bash
cp firmware/include/secrets.example.h firmware/include/secrets.h
```

Edit:

```text
firmware/include/secrets.h
```

Example:

```cpp
#pragma once

#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define AGENT_IP_1 10
#define AGENT_IP_2 94
#define AGENT_IP_3 160
#define AGENT_IP_4 100
```

The Agent IP must be the IPv4 address of the Windows computer running Docker.

On Windows the address can be determined using:

```powershell
ipconfig
```

Select the IPv4 address of the network adapter connected to the same network as HEXA.

`secrets.h` is ignored by Git and must never be committed.

---

# 7. Build the HEXA Firmware

Inside the Dev Container run:

```bash
./scripts/build_firmware.sh
```

The script runs PlatformIO and copies the resulting firmware to:

```text
output/firmware.bin
```

A successful build ends with a PlatformIO `SUCCESS` message.

The first micro-ROS firmware build can take several minutes.

---

# 8. Flash the ESP32-S2

Firmware flashing currently takes place on the Windows host.

## 8.1 Install Python and esptool

Python must be available on Windows:

```powershell
python --version
```

Install esptool:

```powershell
python -m pip install esptool
```

Check:

```powershell
python -m esptool version
```

## 8.2 Determine the COM port

Connect the ESP32-S2 via USB.

The COM port can be found in the Windows Device Manager.

Example:

```text
COM5
```

## 8.3 Flash

From the repository directory in Windows PowerShell:

```powershell
.\scripts\flash.ps1 COM5
```

Replace `COM5` with the actual port.

A successful flash should end with messages similar to:

```text
Hash of data verified.
Hard resetting via RTS pin...
```

---

# 9. Start the micro-ROS Agent

Inside the Dev Container:

```bash
./scripts/start_agent.sh
```

The Agent uses:

```text
Transport: UDP
Port:      8888
Domain:    0
```

A successful start looks similar to:

```text
running... | port: 8888
```

The terminal remains occupied while the Agent is running. This is normal.

Leave this terminal open.

---

# 10. Check the HEXA Connection

Open a second terminal inside the Dev Container.

Run:

```bash
ros2 node list
```

A connected HEXA should provide:

```text
/HEXA_Node
```

List the topics:

```bash
ros2 topic list
```

The HEXA currently provides:

```text
/IMU_Publisher
/Move_Subscriber
/RGBLEDs_Subscriber
/UltrasonicSensors_Publisher
```

---

# 11. Test the Sensors

## IMU

```bash
ros2 topic echo /IMU_Publisher
```

## Ultrasonic Sensors

```bash
ros2 topic echo /UltrasonicSensors_Publisher
```

---

# 12. Test the RGB LEDs

The LED interface uses:

```text
/RGBLEDs_Subscriber
```

with message type:

```text
std_msgs/msg/String
```

Example – right LED red:

```bash
ros2 topic pub --once /RGBLEDs_Subscriber std_msgs/msg/String "{data: 'r-r'}"
```

Right LED:

```text
r-r     red
r-g     green
r-b     blue
r-w     white
r-off   off
```

Left LED:

```text
l-r     red
l-g     green
l-b     blue
l-w     white
l-off   off
```

Example:

```bash
ros2 topic pub --once /RGBLEDs_Subscriber std_msgs/msg/String "{data: 'l-b'}"
```

---

# 13. Test HEXA Movement

The movement interface is:

```text
/Move_Subscriber
```

with message type:

```text
std_msgs/msg/String
```

## Neutral position

```bash
ros2 topic pub --once /Move_Subscriber std_msgs/msg/String "{data: 'N'}"
```

## Walking sequence

```bash
ros2 topic pub --once /Move_Subscriber std_msgs/msg/String "{data: 'S'}"
```

Only execute movement commands when the robot is placed safely and the legs can move freely.

---

# 14. Project Structure

```text
HEXA/
│
├── .devcontainer/
│   ├── Dockerfile
│   └── devcontainer.json
│
├── firmware/
│   ├── include/
│   │   ├── secrets.example.h
│   │   └── secrets.h          # local only, not tracked
│   │
│   ├── src/
│   │   └── main.cpp
│   │
│   └── platformio.ini
│
├── scripts/
│   ├── build_firmware.sh
│   ├── start_agent.sh
│   └── flash.ps1
│
├── output/
│   └── firmware.bin           # generated
│
├── .gitignore
└── README.md
```

---

# 15. Development Workflow

The normal workflow is:

```text
1. Open repository
        ↓
2. Reopen in Container
        ↓
3. Edit firmware
        ↓
4. ./scripts/build_firmware.sh
        ↓
5. Windows: ./scripts/flash.ps1 COMx
        ↓
6. ./scripts/start_agent.sh
        ↓
7. ros2 node list
        ↓
8. Test HEXA
```

---

# 16. Dev Container – Maintainer Documentation

This section explains how the development environment is constructed.

Students normally do **not** need these steps.

## 16.1 Base Image

The container is based on:

```dockerfile
FROM osrf/ros:jazzy-desktop
```

This provides ROS 2 Jazzy on Ubuntu 24.04.

---

## 16.2 Development Tools

The Docker image installs the required Linux development tools using `apt`:

```dockerfile
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
```

---

## 16.3 PlatformIO

PlatformIO is installed inside the container using the PlatformIO Core installer.

It is intentionally installed in the standard PlatformIO location:

```text
/root/.platformio/
```

This is important because `micro_ros_platformio` expects the PlatformIO Python environment at:

```text
/root/.platformio/penv/
```

The Dockerfile installs PlatformIO with:

```dockerfile
RUN curl -fsSL \
    https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py \
    -o /tmp/get-platformio.py \
    && python3 /tmp/get-platformio.py \
    && rm /tmp/get-platformio.py \
    && ln -s /root/.platformio/penv/bin/pio /usr/local/bin/pio \
    && ln -s /root/.platformio/penv/bin/platformio /usr/local/bin/platformio
```

---

## 16.4 micro-ROS Agent

The micro-ROS Agent is built directly into the Docker image.

The source is stored under:

```text
/opt/microros_ws
```

The Dockerfile clones the Jazzy versions of:

```text
micro-ROS-Agent
micro_ros_msgs
```

and builds them using `colcon`.

Example:

```dockerfile
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
```

The resulting installation is located at:

```text
/opt/microros_ws/install
```

It is automatically sourced by the container shell.

This approach avoids requiring students to manually run:

```text
micro_ros_setup
rosdep
create_agent_ws.sh
build_agent.sh
```

---

# 17. Docker Networking

The ESP32 communicates with the micro-ROS Agent via WiFi and UDP.

The Dev Container publishes UDP port `8888`:

```json
"runArgs": [
    "-p",
    "8888:8888/udp"
]
```

The resulting communication path is:

```text
ESP32-S2
    │
    │ WiFi / UDP 8888
    ▼
Windows Host IP
    │
    │ Docker port forwarding
    ▼
Dev Container :8888
    │
    ▼
micro-ROS Agent
    │
    ▼
ROS 2 Jazzy
```

The ESP32 must therefore use the Windows host IPv4 address as its micro-ROS Agent address.

---

# 18. PlatformIO Build Volume

The PlatformIO build directory is stored in a Docker volume:

```json
"mounts": [
    "source=hexa-platformio-build,target=/workspaces/HEXA/firmware/.pio,type=volume"
]
```

This is intentional.

micro-ROS creates a large number of small files during compilation. Building these directly on a Windows bind mount can be significantly slower.

The Docker volume keeps `.pio` on the Linux filesystem and improves build performance.

The source code remains directly accessible from Windows.

---

# 19. ROS Domain

The default ROS domain is:

```text
ROS_DOMAIN_ID=0
```

It is configured in the Dev Container.

All ROS 2 participants that should communicate with HEXA must use the same domain ID.

Check with:

```bash
echo $ROS_DOMAIN_ID
```

---

# 20. Rebuilding the Development Image

After changing the Dockerfile or Dev Container configuration:

```text
Ctrl + Shift + P
```

Select:

```text
Dev Containers: Rebuild and Reopen in Container
```

Docker rebuilds the image and VS Code reconnects to the new container.

After rebuilding, verify:

```bash
echo $ROS_DISTRO
pio --version
ros2 pkg prefix micro_ros_agent
```

Expected:

```text
jazzy
PlatformIO Core ...
/opt/microros_ws/install
```

---

# 21. Creating the Development Environment from Scratch

For a completely new project, the minimum structure is:

```text
project/
├── .devcontainer/
│   ├── Dockerfile
│   └── devcontainer.json
├── firmware/
│   ├── include/
│   ├── src/
│   └── platformio.ini
└── scripts/
```

The required components are:

```text
ROS 2 Jazzy base image
        ↓
Linux development packages
        ↓
PlatformIO Core
        ↓
micro-ROS Agent
        ↓
UDP 8888 Docker forwarding
        ↓
PlatformIO build volume
        ↓
VS Code Dev Container
```

Once `.devcontainer/Dockerfile` and `.devcontainer/devcontainer.json` are committed to Git, another developer only needs to clone the repository and select:

```text
Dev Containers: Reopen in Container
```

Docker then creates the complete development image automatically.

---

# 22. Troubleshooting

## micro-ROS Agent not found

Check:

```bash
ros2 pkg prefix micro_ros_agent
```

Expected:

```text
/opt/microros_ws/install
```

## HEXA node not visible

Check that the Agent is running:

```bash
./scripts/start_agent.sh
```

Check:

```bash
ros2 node list
```

Verify:

- ESP32 and Windows PC are in the same network
- `secrets.h` contains the correct Windows IPv4 address
- UDP port 8888 is published by Docker
- ROS domain IDs match

On Windows:

```powershell
docker ps
```

The container should show:

```text
0.0.0.0:8888->8888/udp
```

## Firmware does not build

Run:

```bash
pio run
```

from:

```text
/workspaces/HEXA/firmware
```

for the full PlatformIO output.

## Firmware cannot be flashed

Check the COM port and verify that the ESP32-S2 is connected via USB.

Test:

```powershell
python -m esptool --chip esp32s2 --port COM5 chip-id
```

Replace `COM5` with the correct port.

---

# 23. Hardware

Main components used by the current HEXA implementation:

- ESP32-S2 Saola-1
- Lynxmotion SSC-32U servo controller
- BNO055 IMU
- Ultrasonic sensors
- RGBW LEDs
- 18 servo motors

The SSC-32U is controlled via TTL UART from the ESP32-S2.

---

# 24. Current Status

The following functions have been tested with ROS 2 Jazzy and micro-ROS:

- micro-ROS WiFi connection
- ROS 2 node discovery
- IMU publisher
- ultrasonic sensor publisher
- RGBW LED subscriber
- movement subscriber
- SSC-32U UART communication
- neutral position command
- walking sequence
