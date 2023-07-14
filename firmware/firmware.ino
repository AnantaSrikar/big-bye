/**
 * @file firmware.ino
 * @author Ananta Srikar
 * @brief Firmware that run on the ESP32 (4MB) to manage WiFi and Display messages on the ST7735 Display
 * @version 0.3
 * @date 2023-05-28
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <WiFiProv.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <qrcode_espi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>	// Make sure v5.13.4 is installed

// Modify as per your setup
#define USERNAME "user"
#define SERVER_ADDR "https://yourwebdomain.here.com"
#define BASICAUTH_USERNAME	"changeUsernameLater"
#define BASICAUTH_PASSWORD	"changePasswordLater"

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
bool has_IP_addr = false;

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

// Finding the center Y coordinate. NOTE: Width and heigth parameters interchanged before switching to landscape
int Y_CENTER = (display.width() / 2) - LTR_PXL;

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
			has_IP_addr = true;
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
		
			displayCenter("Received WiFi credentials!", Y_CENTER, true);
			break;
		
		case ARDUINO_EVENT_PROV_CRED_FAIL:
			Serial.println("\nProvisioning failed!\nPlease reset to factory and retry provisioning\n");
			if(sys_event->event_info.prov_fail_reason == WIFI_PROV_STA_AUTH_ERROR)
			{
				Serial.println("\nWi-Fi AP password incorrect");
				displayCenter("WiFi Password incorrect!", Y_CENTER, true);
			}
			else
			{
				displayCenter("WiFi AP not found!", Y_CENTER, true);
				Serial.println("\nWi-Fi AP not found....Add API \" nvs_flash_erase() \" before beginProvision()");
			}
			delay(1000);
			eraseAllWiFiCredentials();
			break;
		
		case ARDUINO_EVENT_PROV_CRED_SUCCESS:
			Serial.println("\nProvisioning Successful");
			break;
		
		case ARDUINO_EVENT_PROV_END:
			Serial.println("\nProvisioning Ends");
			displayCenter("Provisioning Complete!", Y_CENTER, true);
			delay(1000);
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
	HTTPClient http;
	
	// Set the Authorization header
	http.setAuthorization(BASICAUTH_USERNAME, BASICAUTH_PASSWORD);

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
 * @brief Funtion to fetch and update the number of messages available for a user
 */
void updateNumMsgsForUser()
{
	String str_num_msgs = getHTTPS(user_server_addr);
	num_msgs = str_num_msgs.toInt();

	Serial.println("In updateNumMsgsForUser");

	// Something is wrong in DB, or username is incorrect, or BasicAuth creds failed
	if(num_msgs == 0)
	{
		refetch_needed = false;
		Serial.println("0 detected again, locking!");
		displayCenter("Something is terribly bad...", Y_CENTER, true);

		// Put the ESP32 into a locked state with no further processing
		is_prov_needed = true;
		is_qr_displayed = true;
		return;
	}
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
 * @brief Function that wraps words on the TFT disply
 * 
 * @param text The text you want to print on the screen
 */
void printWithWordWrap(String text)
{
	int wordStart = 0;
	int wordEnd = 0;
	while((text.indexOf(' ', wordStart) >= 0) && ( wordStart <= text.length()))
	{
		wordEnd = text.indexOf(' ', wordStart + 1);
		uint16_t len = display.textWidth(text.substring(wordStart, wordEnd));
		if(display.getCursorX() + len >= display.width())
		{
			display.println();
			if (wordStart > 0)
			{
				wordStart++;
			}
		}
		display.print(text.substring(wordStart, wordEnd));
		wordStart = wordEnd;
	}
}

/**
 * @brief Displays text at the center of the screen, at given Y coordinate
 * 
 * @param screenText Text that needs to be displayed at the center
 */
void displayCenter(String screenText, int y_coord, bool wipe_screen)
{
	if(wipe_screen)
	{
		display.fillScreen(TFT_WHITE);
	}

	display.setTextSize(1);
	display.setTextColor(TFT_BLACK, TFT_WHITE);
	display.drawString(screenText, getCenterXcoord(screenText), y_coord, 1);
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
	displayCenter(heading, 5, true);
	
	// Display the message
	display.setCursor(0, 20);
	printWithWordWrap(msg);

	// Display msg number
	String footer = String(cur_msg_num) + "/" + String(num_msgs);
	displayCenter(footer, display.height() - LTR_PXL - 2, false);
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
	displayCenter("Initialising...", Y_CENTER, true);

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
		// Wait till we get IP address. 
		// Note to self: Spin locks are terrible btw, please use better sync methods in future
		while(!has_IP_addr)
		{
			delay(50);
		}

		updateNumMsgsForUser();
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

	// Update number of messages for a user if we don't have it already
	if(num_msgs == 0)
	{
		updateNumMsgsForUser();
	}

	// Initializing input buttons
	isBTNup = digitalRead(BTN_UP);
	isBTNdown = digitalRead(BTN_DOWN);
	isBTNleft = digitalRead(BTN_LEFT);
	isBTNright = digitalRead(BTN_RIGHT);

	// Combo Buttons
	if(isBTNdown == LOW && isBTNup == LOW)
	{
		Serial.println("Combo button press!");
		displayCenter("Factory reset board?", Y_CENTER, true);
		displayCenter("Press down button to confirm", Y_CENTER + LTR_PXL * 2, false);
		delay(1000);

		refetch_needed = true;
	}

	// Single button presses

	if(isBTNleft == LOW)
	{
		Serial.println("Left button press detected");

		if(cur_msg_num > 1)
		{
			cur_msg_num--;
		}

		// Display that you're viewing the first message if already on first message
		else if(cur_msg_num == 1)
		{
			displayCenter("You're at the first message!", Y_CENTER, true);
			delay(1000);
		}

		refetch_needed = true;
		delay(100);
	}

	else if(isBTNright == LOW)
	{
		Serial.println("Right button press detected");

		if(cur_msg_num < num_msgs)
		{
			cur_msg_num++;
		}

		// Display that you're viewing the last message if already on last message
		else if (cur_msg_num == num_msgs)
		{
			displayCenter("You're at the last message!", Y_CENTER, true);
			delay(1000);
		}

		refetch_needed = true;
		delay(100);
	}

	else if(isBTNup == LOW)
	{
		// screenText = "Up pressed";
	}

	else if(isBTNdown == LOW)
	{
		// screenText = "Down pressed";
	}

	// Refetch and update display only when needed!
	if(refetch_needed)
	{
		// Refetch and update display

		displayCenter("Fetching....", Y_CENTER, true);

		updateDisplayWithMsg();

		refetch_needed = false;
	}

	delay(50);
}