#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"
#include "define.h"
#include "utils.h"
#include "init.h"

volatile int  but1_press_flag = 0;
char str[128];

// trig flag
volatile int start = 1;

volatile int timeout = 0;
volatile int fall_flag = 0;

float rtt_alarm = 2*MAX_TIME*RTT_FREQ;


void TC0_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 0);
	

	trig_pulse(TRIG_PIO, TRIG_PIO_IDX_MASK);
	/** Muda o estado do LED (pisca) **/
	pin_toggle(LED_PIO, LED_PIO_IDX_MASK);
	delay_us(280);
	RTT_init(RTT_FREQ, rtt_alarm, RTT_MR_ALMIEN);
}

void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		timeout++;
	}
	
	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
		   // BLINK Led
	}

}

void but1_callback(void) {
	but1_press_flag = 1;
}


void echo_callback(void) {
	if (!pio_get(ECHO_PIO, PIO_INPUT, ECHO_PIO_IDX_MASK)) {
		fall_flag = 1;
	} 
	
	else if (pio_get(ECHO_PIO, PIO_INPUT, ECHO_PIO_IDX_MASK)) {
		
	}
}


int main (void)
{
	board_init();
	sysclk_init();
	
	// Disable the watchdog
	WDT->WDT_MR = WDT_MR_WDDIS;
	delay_init();

	// Init OLED
	gfx_mono_ssd1306_init();
	
	// Init trigger and led
	output_init(TRIG_PIO, TRIG_PIO_IDX_MASK, TRIG_PIO_ID, 0);
	output_init(LED_PIO, LED_PIO_IDX_MASK, LED_PIO_ID, 1);
	
	// Init echo
	input_init(ECHO_PIO, ECHO_PIO_IDX_MASK, ECHO_PIO_ID, echo_callback, 0);
	
	
	// TC to activate trigger
	TC_init(TC0, ID_TC0, 0, 2);
	tc_start(TC0, 0);
	
	
	/* 
	* Ativa RTT para trabalhar por alarme
	* gerando uma interrupção em 4 s:
	* aguarda 4 segundos
	* tempo[s] = 4 * 16 = 4s (0.25Hz)
	*/
	//RTT_init(4, 16, RTT_MR_ALMIEN);
	
	int rtt_result; 
	double distance;
	double seconds;
	
	/* Insert application code here, after the board has been initialized. */
	while(1) {
		if (fall_flag){
			rtt_result = rtt_read_timer_value(RTT);
			seconds = (double) rtt_result/(RTT_FREQ*2);
			distance = (double) SPEED*seconds*100;
			
			if (distance <= 400 && distance >= 2){
				gfx_mono_draw_string("             ", 0, 0, &sysfont);
				sprintf(str, "%.2lf cm", distance);
				gfx_mono_draw_string(str, 0, 0, &sysfont);
				
				timeout = 0;
			}
			
			if (distance > 400 || distance < 2){
				timeout++;
			}
			
			fall_flag = 0;
		}
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	  
		if (timeout >= 5) {
			gfx_mono_draw_string("             ", 0, 0, &sysfont);
			sprintf(str, "ERROR");
			gfx_mono_draw_string(str, 0,0, &sysfont);
		}
	  
	
	}
	
}