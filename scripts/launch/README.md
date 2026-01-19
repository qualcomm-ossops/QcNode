# Launch scripts for QCNode

This document describes the concepts and steps to connect to target devices (QNX, Linux, Ubuntu) and how to deploy and run the QCNode package.

## Table of Contents
- [Launch scripts for QCNode](#launch-scripts-for-qcnode)
  - [Table of Contents](#table-of-contents)
  - [1. Connection Concepts \& Setup](#1-connection-concepts--setup)
    - [1.1 SSH (Secure Shell) - for QNX](#11-ssh-secure-shell---for-qnx)
    - [1.2 ADB (Android Debug Bridge) - for Linux/Ubuntu](#12-adb-android-debug-bridge---for-linuxubuntu)
  - [2. How to Run the QCNode package](#2-how-to-run-the-qcnode-package)
    - [2.1 For QNX (using SSH/SCP)](#21-for-qnx-using-sshscp)
    - [2.2 For Linux/Ubuntu HGY (using ADB)](#22-for-linuxubuntu-hgy-using-adb)
  - [3. The `qcrun` Script](#3-the-qcrun-script)

---

## 1. Connection Concepts & Setup

We primarily use **SSH** for QNX systems and **ADB** for Linux/Ubuntu systems.

### 1.1 SSH (Secure Shell) - for QNX
SSH is used to securely connect to the QNX device.
If the device is directly connected to your PC, you can connect using its IP address.
If the device is remote (behind a gateway), you may need to set up port forwarding to map the device's port 22 to a local port.

**Remote Access Setup (Optional):**
If accessing via a gateway:
*   **Linux:**
    ```bash
    ssh -fgN -L <gateway_ip>:<local_port>:<device_ip>:22 $USER@localhost
    ```
*   **Windows:** Use `netsh` to forward ports:
    ```cmd
    netsh interface portproxy add v4tov4 listenaddress=<gateway_ip> listenport=14422 connectaddress=192.168.0.1 connectport=22
    ```

    **Usage Examples (after forwarding is set up):**
    ```bash
    # Connect via SSH (mapped to local port 14422)
    ssh -p 14422 root@<gateway_ip>

    # Transfer files via SCP
    scp -P 14422 qcnode.tar.gz root@<gateway_ip>:/data/
    ```

### 1.2 ADB (Android Debug Bridge) - for Linux/Ubuntu
ADB is a versatile command-line tool that lets you communicate with a device (identified by **IP address** or **Serial Number**).
If the device is directly connected (via USB or Network), `adb` can communicate with it directly.
If the device is remote, you can connect to an ADB server running on the gateway.

**Remote Access Setup (Optional):**
*   **Gateway:** Start server with `adb -a -P 5037 nodaemon server start`.
*   **Client:** Connect using `adb -H <gateway_ip>`.

    **Usage Examples:**
    ```bash
    # Connect to shell
    adb -H <gateway_ip> shell

    # Transfer files
    adb -H <gateway_ip> push qcnode.tar.gz /data/
    ```

---

## 2. How to Run the QCNode package

These instructions assume you have network access to the device (either directly or via a forwarded port/tunnel).

### 2.1 For QNX (using SSH/SCP)

```sh
# 1. Push the package to QNX target folder /data
scp qcnode-aarch64-qnx.tar.gz root@<device_ip>:/data/

# 2. Login to device via SSH
ssh root@<device_ip>

# 3. Setup and Run
cd /data
tar xfv qcnode-aarch64-qnx.tar.gz
cd pkg-aarch64-qnx/opt/qcnode

# Launch the QCNode application binaries
./bin/qcrun ./bin/gtest_Memory
./bin/qcrun ./bin/gtest_Logger
```

### 2.2 For Linux/Ubuntu HGY (using ADB)

```sh
# 1. Connect to device
# If using Network ADB:
adb connect <device_ip>
# If using USB, skip 'adb connect' (device is identified by Serial Number)

adb root

# 2. Push the package to target folder /data
# Note: Replace <os> with 'linux' or 'ubuntu' depending on your target
adb push qcnode-aarch64-<os>.tar.gz /data

# 3. Enter Shell and Run
adb shell
cd /data
tar xfv qcnode-aarch64-<os>.tar.gz
cd pkg-aarch64-<os>/opt/qcnode

# Launch the QCNode application binaries
./bin/qcrun ./bin/gtest_Memory
./bin/qcrun ./bin/gtest_Logger
```

---

## 3. The `qcrun` Script

You may notice the use of `./bin/qcrun` before the application binary path in the launch commands (e.g., `./bin/qcrun ./bin/gtest_Memory`).

**Why is it needed?**
The `qcrun` script is a wrapper that prepares the runtime environment required by QCNode applications. It ensures the application runs correctly by:

1.  **Setting Library Paths:** Configures `LD_LIBRARY_PATH` to ensure the application can find the shared libraries included in the package.
2.  **Configuring DSP Paths:** Sets up variables like `CDSP_LIBRARY_PATH` so the application can locate and load DSP libraries.
3.  **Platform-Specific Setup:**
    *   **QNX:** Preloads necessary libraries (e.g., `libsocket.so`) and applies security policies/user contexts (e.g., `qcnode_t` for Nord devices).
    *   **Linux:** Configures SDL video drivers (EGL/GLES) if Adreno drivers are present.

**Usage:**
Always invoke QCNode binaries through this wrapper:
```bash
./bin/qcrun <path_to_binary> [arguments]
