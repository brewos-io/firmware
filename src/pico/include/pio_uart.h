/**
 * PIO-based UART for Raspberry Pi Pico 2 (RP2350)
 * 
 * Uses PIO state machines to implement UART TX and RX on arbitrary GPIO pins.
 * This is needed because GPIO6/GPIO7 map to UART1_CTS/RTS (not TX/RX) in the
 * RP2350 silicon. Hardware UART1 cannot transmit/receive data on these pins.
 * 
 * Uses PIO0: SM0 for TX, SM1 for RX.
 */

#ifndef PIO_UART_H
#define PIO_UART_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize PIO UART on the specified pins.
 * @param tx_pin GPIO pin for TX (data out)
 * @param rx_pin GPIO pin for RX (data in)
 * @param baud_rate Baud rate (e.g., 9600)
 * @return true on success
 */
bool pio_uart_init(uint8_t tx_pin, uint8_t rx_pin, uint32_t baud_rate);

/**
 * Deinitialize PIO UART and release resources.
 */
void pio_uart_deinit(void);

/**
 * Reconfigure PIO UART with new pins and/or baud rate.
 * Deinitializes and reinitializes.
 */
bool pio_uart_reconfigure(uint8_t tx_pin, uint8_t rx_pin, uint32_t baud_rate);

/**
 * Write data (blocking until all bytes are in the TX FIFO).
 * @param data Pointer to data buffer
 * @param len Number of bytes to send
 */
void pio_uart_write_blocking(const uint8_t* data, uint32_t len);

/**
 * Check if data is available to read.
 * @return true if at least one byte is available
 */
bool pio_uart_is_readable(void);

/**
 * Read one byte (blocking if no data available).
 * @return The received byte
 */
uint8_t pio_uart_getc(void);

#endif // PIO_UART_H
