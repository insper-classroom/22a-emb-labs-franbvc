void trig_pulse(Pio *pio, uint32_t mask) {
	pio_set(pio, mask);
	delay_us(10);
	pio_clear(pio, mask);
}

void pin_toggle(Pio *pio, uint32_t mask) {
	if(pio_get_output_data_status(pio, mask))
	pio_clear(pio, mask);
	else
	pio_set(pio,mask);
}