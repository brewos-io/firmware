/**
 * PIO-based UART Implementation for RP2350
 * 
 * Uses PIO state machines to implement UART on arbitrary GPIO pins.
 * Based on the Raspberry Pi Pico PIO UART example programs.
 * 
 * GPIO6/GPIO7 are mapped to UART1_CTS/RTS in the RP2350 silicon,
 * not TX/RX. Hardware UART cannot work on these pins for data transfer.
 * PIO UART solves this by bit-banging the UART protocol in the PIO
 * coprocessor, which runs independently and can use any GPIO.
 */

#include "pio_uart.h"
#include "config.h"

#ifndef UNIT_TEST

#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"

// We use PIO0 for power meter UART
// SM0 = TX, SM1 = RX
static PIO pio = pio0;
static uint sm_tx;
static uint sm_rx;
static uint offset_tx;
static uint offset_rx;
static bool pio_uart_initialized = false;

// ═══════════════════════════════════════════════════════════════════════════════
// PIO TX Program (from Raspberry Pi pico-examples uart_tx.pio)
// 
// Each bit takes 8 PIO cycles. The PIO clock divider is set so that
// 8 cycles = 1 bit period at the desired baud rate.
//
// .program uart_tx
// .side_set 1 opt
//     pull       side 1 [7]  ; Assert stop bit (high), or stall with line idle
//     set x, 7  side 0 [7]  ; Preload bit counter, assert start bit for 8 clocks
// bitloop:
//     out pins, 1            ; Shift 1 bit from OSR to the first OUT pin
//     jmp x-- bitloop   [6] ; Each loop iteration is 8 cycles
// ═══════════════════════════════════════════════════════════════════════════════

static const uint16_t uart_tx_program_instructions[] = {
    //     .wrap_target
    0x9fa0, //  0: pull   block           side 1 [7]
    0xf827, //  1: set    x, 7            side 0 [7]
    0x6001, //  2: out    pins, 1
    0x0642, //  3: jmp    x--, 2                 [6]
    //     .wrap
};

static const struct pio_program uart_tx_program = {
    .instructions = uart_tx_program_instructions,
    .length = 4,
    .origin = -1,
};

// ═══════════════════════════════════════════════════════════════════════════════
// PIO RX Program (from Raspberry Pi pico-examples uart_rx.pio)
//
// .program uart_rx
// start:
//     wait 0 pin 0        ; Stall until start bit is asserted
//     set x, 7    [10]    ; Preload bit counter, delay to bit center
// bitloop:
//     in pins, 1           ; Shift data bit into ISR
//     jmp x-- bitloop [6] ; Each loop iteration is 8 cycles
//     jmp pin good_stop    ; Check stop bit (should be high)
//     irq 4 rel            ; Framing error
//     wait 1 pin 0         ; Wait for line to return to idle
//     jmp start
// good_stop:
//     push                 ; Push ISR contents to FIFO
// ═══════════════════════════════════════════════════════════════════════════════

static const uint16_t uart_rx_program_instructions[] = {
    //     .wrap_target
    0x2020, //  0: wait   0, pin, 0
    0xea27, //  1: set    x, 7            [10]
    0x4001, //  2: in     pins, 1
    0x0642, //  3: jmp    x--, 2                 [6]
    0x00c8, //  4: jmp    pin, 8
    0xc014, //  5: irq    4 rel
    0x20a0, //  6: wait   1, pin, 0
    0x0000, //  7: jmp    0
    0x8020, //  8: push   block
    //     .wrap
};

static const struct pio_program uart_rx_program = {
    .instructions = uart_rx_program_instructions,
    .length = 9,
    .origin = -1,
};

// ═══════════════════════════════════════════════════════════════════════════════
// Helper: Initialize TX state machine
// ═══════════════════════════════════════════════════════════════════════════════

