/*
 *  gpiodrv_out.c - The GPIO device driver.
 */
#include <asm/io.h>       // ioremap()
#include <asm/uaccess.h>  // copy_from_user, copy_to_user
#include <linux/cdev.h>   // cdev_*() * */
#include <linux/device.h>
#include <linux/errno.h> /* error codes */
#include <linux/fs.h>    /* alloc_chrdev_region(), ...  */
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/kdev_t.h> /* MAJOR() */
#include <linux/kernel.h> /* Needed for KERN_INFO  printk()*/
#include <linux/module.h> /* Needed by all modules MODULE macro, THIS_MODULE */
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/types.h> /* dev_t */
#include <uapi/asm-generic/errno-base.h>
#include "WiPiLike.h"

/* User-defined macros */
#define NUM_GPIO_PINS 1
#define MAX_GPIO_NUMBER 1
#define DEVICE_NAME "raspiGpio"
#define BUF_SIZE 512
#define CMD_GPIO02_VALUE 1
#define CMD_MOTOR_LEFT_FOWARD 32
#define CMD_MOTOR_LEFT_BACK 33
#define CMD_MOTOR_LEFT_STOP 34
#define CMD_MOTOR_RIGHT_FOWARD 35
#define CMD_MOTOR_RIGHT_BACK 36
#define CMD_MOTOR_RIGHT_STOP 37
#define CMD_HOUDAI_RIGHT 38
#define CMD_HOUDAI_LEFT 39
#define CMD_HOUDAI_STOP 40
#define CMD_GUN_FIRE 41
#define CMD_GUN_STOP 42
#define CMD_MOTOR_LEFT_ENABLE 43
#define CMD_MOTOR_RIGHT_ENABLE 44

#define PIN_02 2
#define PIN_MOVE_INA1 21
#define PIN_MOVE_INA2 20
#define PIN_MOVE_INB1 26
#define PIN_MOVE_INB2 19
#define PIN_MOVE_ENAA 5
#define PIN_MOVE_ENAB 6
#define PIN_HOUDAI_INA1 13
#define PIN_HOUDAI_INA2 16
#define PIN_HOUDAI_INB1 12

struct raspi_gpio_dev {
  struct cdev cdev;
  //    enum state state;
  //    enum direction dir;
  //    spinlock_t lock;
};

/* Declaration of entry points */
static int raspi_gpio_open(struct inode *inode, struct file *filp);
static long raspi_gpio_ioclt(struct file *file, unsigned int cmd,
                             unsigned long arg);
static int raspi_gpio_close(struct inode *inode, struct file *filp);

/* File operation structure */
static struct file_operations raspi_gpio_fops = {
    .owner = THIS_MODULE,
    .open = raspi_gpio_open,
    .unlocked_ioctl = raspi_gpio_ioclt,
    .release = raspi_gpio_close,
};

/* Forward declaration of functions */
static int raspi_gpio_init(void);
static void raspi_gpio_exit(void);
/* Global varibles for GPIO driver */
struct raspi_gpio_dev *raspi_gpio_devp[NUM_GPIO_PINS];
static dev_t first;
static struct class *raspi_gpio_class;

static int raspi_gpio_close(struct inode *inode, struct file *flip) {
  return 0;
  IoUeMapVals();
}

static int raspi_gpio_open(struct inode *inode, struct file *filp) {
  struct raspi_gpio_dev *raspi_gpio_devp;

  printk(KERN_INFO "GPIO opened\n");
  raspi_gpio_devp = container_of(inode->i_cdev, struct raspi_gpio_dev, cdev);
  filp->private_data = raspi_gpio_devp;
  wiringPiSetupGpioLike();

  return 0;
}

static long raspi_gpio_ioclt(struct file *file, unsigned int cmd,
                             unsigned long arg) {
  switch (cmd) {
    case CMD_GPIO02_VALUE:
      digitalWriteLike(2, arg);
      break;

    case CMD_MOTOR_LEFT_BACK:
      digitalWriteLike(PIN_MOVE_INA1, LOW);
      digitalWriteLike(PIN_MOVE_INA2, HIGH);
      break;

    case CMD_MOTOR_LEFT_FOWARD:
      digitalWriteLike(PIN_MOVE_INA1, HIGH);
      digitalWriteLike(PIN_MOVE_INA2, LOW);
      break;

    case CMD_MOTOR_LEFT_STOP:
      digitalWriteLike(PIN_MOVE_INA1, LOW);
      digitalWriteLike(PIN_MOVE_INA2, LOW);
      break;

    case CMD_MOTOR_RIGHT_BACK:
      digitalWriteLike(PIN_MOVE_INB1, LOW);
      digitalWriteLike(PIN_MOVE_INB2, HIGH);
      break;

    case CMD_MOTOR_RIGHT_FOWARD:
      digitalWriteLike(PIN_MOVE_INB1, HIGH);
      digitalWriteLike(PIN_MOVE_INB2, LOW);
      break;

    case CMD_MOTOR_RIGHT_STOP:
      digitalWriteLike(PIN_MOVE_INB1, LOW);
      digitalWriteLike(PIN_MOVE_INB2, LOW);
      break;

    case CMD_MOTOR_LEFT_ENABLE:
      if (arg != 0 && arg != 1) {
        printk(KERN_DEBUG "arg allowed only 1 or 0.\n");
        return -1;
      }
      digitalWriteLike(PIN_MOVE_ENAA, arg);
      break;

    case CMD_MOTOR_RIGHT_ENABLE:
      if (arg != 0 && arg != 1) {
        printk(KERN_DEBUG "arg allowed only 1 or 0.\n");
        return -1;
      }
      digitalWriteLike(PIN_MOVE_ENAB, arg);
      break;

    case CMD_HOUDAI_LEFT:
      digitalWriteLike(PIN_HOUDAI_INA1, HIGH);
      digitalWriteLike(PIN_HOUDAI_INA2, LOW);
      break;

    case CMD_HOUDAI_RIGHT:
      digitalWriteLike(PIN_HOUDAI_INA1, LOW);
      digitalWriteLike(PIN_HOUDAI_INA2, HIGH);
      break;

    case CMD_HOUDAI_STOP:
      digitalWriteLike(PIN_HOUDAI_INA1, LOW);
      digitalWriteLike(PIN_HOUDAI_INA2, LOW);
      break;

    case CMD_GUN_FIRE:
      digitalWriteLike(PIN_HOUDAI_INB1, HIGH);
      break;

    case CMD_GUN_STOP:
      digitalWriteLike(PIN_HOUDAI_INB1, LOW);
      break;
  }
  return 0;
}

