
# character-device-driver

##  `char_driver.c` — What It Does

This kernel module:

- Registers a custom character device named `/dev/char_dev_demo`
- Allows `read()` and `write()` access to an internal buffer
- Supports two `ioctl()` commands:
  - `MYDRV_RESET`: clears the internal buffer
  - `MYDRV_GET_SIZE`: returns the number of bytes stored

---

###  Key Sections Explained

####  Module Setup and Globals
```c
#define DEVICE_NAME "char_dev_demo"
#define CLASS_NAME "char_class"
```
Creates `/dev/char_dev_demo` and maps it in `/sys/class/char_class/`.

```c
static char buffer[256];
static size_t buffer_size = 0;
```
A 256-byte internal kernel buffer. `buffer_size` tracks how much valid data is present.

####  `dev_read()` and `dev_write()`
```c
copy_to_user(...)  // kernel user
copy_from_user(...) // user kernel
```
The data transfer logic. Safe copying to and from user-space memory.

It also uses:
```c
if (*offset >= buffer_size) return 0;
```
Supports repeated reads with file offsets like a standard file.

#### `dev_ioctl()` — Custom Commands
```c
#define MYDRV_RESET     _IO(...)
#define MYDRV_GET_SIZE  _IOR(..., int)
```

- `MYDRV_RESET`: Sets all 256 bytes of the buffer to 0 with `memset()`
- `MYDRV_GET_SIZE`: Returns how much data has been written into `buffer`

#### `char_init()` — Driver Load
```c
alloc_chrdev_region()   // reserves a major number
cdev_init() + cdev_add() // registers the device
class_create() + device_create() // creates the /dev entry
```

####  `char_exit()` — Driver Unload
```c
device_destroy(), class_destroy(), cdev_del(), unregister_chrdev_region()
```
Clean shutdown so no kernel resources are leaked.

---

##  `Makefile` for Kernel Module

```makefile
obj-m := char_driver.o
KDIR := /lib/modules/$(shell uname -r)/build
```
This points the build to the Linux kernel source headers.

```bash
make
```
Invokes the kernel build system to create `char_driver.ko`.

---

## Debian Packaging (Inside `debian/`)

### control
Standard package metadata (package name, version, architecture, etc.).

### rules
```makefile
%:
	dh $@
```
Tells `debhelper` to take over the build process.

###  postinst / prerm
These are install/uninstall hooks:

- `postinst`:
  - Inserts the kernel module (`insmod`)
  - Creates `/dev/char_dev_demo` with correct major number and permissions

- `prerm`:
  - Removes the module (`rmmod`)
  - Deletes the device file

---

## Final Build & Test Workflow

```bash
make                             # Build the .ko module
dpkg-buildpackage -us -uc        # Generate the .deb
sudo dpkg -i ../char-driver_1.0_amd64.deb  # Install module via APT
```

After that:

```bash
echo "test" > /dev/char_dev_demo
cat /dev/char_dev_demo           # See what you wrote
```
