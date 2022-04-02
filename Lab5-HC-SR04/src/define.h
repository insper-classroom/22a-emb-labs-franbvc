#define LED_PIO PIOA
#define LED_PIO_ID ID_PIOA
#define LED_PIO_IDX 0
#define LED_PIO_IDX_MASK (1 << LED_PIO_IDX)

//#define TRIG_PIO PIOB
//#define TRIG_PIO_ID ID_PIOB
//#define TRIG_PIO_IDX 4
//#define TRIG_PIO_IDX_MASK (1u << TRIG_PIO_IDX)
//
//#define ECHO_PIO PIOA
//#define ECHO_PIO_ID ID_PIOA
//#define ECHO_PIO_IDX 21
//#define ECHO_PIO_IDX_MASK (1u << ECHO_PIO_IDX)

#define TRIG_PIO PIOA
#define TRIG_PIO_ID ID_PIOA
#define TRIG_PIO_IDX 6
#define TRIG_PIO_IDX_MASK (1u << TRIG_PIO_IDX)

#define ECHO_PIO PIOD
#define ECHO_PIO_ID ID_PIOD
#define ECHO_PIO_IDX 30
#define ECHO_PIO_IDX_MASK (1u << ECHO_PIO_IDX)

#define BUT_PIO PIOA
#define BUT_PIO_ID ID_PIOA
#define BUT_PIO_IDX 11
#define BUT_PIO_IDX_MASK (1u << BUT_PIO_IDX)

#define MIN_TIME 0.000058
#define MAX_TIME 0.011764
#define SPEED 340
#define RTT_FREQ 32000