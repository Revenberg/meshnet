#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "User.h"
#include "NodeWebServer.h"
#include "LoraNode.h"
#include "Config.h"

// ====== Config ======
#define DNS_PORT 53
IPAddress apIP(192, 168, 3, 1);
// AP SSID is dynamically set based on node name (see begin() function)
String AP_SSID = "Node";  // Default fallback
String AP_PASS = "";

/*#define MAX_MSGS 20
struct NodeMessage {
    String id;
    String user;
    String text;
    int ttl;
};

NodeMessage messages[MAX_MSGS];
int msgWriteIndex = 0;
*/
// ====== Webserver & DNS ======
AsyncWebServer NodeWebServer::httpServer(80);
DNSServer NodeWebServer::dnsServer;

// ====== Helpers ======
String escapeHtml(const String &input)
{
    String out = input;
    out.replace("&", "&amp;");
    out.replace("<", "&lt;");
    out.replace(">", "&gt;");
    out.replace("\"", "&quot;");
    out.replace("'", "&#39;");
    return out;
}

String getSessionToken(AsyncWebServerRequest *request)
{
    String session = "";
    Serial.println("[INFO] getSessionToken");
    if (request->hasHeader("Cookie"))
    {
        String cookies = request->getHeader("Cookie")->value();
        Serial.println("[INFO] Cookies: " + cookies);

        if (cookies.indexOf("session=") >= 0)
        {
            int start = cookies.indexOf("session=") + 8;
            int end = cookies.indexOf(";", start);
            if (end == -1)
                end = cookies.length();
            session = cookies.substring(start, end);
        }
    }

    Serial.println("[INFO] Session token: " + session);
    if ((session == "") && (request->hasArg("token")))
    {
        Serial.println("[INFO] Found token in request: " + request->arg("token"));

        session = request->arg("token");
    }

    Serial.println("[INFO] Session token: " + session);
    if ((session != "") && (!User::isValidToken(session)))
    {
        Serial.println("[INFO] invalid token: " + session);
        return "";
    }
    Serial.println("[INFO] Session token: " + session);

    return session;
}

