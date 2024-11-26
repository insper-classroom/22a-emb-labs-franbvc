#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

volatile char flag_rtc_alarm = 0;
volatile int  but1_press_flag = 0;
volatile int  sec_change_flag = 0;
char str[128];

#define LED_PIO PIOC
#define LED_PIO_ID ID_PIOC
#define LED_PIO_IDX 8
#define LED_PIO_IDX_MASK (1u << LED_PIO_IDX)

#define LED1_PIO PIOA
#define LED1_PIO_ID ID_PIOA
#define LED1_PIO_IDX 0
#define LED1_PIO_IDX_MASK (1u << LED1_PIO_IDX)

#define LED2_PIO PIOC
#define LED2_PIO_ID ID_PIOC
#define LED2_PIO_IDX 30
#define LED2_PIO_IDX_MASK (1u << LED2_PIO_IDX)

#define LED3_PIO PIOB
#define LED3_PIO_ID ID_PIOB
#define LED3_PIO_IDX 2
#define LED3_PIO_IDX_MASK (1u << LED3_PIO_IDX)

#define BUT1_PIO PIOD
#define BUT1_PIO_ID	ID_PIOD
#define BUT1_PIO_IDX 28
#define BUT1_IDX_MASK (1 << BUT1_PIO_IDX)

void pin_toggle(Pio *pio, uint32_t mask) {
	if(pio_get_output_data_status(pio, mask))
	pio_clear(pio, mask);
	else
	pio_set(pio,mask);
}

void LED_init(int estado) {
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_set_output(LED_PIO, LED_PIO_IDX_MASK, estado, 0, 0);
	
	pmc_enable_periph_clk(LED1_PIO_ID);
	pio_set_output(LED1_PIO, LED1_PIO_IDX_MASK, estado, 0, 0);
	
	pmc_enable_periph_clk(LED2_PIO_ID);
	pio_set_output(LED2_PIO, LED2_PIO_IDX_MASK, estado, 0, 0);
	
	pmc_enable_periph_clk(LED3_PIO_ID);
	pio_set_output(LED3_PIO, LED3_PIO_IDX_MASK, estado, 0, 0);
};

void button_init(Pio *p_pio, const uint32_t ul_mask, uint32_t ul_id, void (*p_handler) (), int only_rise) {
	pmc_enable_periph_clk(ul_id);

	// Configura PIO para lidar com o pino do botão como entrada
	// com pull-up
	pio_configure(p_pio, PIO_INPUT, ul_mask, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(p_pio, ul_mask, 60);

	// Configura interrupção no pino referente ao botao e associa
	// função de callback caso uma interrupção for gerada
	// a função de callback é a: but_callback()
	pio_handler_set(p_pio,
		ul_id,
		ul_mask,
		only_rise ? PIO_IT_RISE_EDGE : PIO_IT_EDGE,
		p_handler
	);

	// Ativa interrupção e limpa primeira IRQ gerada na ativacao
	pio_enable_interrupt(p_pio, ul_mask);
	pio_get_interrupt_status(p_pio);
	
	// Configura NVIC para receber interrupcoes do PIO do botao
	// com prioridade 4 (quanto mais próximo de 0 maior)
	NVIC_EnableIRQ(ul_id);
	NVIC_SetPriority(ul_id, 4); // Prioridade 4
}

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	/* Configura o PMC */
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  freq hz e interrupçcão no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura NVIC*/
	NVIC_SetPriority(ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
}

void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	/* config NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	/* Enable RTT interrupt */
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
		rtt_enable_interrupt(RTT, rttIRQSource);
	else
		rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	
}

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type) {
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc,  irq_type);
}

void TC0_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 0);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(LED1_PIO, LED1_PIO_IDX_MASK);  
}

void TC1_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 1);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(LED_PIO, LED_PIO_IDX_MASK);  
}

void TC3_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC1, 0);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(LED3_PIO, LED3_PIO_IDX_MASK);  
}

void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		RTT_init(4, 16, RTT_MR_RTTINCIEN);
		pin_toggle(LED2_PIO, LED2_PIO_IDX_MASK); 
	}
	
	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
		   // BLINK Led
	}

}

void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
		
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		// o código para irq de alame vem aqui
		flag_rtc_alarm = 1;
	}
	
	/* seccond tick */
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		// o código para irq de segundo vem aqui
		sec_change_flag = 1;
	}

	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}

void but1_callback(void) {
	but1_press_flag = 1;
}


void set_alarm(uint months, uint days, uint hours, uint minutes, uint seconds) {
	
    /* Leitura do valor atual do RTC */
    uint32_t current_hour, current_min, current_sec;
    uint32_t current_year, current_month, current_day, current_week;
    rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
    rtc_get_date(RTC, &current_year, &current_month, &current_day, &current_week);
    
    /* configura alarme do RTC para daqui 20 segundos */
    rtc_set_date_alarm(RTC, 1, current_month + months, 1, current_day + days);
    rtc_set_time_alarm(RTC, 1, current_hour + hours, 1, current_min + minutes, 1, current_sec + seconds);
}



int main (void)
{
	board_init();
	sysclk_init();

	gfx_mono_ssd1306_init();
	LED_init(1);
	button_init(BUT1_PIO, BUT1_IDX_MASK, BUT1_PIO_ID, but1_callback, 0);
	
	TC_init(TC0, ID_TC0, 0, 4);
	tc_start(TC0, 0);
	
	TC_init(TC0, ID_TC1, 1, 5);
	tc_start(TC0, 1);
	
	TC_init(TC1, ID_TC3, 0, 4);
	
	/* 
	* Ativa RTT para trabalhar por alarme
	* gerando uma interrupção em 4 s:
	* aguarda 4 segundos
	* tempo[s] = 4 * 16 = 4s (0.25Hz)
	*/
	RTT_init(4, 16, RTT_MR_ALMIEN);
	
	calendar rtc_initial = {2022, 3, 12, 12, 18, 57 ,0};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN | RTC_IER_SECEN);
	
	uint32_t current_hour, current_min, current_sec;
	uint32_t current_year, current_month, current_day, current_week;
	

  /* Insert application code here, after the board has been initialized. */
	while(1) {
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
		
		if(flag_rtc_alarm) {
			tc_start(TC1, 0);
			flag_rtc_alarm = 0;
		}
		
		if (sec_change_flag) {
			rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
			gfx_mono_draw_string("           ", 0, 0, &sysfont);
			sprintf(str, "%d:%d:%d", current_hour, current_min, current_sec);
			gfx_mono_draw_string(str, 0,0, &sysfont);
			sec_change_flag = 0;
		}
		
		if (but1_press_flag) {
			set_alarm(0, 0, 0, 0, 20);
			but1_press_flag = 0;
		}
	}
}