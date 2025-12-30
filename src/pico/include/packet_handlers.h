#ifndef PACKET_HANDLERS_H
#define PACKET_HANDLERS_H

#include "protocol.h"

/**
 * Packet Handler Module
 * 
 * Breaks down large packet handling into smaller, testable functions.
 * Each handler is responsible for one command type.
 */

// =============================================================================
// Individual Packet Handlers
// =============================================================================

/**
 * Handle PING command
 * @param packet Received packet
 */
void handle_cmd_ping(const packet_t* packet);

/**
 * Handle SET_TEMP command
 * @param packet Received packet
 */
void handle_cmd_set_temp(const packet_t* packet);

/**
 * Handle SET_PID command
 * @param packet Received packet
 */
void handle_cmd_set_pid(const packet_t* packet);

/**
 * Handle BREW command
 * @param packet Received packet
 */
void handle_cmd_brew(const packet_t* packet);

/**
 * Handle MODE command
 * @param packet Received packet
 */
void handle_cmd_mode(const packet_t* packet);

/**
 * Handle GET_CONFIG command
 * @param packet Received packet
 */
void handle_cmd_get_config(const packet_t* packet);

/**
 * Handle CONFIG command
 * @param packet Received packet
 */
void handle_cmd_config(const packet_t* packet);

/**
 * Handle GET_ENV_CONFIG command
 * @param packet Received packet
 */
void handle_cmd_get_env_config(const packet_t* packet);

/**
 * Handle CLEANING commands
 * @param packet Received packet
 */
void handle_cmd_cleaning(const packet_t* packet);

/**
 * Handle GET_STATISTICS command
 * @param packet Received packet
 */
void handle_cmd_get_statistics(const packet_t* packet);

/**
 * Handle DEBUG command
 * @param packet Received packet
 */
void handle_cmd_debug(const packet_t* packet);

/**
 * Handle SET_ECO command
 * @param packet Received packet
 */
void handle_cmd_set_eco(const packet_t* packet);

/**
 * Handle BOOTLOADER command
 * @param packet Received packet
 */
void handle_cmd_bootloader(const packet_t* packet);

/**
 * Handle DIAGNOSTICS command
 * @param packet Received packet
 */
void handle_cmd_diagnostics(const packet_t* packet);

/**
 * Handle POWER_METER commands
 * @param packet Received packet
 */
void handle_cmd_power_meter(const packet_t* packet);

/**
 * Handle GET_BOOT command
 * @param packet Received packet
 */
void handle_cmd_get_boot(const packet_t* packet);

/**
 * Handle LOG_CONFIG command
 * @param packet Received packet
 */
void handle_cmd_log_config(const packet_t* packet);

#endif // PACKET_HANDLERS_H
