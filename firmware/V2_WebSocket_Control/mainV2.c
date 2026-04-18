#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define RELAY_PIN 13      // gripper relay
#define LED_PIN   15      // status LED

int relay_state = LOW;

const char* ssid = "kiki";
const char* password = "150SSS9006";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Gripper Control</title>
<style>
  body { 
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
    background: #121216; color: #fff; text-align: center; margin: 0; padding: 40px 10px; 
  }
  .card { 
    background: #1e1e28; border-radius: 16px; padding: 40px 20px; 
    box-shadow: 0 10px 40px rgba(0,0,0,0.8); display: inline-block; width: 100%; max-width: 420px; 
    border: 1px solid #333;
  }
  h1 { margin-top: 0; letter-spacing: 2px; color: #f0f0f0; text-transform: uppercase; font-size: 24px; }
  
  /* --- Realistic Mechanical Gripper Animation --- */
  .gripper-container { 
    position: relative; width: 240px; height: 130px; 
    margin: 20px auto 40px auto; 
  }
  
  /* Bottom Aluminum Rail */
  .rail {
    position: absolute; bottom: 0; left: 0; right: 0;
    height: 24px; 
    background: linear-gradient(0deg, #444 0%, #777 50%, #444 100%);
    border-radius: 4px; box-shadow: 0 8px 15px rgba(0,0,0,0.6);
    border: 1px solid #222;
  }

  /* Central Dark Housing (Coil/Motor block) */
  .housing {
    position: absolute; bottom: 0; left: 50%; transform: translateX(-50%);
    width: 70px; height: 60px; 
    background: linear-gradient(135deg, #2a2a2a 0%, #111 100%);
    border: 2px solid #000; border-radius: 6px; z-index: 3;
    box-shadow: inset 0 2px 4px rgba(255,255,255,0.1), 0 5px 15px rgba(0,0,0,0.8);
  }
  /* Little detail line on the housing */
  .housing::after {
    content: ''; position: absolute; top: 15px; left: 15px; right: 15px; bottom: 15px;
    border: 1px solid #444; border-radius: 3px;
  }

  /* The Sliding Jaws (Group) */
  .jaw { 
    position: absolute; bottom: 4px; 
    width: 50px; height: 90px; 
    transition: transform 0.5s cubic-bezier(0.25, 1, 0.4, 1); /* Smooth mechanical slide */
    z-index: 2;
  }
  .jaw.left { left: 10px; }
  .jaw.right { right: 10px; }
  
  /* Jaw Mount (The wider bottom part) */
  .jaw-base {
    position: absolute; bottom: 0; width: 100%; height: 35px;
    background: linear-gradient(180deg, #ddd 0%, #999 100%);
    border-radius: 4px; border: 1px solid #555;
    box-shadow: inset 0 2px 5px rgba(255,255,255,0.5);
  }
  
  /* Jaw Fingers (The actual clamping tips) */
  .jaw-finger {
    position: absolute; bottom: 34px; width: 18px; height: 56px;
    background: linear-gradient(90deg, #d4d4d4, #f5f5f5, #b8b8b8);
    border: 1px solid #666; border-bottom: none;
    border-radius: 4px 4px 0 0;
  }
  
  /* Position fingers on the inner edges to make the "L" shape */
  .jaw.left .jaw-finger { right: 4px; }
  .jaw.right .jaw-finger { left: 4px; }
  
  /* Add grip "teeth" pattern to the inner edges */
  .jaw.left .jaw-finger { border-right: 4px dashed #555; }
  .jaw.right .jaw-finger { border-left: 4px dashed #555; }

  /* Linear Slide Distances */
  .open .jaw.left { transform: translateX(0); }
  .open .jaw.right { transform: translateX(0); }
  
  /* Adjust these values if the jaws overlap or don't close enough */
  .closed .jaw.left { transform: translateX(55px); }
  .closed .jaw.right { transform: translateX(-55px); }

  /* UI Elements */
  .status { font-size: 20px; font-weight: 800; margin-bottom: 25px; letter-spacing: 3px; transition: color 0.3s; }
  .status.open { color: #00e676; text-shadow: 0 0 10px rgba(0, 230, 118, 0.4); }
  .status.closed { color: #ff3d00; text-shadow: 0 0 10px rgba(255, 61, 0, 0.4); }
  
  .btn-container { display: flex; justify-content: center; gap: 15px; }
  .btn { 
    background: #2a2a35; padding: 14px 20px; border-radius: 8px; font-size: 15px; font-weight: bold; 
    cursor: pointer; transition: all 0.2s ease; border: 1px solid #444; color: #fff; flex: 1; text-transform: uppercase;
  }
  .btn:active { transform: scale(0.95); }
  .btn.open-btn { border-bottom: 3px solid #00e676; }
  .btn.open-btn:hover { background: rgba(0, 230, 118, 0.1); border-color: #00e676; }
  .btn.close-btn { border-bottom: 3px solid #ff3d00; }
  .btn.close-btn:hover { background: rgba(255, 61, 0, 0.1); border-color: #ff3d00; }
</style>
</head>
<body>

<div class="card">
  <h1>END-EFFECTOR Control</h1>
  
  <div id="gripper-ui" class="gripper-container open">
    <div class="rail"></div>
    <div class="housing"></div>
    
    <!-- Left Jaw -->
    <div class="jaw left">
      <div class="jaw-base"></div>
      <div class="jaw-finger"></div>
    </div>
    
    <!-- Right Jaw -->
    <div class="jaw right">
      <div class="jaw-base"></div>
      <div class="jaw-finger"></div>
    </div>
  </div>
  
  <div id="status-txt" class="status open">STATE: OPEN</div>
  
  <div class="btn-container">
    <button class="btn open-btn" onclick="sendCommand('open')">Open Jaws</button>
    <button class="btn close-btn" onclick="sendCommand('close')">Close Jaws</button>
  </div>
</div>

<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  
  function onLoad(event) { initWebSocket(); }
  
  function initWebSocket() {
    websocket = new WebSocket(gateway);
    websocket.onmessage = onMessage;
  }
  
  function onMessage(event) {
    var state = event.data;
    var ui = document.getElementById('gripper-ui');
    var txt = document.getElementById('status-txt');
    
    if (state === "1") {
      ui.className = "gripper-container closed";
      txt.className = "status closed";
      txt.innerHTML = "CLOSED";
    } else {
      ui.className = "gripper-container open";
      txt.className = "status open";
      txt.innerHTML = "OPEN";
    }
  }
  
  function sendCommand(action) {
    websocket.send(action);
  }
</script>
</body>
</html>
)rawliteral";

// ------- WEBSOCKET HANDLERS -------
void notifyClients() {
  ws.textAll(String(relay_state));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    
    if (message == "open") {
      Serial.println("WebSocket: Open gripper");
      relay_state = LOW;
      digitalWrite(RELAY_PIN, relay_state);
      notifyClients();
    } 
    else if (message == "close") {
      Serial.println("WebSocket: Close gripper");
      relay_state = HIGH;
      digitalWrite(RELAY_PIN, relay_state);
      notifyClients();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected\n", client->id());
      client->text(String(relay_state)); // Send current state on connect
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

// ------- SETUP & LOOP -------
void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, relay_state);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("Connecting WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Blink while connecting
    delay(250);
  }
  
  digitalWrite(LED_PIN, HIGH); // Solid when connected
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Web Server Route
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // WebSocket Setup
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.begin();
}

void loop() {
  ws.cleanupClients(); // Keeps WebSockets running smoothly
}
