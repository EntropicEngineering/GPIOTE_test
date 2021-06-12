//
// Created by Benjamin Riggs on 6/11/21.
//

#include <logging/log.h>
#include <hal/nrf_gpio.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>

LOG_MODULE_REGISTER(GPIOTE_test, LOG_LEVEL_INF);

#define APP_ERROR_CHECK(FN) \
do {              \
    int err = (FN);     \
    if (err != NRFX_SUCCESS) { \
          LOG_ERR( #FN " failed with %d", err); \
          return -1;\
    }\
} while(0)

#define LED1_PIN    13
#define LED2_PIN    14
#define LED3_PIN    15
#define LED4_PIN    16

static nrfx_timer_t const m_timer0 = NRFX_TIMER_INSTANCE(0);
static nrfx_timer_t const m_timer1 = NRFX_TIMER_INSTANCE(1);

static void pulse_timer_irq_evt_handler(nrf_timer_event_t __unused event_type, void __unused *p_context) {}

int timer_init(void) {
    nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG;
    timer_config.frequency = NRF_TIMER_FREQ_1MHz;
    timer_config.mode = NRF_TIMER_MODE_TIMER;
    timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;

    APP_ERROR_CHECK(nrfx_timer_init(&m_timer0, &timer_config, pulse_timer_irq_evt_handler));
    APP_ERROR_CHECK(nrfx_timer_init(&m_timer1, &timer_config, pulse_timer_irq_evt_handler));
    return 0;
}

int gpiote_init(void) {
    APP_ERROR_CHECK(nrfx_gpiote_init(7));

    uint32_t task_addr;
    uint32_t evt_addr;
    nrf_ppi_channel_t ppi_channel;

    nrfx_gpiote_out_config_t gpiote_config = NRFX_GPIOTE_CONFIG_OUT_TASK_TOGGLE(false);

#define SET_PPI \
APP_ERROR_CHECK(nrfx_ppi_channel_alloc(&ppi_channel)); \
APP_ERROR_CHECK(nrfx_ppi_channel_assign(ppi_channel, evt_addr, task_addr)); \
APP_ERROR_CHECK(nrfx_ppi_channel_enable(ppi_channel))

    // Toggle LED 1 on timer0 channels 0 & 1
    APP_ERROR_CHECK(nrfx_gpiote_out_init(LED1_PIN, &gpiote_config));
    task_addr = nrfx_gpiote_out_task_addr_get(LED1_PIN);
    evt_addr = nrfx_timer_compare_event_address_get(&m_timer0, 0);
    SET_PPI;
    evt_addr = nrfx_timer_compare_event_address_get(&m_timer0, 1);
    SET_PPI;
    nrfx_gpiote_out_task_enable(LED1_PIN);
    // Toggle LED 2 on PPI FORK of PPI triggered by timer0 channel 1
    APP_ERROR_CHECK(nrfx_gpiote_out_init(LED2_PIN, &gpiote_config));
    task_addr = nrfx_gpiote_out_task_addr_get(LED2_PIN);
    APP_ERROR_CHECK(nrfx_ppi_channel_fork_assign(ppi_channel, task_addr));

    // Toggle LED 3 on timer1 channel 0
    APP_ERROR_CHECK(nrfx_gpiote_out_init(LED3_PIN, &gpiote_config));
    task_addr = nrfx_gpiote_out_task_addr_get(LED3_PIN);
    evt_addr = nrfx_timer_compare_event_address_get(&m_timer1, 0);
    SET_PPI;
    // Toggle LED 4 on timer1 channel 1
    APP_ERROR_CHECK(nrfx_gpiote_out_init(LED4_PIN, &gpiote_config));
    task_addr = nrfx_gpiote_out_task_addr_get(LED4_PIN);
    evt_addr = nrfx_timer_compare_event_address_get(&m_timer1, 1);
    SET_PPI;
    // Toggle LED 3 on PPI FORK of PPI triggered by timer1 channel 1
    task_addr = nrfx_gpiote_out_task_addr_get(LED3_PIN);
    APP_ERROR_CHECK(nrfx_ppi_channel_fork_assign(ppi_channel, task_addr));

    return 0;
}

void main(void) {

    int ret;
    ret = timer_init();
    if (ret) return;
    ret = gpiote_init();
    if (ret) return;

    nrfx_timer_extended_compare(&m_timer0, 0, 1000000, 0, false);
    nrfx_timer_extended_compare(&m_timer0, 1, 2000000, TIMER_SHORTS_COMPARE1_CLEAR_Enabled, false);

    nrfx_timer_extended_compare(&m_timer1, 0, 1000000, 0, false);
    nrfx_timer_extended_compare(&m_timer1, 1, 2000000, TIMER_SHORTS_COMPARE1_CLEAR_Enabled, false);

    nrfx_timer_enable(&m_timer0);
    nrfx_timer_enable(&m_timer1);
}
