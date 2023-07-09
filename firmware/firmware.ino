/**
 * @file firmware.ino
 * @author Ananta Srikar
 * @brief Firmware that run on the ESP32 (4MB) to manage WiFi and Display messages on the ST7735 Display
 * @version 0.1
 * @date 2023-05-28
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "WiFiProv.h"
#include "WiFi.h"
#include "esp_wifi.h"

#include <SPI.h>
#include <TFT_eSPI.h>
#include <qrcode_espi.h>
#include <HTTPClient.h>

#define USERNAME "user"
#define SERVER_ADDR "https://yourwebdomain.here.com"

#define BTN_LEFT	14
#define BTN_RIGHT	13
#define BTN_UP		27
#define BTN_DOWN	12

int reconnect_try = 0;
bool is_prov_needed = false;
bool is_qr_displayed = false;

bool isBTNup = false;
bool isBTNdown = false;
bool isBTNleft = false;
bool isBTNright = false;

TFT_eSPI display = TFT_eSPI();
QRcode_eSPI qrcode(&display);

/**
 * @brief Function that erases WiFi credentials
 * 
 */
void eraseAllWiFiCredentials()
{
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
 * @brief Function to display the QR code when provisioning is needed
 * 
 */
void displayProvQRcode()
{
	qrcode.init();

	String qr_prov = "{\"ver\":\"v1\",\"name\":\"PROV_BIGBYE\",\"pop\":\"";
	qr_prov += USERNAME;
	qr_prov += "\",\"transport\":\"softap\"}";
	qrcode.create(qr_prov);

	display.setTextSize(1);
	display.setTextColor(TFT_BLACK, TFT_WHITE);
	display.drawString("Scan with ESP SoftAP Prov", 5, 5, 1);
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
			is_prov_needed = true;
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
			is_prov_needed = false;
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
	// Setup BTN Inputs
	pinMode(BTN_LEFT, INPUT_PULLUP);
	pinMode(BTN_RIGHT, INPUT_PULLUP);
	pinMode(BTN_UP, INPUT_PULLUP);
	pinMode(BTN_DOWN, INPUT_PULLUP);

	// Run UART at 115200 baud rate.
	Serial.begin(115200);

	// WiFi 
	WiFi.onEvent(SysProvEvent);
	WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, USERNAME, "PROV_BIGBYE");

	// Diplay initialisation
	display.init();
	display.setRotation(3);
	display.fillScreen(TFT_WHITE);
}

/**
 * @brief Runs forever after the setup
 */
void loop()
{
	if(is_prov_needed)
	{
		if(!is_qr_displayed)
		{
			displayProvQRcode();
			is_qr_displayed = true;
		}
		return;
	}

	isBTNup = digitalRead(BTN_UP);
	isBTNdown = digitalRead(BTN_DOWN);
	isBTNleft = digitalRead(BTN_LEFT);
	isBTNright = digitalRead(BTN_RIGHT);

	Serial.println(isBTNup);
	Serial.println(isBTNdown);
	Serial.println(isBTNleft);
	Serial.println(isBTNright);
  
	
	is_qr_displayed = false;

	String screenText;
	HTTPClient http;

	// TODO: User specific domain
	http.begin(SERVER_ADDR);
	int httpCode = http.GET();

	// HTTP code will be negative on error
	if(httpCode > 0)
	{
		Serial.printf("[HTTP] GET - code: %d\n", httpCode);
		
		if(httpCode == HTTP_CODE_OK)
		{
			screenText = http.getString();
			Serial.println(screenText);
			
			display.fillScreen(TFT_WHITE);
			display.setTextSize(1);
			display.setTextColor(TFT_BLACK, TFT_WHITE);
			display.drawString(screenText, 5, 64, 1);
		}
	}

	else
	{
		Serial.printf("[HTTP] GET - error: %s\n", http.errorToString(httpCode).c_str());
	}

	http.end();

	if(isBTNdown == LOW)
	{
		screenText = "Down pressed";
	}
	else if(isBTNleft == LOW)
	{
		screenText = "Left pressed";
	}
	else if(isBTNright == LOW)
	{
		screenText = "Right pressed";
	}
	else if(isBTNup == LOW)
	{
		screenText = "Up pressed";
	}
	else
	{
		screenText = "Idle state";
	}
	
	display.fillScreen(TFT_WHITE);
	display.setTextSize(1);
	display.setTextColor(TFT_BLACK, TFT_WHITE);
	display.drawString(screenText, 5, 64, 1);

	delay(100);
}
