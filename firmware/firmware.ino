/**
 * @file firmware.ino
 * @author Ananta Srikar
 * @brief Firmware that run on the ESP32 (4MB) to manage WiFi and Display messages on the SD
 * @version 0.1
 * @date 2023-05-28
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "WiFiProv.h"
#include "WiFi.h"
#include "esp_wifi.h"

#define USERNAME "user"

int reconnect_try = 0;

/**
 * @brief Function that erases WiFi credentials
 * 
 */
void eraseAllWiFiCredentials() {
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg); //initiate and allocate wifi resources (does not matter if connection fails)
	delay(2000); //wait a bit
	if(esp_wifi_restore()!=ESP_OK)
	{
		Serial.println("WiFi is not initialized by esp_wifi_init ");
	}
	else
	{
		Serial.println("WiFi Configurations Cleared!");
	}
	
	delay(1000);

	ESP.restart();
}

/**
 * @brief Function that handles ESP32's WiFi
 * 
 * @param sys_event Arduino event that is linked to Wifi
 */
void SysProvEvent(arduino_event_t *sys_event)
{
	switch (sys_event->event_id)
	{
		case ARDUINO_EVENT_WIFI_STA_GOT_IP:
			Serial.print("\nConnected IP address : ");
			Serial.println(IPAddress(sys_event->event_info.got_ip.ip_info.ip.addr));
			break;

		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
			Serial.println("\nDisconnected. Connecting to the AP again... ");
			reconnect_try++;

			// Try reprovisioning if unable to connect
			if(reconnect_try > 3)
			{
				reconnect_try = 0;
				eraseAllWiFiCredentials();
			}

			break;

		case ARDUINO_EVENT_PROV_START:
			Serial.println("\nProvisioning started\nGive Credentials of your access point using \" Android app \"");
			break;

		case ARDUINO_EVENT_PROV_CRED_RECV:
			Serial.println("\nReceived Wi-Fi credentials");
			Serial.print("\tSSID : ");
			Serial.println((const char *) sys_event->event_info.prov_cred_recv.ssid);
			Serial.print("\tPassword : ");
			Serial.println((char const *) sys_event->event_info.prov_cred_recv.password);
			break;
		
		case ARDUINO_EVENT_PROV_CRED_FAIL: 
			Serial.println("\nProvisioning failed!\nPlease reset to factory and retry provisioning\n");
			if(sys_event->event_info.prov_fail_reason == WIFI_PROV_STA_AUTH_ERROR) 
				Serial.println("\nWi-Fi AP password incorrect");
			else
				Serial.println("\nWi-Fi AP not found....Add API \" nvs_flash_erase() \" before beginProvision()");
			break;
		
		case ARDUINO_EVENT_PROV_CRED_SUCCESS:
			Serial.println("\nProvisioning Successful");
			break;
		case ARDUINO_EVENT_PROV_END:
			Serial.println("\nProvisioning Ends");
			break;
		default:
			break;
	}
}

/**
 * @brief Setup the board before running it
 */
void setup()
{
	// Run UART at 115200 baud rate.
	Serial.begin(115200);

	// WiFi 
	WiFi.onEvent(SysProvEvent);
	WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, USERNAME, "Prov_BIGBYE");
}

/**
 * @brief Runs forever after the setup
 */
void loop()
{
}
