#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

#define LED_PIO PIOA
#define LED_PIO_ID ID_PIOA
#define LED_PIO_IDX 0
#define LED_IDX_MASK (1 << LED_PIO_IDX)

#define BUT1_PIO PIOD
#define BUT1_PIO_ID ID_PIOD
#define BUT1_PIO_IDX 28
#define BUT1_IDX_MASK (1 << BUT1_PIO_IDX)

#define BUT2_PIO PIOC
#define BUT2_PIO_ID ID_PIOC
#define BUT2_PIO_IDX 31
#define BUT2_IDX_MASK (1 << BUT2_PIO_IDX)

#define BUT3_PIO PIOA
#define BUT3_PIO_ID ID_PIOA
#define BUT3_PIO_IDX 19
#define BUT3_IDX_MASK (1 << BUT3_PIO_IDX)

volatile char but1_flag;
volatile char but2_flag;
volatile char but3_flag;
volatile char start_count;

void but_callback(void) {
    if (!pio_get(BUT1_PIO, PIO_INPUT, BUT1_IDX_MASK)) {
        but1_flag = 1;
		start_count = 1;
    } else {
		start_count = 0;
    }
    if (!pio_get(BUT2_PIO, PIO_INPUT, BUT2_IDX_MASK)) {
        but2_flag = 1;
    } else {
        but2_flag = 0;
    }
    if (!pio_get(BUT3_PIO, PIO_INPUT, BUT3_IDX_MASK)) {
        but3_flag = 1;
    } else {
        but3_flag = 0;
    }
}

void pisca_led(int t) {
    gfx_mono_draw_string("|", 0, 16, &sysfont);
    gfx_mono_draw_string("|", 40, 16, &sysfont);
    for (int i = 0; i < 30; i++) {
        pio_clear(LED_PIO, LED_IDX_MASK);
        delay_ms(t);
        pio_set(LED_PIO, LED_IDX_MASK);
        delay_ms(t);
        gfx_mono_draw_string(".", i + 5, 16, &sysfont);
    }
    gfx_mono_draw_string("    ", 0, 16, &sysfont);
}

void add_freq(int *p) {
    if (*p == 100) {
        return 100;
    }
    *p -= 100;
}

void remove_freq(int *p) {
    *p += 100;
}

void io_init(void) {
    pmc_enable_periph_clk(LED_PIO_ID);
    pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT);

    pmc_enable_periph_clk(BUT1_PIO_ID);
    pio_configure(BUT1_PIO, PIO_INPUT, BUT1_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
    pio_set_debounce_filter(BUT1_PIO, BUT1_IDX_MASK, 60);
    pio_handler_set(BUT1_PIO,
                    BUT1_PIO_ID,
                    BUT1_IDX_MASK,
                    PIO_IT_EDGE,
                    but_callback);
    pio_enable_interrupt(BUT1_PIO, BUT1_IDX_MASK);
    pio_get_interrupt_status(BUT1_PIO);
    NVIC_EnableIRQ(BUT1_PIO_ID);
    NVIC_SetPriority(BUT1_PIO_ID, 4);

    pmc_enable_periph_clk(BUT2_PIO_ID);
    pio_configure(BUT2_PIO, PIO_INPUT, BUT2_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
    pio_set_debounce_filter(BUT2_PIO, BUT2_IDX_MASK, 60);
    pio_handler_set(BUT2_PIO,
                    BUT2_PIO_ID,
                    BUT2_IDX_MASK,
                    PIO_IT_EDGE,
                    but_callback);
    pio_enable_interrupt(BUT2_PIO, BUT2_IDX_MASK);
    pio_get_interrupt_status(BUT2_PIO);
    NVIC_EnableIRQ(BUT2_PIO_ID);
    NVIC_SetPriority(BUT2_PIO_ID, 4);

    pmc_enable_periph_clk(BUT3_PIO_ID);
    pio_configure(BUT3_PIO, PIO_INPUT, BUT3_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
    pio_set_debounce_filter(BUT3_PIO, BUT3_IDX_MASK, 60);
    pio_handler_set(BUT3_PIO,
                    BUT3_PIO_ID,
                    BUT3_IDX_MASK,
                    PIO_IT_EDGE,
                    but_callback);
    pio_enable_interrupt(BUT3_PIO, BUT3_IDX_MASK);
    pio_get_interrupt_status(BUT3_PIO);
    NVIC_EnableIRQ(BUT3_PIO_ID);
    NVIC_SetPriority(BUT3_PIO_ID, 4);
}

int main(void) {
    int freq = 1000;
    double show_freq = (double)1000 / freq;
    int hold_button_count = 0;

    board_init();
    sysclk_init();
    io_init();
    delay_init();

    char str[128];
    sprintf(str, "%.2lf Hz", show_freq);
    gfx_mono_ssd1306_init();
    gfx_mono_draw_string(str, 50, 16, &sysfont);

    while (1) {
        if (but1_flag) {
            if (start_count) {
                hold_button_count++;
            } else {
                if (hold_button_count > 10000000) {
                    remove_freq(&freq);
                } else {
                    add_freq(&freq);
                }
                show_freq = (double)1000 / freq;
                sprintf(str, "%.2lf Hz", show_freq);
                gfx_mono_draw_string(str, 50, 16, &sysfont);
                but1_flag = 0;
				hold_button_count = 0;
            }
        }

        if (but2_flag) {
            pisca_led(freq);
            but2_flag = 0;
        }

        if (but3_flag) {
            remove_freq(&freq);
            show_freq = (double)1000 / freq;
            sprintf(str, "%.2lf Hz", show_freq);
            gfx_mono_draw_string(str, 50, 16, &sysfont);
            but3_flag = 0;
        }
    }
}