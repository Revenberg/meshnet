#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "User.h"
#include "NodeWebServer.h"
#include "LoraNode.h"

// ====== Config ======
#define DNS_PORT 53
IPAddress apIP(192, 168, 3, 1);
String AP_SSID = "MeshNet V0.9.4";
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
bool NodeWebServer::usersSynced = false;
bool NodeWebServer::pagesSynced = false;
String NodeWebServer::teamNames[MAX_TEAM_PAGES];
String NodeWebServer::teamPages[MAX_TEAM_PAGES];

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

void NodeWebServer::setUsersSynced(bool synced) { usersSynced = synced; }
void NodeWebServer::setPagesSynced(bool synced) { pagesSynced = synced; }
bool NodeWebServer::isUsersSynced() { return usersSynced; }
bool NodeWebServer::isPagesSynced() { return pagesSynced; }

void NodeWebServer::storeTeamPage(const String &team, const String &html)
{
  for (int i = 0; i < MAX_TEAM_PAGES; i++)
  {
    if (teamNames[i] == team || teamNames[i].length() == 0)
    {
      teamNames[i] = team;
      teamPages[i] = html;
      return;
    }
  }
}

String NodeWebServer::getTeamPage(const String &team)
{
  for (int i = 0; i < MAX_TEAM_PAGES; i++)
  {
    if (teamNames[i] == team)
    {
      return teamPages[i];
    }
  }
  return "";
}

bool NodeWebServer::hasTeamPage(const String &team)
{
  for (int i = 0; i < MAX_TEAM_PAGES; i++)
  {
    if (teamNames[i] == team && teamPages[i].length() > 0)
    {
      return true;
    }
  }
  return false;
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
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: Arial, sans-serif;
      background: #f5f5f5;
      padding: 20px;
    }
    .header {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      padding: 30px;
      border-radius: 12px;
      margin-bottom: 30px;
      box-shadow: 0 4px 15px rgba(0,0,0,0.1);
    }
    .sync-status {
      background: #fff3cd;
      color: #856404;
      padding: 12px 16px;
      border: 1px solid #ffeeba;
      border-radius: 8px;
      margin-bottom: 20px;
      font-size: 0.95em;
    }
    .sync-status.ok {
      background: #d4edda;
      color: #155724;
      border-color: #c3e6cb;
    }
    .header h1 {
      margin-bottom: 10px;
      font-size: 2em;
    }
    .user-info {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 20px;
      margin-bottom: 20px;
    }
    .user-card {
      background: white;
      padding: 20px;
      border-radius: 8px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.1);
    }
    .user-card .label {
      color: #999;
      font-size: 0.9em;
      margin-bottom: 5px;
    }
    .user-card .value {
      font-size: 1.5em;
      font-weight: bold;
      color: #667eea;
    }
    .box { 
      margin-top: 20px; 
      padding: 20px; 
      border: 1px solid #ddd;
      border-radius: 8px;
      background: white;
    }
    .box h3 {
      margin-bottom: 15px;
      color: #333;
    }
    .box ul {
      list-style: none;
      padding-left: 0;
    }
    .box li {
      padding: 8px;
      border-bottom: 1px solid #f0f0f0;
    }
    .box li:last-child {
      border-bottom: none;
    }
    input[type=text], input[type=password] { 
      width: 100%; 
      padding: 8px; 
      margin-bottom: 10px;
      border: 1px solid #ddd;
      border-radius: 4px;
    }
    input[type=submit] { 
      padding: 10px 20px;
      background: #667eea;
      color: white;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 1em;
    }
    input[type=submit]:hover {
      background: #764ba2;
    }
    .message-form {
      background: white;
      padding: 20px;
      border-radius: 8px;
      margin-bottom: 20px;
    }
    .message-form form {
      display: flex;
      gap: 10px;
    }
    .message-form input[type=text] {
      flex: 1;
      margin-bottom: 0;
    }
    .logout-btn {
      background: #ff6b6b;
      color: white;
      padding: 10px 20px;
      text-decoration: none;
      border-radius: 4px;
      display: inline-block;
      margin-top: 10px;
    }
    .logout-btn:hover {
      background: #ff5252;
    }
    @media (max-width: 768px) {
      .user-info {
        grid-template-columns: 1fr;
      }
      .message-form form {
        flex-direction: column;
      }
      .header {
        padding: 20px;
      }
      .header h1 {
        font-size: 1.5em;
      }
    }
  </style>