static void pio_uart_tx_init(PIO pio, uint sm, uint offset, uint pin_tx, uint baud) {
    // Configure the TX pin as PIO output
    pio_sm_set_pins_with_mask(pio, sm, 1u << pin_tx, 1u << pin_tx);  // Set pin high (idle)
    pio_sm_set_pindirs_with_mask(pio, sm, 1u << pin_tx, 1u << pin_tx);  // Set as output
    pio_gpio_init(pio, pin_tx);
    
    pio_sm_config c = pio_get_default_sm_config();
    
    // OUT shifts to right, autopull disabled
    sm_config_set_out_shift(&c, true, false, 32);
    
    // Map the OUT and side-set pins
    sm_config_set_out_pins(&c, pin_tx, 1);
    sm_config_set_sideset_pins(&c, pin_tx);
    sm_config_set_sideset(&c, 2, true, false);  // 1 side-set bit, optional, no pindirs
    
    // Set wrap points
    sm_config_set_wrap(&c, offset, offset + uart_tx_program.length - 1);
    
    // Clock divider for baud rate: 8 PIO cycles per bit
    float div = (float)clock_get_hz(clk_sys) / (8 * baud);
    sm_config_set_clkdiv(&c, div);
    
    // FIFO: TX only (join both FIFOs for TX to get 8-deep FIFO)
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Helper: Initialize RX state machine
// ═══════════════════════════════════════════════════════════════════════════════

static void pio_uart_rx_init(PIO pio, uint sm, uint offset, uint pin_rx, uint baud) {
    // Configure the RX pin as PIO input
    pio_sm_set_consecutive_pindirs(pio, sm, pin_rx, 1, false);  // Input
    pio_gpio_init(pio, pin_rx);
    gpio_pull_up(pin_rx);  // Pull up for idle state (UART idle = high)
    
    pio_sm_config c = pio_get_default_sm_config();
    
    // IN shifts to right, autopush at 8 bits
    sm_config_set_in_shift(&c, true, false, 32);
    
    // Map the IN and JMP pins
    sm_config_set_in_pins(&c, pin_rx);
    sm_config_set_jmp_pin(&c, pin_rx);
    
    // Set wrap points
    sm_config_set_wrap(&c, offset, offset + uart_rx_program.length - 1);
    
    // Clock divider for baud rate: 8 PIO cycles per bit
    float div = (float)clock_get_hz(clk_sys) / (8 * baud);
    sm_config_set_clkdiv(&c, div);
    
    // FIFO: RX only (join both FIFOs for RX to get 8-deep FIFO)
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Public API
// ═══════════════════════════════════════════════════════════════════════════════

bool pio_uart_init(uint8_t tx_pin, uint8_t rx_pin, uint32_t baud_rate) {
    if (pio_uart_initialized) {
        pio_uart_deinit();
    }
    
    // Add TX program to PIO
    if (!pio_can_add_program(pio, &uart_tx_program)) {
        LOG_PRINT("PIO UART: Cannot add TX program (PIO0 full)\n");
        return false;
    }
    offset_tx = pio_add_program(pio, &uart_tx_program);
    
    // Add RX program to PIO
    if (!pio_can_add_program(pio, &uart_rx_program)) {
        LOG_PRINT("PIO UART: Cannot add RX program (PIO0 full)\n");
        pio_remove_program(pio, &uart_tx_program, offset_tx);
        return false;
    }
    offset_rx = pio_add_program(pio, &uart_rx_program);
    
    // Claim state machines
    sm_tx = pio_claim_unused_sm(pio, false);
    if (sm_tx == -1u) {
        LOG_PRINT("PIO UART: No free SM for TX\n");
        pio_remove_program(pio, &uart_tx_program, offset_tx);
        pio_remove_program(pio, &uart_rx_program, offset_rx);
        return false;
    }
    
    sm_rx = pio_claim_unused_sm(pio, false);
    if (sm_rx == -1u) {
        LOG_PRINT("PIO UART: No free SM for RX\n");
        pio_sm_unclaim(pio, sm_tx);
        pio_remove_program(pio, &uart_tx_program, offset_tx);
        pio_remove_program(pio, &uart_rx_program, offset_rx);
        return false;
    }
    
    // Initialize state machines
    pio_uart_tx_init(pio, sm_tx, offset_tx, tx_pin, baud_rate);
    pio_uart_rx_init(pio, sm_rx, offset_rx, rx_pin, baud_rate);
    
    pio_uart_initialized = true;
    LOG_PRINT("PIO UART: Initialized (TX=GPIO%d, RX=GPIO%d, %u baud, PIO0 SM%d/%d)\n",
              tx_pin, rx_pin, baud_rate, sm_tx, sm_rx);
    
    return true;
}

void pio_uart_deinit(void) {
    if (!pio_uart_initialized) return;
    
    pio_sm_set_enabled(pio, sm_tx, false);
    pio_sm_set_enabled(pio, sm_rx, false);
    pio_sm_unclaim(pio, sm_tx);
    pio_sm_unclaim(pio, sm_rx);
    pio_remove_program(pio, &uart_tx_program, offset_tx);
    pio_remove_program(pio, &uart_rx_program, offset_rx);
    
    pio_uart_initialized = false;
}

bool pio_uart_reconfigure(uint8_t tx_pin, uint8_t rx_pin, uint32_t baud_rate) {
    pio_uart_deinit();
    return pio_uart_init(tx_pin, rx_pin, baud_rate);
}

void pio_uart_write_blocking(const uint8_t* data, uint32_t len) {
    if (!pio_uart_initialized || !data) return;
    
    for (uint32_t i = 0; i < len; i++) {
        // Put byte into TX FIFO (blocks if FIFO is full)
        pio_sm_put_blocking(pio, sm_tx, (uint32_t)data[i]);
    }
}

bool pio_uart_is_readable(void) {
    if (!pio_uart_initialized) return false;
    return !pio_sm_is_rx_fifo_empty(pio, sm_rx);
}

uint8_t pio_uart_getc(void) {
    if (!pio_uart_initialized) return 0;
    // Read from RX FIFO (blocks if empty)
    return (uint8_t)(pio_sm_get_blocking(pio, sm_rx) >> 24);
}

#else
// Unit test stubs
bool pio_uart_init(uint8_t tx_pin, uint8_t rx_pin, uint32_t baud_rate) { (void)tx_pin; (void)rx_pin; (void)baud_rate; return true; }
void pio_uart_deinit(void) {}
bool pio_uart_reconfigure(uint8_t tx_pin, uint8_t rx_pin, uint32_t baud_rate) { (void)tx_pin; (void)rx_pin; (void)baud_rate; return true; }
void pio_uart_write_blocking(const uint8_t* data, uint32_t len) { (void)data; (void)len; }
bool pio_uart_is_readable(void) { return false; }
uint8_t pio_uart_getc(void) { return 0; }
#endif