const char PAGE_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1, viewport-fit=cover'>
  <title>MeshNet Node</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body { width: 100%; height: 100%; }
    body { 
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: #f0f2f5;
      color: #333;
      line-height: 1.6;
      font-size: 16px;
    }
    .container {
      max-width: 1200px;
      margin: 0 auto;
      padding: 10px;
    }
    .header {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      padding: 20px 15px;
      border-radius: 12px;
      margin-bottom: 15px;
      box-shadow: 0 4px 15px rgba(0,0,0,0.15);
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .header h1 {
      font-size: 1.5em;
      font-weight: 700;
      margin: 0;
    }
    .logout-btn {
      background: rgba(255,255,255,0.2);
      color: white;
      padding: 8px 15px;
      text-decoration: none;
      border-radius: 6px;
      border: 1px solid rgba(255,255,255,0.3);
      cursor: pointer;
      font-size: 0.9em;
      transition: all 0.3s;
      min-height: 44px;
      display: flex;
      align-items: center;
    }
    .logout-btn:active {
      background: rgba(255,255,255,0.3);
    }
    .card {
      background: white;
      padding: 15px;
      border-radius: 10px;
      margin-bottom: 12px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.08);
    }
    .card h2 {
      font-size: 1.2em;
      margin-bottom: 12px;
      color: #667eea;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .user-info {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
      margin-bottom: 15px;
    }
    .user-stat {
      background: white;
      padding: 12px;
      border-radius: 8px;
      text-align: center;
      box-shadow: 0 2px 8px rgba(0,0,0,0.06);
    }
    .user-stat-label {
      font-size: 0.85em;
      color: #999;
      margin-bottom: 6px;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    .user-stat-value {
      font-size: 1.4em;
      font-weight: 700;
      color: #667eea;
      word-break: break-word;
    }
    .message-form {
      background: white;
      padding: 15px;
      border-radius: 10px;
      margin-bottom: 15px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.08);
    }
    .message-form h3 {
      font-size: 1.1em;
      margin-bottom: 12px;
      color: #333;
    }
    .form-group {
      display: flex;
      gap: 8px;
      flex-direction: column;
    }
    .form-group input[type=text] {
      flex: 1;
      padding: 12px;
      border: 1px solid #ddd;
      border-radius: 6px;
      font-size: 1em;
      font-family: inherit;
    }
    .form-group input[type=text]:focus {
      outline: none;
      border-color: #667eea;
      box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
    }
    .form-group input[type=submit] {
      padding: 12px 20px;
      background: #667eea;
      color: white;
      border: none;
      border-radius: 6px;
      cursor: pointer;
      font-size: 1em;
      font-weight: 600;
      min-height: 44px;
      transition: background 0.3s;
    }
    .form-group input[type=submit]:active {
      background: #764ba2;
    }
    .list-item {
      padding: 12px;
      border-bottom: 1px solid #f0f0f0;
      font-size: 0.95em;
    }
    .list-item:last-child {
      border-bottom: none;
    }
    .list-item strong {
      color: #333;
      display: block;
      margin-bottom: 4px;
    }
    .list-item small {
      color: #999;
      font-size: 0.85em;
    }
    .node-badge {
      display: inline-block;
      padding: 2px 8px;
      background: #e8f0fe;
      color: #667eea;
      border-radius: 12px;
      font-size: 0.8em;
      margin-right: 6px;
    }
    .link-list {
      list-style: none;
    }
    .link-list li {
      margin-bottom: 8px;
    }
    .link-list a {
      display: block;
      padding: 12px;
      background: #f8f9fa;
      color: #667eea;
      text-decoration: none;
      border-radius: 6px;
      border-left: 3px solid #667eea;
      transition: all 0.2s;
      font-weight: 500;
      min-height: 44px;
      display: flex;
      align-items: center;
    }
    .link-list a:active {
      background: #e8f0fe;
    }
    @media (min-width: 768px) {
      .user-info {
        grid-template-columns: 1fr 1fr 1fr;
      }
      .form-group {
        flex-direction: row;
      }
      .form-group input[type=text] {
        flex: 1;
      }
      .form-group input[type=submit] {
        flex: 0 0 auto;
        padding: 12px 40px;
      }
    }
    @media (max-width: 360px) {
      .header {
        flex-direction: column;
        gap: 10px;
        align-items: flex-start;
      }
      .header h1 {
        font-size: 1.3em;
      }
      .container {
        padding: 8px;
      }
      .card {
        padding: 12px;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>🎮 MeshNet</h1>
      <a href='/logout' class='logout-btn'>Logout</a>
    </div>

    <div class="user-info">
      <div class="user-stat">
        <div class="user-stat-label">Player</div>
        <div class="user-stat-value">%USERNAME%</div>
      </div>
      <div class="user-stat">
        <div class="user-stat-label">Team</div>
        <div class="user-stat-value">%TEAM%</div>
      </div>
      <div class="user-stat">
        <div class="user-stat-label">Players</div>
        <div class="user-stat-value">%USER_COUNT%</div>
      </div>
    </div>

    <div class="message-form">
      <h3>💬 Send Message</h3>
      <form action='/msg' method='POST'>
        <div class="form-group">
          <input type='text' name='msg' placeholder='Type your message...' required>
          <input type='hidden' name='token' value='%TOKEN%'>
          <input type='submit' value='Send'>
        </div>
      </form>
    </div>

    %ONLINE_NODES%
  </div>
</body>
</html>
)rawliteral";

String NodeWebServer::makePage(String session)
{
    String page(PAGE_INDEX);
    String username = User::getNameBySession(session);
    String team = User::getUserTeamBySession(session);
    String title = "Game on Webserver";
    
    // Links
    String links = "<div class='card'><h2>⚙️ Menu</h2><ul class='link-list'>";
    links += "<li><a href='/'>🏠 Home</a></li>";
    links += "<li><a href='/register.html'>📝 Register Player</a></li>";
    links += "<li><a href='/debug.html'>🔧 Debug Info</a></li>";
    links += "</ul></div>";
    
    // Online nodes list
    String nodeList = "<div class='card'><h2>🟢 Online Nodes</h2>";
    int onlineCount = LoraNode::getOnlineCount();
    const OnlineNode *onlineNodes = LoraNode::getOnlineNodes();
    Serial.println("[INFO] Online nodes:" + String(onlineCount));
    if (onlineCount == 0) {
        nodeList += "<p style='color: #999; padding: 12px;'>No other nodes online</p>";
    } else {
        nodeList += "<p style='padding: 0 12px; color: #666; font-size: 0.9em;'>Total: <strong>" + String(onlineCount) + " nodes</strong></p>";
        for (int i = 0; i < onlineCount; i++) {
            unsigned long now = millis();
            unsigned long lastSeen = onlineNodes[i].lastSeen;
            unsigned long secondsAgo = (now > lastSeen) ? (now - lastSeen) / 1000 : 0;
            nodeList += "<div class='list-item'>";
            nodeList += "<strong>" + escapeHtml(onlineNodes[i].name) + "</strong>";
            nodeList += "<small>📶 " + String(onlineNodes[i].rssi) + " dBm | Last: " + String(secondsAgo) + "s ago</small>";
            nodeList += "</div>";
        }
    }
    nodeList += "</div>";
    
    // Users list
    String userList = "<div class='card'><h2>👥 Players on Node</h2>";
    int userCount = User::getUserCount();
    if (userCount == 0) {
        userList += "<p style='color: #999; padding: 12px;'>No other players on this node</p>";
    } else {
        userList += "<p style='padding: 0 12px; color: #666; font-size: 0.9em;'>Total: <strong>" + String(userCount) + " players</strong></p>";
        for (int i = 0; i < userCount; i++) {
            userList += "<div class='list-item'>";
            userList += "<strong>" + escapeHtml(User::getUserName(i)) + "</strong>";
            userList += "<small><span class='node-badge'>Team</span>" + escapeHtml(User::getUserTeam(i)) + "</small>";
            userList += "</div>";
        }
    }
    userList += "</div>";
    
    // IP injection
    IPAddress ip = WiFi.softAPIP();
    String ipStr = ip.toString();
    page.replace("%AP_IP%", ipStr);

    page.replace("%USERNAME%", (username.length() > 0) ? username : "Guest");
    page.replace("%TEAM%", (team.length() > 0) ? team : "Solo");
    page.replace("%TITLE%", title);
    page.replace("%TOKEN%", session);
    page.replace("%USER_COUNT%", String(userCount));
    page.replace("%ONLINE_NODES%", links + nodeList + userList);
    
    return page;
}

// ====== Routes ======
void NodeWebServer::setupRoot()
{
    // Handler for both "/" and "/index.html"
    auto rootHandler = [](AsyncWebServerRequest *request)
    {
        Serial.println("[INFO] getSessionToken 2");    
        String session = getSessionToken(request);
        Serial.println("[INFO] Session token: " + session);
        if (session == "" ) {
            String welcomePage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1, viewport-fit=cover'>
  <title>MeshNet Login</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body { width: 100%; height: 100%; }
    body { 
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container {
      background: white;
      padding: 25px;
      border-radius: 16px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.25);
      text-align: center;
      max-width: 420px;
      width: 100%;
    }
    h1 {
      color: #667eea;
      margin-bottom: 8px;
      font-size: 2.2em;
      font-weight: 700;
    }
    .subtitle {
      color: #666;
      font-size: 1em;
      margin-bottom: 25px;
      font-weight: 500;
    }
    .description {
      margin-bottom: 28px;
      color: #888;
      font-size: 0.95em;
      line-height: 1.5;
    }
    .button-group {
      display: flex;
      flex-direction: column;
      gap: 10px;
      margin-bottom: 25px;
    }
    .button {
      display: block;
      padding: 14px 24px;
      font-size: 1em;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      text-decoration: none;
      transition: all 0.3s;
      font-weight: 600;
      min-height: 48px;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .button-login {
      background: #667eea;
      color: white;
    }
    .button-login:active {
      transform: scale(0.98);
      box-shadow: 0 3px 12px rgba(102, 126, 234, 0.4);
    }
    .button-register {
      background: #764ba2;
      color: white;
    }
    .button-register:active {
      transform: scale(0.98);
      box-shadow: 0 3px 12px rgba(118, 75, 162, 0.4);
    }
    .info {
      padding-top: 20px;
      border-top: 1px solid #f0f0f0;
      color: #999;
      font-size: 0.85em;
      line-height: 1.6;
    }
    .info-item {
      margin: 6px 0;
    }
    .info-label {
      color: #bbb;
      font-size: 0.8em;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    @media (max-width: 360px) {
      .container {
        padding: 20px;
        border-radius: 12px;
      }
      h1 {
        font-size: 1.8em;
      }
      .subtitle {
        font-size: 0.95em;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🎮 MeshNet</h1>
    <p class="subtitle">Gaming Network</p>
    <p class="description">Connect with friends and play together on the mesh network</p>
    <div class="button-group">
      <a href='/login.html' class='button button-login'>🔓 Login</a>
      <a href='/register.html' class='button button-register'>📝 Register</a>
    </div>
    <div class="info">
      <div class="info-item">
        <div class="info-label">Version</div>
        <strong>V0.8.1</strong>
      </div>
      <div class="info-item">
        <div class="info-label">Gateway</div>
        <strong>192.168.3.1</strong>
      </div>
    </div>
  </div>
</body>
</html>
  </div>
</body>
</html>
)rawliteral";
            request->send(200, "text/html; charset=UTF-8", welcomePage);
        } else {
            Serial.println("[INFO] Session token: " + session);
            String page = makePage(session);
            AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", page);
            response->addHeader("Set-Cookie", "session=" + session + "; Path=/; Max-Age=86400");
            request->send(response);
        }
    };
    
    httpServer.on("/", HTTP_GET, rootHandler);
    httpServer.on("/index.html", HTTP_GET, rootHandler);
}
void NodeWebServer::setupCaptivePortal()
{
    String page = "<html><head><meta http-equiv='refresh' content='10; url=http://192.168.3.1/index.html'></head><body>"
                  "<h3>Als je niet automatisch doorgestuurd wordt, klik <a href='http://192.168.3.1/index.html'>hier</a>.</h3>"
                  "<p>Voor volledige toegang: "
                  "<a href='http://192.168.3.1/' target='_blank'>Open in Safari</a></p>"
                  "</body></html>";

    // Captive portal common URLs
    httpServer.on("/generate_204.html", HTTP_GET, [page](AsyncWebServerRequest *request)
                  { request->send(204); });
    httpServer.on("/generate_204", HTTP_GET, [page](AsyncWebServerRequest *request)
                  { request->send(204); });
    httpServer.on("/fwlink.html", HTTP_GET, [page](AsyncWebServerRequest *request)
                  { request->send(204); });
    httpServer.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send(204); });
    httpServer.onNotFound([](AsyncWebServerRequest *request)
                          {
    // Redirect onbekende URLs naar home (Captive Portal gedrag)
    AsyncWebServerResponse *response = request->beginResponse(302);
    response->addHeader("Location", "/");
    request->send(response);
});
}

// Helper function to send login data to Docker API (non-blocking)
void sendLoginToApi(const String &user, const String &team)
{
    // Get node name/ID
    String nodeId = WiFi.macAddress();  // Use MAC address as unique node ID
    
    // Create JSON payload
    String payload = "{\"username\":\"" + user + "\",\"teamName\":\"" + team + "\"}";
    
    // Send to Docker API (non-blocking, using delay-free approach)
    WiFiClient client;
    const char* apiHost = "127.0.0.1";
    const int apiPort = 3001;
    
    // Try to connect without blocking
    if (client.connect(apiHost, apiPort)) {
        String path = "/api/node/" + nodeId + "/connection";
        
        client.print("POST " + path + " HTTP/1.1\r\n");
        client.print("Host: " + String(apiHost) + ":" + String(apiPort) + "\r\n");
        client.print("Content-Type: application/json\r\n");
        client.print("Content-Length: " + String(payload.length()) + "\r\n");
        client.print("Connection: close\r\n\r\n");
        client.print(payload);
        
        Serial.println("[API] Sent connection data to Docker backend for user: " + user);
        
        // Close connection immediately (don't wait for response)
        client.stop();
    } else {
        Serial.println("[API] Failed to connect to Docker backend");
    }
}

void NodeWebServer::setupLogin()
{
    httpServer.on("/login.html", HTTP_GET, [](AsyncWebServerRequest *request)
                  { 
                      String loginPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1, viewport-fit=cover'>
  <title>MeshNet Login</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body { width: 100%; height: 100%; }
    body { 
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container {
      background: white;
      padding: 25px;
      border-radius: 16px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.25);
      max-width: 420px;
      width: 100%;
    }
    h1 {
      color: #667eea;
      margin-bottom: 8px;
      font-size: 1.8em;
      font-weight: 700;
      text-align: center;
    }
    .subtitle {
      color: #999;
      font-size: 0.95em;
      text-align: center;
      margin-bottom: 28px;
    }
    .form-group {
      margin-bottom: 16px;
    }
    label {
      display: block;
      font-size: 0.9em;
      color: #333;
      margin-bottom: 6px;
      font-weight: 600;
    }
    input[type=text], input[type=password] {
      width: 100%;
      padding: 12px;
      border: 1px solid #ddd;
      border-radius: 8px;
      font-size: 1em;
      font-family: inherit;
      transition: border-color 0.3s;
    }
    input[type=text]:focus, input[type=password]:focus {
      outline: none;
      border-color: #667eea;
      box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
    }
    input[type=submit] {
      width: 100%;
      padding: 14px;
      background: #667eea;
      color: white;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      font-size: 1em;
      font-weight: 600;
      min-height: 48px;
      margin-top: 8px;
      transition: background 0.3s;
    }
    input[type=submit]:active {
      background: #764ba2;
      transform: scale(0.98);
    }
    .back-link {
      text-align: center;
      margin-top: 16px;
    }
    .back-link a {
      color: #667eea;
      text-decoration: none;
      font-size: 0.9em;
      display: inline-block;
      padding: 10px;
    }
    .back-link a:active {
      color: #764ba2;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🔓 Login</h1>
    <p class="subtitle">Enter your credentials</p>
    <form method='POST' action='/login'>
      <div class="form-group">
        <label for="user">Username</label>
        <input type='text' id='user' name='user' required autocomplete='username' autofocus>
      </div>
      <div class="form-group">
        <label for="pass">Password</label>
        <input type='password' id='pass' name='pass' required autocomplete='current-password'>
      </div>
      <input type='submit' value='Login'>
      <div class="back-link">
        <a href='/'>← Back</a>
      </div>
    </form>
  </div>
</body>
</html>
)rawliteral";
                      request->send(200, "text/html", loginPage);
                  });

    httpServer.on("/login", HTTP_POST, [](AsyncWebServerRequest *request)
                  {
        String user, pass;
        if (request->hasArg("user")) user = request->arg("user");
        if (request->hasArg("pass")) pass = request->arg("pass");

        if (User::isValidLogin(user, pass)) {
            String session = User::createSession(user);
            
            // Get user team
            String team = User::getUserTeamByName(user);
            
            // Send connection info to Docker API (non-blocking)
            Serial.println("[INFO] Login successful: " + user + " | Team: " + team);
            sendLoginToApi(user, team);
            
            AsyncWebServerResponse *response = request->beginResponse(303);
            response->addHeader("Set-Cookie", "session=" + session + "; Path=/; Max-Age=86400");
            response->addHeader("Location", "/");
            request->send(response);
        } else {
            String debugPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1, viewport-fit=cover'>
  <title>Login Failed</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container {
      background: white;
      padding: 25px;
      border-radius: 16px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.25);
      max-width: 420px;
      width: 100%;
    }
    h2 {
      color: #ff6b6b;
      margin-bottom: 16px;
      font-size: 1.3em;
    }
    .error-box {
      background: #fff5f5;
      border: 1px solid #ffc0cb;
      border-radius: 8px;
      padding: 12px;
      margin-bottom: 16px;
      color: #c92a2a;
      font-size: 0.9em;
    }
    .info-box {
      background: #f0f2f5;
      border-radius: 8px;
      padding: 16px;
      margin-bottom: 16px;
    }
    .info-box h4 {
      color: #333;
      margin-bottom: 10px;
      font-size: 0.95em;
    }
    .info-box ul {
      list-style: none;
      padding-left: 0;
    }
    .info-box li {
      padding: 6px;
      color: #666;
      font-size: 0.9em;
      background: white;
      border-radius: 4px;
      margin-bottom: 4px;
    }
    .button {
      display: block;
      padding: 12px;
      background: #667eea;
      color: white;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      text-decoration: none;
      font-size: 1em;
      font-weight: 600;
      text-align: center;
      min-height: 48px;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .button:active {
      background: #764ba2;
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>❌ Login Failed</h2>
    <div class="error-box">
      Invalid username or password. Please try again.
    </div>
    <div class="info-box">
      <h4>Available Users on Node:</h4>
      <ul>
)rawliteral";
            for (int i = 0; i < User::getUserCount(); i++) {
                debugPage += "<li>" + escapeHtml(User::getUserName(i)) + "</li>";
            }
            debugPage += R"rawliteral(
      </ul>
    </div>
    <a href='/login.html' class='button'>Try Again</a>
  </div>
</body>
</html>
)rawliteral";
            Serial.println("[LOGIN FAIL] User: " + user + " | Available users: " + String(User::getUserCount()));
            request->send(403, "text/html", debugPage);
        } });
}

void NodeWebServer::setupRegister()
{
    httpServer.on("/register.html", HTTP_GET, [](AsyncWebServerRequest *request)
                  { 
                      String registerPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1, viewport-fit=cover'>
  <title>MeshNet Register</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body { width: 100%; height: 100%; }
    body { 
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container {
      background: white;
      padding: 25px;
      border-radius: 16px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.25);
      max-width: 420px;
      width: 100%;
    }
    h1 {
      color: #667eea;
      margin-bottom: 8px;
      font-size: 1.8em;
      font-weight: 700;
      text-align: center;
    }
    .subtitle {
      color: #999;
      font-size: 0.95em;
      text-align: center;
      margin-bottom: 28px;
    }
    .form-group {
      margin-bottom: 16px;
    }
    label {
      display: block;
      font-size: 0.9em;
      color: #333;
      margin-bottom: 6px;
      font-weight: 600;
    }
    input[type=text], input[type=password] {
      width: 100%;
      padding: 12px;
      border: 1px solid #ddd;
      border-radius: 8px;
      font-size: 1em;
      font-family: inherit;
      transition: border-color 0.3s;
    }
    input[type=text]:focus, input[type=password]:focus {
      outline: none;
      border-color: #667eea;
      box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
    }
    input[type=submit] {
      width: 100%;
      padding: 14px;
      background: #667eea;
      color: white;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      font-size: 1em;
      font-weight: 600;
      min-height: 48px;
      margin-top: 8px;
      transition: background 0.3s;
    }
    input[type=submit]:active {
      background: #764ba2;
      transform: scale(0.98);
    }
    .back-link {
      text-align: center;
      margin-top: 16px;
    }
    .back-link a {
      color: #667eea;
      text-decoration: none;
      font-size: 0.9em;
      display: inline-block;
      padding: 10px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>📝 Register</h1>
    <p class="subtitle">Create new account</p>
    <form method='POST' action='/register'>
      <div class="form-group">
        <label for="user">Username</label>
        <input type='text' id='user' name='user' required autocomplete='username' autofocus>
      </div>
      <div class="form-group">
        <label for="pass">Password</label>
        <input type='password' id='pass' name='pass' required autocomplete='new-password'>
      </div>
      <div class="form-group">
        <label for="team">Team Name (optional)</label>
        <input type='text' id='team' name='team' autocomplete='off'>
      </div>
      <input type='submit' value='Register'>
      <div class="back-link">
        <a href='/'>← Back</a>
      </div>
    </form>
  </div>
</body>
</html>
)rawliteral";
                      request->send(200, "text/html", registerPage);
                  });

    httpServer.on("/register", HTTP_POST, [](AsyncWebServerRequest *request)
                  {
        String user, pass, team;
        if (request->hasArg("user")) user = request->arg("user");
        if (request->hasArg("pass")) pass = request->arg("pass");
        if (request->hasArg("team")) team = request->arg("team");

        if (User::registerUser(user, pass, team)) {
            String session = User::createSession(user);
            
            // Send registration info to Docker API
            sendLoginToApi(user, team);
            
            AsyncWebServerResponse *response = request->beginResponse(303);
            response->addHeader("Set-Cookie", "session=" + session + "; Path=/; Max-Age=86400");
            response->addHeader("Location", "/");
            request->send(response);
        } else {
            String errorPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1, viewport-fit=cover'>
  <title>Registration Failed</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container {
      background: white;
      padding: 25px;
      border-radius: 16px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.25);
      max-width: 420px;
      width: 100%;
    }
    h2 {
      color: #ff6b6b;
      margin-bottom: 16px;
      font-size: 1.3em;
    }
    .error-box {
      background: #fff5f5;
      border: 1px solid #ffc0cb;
      border-radius: 8px;
      padding: 12px;
      margin-bottom: 16px;
      color: #c92a2a;
    }
    .button {
      display: block;
      padding: 12px;
      background: #667eea;
      color: white;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      text-decoration: none;
      font-size: 1em;
      font-weight: 600;
      text-align: center;
      min-height: 48px;
      display: flex;
      align-items: center;
      justify-content: center;
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>❌ Registration Failed</h2>
    <div class="error-box">
      This username already exists. Please choose another name.
    </div>
    <a href='/register.html' class='button'>Try Another Username</a>
  </div>
</body>
</html>
)rawliteral";
            request->send(400, "text/html", errorPage);
        } 
    });
}

