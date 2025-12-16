# Linux Kernel Queue Project

This repository contains a reference implementation of a **Linux Character Device Driver** that implements a **FIFO Queue**, along with a **multi-threaded user-space application** that interacts with the driver.

## Project Structure

```text
.
├── myQueue.c   # Kernel module source code (character device driver)
├── main.c      # User-space program (Pthreads + Semaphores)
├── Makefile    # Build script for the kernel module
└── README.md
````

---

## Prerequisites

You need a Linux environment with kernel headers installed.

### Install required packages

```bash
sudo apt-get update
sudo apt-get install build-essential linux-headers-$(uname -r)
```

---

## How to Build and Run

### Compile the Kernel Module

```bash
make
```

This will generate the kernel module:

```
myQueue.ko
```

---

### Load the Module

Insert the driver into the kernel:

```bash
sudo insmod myQueue.ko
```

Verify that it loaded successfully:

```bash
dmesg | tail
ls /dev/myQueue
```

---

### Set Device Permissions

By default, only root can access the device.
Grant read/write permissions to all users:

```bash
sudo chmod 666 /dev/myQueue
```

---

### Compile the User-Space Application

```bash
gcc main.c -o main -lpthread
```

---

### Run the Application

```bash
./main
```

---

### Clean Up

Remove the kernel module and clean build files:

```bash
sudo rmmod myQueue
make clean
```

---

## Troubleshooting

### Permission Denied

* Make sure you ran:

  ```bash
  sudo chmod 666 /dev/myQueue
  ```
* Ensure the module is loaded (`lsmod | grep myQueue`).

### Make Fails

* Verify kernel headers are installed for your current kernel:

  ```bash
  uname -r
  ```

---

## Notes

* This project is intended for **learning and experimentation** with:

  * Linux Kernel Modules
  * Character Device Drivers
  * FIFO queues
  * Multi-threaded user-space synchronization
