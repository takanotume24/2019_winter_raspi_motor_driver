/*
 * Many are quoted from wiringPi.h:
****
 */

// wiringPi modes

#define	WPI_MODE_PINS		 0
#define	WPI_MODE_GPIO		 1
#define	WPI_MODE_GPIO_SYS	 2
#define	WPI_MODE_PHYS		 3
#define	WPI_MODE_PIFACE		 4
#define	WPI_MODE_UNINITIALISED	-1

// Pin modes

#define	INPUT			 0
#define	OUTPUT			 1
#define	PWM_OUTPUT		 2
#define	GPIO_CLOCK		 3

#define	LOW			 0
#define	HIGH			 1

// PWM

#define	PWM_MODE_MS		0
#define	PWM_MODE_BAL		1

// Interrupt levels

#define	INT_EDGE_SETUP		0
#define	INT_EDGE_FALLING	1
#define	INT_EDGE_RISING		2
#define	INT_EDGE_BOTH		3


extern int wiringPiSetupGpioLike(void);
extern void pinModeLike(int pin, int mode);
extern void pwmWriteLike(int pin, int value);
extern void IoUeMapVals(void);
extern void digitalWriteLike(int pin, int value);
extern int digitalReadLike(int pin);