static int __init raspi_gpio_init(void) {
  int i = 0, ret, index = 0;

  printk(KERN_DEBUG "start gpio_init\n");

  // キャラクタデバイス番号の動的取得
  if (alloc_chrdev_region(
          &first,         // 最初のデバイス番号が入る
          0,              // マイナー番号の開始番号
          NUM_GPIO_PINS,  // 取得するマイナー番号数
          DEVICE_NAME  // モジュール名(/proc/devicesに出力する名前)
          ) < 0) {
    printk(KERN_DEBUG "Cannot register device\n");
    return -1;
  }

  // ドライバのクラス登録
  // /sys/class 配下にドライバ情報を作成
  if ((raspi_gpio_class =
           class_create(THIS_MODULE,  // 固定
                        DEVICE_NAME  // クラス名。/sys/class/(クラス名)
                        )) == NULL) {
    printk(KERN_DEBUG "Cannot create class %s\n", DEVICE_NAME);
    unregister_chrdev_region(first, NUM_GPIO_PINS);
    return -EINVAL;
  }

  raspi_gpio_devp[index] = kmalloc(sizeof(struct raspi_gpio_dev), GFP_KERNEL);
  if (!raspi_gpio_devp[index]) {
    printk("Bad kmalloc\n");
    return -ENOMEM;
  }

  raspi_gpio_devp[index]->cdev.owner = THIS_MODULE;

  // キャラクタデバイス初期化
  // ファイルオペレーション構造体の指定もする
  cdev_init(&raspi_gpio_devp[index]->cdev, &raspi_gpio_fops);

  // デバイスに追加する
  if ((ret = cdev_add(&raspi_gpio_devp[index]->cdev, (first + i), 1))) {
    printk(KERN_ALERT "Error %d adding cdev\n", ret);
    device_destroy(raspi_gpio_class, MKDEV(MAJOR(first), MINOR(first) + i));
    class_destroy(raspi_gpio_class);
    unregister_chrdev_region(first, NUM_GPIO_PINS);
    return ret;
  }

  // マイナー番号に基づくデバイスの作成
  // devlist[]に対応して、それぞれのデバイスを設定しています。
  if (device_create(raspi_gpio_class, NULL,
                    MKDEV(MAJOR(first), MINOR(first) + i), NULL, "%s%d",
                    DEVICE_NAME, i) == NULL) {
    class_destroy(raspi_gpio_class);
    unregister_chrdev_region(first, NUM_GPIO_PINS);
    return -1;
  }

  wiringPiSetupGpioLike();
  pinModeLike(PIN_02, OUTPUT);
  pinModeLike(PIN_MOVE_INB2, OUTPUT);
  pinModeLike(PIN_MOVE_INB1, OUTPUT);
  pinModeLike(PIN_MOVE_INA2, OUTPUT);
  pinModeLike(PIN_MOVE_INA1, OUTPUT);
  pinModeLike(PIN_MOVE_ENAA, OUTPUT);
  pinModeLike(PIN_MOVE_ENAB, OUTPUT);
  pinModeLike(PIN_HOUDAI_INA1, OUTPUT);
  pinModeLike(PIN_HOUDAI_INA2, OUTPUT);
  pinModeLike(PIN_HOUDAI_INB1, OUTPUT);

  return 0;
}

static void __exit raspi_gpio_exit(void) {
  int i = 0;

  // デバイス番号の返却
  unregister_chrdev_region(first, NUM_GPIO_PINS);

  kfree(raspi_gpio_devp[i]);

  device_destroy(raspi_gpio_class, MKDEV(MAJOR(first), MINOR(first) + i));
  class_destroy(raspi_gpio_class);

  digitalWriteLike(2, LOW);
  IoUeMapVals();
}

module_init(raspi_gpio_init);
module_exit(raspi_gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("info@tomosoft.jp");
MODULE_DESCRIPTION("GPIO device driver");
