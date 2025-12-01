/**
 * BrewOS Cloud Pairing Screen
 * 
 * Shows QR code and pairing code for cloud integration
 */

#ifndef SCREEN_CLOUD_H
#define SCREEN_CLOUD_H

#include <lvgl.h>
#include "ui.h"

/**
 * Create the cloud pairing screen
 */
lv_obj_t* screen_cloud_create(void);

/**
 * Update the cloud screen with pairing info
 * @param device_id The device ID (e.g., "BRW-XXXXXXXX")
 * @param token The pairing token
 * @param url The full pairing URL
 * @param expires_in Seconds until token expires
 */
void screen_cloud_update(const char* device_id, const char* token, 
                         const char* url, uint32_t expires_in);

/**
 * Show "generating" state while fetching new token
 */
void screen_cloud_show_loading(void);

/**
 * Show error state
 */
void screen_cloud_show_error(const char* message);

/**
 * Handle encoder navigation
 */
void screen_cloud_encoder(int direction);

/**
 * Handle selection (refresh button)
 */
void screen_cloud_select(void);

/**
 * Callback for refresh button press
 */
typedef void (*cloud_refresh_callback_t)(void);
void screen_cloud_set_refresh_callback(cloud_refresh_callback_t callback);

#endif // SCREEN_CLOUD_H

