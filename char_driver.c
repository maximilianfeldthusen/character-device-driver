
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "char_dev_demo"
#define CLASS_NAME "char_class"

#define MYDRV_IOC_MAGIC 'x'
#define MYDRV_RESET     _IO(MYDRV_IOC_MAGIC, 0)
#define MYDRV_GET_SIZE  _IOR(MYDRV_IOC_MAGIC, 1, int)

static dev_t dev_number;
static struct cdev char_cdev;
static struct class *char_class;
static char buffer[256];
static size_t buffer_size = 0;

static ssize_t dev_read(struct file *filp, char __user *user_buf, size_t len, loff_t *offset) {
    if (*offset >= buffer_size) return 0;
    if (copy_to_user(user_buf, buffer + *offset, len)) return -EFAULT;
    *offset += len;
    return len;
}

static ssize_t dev_write(struct file *filp, const char __user *user_buf, size_t len, loff_t *offset) {
    if (len > sizeof(buffer)) return -EINVAL;
    if (copy_from_user(buffer, user_buf, len)) return -EFAULT;
    buffer_size = len;
    return len;
}

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case MYDRV_RESET:
            memset(buffer, 0, sizeof(buffer));
            buffer_size = 0;
            return 0;
        case MYDRV_GET_SIZE:
            if (copy_to_user((int __user *)arg, &buffer_size, sizeof(int)))
                return -EFAULT;
            return 0;
        default:
            return -EINVAL;
    }
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read  = dev_read,
    .write = dev_write,
    .unlocked_ioctl = dev_ioctl,
};

static int __init char_init(void) {
    if (alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME) < 0)
        return -1;

    cdev_init(&char_cdev, &fops);
    if (cdev_add(&char_cdev, dev_number, 1) == -1)
        goto fail_chrdev;

    char_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(char_class)) goto fail_class;

    device_create(char_class, NULL, dev_number, NULL, DEVICE_NAME);
    printk(KERN_INFO "char_dev_demo loaded\n");
    return 0;

fail_class:
    cdev_del(&char_cdev);
fail_chrdev:
    unregister_chrdev_region(dev_number, 1);
    return -1;
}

static void __exit char_exit(void) {
    device_destroy(char_class, dev_number);
    class_destroy(char_class);
    cdev_del(&char_cdev);
    unregister_chrdev_region(dev_number, 1);
    printk(KERN_INFO "char_dev_demo unloaded\n");
}

module_init(char_init);
module_exit(char_exit);

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Character device with ioctl");



