/*  
 *  gpiodrv_out.c - The GPIO device driver. 
 */
#include <linux/module.h>   /* Needed by all modules MODULE macro, THIS_MODULE */
#include <linux/kernel.h>   /* Needed for KERN_INFO  printk()*/
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/time.h>
#include <asm/io.h>   // ioremap()


// Pin modes
#define	INPUT			 0
#define	OUTPUT			 1
#define	PWM_OUTPUT		 2
#define	GPIO_CLOCK		 3

//#define RASPBERRY_PI_PERI_BASE 0x3F000000
#define BCM2708_PERI_BASE 0x3F000000  //in kernelsrc/arch/arm/mach-bcm2709/include/mach/platform.h
//#define GPIO_BASE (RASPBERRY_PI_PERI_BASE + 0x00200000)
#define GPIO_BASE (BCM2708_PERI_BASE + 0x200000)  //in kernelsrc/arch/arm/mach-bcm2709/include/mach/platform.h
#define GPIO_PWM (BCM2708_PERI_BASE + 0x0020C000)
#define	PWM_CONTROL 0
#define	PWM0_RANGE  4
#define	PWM0_DATA   5
#define	PWM1_RANGE  8
#define	PWM1_DATA   9
#define GPIO_CLOCK_BASE (BCM2708_PERI_BASE + 0x00101000)
#define	PWMCLK_CNTL	40
#define	PWMCLK_DIV	41
#define	BCM_PASSWORD		0x5A000000



//#define INP_GPIO(g) *(gpiomap+((g)/10)) &= ~(7<<(((g)%10)*3)) //GPIOをINPUTに設定する
#define INP_GPIO(g) iowrite32(\
    ioread32( (void*)((int *)gpiomap+(g)/10) ) & ~(7<<(((g)%10)*3)), \
    (void*)((int *)gpiomap+(g)/10) )
//#define OUT_GPIO(g) *(gpiomap+((g)/10)) |=  (1<<(((g)%10)*3)) //GPIOをOUTPUTに設定する
#define OUT_GPIO(g) iowrite32(\
	ioread32( (void*)((int *)gpiomap+(g)/10) ) | (1<<(((g)%10)*3)), \
	(void*)((int *)gpiomap+(g)/10) )
//#define GPIO_SET(g) *(gpiomap+7) = (0x1 << (g)) //GPIOをONにする
#define GPIO_SET(g) iowrite32( 1<<(g), (void*)((int *)gpiomap+7) )
//#define GPIO_CLR(g) *(gpiomap+10) = (0x1 << (g)) //GPIOをOFFにする
#define GPIO_CLR(g) iowrite32( 1<<(g), (void*)((int *)gpiomap+10) )
#define SET_GPIO_ALT(g,a) iowrite32(\
	ioread32( (void*)((int *)gpiomap+(g)/10) ) | (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3)), \
	(void*)((int *)gpiomap+(g)/10) )


void *gpiomap;
void *pwmmap;
void *clkmap;


void digitalWriteLike(int pin, int value)
{
	if(pin < 2 || pin > 26){
		printk(KERN_ALERT "pin number must be >=2 and <=26\n");
		return;
	}
	if(value < 0 || value > 1){
		printk(KERN_ALERT "value must be 0 or 1.\n");
		return;
	}
	if(value==0)
		GPIO_CLR(pin);
	else
		GPIO_SET(pin);
}


int digitalReadLike(int pin)
{
	uint32_t gpvalue;
	//#define GPIO_GET_PIN *(gpiomap+13) //GPIOの状態を取得する
	gpvalue = ioread32( (void*)((int *)gpiomap+13) );
	// gpvalue shows the status of GPIO00 to GPIO31
    if ((gpvalue & (1 << (pin & 31))) != 0)
      return 1 ;
    else
      return 0 ;
}


