#include <DIYables_ESP32_WebServer.h>

#define RELAY_PIN 13      // gripper relay
#define LED_PIN  15    // status LED

int relay_state = LOW;
bool wifiConnected = false;

// WiFi credentials
const char WIFI_SSID[]     = "kiki";
const char WIFI_PASSWORD[] = "150SSS9006";

// Web server
DIYables_ESP32_WebServer server;

// ------- HTML PAGE -------
String getHTML() {
	String stateText = (relay_state == LOW) ? "OPEN" : "CLOSED";
	String stateColor = (relay_state == LOW) ? "green" : "red";

	String html = R"html(
	<!DOCTYPE html>
	<html>
	<head>
	<meta name="viewport" content="width=device-width, initial-scale=1">
	<title>Gripper Control</title>
	<style>
	body { font-family: Arial; background:#111; color:#eee; text-align:center; }
	h1   { margin-top:20px; }
	.card { background:#222; border-radius:15px; padding:20px; display:inline-block; margin-top:20px; }
	.state { font-size:20px; margin-bottom:15px; }
	.btn  { display:inline-block; padding:10px 25px; margin:5px;
	border-radius:20px; border:none; font-size:16px; cursor:pointer; }
	.on  { background:#28a745; color:white; }
	.off { background:#dc3545; color:white; }
	/* Simple gripper animation */
	.gripper { margin:20px auto; width:120px; height:80px; position:relative; }
	.finger {
		position:absolute; width:40px; height:8px; background:#00d0ff;
		top:36px; transform-origin:0 50%; transition:transform 0.3s;
	}
	.finger.right { right:0; transform-origin:100% 50%; }
	.open .finger.left  { transform:rotate(-25deg); }
	.open .finger.right { transform:rotate(25deg); }
	.closed .finger.left  { transform:rotate(-5deg); }
	.closed .finger.right { transform:rotate(5deg); }
	</style>
	</head>
	<body>
	<h1>ESP32‑C3 Gripper</h1>
	<div class="card">
	<div class="gripper )html";

	html += (relay_state == LOW) ? "open" : "closed";
	html += R"html(">
	<div class="finger left"></div>
	<div class="finger right"></div>
	</div>
	<p class="state">Gripper: <span style="color:)html";
	html += stateColor;
	html += R"html(; font-weight:bold;">)html";
	html += stateText;
	html += R"html(</span></p>
	<a href="/relay1/on"><button class="btn on">Close</button></a>
	<a href="/relay1/off"><button class="btn off">Open</button></a>
	</div>
	</body>
	</html>
	)html";

	return html;
}

// ------- HANDLERS -------
void handleHome(WiFiClient &client, const String&, const String&, const QueryParams&, const String&) {
	Serial.println("ESP32 Web Server: New request received");
	server.sendResponse(client, getHTML().c_str());
}

void handleRelayOn(WiFiClient &client, const String&, const String&, const QueryParams&, const String&) {
	Serial.println("ESP32 Web Server: Close gripper");
	relay_state = HIGH;
	digitalWrite(RELAY_PIN, relay_state);
	server.sendResponse(client, getHTML().c_str());
}

void handleRelayOff(WiFiClient &client, const String&, const String&, const QueryParams&, const String&) {
	Serial.println("ESP32 Web Server: Open gripper");
	relay_state = LOW;
	digitalWrite(RELAY_PIN, relay_state);
	server.sendResponse(client, getHTML().c_str());
}

// ------- SETUP & LOOP -------
void setup() {
	Serial.begin(9600);
	pinMode(RELAY_PIN, OUTPUT);
	digitalWrite(RELAY_PIN, LOW);

	pinMode(LED_PIN, OUTPUT);

	Serial.println("Connecting WiFi...");

	// start Wi‑Fi + server
	server.begin(WIFI_SSID, WIFI_PASSWORD);

	// blink until connected
	while (WiFi.status() != WL_CONNECTED) {
		digitalWrite(LED_PIN, HIGH);
		delay(200);
		digitalWrite(LED_PIN, LOW);
		delay(200);
	}

	// connected: LED solid ON
	digitalWrite(LED_PIN, HIGH);

	// routes
	server.addRoute("/", handleHome);
	server.addRoute("/relay1/on", handleRelayOn);
	server.addRoute("/relay1/off", handleRelayOff);
}

void loop() {
	server.handleClient();
}


