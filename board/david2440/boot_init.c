#include <common.h>
#include <s3c2410.h>


static inline void delay (unsigned long loops)
{
	__asm__ volatile ("1:\n"
	  "subs %0, %1, #1\n"
	  "bne 1b":"=r" (loops):"0" (loops));
}


void clock_init (void)
{
	S3C24X0_CLOCK_POWER * const clk_power = S3C24X0_GetBase_CLOCK_POWER();

	clk_power->CLKDIVN = S3C2440_CLKDIV;

	__asm__( "mrc p15, 0, r1, c1, c0, 0\n" /* read ctrl register */
			"orr r1, r1, #0xc0000000\n" /* Asynchronous */
			"mcr p15, 0, r1, c1, c0, 0\n" /* write ctrl register */
			:::"r1"
			);

	clk_power->LOCKTIME = 0xFFFFFF;
	clk_power->MPLLCON = S3C2440_MPLL_400MHZ;

	delay(4000);
	clk_power->UPLLCON = S3C2440_UPLL_48MHZ;
	delay(8000);
		
}