int wiringPiSetupGpioLike(void){
	gpiomap = ioremap_nocache(GPIO_BASE,PAGE_SIZE);
	pwmmap = ioremap_nocache(GPIO_PWM,PAGE_SIZE);
	clkmap = ioremap_nocache(GPIO_CLOCK_BASE,PAGE_SIZE);

	printk(KERN_INFO "Don't forget IoUeMapVals() after use.\n");
	return 0;
}


void pinModeLike(int pin, int mode){
//	uint32_t pwm_control;

	if(pin < 2 || pin > 26){
		printk(KERN_ALERT "pin number must be >=2 and <=26\n");
		return;
	}

    if(mode == INPUT){
		INP_GPIO(pin);
		printk(KERN_INFO "GPIO%2d is configured as input.\n", pin);
		return;
	}
    if(mode == OUTPUT){
		OUT_GPIO(pin);
		printk(KERN_INFO "GPIO%2d is configured as output.\n", pin);
		return;
	}
	/*
    if(mode == PWM_OUTPUT){
		if(pin != 18){
			printk(KERN_ALERT "Pin number must be 18 for PWM\n");
			return;
		}

		// GPIO10の拡張番号を5に設定
		SET_GPIO_ALT(18, 5);
		// pwmSetMode(MS);
		// *(pwm + PWM_CONTROL) = PWM0_ENABLE | PWM1_ENABLE | PWM0_MS_MODE | PWM1_MS_MODE;
		iowrite32( 0x8181, (void*)((int *)pwmmap+PWM_CONTROL) );

		// pwmSetRange(1024)
		iowrite32( 1024, (void*)((int *)pwmmap+PWM0_RANGE) );
		iowrite32( 1024, (void*)((int *)pwmmap+PWM1_RANGE) );

		// pwmSetClock (2)
		// 19.2M / 128 / 1024 = 147 kHz
		//pwm_control = *(pwm + PWM_CONTROL) ;		// preserve PWM_CONTROL
		pwm_control = ioread32( (void*)((int *)pwmmap+PWM_CONTROL) );
		// *(pwm + PWM_CONTROL) = 0 ;				// Stop PWM
		iowrite32( 0, (void*)((int *)pwmmap+PWM_CONTROL) );
		// *(clk + PWMCLK_CNTL) = BCM_PASSWORD | 0x01 ;	// Stop PWM Clock
		iowrite32( BCM_PASSWORD | 0x01, (void*)((int *)clkmap+PWMCLK_CNTL) );
		// *(clk + PWMCLK_DIV)  = BCM_PASSWORD | (divisor << 12) ;
		// divisor = 128 of pwmSetClock (128)
		iowrite32( BCM_PASSWORD | (128 << 12), (void*)((int *)clkmap+PWMCLK_DIV) );
		// *(clk + PWMCLK_CNTL) = BCM_PASSWORD | 0x11 ;	// Start PWM clock
		iowrite32( BCM_PASSWORD | 0x11, (void*)((int *)clkmap+PWMCLK_CNTL) );
		// *(pwm + PWM_CONTROL) = pwm_control ;		// restore PWM_CONTROL
		iowrite32( pwm_control, (void*)((int *)pwmmap+PWM_CONTROL) );

		printk(KERN_INFO "GPIO18 is configured as PWM output.\n");
		return;
	}
	*/
}

/*
void pwmWriteLike(int pin, int value){
	if(pin != 18){
		printk(KERN_ALERT "Pin number must be 18 for PWM\n");
		return;
	}
	if(value < 0 || value > 1024){
		printk(KERN_ALERT "value must be >=0 and <= 1024\n");
		return;
	}
	iowrite32( value, (void*)((int *)pwmmap+PWM0_DATA) );

	printk(KERN_INFO "Duty ratio (GPIO18) is set as %d/1024.\n", value);
}
*/

void IoUeMapVals(void)
{
	iounmap(gpiomap);
	iounmap(pwmmap);
	iounmap(clkmap);
	printk(KERN_INFO "iounmap valuables is done.\n");
}
