/**
 * @file firmware.ino
 * @author Ananta Srikar
 * @brief Firmware that run on the ESP32 (4MB) to manage WiFi and Display messages on the ST7735 Display
 * @version 0.2
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
#include <ArduinoJson.h>	// Make sure v5.13.4 is installed

#define USERNAME "user"
#define SERVER_ADDR "https://yourwebdomain.here.com"

// Each character width is approximately 6 pixels with text size 1
#define LTR_PXL	6

#define BTN_LEFT	14
#define BTN_RIGHT	13
#define BTN_UP		27
#define BTN_DOWN	12

#define JSON_PARSE_CAP 2048

int reconnect_try = 0;
bool is_prov_needed = false;
bool is_qr_displayed = false;

bool isBTNup = false;
bool isBTNdown = false;
bool isBTNleft = false;
bool isBTNright = false;

String user_server_addr = "";
bool refetch_needed = true;
int cur_msg_num = 1;
int num_msgs = 0;

// Display setup
TFT_eSPI display = TFT_eSPI();
QRcode_eSPI qrcode(&display);

// JSON parser setup
StaticJsonBuffer<JSON_PARSE_CAP> JSONBuffer;

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
 * @brief Funtion that runs a HTTPS get request on a specified URL
 * 
 * @param url URL to run the GET request on
 * @return String output of the GET request
 */
String getHTTPS(String url)
{

	Serial.println(url);

	HTTPClient http;

	// Run the HTTP get
	http.begin(url);
	int httpCode = http.GET();

	// HTTP code will be negative on error
	if(httpCode > 0)
	{
		Serial.printf("[HTTP] GET - code: %d\n", httpCode);
		
		// Return the result of the request if everything went well
		if(httpCode == HTTP_CODE_OK)
		{
			return http.getString();
		}
	}

	else
	{
		Serial.printf("[HTTP] GET - error: %s\n", http.errorToString(httpCode).c_str());
		return "ERROR";
	}

	http.end();
}

/**
 * @brief Funtion to fetch the number of messages available for a user
 * 
 * @return int number of messages for a user
 */
int numMsgsForUser()
{
	String str_num_msgs = getHTTPS(user_server_addr);
	return str_num_msgs.toInt();
}

/**
 * @brief Get the X coord for the text to print at the center
 * 
 * @param text The text you want to display
 * @return int the X coordinate to print text on the display
 */
int getCenterXcoord(String text)
{
	// Each character width is approximately 6 pixels with text size 1
	int x_coord = (display.width() - (text.length() * LTR_PXL)) / 2;

	if(x_coord < 0 || x_coord > display.width())
	{
		return 0;
	}

	return x_coord;
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
	display.drawString("WiFi connection required!", 5, 118, 1);
}

/**
 * @brief Displays text at the center of the screen
 * 
 * @param screenText Text that needs to be displayed at the center
 */
void displayCenter(String screenText)
{
	display.fillScreen(TFT_WHITE);
	display.setTextSize(1);
	display.setTextColor(TFT_BLACK, TFT_WHITE);
	display.drawString(screenText, getCenterXcoord(screenText), (display.height() - LTR_PXL) / 2, 1);
}

/**
 * @brief Function that updates the display with the right message number
 * 
 */
void updateDisplayWithMsg()
{
	// Get the full message
	String cur_msg_url = user_server_addr;
	cur_msg_url += "/";
	cur_msg_url += String(cur_msg_num);

	// Get current Message JSON and parse it
	JsonObject& parsed = JSONBuffer.parseObject(getHTTPS(cur_msg_url));

	if(!parsed.success())
	{
		Serial.println("\n\nJSON parsing failed!\n\n");
		delay(5000);
	}

	String heading = parsed["heading"];
	String msg = parsed["msg"];

	Serial.println(heading);
	
	display.fillScreen(TFT_WHITE);
	display.setTextSize(1);
	display.setTextColor(TFT_BLACK, TFT_WHITE);

	// Display heading
	display.drawString(heading, getCenterXcoord(heading), 5, 1);

	display.setCursor(5, 15);
	display.println(msg);
}

/**
 * @brief Setup the board before running it
 */
void setup()
{
	// Run UART at 115200 baud rate.
	Serial.begin(115200);

	// Diplay initialisation
	display.init();
	display.setRotation(3);
	display.fillScreen(TFT_WHITE);
	displayCenter("Initialising...");

	// Setup BTN Inputs
	pinMode(BTN_LEFT, INPUT_PULLUP);
	pinMode(BTN_RIGHT, INPUT_PULLUP);
	pinMode(BTN_UP, INPUT_PULLUP);
	pinMode(BTN_DOWN, INPUT_PULLUP);

	// WiFi 
	WiFi.onEvent(SysProvEvent);
	WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, USERNAME, "PROV_BIGBYE");

	// User specific URL
	user_server_addr = SERVER_ADDR;
	user_server_addr += "/user/";
	user_server_addr += USERNAME;

	// Wait till we attempt to connect to WiFi
	delay(2000);

	if(!is_prov_needed)
	{
		num_msgs = numMsgsForUser();
		Serial.println(num_msgs);

		// Something is wrong on DB or username is incorrect
		if(num_msgs == 0)
		{
			displayCenter("Something is configured incorrectly...");

			// Put the ESP32 into a locked state with no further processing
			is_prov_needed = true;
			is_qr_displayed = true;
			return;
		}
	}
}

/**
 * @brief Runs forever after the setup
 */
void loop()
{
	// Pre WiFi Provisioning checks
	if(is_prov_needed)
	{
		if(!is_qr_displayed)
		{
			displayProvQRcode();
			is_qr_displayed = true;
		}

		delay(10000);
		return;
	}

	// Since provisioning is done
	is_qr_displayed = false;

	// Initializing input buttons
	isBTNup = digitalRead(BTN_UP);
	isBTNdown = digitalRead(BTN_DOWN);
	isBTNleft = digitalRead(BTN_LEFT);
	isBTNright = digitalRead(BTN_RIGHT);


	// Refetch and update display only when needed!
	if(refetch_needed)
	{
		// Refetch and update display

		displayCenter("Fetching....");

		// TODO: Display message heading, message and footer
		updateDisplayWithMsg();

		refetch_needed = false;
	}

	// TODO: Combo buttons

	// Single button presses

	if(isBTNleft == LOW)
	{
		delay(100);
		Serial.println("Left button press detected");

		refetch_needed = true;

		if(cur_msg_num > 1)
		{
			cur_msg_num--;
			
		}

		// Display that you're viewing the first message if already on first message
		else if(cur_msg_num == 1)
		{
			displayCenter("You're at the first message!");
			delay(500);
		}
	}

	else if(isBTNright == LOW)
	{
		delay(100);
		Serial.println("Right button press detected");

		refetch_needed = true;

		if(cur_msg_num < num_msgs)
		{
			cur_msg_num++;
		}

		// Display that you're viewing the last message if already on last message
		else if (cur_msg_num == num_msgs)
		{
			displayCenter("You're at the last message!");
			delay(500);
		}
	}

	else if(isBTNup == LOW)
	{
		// screenText = "Up pressed";
	}

	else if(isBTNdown == LOW)
	{
		// screenText = "Down pressed";
	}

	delay(50);
}
