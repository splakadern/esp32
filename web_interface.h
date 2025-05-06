// Header File: web_interface.h
#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <WebServer.h> // For WebServer class

// --- Function Declarations ---

// Initializes and starts the web interface
void start_web_interface();

// Handles incoming client requests for the web server
void web_interface_handle_client();

#endif // WEB_INTERFACE_H