void NodeWebServer::setupLogout()
{
    httpServer.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
        AsyncWebServerResponse *response = request->beginResponse(303);
        response->addHeader("Set-Cookie", "session=; Max-Age=0");
        response->addHeader("Location", "/");
        request->send(response); });
}

void NodeWebServer::setupDebug()
{
    httpServer.on("/debug.html", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
        String page = "<h2>Debug info</h2>";
        page += "<p>Aantal berichten: " + String(MAX_MSGS) + "</p>";
        request->send(200, "text/html", page); });
}

void NodeWebServer::setupMessages()
{
    // GET: toon berichten
    httpServer.on("/msg", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
        String token;

        Serial.println("[INFO] getSessionToken 3");
    
        String session = getSessionToken(request);
        if (session == "" ) {
            request->send(403, "text/html", "<p>Ongeldig of ontbrekend token.</p>");
            return;
        }

        String content = "<h2>Berichten</h2>";
        content += "<form method='POST' action='/msg'>";
        content += "Bericht: <input type='text' name='msg'><br>";
        content += "<input type='submit' value='Stuur'></form><br>";

        NodeMessage msg;
        for (int i = 0; i < LoraNode::getMsgCount(); i++) {
            msg = LoraNode::getMessage(i);
            content += "<b>" + escapeHtml(msg.user) + ":</b> "
                       + escapeHtml(msg.object) + " "
                       + escapeHtml(msg.function) + " "
                       + escapeHtml(msg.parameters) +  "<br>";
        }

        request->send(200, "text/html", content); });

    // POST: voeg bericht toe
    httpServer.on("/msg", HTTP_POST, [](AsyncWebServerRequest *request)
                  {
        String msg, user;
        Serial.println("[INFO] getSessionToken 4");
    
        String session = getSessionToken(request);
        if (session == "") {
            request->send(403, "text/html", "<p>Ongeldig of ontbrekend token.</p>");
            return;
        }
        if (request->hasArg("msg")) msg = request->arg("msg");

        user = User::getNameBySession(session);

        NodeMessage nodeMessage;
        nodeMessage.msgId = String(millis());
        nodeMessage.user = user;
        nodeMessage.TTL = 3;
        nodeMessage.timestamp = millis();
        nodeMessage.object = "MSG";
        nodeMessage.function = "SEND";
        nodeMessage.parameters = msg;

        LoraNode::addMessage(nodeMessage);

        AsyncWebServerResponse *response = request->beginResponse(303);
        response->addHeader("Location", "/msg");
        request->send(response); });
}

