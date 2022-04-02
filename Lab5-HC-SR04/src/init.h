void output_init(Pio *p_pio, const uint32_t ul_mask, uint32_t ul_id, int estado) {
	pmc_enable_periph_clk(ul_id);
	pio_set_output(p_pio, ul_mask, estado, 0, 0);
};

// BUT1_PIO -- p_pio
// BUT1_IDX_MASK -- ul_mask
// BUT1_PIO_ID -- ul_id

void input_init(Pio *p_pio, const uint32_t ul_mask, uint32_t ul_id, void (*p_handler) (), int only_rise) {
	pmc_enable_periph_clk(ul_id);

	// Configura PIO para lidar com o pino do botão como entrada
	// com pull-up
	//pio_configure(p_pio, PIO_INPUT, ul_mask, PIO_PULLUP | PIO_DEBOUNCE);
	// sem pull-up
	pio_configure(p_pio, PIO_INPUT, ul_mask, PIO_DEFAULT);
	//pio_set_debounce_filter(p_pio, ul_mask, 60);

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