</head>
<body>
  <div class="header">
    <h1>🎮 GhostNet Dashboard</h1>
    <a href='/logout' class='logout-btn'>Logout</a>
  </div>

  %SYNC_STATUS%

  <div class="user-info">
    <div class="user-card">
      <div class="label">Player Name</div>
      <div class="value">%USERNAME%</div>
    </div>
    <div class="user-card">
      <div class="label">Team / Group</div>
      <div class="value">%TEAM%</div>
    </div>
  </div>

  <div class="message-form">
    <h3>Send Message</h3>
    <form action='/msg' method='POST'>
      <input type='text' name='msg' placeholder='Type your message...'>
      <input type='hidden' name='token' value='%TOKEN%'>
      <input type='submit' value='Send'>
    </form>
  </div>

  %TEAM_PAGE%

  %ONLINE_NODES%

</body>
</html>
)rawliteral";

String NodeWebServer::makePage(String session)
{
    String page(PAGE_INDEX);
    String username = User::getNameBySession(session);
    String team = User::getUserTeamBySession(session);
    String title = "Game on Webserver";
  String syncStatus;
  bool usersOk = NodeWebServer::isUsersSynced();
  bool pagesOk = NodeWebServer::isPagesSynced();
  if (usersOk && pagesOk)
  {
    syncStatus = "<div class='sync-status ok'>✅ Users en pagina's gesynchroniseerd</div>";
  }
  else
  {
    syncStatus = "<div class='sync-status'>";
    syncStatus += "⚠️ Sync status: ";
    if (!usersOk) syncStatus += "users ontbreken ";
    if (!pagesOk) syncStatus += "pagina's ontbreken ";
    syncStatus += "</div>";
  }
  String teamPageSection;
  if (team.length() > 0 && NodeWebServer::hasTeamPage(team))
  {
    teamPageSection = "<div class='box'><h3>📄 Team Pagina</h3>" + NodeWebServer::getTeamPage(team) + "</div>";
  }
  else if (team.length() > 0)
  {
    teamPageSection = "<div class='box'><h3>📄 Team Pagina</h3><p>Geen pagina gevonden voor team: " + escapeHtml(team) + "</p></div>";
  }
  else
  {
    teamPageSection = "<div class='box'><h3>📄 Team Pagina</h3><p>Geen team gekoppeld aan gebruiker.</p></div>";
  }
    
    // Links
    String links = "<div class='box'><h3>⚙️ Links</h3><ul>";
    links += "<li><a href='/'>Home</a></li>";
    links += "<li><a href='/register.html'>Register New Player</a></li>";
    links += "<li><a href='/debug.html'>Debug Info</a></li>";
    links += "</ul></div>";
    
    // Online nodes list
    String nodeList = "<div class='box'><h3>🟢 Online Nodes</h3>";
    int onlineCount = LoraNode::getOnlineCount();
    const OnlineNode *onlineNodes = LoraNode::getOnlineNodes();
    Serial.println("[INFO] Online nodes:" + String(onlineCount));
    nodeList += "<p><strong>Total: " + String(onlineCount) + " nodes</strong></p><ul>";
    for (int i = 0; i < onlineCount; i++)
    {
        unsigned long now = millis();
        unsigned long lastSeen = onlineNodes[i].lastSeen;
        unsigned long secondsAgo = (now > lastSeen) ? (now - lastSeen) / 1000 : 0;
        nodeList += "<li><strong>" + escapeHtml(onlineNodes[i].name) + "</strong> | RSSI: " + String(onlineNodes[i].rssi) + " dBm | Last seen: " + String(secondsAgo) + "s ago</li>";
    }
    nodeList += "</ul></div>";
    
    // Users list
    String userList = "<div class='box'><h3>👥 Players on Node</h3>";
    int userCount = User::getUserCount();
    userList += "<p><strong>Total: " + String(userCount) + " players</strong></p><ul>";
    for (int i = 0; i < userCount; i++)
    {
        userList += "<li><strong>" + escapeHtml(User::getUserName(i)) + "</strong> | Team: " + escapeHtml(User::getUserTeam(i)) + "</li>";
    }
    userList += "</ul></div>";
    
    // IP injection
    IPAddress ip = WiFi.softAPIP();
    String ipStr = ip.toString();
    page.replace("%AP_IP%", ipStr);

    page.replace("%USERNAME%", (username.length() > 0) ? username : "Guest");
    page.replace("%TEAM%", (team.length() > 0) ? team : "No Team");
    page.replace("%TITLE%", title);
    page.replace("%TOKEN%", session);
    page.replace("%USER_COUNT%", String(userCount));
  page.replace("%SYNC_STATUS%", syncStatus);
  page.replace("%TEAM_PAGE%", teamPageSection);
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
          String syncStatus;
          bool usersOk = NodeWebServer::isUsersSynced();
          bool pagesOk = NodeWebServer::isPagesSynced();
          if (usersOk && pagesOk)
          {
            syncStatus = "<div style='background:#d4edda;color:#155724;border:1px solid #c3e6cb;padding:10px;border-radius:8px;margin-bottom:20px;'>✅ Users en pagina's gesynchroniseerd</div>";
          }
          else
          {
            syncStatus = "<div style='background:#fff3cd;color:#856404;border:1px solid #ffeeba;padding:10px;border-radius:8px;margin-bottom:20px;'>";
            syncStatus += "⚠️ Sync status: ";
            if (!usersOk) syncStatus += "users ontbreken ";
            if (!pagesOk) syncStatus += "pagina's ontbreken ";
            syncStatus += "</div>";
          }
            String welcomePage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: Arial, sans-serif; 
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .container {
      background: white;
      padding: 40px;
      border-radius: 12px;
      box-shadow: 0 10px 25px rgba(0,0,0,0.2);
      text-align: center;
      max-width: 500px;
      width: 90%;
    }
    h1 {
      color: #667eea;
      margin-bottom: 10px;
      font-size: 2.5em;
    }
    .subtitle {
      color: #666;
      font-size: 1.1em;
      margin-bottom: 30px;
    }
    .button {
      display: inline-block;
      margin: 10px;
      padding: 12px 30px;
      font-size: 1.1em;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      text-decoration: none;
      transition: transform 0.2s, box-shadow 0.2s;
    }
    .button-login {
      background: #667eea;
      color: white;
    }
    .button-login:hover {
      transform: translateY(-2px);
      box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
    }
    .button-register {
      background: #764ba2;
      color: white;
    }
    .button-register:hover {
      transform: translateY(-2px);
      box-shadow: 0 5px 15px rgba(118, 75, 162, 0.4);
    }
    .info {
      margin-top: 30px;
      padding-top: 20px;
      border-top: 1px solid #ddd;
      color: #999;
      font-size: 0.9em;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🎮 GhostNet</h1>
    <p class="subtitle">Welcome to the Gaming Network</p>
    <p style="margin-bottom: 30px; color: #555;">Please login or register to continue</p>
    %SYNC_STATUS%
    <a href='/login.html' class='button button-login'>Login</a>
    <a href='/register.html' class='button button-register'>Register</a>
    <div class="info">
      <p>WiFi Network: ghostnet V0.7.4</p>
      <p>Gateway: 192.168.3.1</p>
    </div>
  </div>
</body>
</html>
)rawliteral";
      welcomePage.replace("%SYNC_STATUS%", syncStatus);
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
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: Arial, sans-serif; 
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container {
      background: white;
      padding: 50px 30px;
      border-radius: 16px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.2);
      text-align: center;
      width: 100%;
      max-width: 420px;
    }
    h1 {
      color: #667eea;
      font-size: 3em;
      margin-bottom: 10px;
    }
    .subtitle {
      color: #999;
      font-size: 1.1em;
      margin-bottom: 40px;
    }
    .form-group {
      margin-bottom: 20px;
      text-align: left;
    }
    label {
      display: block;
      font-weight: bold;
      color: #333;
      margin-bottom: 8px;
      font-size: 1.1em;
    }
    input[type=text], input[type=password] {
      width: 100%;
      padding: 16px;
      font-size: 1.1em;
      border: 2px solid #ddd;
      border-radius: 8px;
      transition: border 0.3s;
    }
    input[type=text]:focus, input[type=password]:focus {
      outline: none;
      border: 2px solid #667eea;
    }
    input[type=submit] {
      width: 100%;
      padding: 16px;
      font-size: 1.2em;
      font-weight: bold;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      margin-top: 20px;
      transition: transform 0.2s, box-shadow 0.2s;
    }
    input[type=submit]:active {
      transform: scale(0.98);
    }
    input[type=submit]:hover {
      box-shadow: 0 5px 20px rgba(102, 126, 234, 0.4);
    }
    .back-link {
      margin-top: 20px;
      text-align: center;
    }
    .back-link a {
      color: #667eea;
      text-decoration: none;
      font-size: 1em;
    }
    .back-link a:hover {
      text-decoration: underline;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🎮</h1>
    <p class="subtitle">Login to MeshNet</p>
    <form method='POST' action='/login'>
      <div class="form-group">
        <label for="user">Username</label>
        <input type='text' name='user' id='user' placeholder='test or sander' required autofocus>
      </div>
      <div class="form-group">
        <label for="pass">Password</label>
        <input type='password' name='pass' id='pass' placeholder='Enter password' required>
      </div>
      <input type='submit' value='Login'>
    </form>
    <div class="back-link">
      <a href='/'>← Back</a>
    </div>
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

        Serial.println("\n\n========================================");
        Serial.println("     LOGIN ATTEMPT");
        Serial.println("========================================");
        Serial.printf("[LOGIN] Raw inputs from form:\n");
        Serial.printf("  - username field: '%s' (length=%d)\n", user.c_str(), user.length());
        Serial.printf("  - password field: '%s' (length=%d)\n", pass.c_str(), pass.length());
        
        // Hash the password before comparing with stored hash
        String passHash = User::hashPassword(pass);
        Serial.printf("[LOGIN] Password hash calculated: '%s'\n", passHash.c_str());
        
        if (User::isValidLogin(user, passHash)) {
            String session = User::createSession(user);
            
            // Get user team
            String team = User::getUserTeamByName(user);
            
            // Send connection info to Docker API (non-blocking)
            Serial.println("\n[LOGIN] ✓ SUCCESS!");
            Serial.printf("  - Authenticated as: '%s'\n", user.c_str());
            Serial.printf("  - Team: '%s'\n", team.c_str());
            Serial.printf("  - Session token: '%s'\n", session.c_str());
            Serial.println("========================================\n");
            
            sendLoginToApi(user, team);
            
            AsyncWebServerResponse *response = request->beginResponse(303);
            response->addHeader("Set-Cookie", "session=" + session + "; Path=/; Max-Age=86400");
            response->addHeader("Location", "/");
            request->send(response);
        } else {
            Serial.println("\n[LOGIN] ✗ FAILED!");
            Serial.printf("  - User not found or password mismatch\n");
            Serial.println("========================================\n");
            
            String debugMsg = "<h3>❌ Login Failed - DEBUG INFO</h3>";
            debugMsg += "<p><strong>Attempted User:</strong> " + user + "</p>";
            debugMsg += "<p><strong>Password Entered:</strong> " + pass + "</p>";
            debugMsg += "<p><strong>Password Hash:</strong> " + passHash + "</p>";
            debugMsg += "<h4>Users on Node (" + String(User::getUserCount()) + "):</h4><ul>";
            for (int i = 0; i < User::getUserCount(); i++) {
                debugMsg += "<li>" + User::getUserName(i) + "</li>";
            }
            debugMsg += "</ul>";
            debugMsg += "<p><a href='/login.html'>Back to Login</a></p>";
            Serial.println("[LOGIN FAIL] User: " + user + " | Available users: " + String(User::getUserCount()));
            request->send(403, "text/html", debugMsg);
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
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: Arial, sans-serif; 
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container {
      background: white;
      padding: 50px 30px;
      border-radius: 16px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.2);
      text-align: center;
      width: 100%;
      max-width: 420px;
    }
    h1 {
      color: #667eea;
      font-size: 3em;
      margin-bottom: 10px;
    }
    .subtitle {
      color: #999;
      font-size: 1.1em;
      margin-bottom: 40px;
    }
    .form-group {
      margin-bottom: 20px;
      text-align: left;
    }
    label {
      display: block;
      font-weight: bold;
      color: #333;
      margin-bottom: 8px;
      font-size: 1.1em;
    }
    input[type=text], input[type=password] {
      width: 100%;
      padding: 16px;
      font-size: 1.1em;
      border: 2px solid #ddd;
      border-radius: 8px;
      transition: border 0.3s;
    }
    input[type=text]:focus, input[type=password]:focus {
      outline: none;
      border: 2px solid #667eea;
    }
    input[type=submit] {
      width: 100%;
      padding: 16px;
      font-size: 1.2em;
      font-weight: bold;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      margin-top: 20px;
      transition: transform 0.2s, box-shadow 0.2s;
    }
    input[type=submit]:active {
      transform: scale(0.98);
    }
    input[type=submit]:hover {
      box-shadow: 0 5px 20px rgba(102, 126, 234, 0.4);
    }
    .back-link {
      margin-top: 20px;
      text-align: center;
    }
    .back-link a {
      color: #667eea;
      text-decoration: none;
      font-size: 1em;
    }
    .back-link a:hover {
      text-decoration: underline;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🎮</h1>
    <p class="subtitle">Create Account</p>
    <form method='POST' action='/register'>
      <div class="form-group">
        <label for="user">Username</label>
        <input type='text' name='user' id='user' placeholder='Choose username' required autofocus>
      </div>
      <div class="form-group">
        <label for="pass">Password</label>
        <input type='password' name='pass' id='pass' placeholder='Enter password' required>
      </div>
      <div class="form-group">
        <label for="team">Team</label>
        <input type='text' name='team' id='team' placeholder='Your team name' required>
      </div>
      <input type='submit' value='Register'>
    </form>
    <div class="back-link">
      <a href='/'>← Back</a>
    </div>
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
            AsyncWebServerResponse *response = request->beginResponse(303);
            response->addHeader("Set-Cookie", "session=" + session + "; Path=/; Max-Age=86400");
            response->addHeader("Location", "/");
            request->send(response);
        } else {
            request->send(400, "text/html", "<p>Gebruiker bestaat al</p>");
        } });
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