// ====== Init ======
void NodeWebServer::webserverSetup()
{
    Serial.println("[DEBUG] Starting webserver setup...");
    
    WiFi.mode(WIFI_AP);
    Serial.println("[DEBUG] WiFi mode set to AP");
    
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    Serial.println("[DEBUG] Soft AP config set");
    
    // Set AP SSID dynamically - prefix + MAC + version
    // Format: MeshNode-AABBCCDDEEFF_V1.0.1
    String mac = WiFi.softAPmacAddress();
    mac.replace(":", "");
    mac.toUpperCase();
    String baseSsid = String(WIFI_AP_SSID_PREFIX) + mac;
    AP_SSID = baseSsid + "_V" + String(MESHNET_VERSION);
    
    WiFi.softAP(AP_SSID.c_str(), AP_PASS.c_str());
    Serial.println("[DEBUG] Soft AP started: " + AP_SSID);

    setupRoot();
    Serial.println("[DEBUG] setupRoot done");
    
    setupCaptivePortal();
    Serial.println("[DEBUG] setupCaptivePortal done");
    
    setupLogin();
    Serial.println("[DEBUG] setupLogin done");
    
    setupRegister();
    Serial.println("[DEBUG] setupRegister done");
    
    setupLogout();
    Serial.println("[DEBUG] setupLogout done");
    
    setupDebug();
    Serial.println("[DEBUG] setupDebug done");
    
    setupMessages();
    Serial.println("[DEBUG] setupMessages done");

    httpServer.begin();
    Serial.println("[DEBUG] httpServer.begin() called");
    
    dnsServer.start(DNS_PORT, "*", apIP);
    Serial.println("[DEBUG] dnsServer.start() called");

    Serial.println("[INFO] Async Webserver gestart");
}

void NodeWebServer::webserverLoop()
{
    dnsServer.processNextRequest();
}
