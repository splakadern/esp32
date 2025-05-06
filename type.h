// Header File: type.h
#ifndef TYPE_H
#define TYPE_H

// Include the ESP-IDF Wi-Fi types header where wifi_promiscuous_filter_t
// and the WIFI_PROMIS_FILTER_MASK_ macros are defined, and potentially
// other types like wifi_pkt_rx_ctrl_t and wifi_promiscuous_pkt_t if they are not defined here.
// This is CRUCIAL for fixing "unknown type name" errors related to ESP-IDF structs.
#include <esp_wifi.h>
#include <esp_wifi_types.h> // Often needed for structures like wifi_pkt_rx_ctrl_t and wifi_promiscuous_pkt_t

// --- Custom Structure Definitions ---
// These were provided in your snippets

typedef struct {
  // Frame Control field (2 bytes) - 0xC000 is the Type/Subtype for Deauthentication
  uint8_t frame_control[2] = { 0xC0, 0x00 };
  uint8_t duration[2];     // Duration field (2 bytes)
  uint8_t station[6];      // Destination MAC Address (the station to deauthenticate)
  uint8_t sender[6];       // Source MAC Address (the AP's MAC)
  uint8_t access_point[6]; // BSSID (the AP's MAC)
  // Fragment number (lower 4 bits) and Sequence number (upper 12 bits)
  // The values 0xF0, 0xFF give sequence 4095, fragment 0. You might need to manage this.
  uint8_t fragment_sequence[2] = { 0xF0, 0xFF }; // This might be incorrectly structured. Usually, sequence is 12 bits, fragment is 4 bits. A uint16_t might be better.
  uint16_t reason;         // Reason Code (2 bytes)
} deauth_frame_t;

// CORRECTED: Renamed 'dest' to 'addr1' and 'src' to 'addr2' to match deauth.ino usage
typedef struct {
  uint16_t frame_ctrl;    // Frame Control (2 bytes)
  uint16_t duration;      // Duration/ID (2 bytes)
  uint8_t addr1[6];       // Address 1 (Destination MAC / Receiver MAC) - Renamed from 'dest'
  uint8_t addr2[6];       // Address 2 (Source MAC / Transmitter MAC) - Renamed from 'src'
  uint8_t bssid[6];       // Address 3 (BSSID) - Name matches usage in deauth.ino
  uint16_t sequence_ctrl; // Sequence Control (2 bytes) - Sequence number (12 bits) + Fragment number (4 bits)
  uint8_t addr4[6];       // Address 4 (only in WDS) - Note: Your sniffer code doesn't seem to handle addr4 explicitly.
} mac_hdr_t;

// Structure representing a standard Wi-Fi packet payload following the MAC header
// The payload[0] is a flexible array member, meaning the actual data follows after the header.
// This structure is often similar to esp_wifi_80211_packet_t or similar ESP-IDF types.
// Note: The sniffer code casts raw_packet->payload to wifi_packet_t* which contains mac_hdr_t.
// This definition aligns with that casting and is needed because your sniffer uses `packet->hdr`.
typedef struct {
  mac_hdr_t hdr; // The MAC header member
  uint8_t payload[0]; // Rest of the packet data (flexible array member)
} wifi_packet_t;

// Structure for the raw promiscuous mode packet received from the Wi-Fi driver.
// This structure is typically defined in ESP-IDF headers (like esp_wifi_types.h).
// It contains receive control information and the raw 802.11 frame data.
// REMOVED: This definition was conflicting because wifi_promiscuous_pkt_t is defined in ESP-IDF headers.
/*
typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl; // Receive control information provided by ESP-IDF
    uint8_t payload[0]; // Raw 802.11 frame data, starting with mac_hdr_t
} wifi_promiscuous_pkt_t;
*/


// Structure provided by esp_wifi.h for configuring the promiscuous mode filter
// This requires esp_wifi.h to be included BEFORE this definition.
const wifi_promiscuous_filter_t filt = {
  .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA // Filter for Management and Data frames
  // Add other masks if needed, e.g., WIFI_PROMIS_FILTER_MASK_MISC for control frames
};

// --- Add any other necessary definitions or declarations from your original type.h ---


#endif // TYPE_H
