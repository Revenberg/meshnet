#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ctype.h>
#include <stdlib.h>
#include <Preferences.h>
#include "User.h"
#include "NodeWebServer.h"
#include "LoraNode.h"
#include "version.h"

// ====== Config ======
#define DNS_PORT 53
IPAddress apIP(192, 168, 3, 1);
String AP_SSID = String(FIRMWARE_NAME) + "-" + FIRMWARE_VERSION;
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
String NodeWebServer::teamPageUpdatedAt[MAX_TEAM_PAGES];
static Preferences pagesPrefs;
static unsigned long lastSyncRequestMs = 0;

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

String toLowerCopy(const String &input)
{
  String out = input;
  out.toLowerCase();
  return out;
}

String urlDecodeSegment(const String &input)
{
  String out;
  out.reserve(input.length());
  for (size_t i = 0; i < input.length(); i++)
  {
    char c = input.charAt(i);
    if (c == '+')
    {
      out += ' ';
    }
    else if (c == '%' && i + 2 < input.length())
    {
      char hex[3] = { (char)input.charAt(i + 1), (char)input.charAt(i + 2), 0 };
      char decoded = (char)strtol(hex, nullptr, 16);
      out += decoded;
      i += 2;
    }
    else
    {
      out += c;
    }
  }
  return out;
}

String slugifyTeam(const String &team)
{
  String out;
  out.reserve(team.length());
  for (size_t i = 0; i < team.length(); i++)
  {
    char c = team.charAt(i);
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
    {
      out += (char)tolower(c);
    }
    else if (c == ' ' || c == '-' || c == '_')
    {
      if (out.length() == 0 || out.charAt(out.length() - 1) == '-')
      {
        continue;
      }
      out += '-';
    }
  }
  return out;
}

static String normalizeTeamName(const String &team)
{
  String out = team;
  out.trim();
  out.toLowerCase();
  return out;
}

String NodeWebServer::findTeamNameBySlug(const String &slug)
{
  String normalized = toLowerCopy(urlDecodeSegment(slug));
  normalized.replace("_", "-");
  for (int i = 0; i < MAX_TEAM_PAGES; i++)
  {
    if (teamNames[i].length() == 0)
    {
      continue;
    }
    String teamName = teamNames[i];
    if (toLowerCopy(teamName) == normalized)
    {
      return teamName;
    }
    if (slugifyTeam(teamName) == normalized)
    {
      return teamName;
    }
  }
  return "";
}

void NodeWebServer::setUsersSynced(bool synced) { usersSynced = synced; }
void NodeWebServer::setPagesSynced(bool synced) { pagesSynced = synced; }
bool NodeWebServer::isUsersSynced() { return usersSynced; }
bool NodeWebServer::isPagesSynced()
{
  if (pagesSynced)
  {
    return true;
  }
  for (int i = 0; i < MAX_TEAM_PAGES; i++)
  {
    if (teamNames[i].length() > 0 && teamPages[i].length() > 0)
    {
      return true;
    }
  }
  return false;
}

void NodeWebServer::savePagesNVS()
{
  pagesPrefs.begin("NodePages", false);
  pagesPrefs.clear();

  int stored = 0;
  for (int i = 0; i < MAX_TEAM_PAGES; i++)
  {
    if (teamNames[i].length() == 0 || teamPages[i].length() == 0)
    {
      continue;
    }
    pagesPrefs.putString(("page" + String(stored) + "_team").c_str(), teamNames[i]);
    pagesPrefs.putString(("page" + String(stored) + "_html").c_str(), teamPages[i]);
    pagesPrefs.putString(("page" + String(stored) + "_updated").c_str(), teamPageUpdatedAt[i]);
    stored++;
  }

  pagesPrefs.putInt("pageCount", stored);
  pagesPrefs.end();
  Serial.printf("[TEAM-PAGE] Saved %d pages to NVS\n", stored);
}

void NodeWebServer::loadPagesNVS()
{
  for (int i = 0; i < MAX_TEAM_PAGES; i++)
  {
    teamNames[i] = "";
    teamPages[i] = "";
    teamPageUpdatedAt[i] = "";
  }

  pagesPrefs.begin("NodePages", true);
  int count = pagesPrefs.getInt("pageCount", 0);
  int stored = 0;
  for (int i = 0; i < count && stored < MAX_TEAM_PAGES; i++)
  {
    String team = pagesPrefs.getString(("page" + String(i) + "_team").c_str(), "");
    String html = pagesPrefs.getString(("page" + String(i) + "_html").c_str(), "");
    String updated = pagesPrefs.getString(("page" + String(i) + "_updated").c_str(), "");
    if (team.length() == 0 || html.length() == 0)
    {
      continue;
    }
    teamNames[stored] = team;
    teamPages[stored] = html;
    teamPageUpdatedAt[stored] = updated;
    stored++;
  }
  pagesPrefs.end();

  pagesSynced = stored > 0;
  Serial.printf("[TEAM-PAGE] Loaded %d pages from NVS\n", stored);
}

void NodeWebServer::clearPages(bool clearNvs)
{
  for (int i = 0; i < MAX_TEAM_PAGES; i++)
  {
    teamNames[i] = "";
    teamPages[i] = "";
    teamPageUpdatedAt[i] = "";
  }
  pagesSynced = false;

  if (clearNvs)
  {
    pagesPrefs.begin("NodePages", false);
    pagesPrefs.clear();
    pagesPrefs.end();
    Serial.println("[TEAM-PAGE] Cleared pages from NVS");
  }
}

void NodeWebServer::storeTeamPage(const String &team, const String &html, const String &updatedAt)
{
  String trimmedTeam = team;
  trimmedTeam.trim();
  String normalized = normalizeTeamName(trimmedTeam);
  for (int i = 0; i < MAX_TEAM_PAGES; i++)
  {
    String existingNorm = normalizeTeamName(teamNames[i]);
    if (existingNorm == normalized || teamNames[i].length() == 0)
    {
      teamNames[i] = trimmedTeam;
      teamPages[i] = html;
      teamPageUpdatedAt[i] = updatedAt;
      Serial.printf("[TEAM-PAGE] Stored team page: %s (len=%d) slot=%d updated=%s\n", team.c_str(), html.length(), i, updatedAt.c_str());
      savePagesNVS();
      return;
    }
  }
}

String NodeWebServer::getTeamPage(const String &team)
{
  String normalized = normalizeTeamName(team);
  for (int i = 0; i < MAX_TEAM_PAGES; i++)
  {
    if (normalizeTeamName(teamNames[i]) == normalized)
    {
      return teamPages[i];
    }
  }
  String resolved = NodeWebServer::findTeamNameBySlug(team);
  if (resolved.length() > 0)
  {
    for (int i = 0; i < MAX_TEAM_PAGES; i++)
    {
      if (teamNames[i] == resolved)
      {
        return teamPages[i];
      }
    }
  }
  return "";
}

String NodeWebServer::getTeamPageUpdatedAt(const String &team)
{
  String normalized = normalizeTeamName(team);
  for (int i = 0; i < MAX_TEAM_PAGES; i++)
  {
    if (normalizeTeamName(teamNames[i]) == normalized)
    {
      return teamPageUpdatedAt[i];
    }
  }
  String resolved = NodeWebServer::findTeamNameBySlug(team);
  if (resolved.length() > 0)
  {
    for (int i = 0; i < MAX_TEAM_PAGES; i++)
    {
      if (teamNames[i] == resolved)
      {
        return teamPageUpdatedAt[i];
      }
    }
  }
  return "";
}

bool NodeWebServer::hasTeamPage(const String &team)
{
  String normalized = normalizeTeamName(team);
  for (int i = 0; i < MAX_TEAM_PAGES; i++)
  {
    if (normalizeTeamName(teamNames[i]) == normalized && teamPages[i].length() > 0)
    {
      return true;
    }
  }
  String resolved = NodeWebServer::findTeamNameBySlug(team);
  if (resolved.length() > 0)
  {
    for (int i = 0; i < MAX_TEAM_PAGES; i++)
    {
      if (teamNames[i] == resolved && teamPages[i].length() > 0)
      {
        return true;
      }
    }
  }
  return false;
}

int NodeWebServer::getStoredPagesCount()
{
  int count = 0;
  for (int i = 0; i < MAX_TEAM_PAGES; i++)
  {
    if (teamNames[i].length() > 0 && teamPages[i].length() > 0)
    {
      count++;
    }
  }
  return count;
}

String NodeWebServer::getTeamNameAt(int index)
{
  if (index < 0 || index >= MAX_TEAM_PAGES)
  {
    return "";
  }
  return teamNames[index];
}

String NodeWebServer::getTeamUpdatedAtAt(int index)
{
  if (index < 0 || index >= MAX_TEAM_PAGES)
  {
    return "";
  }
  return teamPageUpdatedAt[index];
}

int NodeWebServer::getTeamPageLengthAt(int index)
{
  if (index < 0 || index >= MAX_TEAM_PAGES)
  {
    return 0;
  }
  return teamPages[index].length();
}

int NodeWebServer::getMaxTeamPages()
{
  return MAX_TEAM_PAGES;
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
    <h1>🎮 MeshNet Dashboard</h1>
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

  <script>
    (function () {
      const AP_HOST = "192.168.3.1";
      const AP_URL = "http://192.168.3.1/";
      const TOKEN_KEY = "meshnetSession";
      const initialToken = "%TOKEN%";

      function getCookie(name) {
        const match = document.cookie.match(new RegExp('(?:^|; )' + name + '=([^;]*)'));
        return match ? decodeURIComponent(match[1]) : "";
      }

      function storeToken(token) {
        if (token && token.length > 0) {
          localStorage.setItem(TOKEN_KEY, token);
        }
      }

      function getStoredToken() {
        return localStorage.getItem(TOKEN_KEY) || "";
      }

      function clearToken() {
        localStorage.removeItem(TOKEN_KEY);
      }

      function redirectToAp(token) {
        const target = token ? (AP_URL + "?token=" + encodeURIComponent(token)) : AP_URL;
        if (window.location.href !== target) {
          window.location.href = target;
        }
      }

      function ensureSession() {
        const url = new URL(window.location.href);
        if (url.searchParams.get('logout') === '1') {
          clearToken();
          url.searchParams.delete('logout');
          history.replaceState({}, '', url.toString());
        }

        if (initialToken && initialToken.length > 0) {
          storeToken(initialToken);
        }

        const cookieToken = getCookie('session');
        if (cookieToken) {
          storeToken(cookieToken);
        } else {
          const cached = getStoredToken();
          if (cached && !url.searchParams.get('token')) {
            redirectToAp(cached);
          }
        }
      }

      function attemptReconnect() {
        if (window.location.hostname !== AP_HOST) {
          const cached = getStoredToken();
          redirectToAp(cached);
        }
      }

      window.addEventListener('online', attemptReconnect);
      document.addEventListener('visibilitychange', function () {
        if (!document.hidden) {
          attemptReconnect();
        }
      });
      setInterval(attemptReconnect, 15000);
      ensureSession();
    })();
  </script>

</body>
</html>
)rawliteral";

String NodeWebServer::makePage(String session)
{
    String page(PAGE_INDEX);
    String username = User::getNameBySession(session);
    String team = User::getUserTeamBySession(session);
    String title = String(FIRMWARE_NAME) + " V" + FIRMWARE_VERSION;
  String syncStatus;
  bool usersOk = NodeWebServer::isUsersSynced();
  bool pagesOk = NodeWebServer::isPagesSynced();
  const unsigned long nowMs = millis();
  if ((!usersOk || !pagesOk) && (nowMs - lastSyncRequestMs > 10000)) {
    if (!usersOk) {
      LoraNode::requestUsers();
    }
    if (!pagesOk) {
      LoraNode::requestPages();
    }
    lastSyncRequestMs = nowMs;
  }
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
  bool hasTeamPage = (team.length() > 0 && NodeWebServer::hasTeamPage(team));
  String teamSlug = (team.length() > 0) ? slugifyTeam(team) : "";
  if (team.length() > 0 && hasTeamPage)
  {
    String updatedAt = NodeWebServer::getTeamPageUpdatedAt(team);
    String updatedHtml = updatedAt.length() > 0 ? ("<p><small>Laatst bijgewerkt: " + escapeHtml(updatedAt) + "</small></p>") : "";
    teamPageSection = "<div class='box' id='team-page'><h3>📄 Team Pagina</h3>" + updatedHtml + NodeWebServer::getTeamPage(team) + "</div>";
  }
  else if (team.length() > 0)
  {
    teamPageSection = "<div class='box' id='team-page'><h3>📄 Team Pagina</h3><p>Geen pagina gevonden voor team: " + escapeHtml(team) + "</p></div>";
  }
  else
  {
    teamPageSection = "<div class='box' id='team-page'><h3>📄 Team Pagina</h3><p>Geen team gekoppeld aan gebruiker.</p></div>";
  }

  Serial.printf("[TEAM-PAGE] user=%s team=%s slug=%s hasPage=%s\n",
                username.c_str(),
                team.c_str(),
                teamSlug.c_str(),
                hasTeamPage ? "yes" : "no");
    
    // Links
    String links = "<div class='box'><h3>⚙️ Links</h3><ul>";
    links += "<li><a href='/'>Home</a></li>";
    if (teamSlug.length() > 0)
    {
      links += "<li><a href='" + String("/") + teamSlug + "'>Team Pagina</a></li>";
    }
    links += "<li><a href='/debug.html'>Debug Info</a></li>";
    links += "</ul></div>";

    // All team pages overview
    String allPagesList = "<div class='box'><h3>📚 Alle Team Pagina's</h3><ul>";
    int allPagesCount = 0;
    for (int i = 0; i < MAX_TEAM_PAGES; i++)
    {
      if (teamNames[i].length() == 0 || teamPages[i].length() == 0)
      {
        continue;
      }
      String pageTeam = teamNames[i];
      String pageSlug = slugifyTeam(pageTeam);
      if (pageSlug.length() == 0)
      {
        continue;
      }
      String updatedAt = teamPageUpdatedAt[i];
      allPagesList += "<li><a href='" + String("/") + pageSlug + "'>" + escapeHtml(pageTeam) + "</a>";
      if (updatedAt.length() > 0)
      {
        allPagesList += " <small>(Laatst bijgewerkt: " + escapeHtml(updatedAt) + ")</small>";
      }
      allPagesList += "</li>";
      allPagesCount++;
    }
    if (allPagesCount == 0)
    {
      allPagesList += "<li>Geen pagina's beschikbaar.</li>";
    }
    allPagesList += "</ul></div>";
    
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
    page.replace("%ONLINE_NODES%", links + allPagesList + nodeList + userList);
    
    return page;
}

  String buildTeamPageHtml(const String &teamName, const String &teamSlug, const String &username, bool hasPage, bool usersOk, bool pagesOk, const String &teamHtml)
  {
    String page;
    page += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    page += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    page += "<style>body{font-family:Arial,sans-serif;background:#f5f5f5;padding:20px;}";
    page += ".card{background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,0.1);}";
    page += ".meta{color:#666;font-size:0.9em;margin-bottom:10px;}";
    page += ".link{display:inline-block;margin-bottom:15px;}";
    page += "pre{background:#f0f0f0;padding:10px;border-radius:6px;overflow:auto;}";
    page += "</style></head><body>";
    page += "<a class='link' href='/'>← Back</a>";
    page += "<div class='card'>";
    page += "<h2>📄 Team Pagina</h2>";
    page += "<div class='meta'>Team: <strong>" + escapeHtml(teamName) + "</strong></div>";
    if (hasPage)
    {
      String updatedAt = NodeWebServer::getTeamPageUpdatedAt(teamName);
      if (updatedAt.length() > 0)
      {
        page += "<div class='meta'>Laatst bijgewerkt: <strong>" + escapeHtml(updatedAt) + "</strong></div>";
      }
      page += teamHtml;
    }
    else
    {
      page += "<p>Geen pagina gevonden voor team: " + escapeHtml(teamName) + "</p>";
    }
    page += "<h3>Debug</h3>";
    page += "<pre>";
    page += "user=" + escapeHtml(username) + "\n";
    page += "teamSlug=" + escapeHtml(teamSlug) + "\n";
    page += "hasPage=" + String(hasPage ? "true" : "false") + "\n";
    page += "usersSynced=" + String(usersOk ? "true" : "false") + "\n";
    page += "pagesSynced=" + String(pagesOk ? "true" : "false") + "\n";
    page += "pageLength=" + String(teamHtml.length()) + "\n";
    page += "</pre>";
    page += "</div>";
    page += "<script>";
    page += "(function(){";
    page += "const AP_HOST='192.168.3.1';";
    page += "const AP_URL='http://192.168.3.1/';";
    page += "const TOKEN_KEY='meshnetSession';";
    page += "function getCookie(name){const m=document.cookie.match(new RegExp('(?:^|; )'+name+'=([^;]*)'));return m?decodeURIComponent(m[1]):'';}";
    page += "function storeToken(t){if(t&&t.length>0){localStorage.setItem(TOKEN_KEY,t);}}";
    page += "function getStoredToken(){return localStorage.getItem(TOKEN_KEY)||'';}";
    page += "function clearToken(){localStorage.removeItem(TOKEN_KEY);}";
    page += "function redirectToAp(t){const target=t?AP_URL+'?token='+encodeURIComponent(t):AP_URL;if(window.location.href!==target){window.location.href=target;}}";
    page += "function ensureSession(){const url=new URL(window.location.href);if(url.searchParams.get('logout')==='1'){clearToken();url.searchParams.delete('logout');history.replaceState({},'',url.toString());}const cookieToken=getCookie('session');if(cookieToken){storeToken(cookieToken);}else{const cached=getStoredToken();if(cached&&!url.searchParams.get('token')){redirectToAp(cached);}}}";
    page += "function attemptReconnect(){if(window.location.hostname!==AP_HOST){const cached=getStoredToken();redirectToAp(cached);}}";
    page += "window.addEventListener('online',attemptReconnect);";
    page += "document.addEventListener('visibilitychange',function(){if(!document.hidden){attemptReconnect();}});";
    page += "setInterval(attemptReconnect,15000);";
    page += "ensureSession();";
    page += "})();";
    page += "</script>";
    page += "</body></html>";
    return page;
  }

static String buildLoginPageHtml(const String &syncStatus, const String &loginDisabled, const String &inputDisabled)
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
      margin-bottom: 30px;
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
    %SYNC_STATUS%
    <form method='POST' action='/login'>
      <div class="form-group">
        <label for="user">Username</label>
        <input type='text' name='user' id='user' placeholder='test or sander' required autofocus %INPUT_DISABLED%>
      </div>
      <div class="form-group">
        <label for="pass">Password</label>
        <input type='password' name='pass' id='pass' placeholder='Enter password' required %INPUT_DISABLED%>
      </div>
      <input type='submit' value='Login'%LOGIN_DISABLED%>
    </form>
    <div class="back-link">
      <a href='/'>← Back</a>
    </div>
  </div>

  <script>
    (function () {
      const AP_HOST = "192.168.3.1";
      const AP_URL = "http://192.168.3.1/";
      const TOKEN_KEY = "meshnetSession";

      function getCookie(name) {
        const match = document.cookie.match(new RegExp('(?:^|; )' + name + '=([^;]*)'));
        return match ? decodeURIComponent(match[1]) : "";
      }

      function storeToken(token) {
        if (token && token.length > 0) {
          localStorage.setItem(TOKEN_KEY, token);
        }
      }

      function getStoredToken() {
        return localStorage.getItem(TOKEN_KEY) || "";
      }

      function clearToken() {
        localStorage.removeItem(TOKEN_KEY);
      }

      function redirectToAp(token) {
        const target = token ? (AP_URL + "?token=" + encodeURIComponent(token)) : AP_URL;
        if (window.location.href !== target) {
          window.location.href = target;
        }
      }

      function ensureSession() {
        const url = new URL(window.location.href);
        if (url.searchParams.get('logout') === '1') {
          clearToken();
          url.searchParams.delete('logout');
          history.replaceState({}, '', url.toString());
        }

        const cookieToken = getCookie('session');
        if (cookieToken) {
          storeToken(cookieToken);
        } else {
          const cached = getStoredToken();
          if (cached && !url.searchParams.get('token')) {
            redirectToAp(cached);
          }
        }
      }

      function attemptReconnect() {
        if (window.location.hostname !== AP_HOST) {
          const cached = getStoredToken();
          redirectToAp(cached);
        }
      }

      window.addEventListener('online', attemptReconnect);
      document.addEventListener('visibilitychange', function () {
        if (!document.hidden) {
          attemptReconnect();
        }
      });
      setInterval(attemptReconnect, 15000);
      ensureSession();
    })();
  </script>
</body>
</html>
)rawliteral";

  loginPage.replace("%SYNC_STATUS%", syncStatus);
  loginPage.replace("%LOGIN_DISABLED%", loginDisabled);
  loginPage.replace("%INPUT_DISABLED%", inputDisabled);
  return loginPage;
}

static String buildHomeTeamPageHtml(const String &session)
{
  String username = User::getNameBySession(session);
  String team = User::getUserTeamBySession(session);
  String teamHtml = (team.length() > 0) ? NodeWebServer::getTeamPage(team) : "";
  bool hasPage = (team.length() > 0 && NodeWebServer::hasTeamPage(team));
  String updatedAt = (team.length() > 0) ? NodeWebServer::getTeamPageUpdatedAt(team) : "";

  String syncStatus;
  bool usersOk = NodeWebServer::isUsersSynced();
  bool pagesOk = NodeWebServer::isPagesSynced();
  if (usersOk && pagesOk)
  {
    syncStatus = "<div class='sync ok'>✅ Users en pagina's gesynchroniseerd</div>";
  }
  else
  {
    syncStatus = "<div class='sync warn'>⚠️ Sync status: ";
    if (!usersOk) syncStatus += "users ontbreken ";
    if (!pagesOk) syncStatus += "pagina's ontbreken ";
    syncStatus += "</div>";
  }

  String page;
  page += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  page += "<style>";
  page += "body{font-family:Arial,sans-serif;background:#f5f5f5;margin:0;padding:20px;}";
  page += ".header{display:flex;justify-content:space-between;align-items:center;background:#fff;padding:16px 20px;border-radius:10px;box-shadow:0 2px 8px rgba(0,0,0,0.1);}";
  page += ".title{margin:0;font-size:1.6em;color:#333;}";
  page += ".meta{color:#777;font-size:0.9em;}";
  page += ".actions a{margin-left:10px;text-decoration:none;padding:8px 12px;border-radius:6px;background:#667eea;color:#fff;font-size:0.9em;}";
  page += ".actions a.secondary{background:#999;}";
  page += ".card{background:#fff;margin-top:16px;padding:20px;border-radius:10px;box-shadow:0 2px 8px rgba(0,0,0,0.08);}";
  page += ".sync{margin-top:14px;padding:10px 12px;border-radius:8px;font-size:0.9em;}";
  page += ".sync.ok{background:#d4edda;color:#155724;border:1px solid #c3e6cb;}";
  page += ".sync.warn{background:#fff3cd;color:#856404;border:1px solid #ffeeba;}";
  page += "</style></head><body>";
  page += "<div class='header'>";
  page += "<div><h1 class='title'>📄 Team Pagina</h1>";
  page += "<div class='meta'>" + escapeHtml(username) + " · " + escapeHtml(team.length() > 0 ? team : "Geen team") + "</div></div>";
  page += "<div class='actions'><a class='secondary' href='/admin'>Admin</a><a href='/logout'>Logout</a></div>";
  page += "</div>";
  page += syncStatus;

  page += "<div class='card'>";
  if (hasPage)
  {
    if (updatedAt.length() > 0)
    {
      page += "<p class='meta'>Laatst bijgewerkt: <strong>" + escapeHtml(updatedAt) + "</strong></p>";
    }
    page += teamHtml;
  }
  else if (team.length() > 0)
  {
    page += "<p>Geen pagina gevonden voor team: " + escapeHtml(team) + "</p>";
  }
  else
  {
    page += "<p>Geen team gekoppeld aan gebruiker.</p>";
  }
  page += "</div>";

  page += "<script>(function(){";
  page += "const AP_HOST='192.168.3.1';const AP_URL='http://192.168.3.1/';const TOKEN_KEY='meshnetSession';";
  page += "function getCookie(name){const m=document.cookie.match(new RegExp('(?:^|; )'+name+'=([^;]*)'));return m?decodeURIComponent(m[1]):'';}";
  page += "function storeToken(t){if(t&&t.length>0){localStorage.setItem(TOKEN_KEY,t);}}";
  page += "function getStoredToken(){return localStorage.getItem(TOKEN_KEY)||'';}";
  page += "function clearToken(){localStorage.removeItem(TOKEN_KEY);}";
  page += "function redirectToAp(t){const target=t?AP_URL+'?token='+encodeURIComponent(t):AP_URL;if(window.location.href!==target){window.location.href=target;}}";
  page += "function ensureSession(){const url=new URL(window.location.href);if(url.searchParams.get('logout')==='1'){clearToken();url.searchParams.delete('logout');history.replaceState({},'',url.toString());}const cookieToken=getCookie('session');if(cookieToken){storeToken(cookieToken);}else{const cached=getStoredToken();if(cached&&!url.searchParams.get('token')){redirectToAp(cached);}}}";
  page += "function attemptReconnect(){if(window.location.hostname!==AP_HOST){const cached=getStoredToken();redirectToAp(cached);}}";
  page += "window.addEventListener('online',attemptReconnect);";
  page += "document.addEventListener('visibilitychange',function(){if(!document.hidden){attemptReconnect();}});";
  page += "setInterval(attemptReconnect,15000);";
  page += "ensureSession();";
  page += "})();</script>";
  page += "</body></html>";
  return page;
}

static String buildAdminPagesHtml(const String &session)
{
  String username = User::getNameBySession(session);
  String team = User::getUserTeamBySession(session);
  String page;
  page += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  page += "<style>";
  page += "body{font-family:Arial,sans-serif;background:#f5f5f5;margin:0;padding:20px;}";
  page += ".header{display:flex;justify-content:space-between;align-items:center;background:#fff;padding:16px 20px;border-radius:10px;box-shadow:0 2px 8px rgba(0,0,0,0.1);}";
  page += ".title{margin:0;font-size:1.6em;color:#333;}";
  page += ".meta{color:#777;font-size:0.9em;}";
  page += ".actions a{margin-left:10px;text-decoration:none;padding:8px 12px;border-radius:6px;background:#667eea;color:#fff;font-size:0.9em;}";
  page += ".actions a.secondary{background:#999;}";
  page += ".card{background:#fff;margin-top:16px;padding:20px;border-radius:10px;box-shadow:0 2px 8px rgba(0,0,0,0.08);}";
  page += ".pages-list{list-style:none;padding-left:0;margin:0;}";
  page += ".pages-list li{padding:10px 0;border-bottom:1px solid #eee;}";
  page += ".pages-list li:last-child{border-bottom:none;}";
  page += ".pages-list a{text-decoration:none;color:#667eea;}";
  page += "</style></head><body>";
  page += "<div class='header'>";
  page += "<div><h1 class='title'>📚 Huidige Pagina's</h1>";
  page += "<div class='meta'>" + escapeHtml(username) + " · " + escapeHtml(team.length() > 0 ? team : "Geen team") + "</div></div>";
  page += "<div class='actions'><a class='secondary' href='/'>Home</a><a href='/logout'>Logout</a></div>";
  page += "</div>";

  page += "<div class='card'>";
  page += "<h3>Alle team pagina's</h3>";
  page += "<ul class='pages-list'>";
  int allPagesCount = 0;
  int totalTeams = NodeWebServer::getMaxTeamPages();
  for (int i = 0; i < totalTeams; i++)
  {
    String pageTeam = NodeWebServer::getTeamNameAt(i);
    int pageLength = NodeWebServer::getTeamPageLengthAt(i);
    if (pageTeam.length() == 0 || pageLength == 0)
    {
      continue;
    }
    String pageSlug = slugifyTeam(pageTeam);
    String updatedAt = NodeWebServer::getTeamUpdatedAtAt(i);
    page += "<li><strong>" + escapeHtml(pageTeam) + "</strong>";
    if (pageSlug.length() > 0)
    {
      page += " · <a href='" + String("/") + pageSlug + "'>Open</a>";
    }
    if (updatedAt.length() > 0)
    {
      page += "<div class='meta'>Laatst bijgewerkt: " + escapeHtml(updatedAt) + "</div>";
    }
    page += "<div class='meta'>Lengte: " + String(pageLength) + "</div>";
    page += "</li>";
    allPagesCount++;
  }
  if (allPagesCount == 0)
  {
    page += "<li>Geen pagina's beschikbaar.</li>";
  }
  page += "</ul></div>";

  page += "<script>(function(){";
  page += "const AP_HOST='192.168.3.1';const AP_URL='http://192.168.3.1/';const TOKEN_KEY='meshnetSession';";
  page += "function getCookie(name){const m=document.cookie.match(new RegExp('(?:^|; )'+name+'=([^;]*)'));return m?decodeURIComponent(m[1]):'';}";
  page += "function storeToken(t){if(t&&t.length>0){localStorage.setItem(TOKEN_KEY,t);}}";
  page += "function getStoredToken(){return localStorage.getItem(TOKEN_KEY)||'';}";
  page += "function clearToken(){localStorage.removeItem(TOKEN_KEY);}";
  page += "function redirectToAp(t){const target=t?AP_URL+'?token='+encodeURIComponent(t):AP_URL;if(window.location.href!==target){window.location.href=target;}}";
  page += "function ensureSession(){const url=new URL(window.location.href);if(url.searchParams.get('logout')==='1'){clearToken();url.searchParams.delete('logout');history.replaceState({},'',url.toString());}const cookieToken=getCookie('session');if(cookieToken){storeToken(cookieToken);}else{const cached=getStoredToken();if(cached&&!url.searchParams.get('token')){redirectToAp(cached);}}}";
  page += "function attemptReconnect(){if(window.location.hostname!==AP_HOST){const cached=getStoredToken();redirectToAp(cached);}}";
  page += "window.addEventListener('online',attemptReconnect);";
  page += "document.addEventListener('visibilitychange',function(){if(!document.hidden){attemptReconnect();}});";
  page += "setInterval(attemptReconnect,15000);";
  page += "ensureSession();";
  page += "})();</script>";
  page += "</body></html>";
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
          bool hasUsers = (User::getUserCount() > 0);
          const unsigned long nowMs = millis();
          if ((!usersOk || !pagesOk) && (nowMs - lastSyncRequestMs > 120000)) {
            if (!usersOk && !LoraNode::isUsersSyncInProgress()) {
              LoraNode::requestUsers();
            } else if (!pagesOk) {
              LoraNode::requestPages();
            }
            lastSyncRequestMs = nowMs;
          }
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
            syncStatus += "<br>⏳ Wacht 2 minuten en probeer opnieuw.</div>";
          }
          String loginDisabled = (usersOk && hasUsers) ? "" : " disabled";
          String inputDisabled = (usersOk && hasUsers) ? "" : " disabled";
          String loginPage = buildLoginPageHtml(syncStatus, loginDisabled, inputDisabled);
          AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", loginPage);
          response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
          response->addHeader("Pragma", "no-cache");
          response->addHeader("Expires", "0");
          request->send(response);
        } else {
            Serial.println("[INFO] Session token: " + session);
            String page = buildHomeTeamPageHtml(session);
            AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", page);
            response->addHeader("Set-Cookie", "session=" + session + "; Path=/; Max-Age=86400");
            response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
            response->addHeader("Pragma", "no-cache");
            response->addHeader("Expires", "0");
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
    String path = request->url();
    String slug = path.startsWith("/") ? path.substring(1) : path;
    bool looksLikeSlug = slug.length() > 0 && slug.indexOf('/') == -1 && slug.indexOf('.') == -1;

    if (looksLikeSlug)
    {
      String session = getSessionToken(request);
      if (session.length() > 0)
      {
        String username = User::getNameBySession(session);
        String userTeam = User::getUserTeamBySession(session);
        String userTeamSlug = slugifyTeam(userTeam);
        String resolvedTeam = "";

        if (slug == userTeamSlug)
        {
          resolvedTeam = userTeam;
        }
        else
        {
                resolvedTeam = NodeWebServer::findTeamNameBySlug(slug);
        }

        bool hasPage = resolvedTeam.length() > 0 && NodeWebServer::hasTeamPage(resolvedTeam);
        Serial.printf("[TEAM-PAGE] path=%s slug=%s user=%s team=%s resolved=%s hasPage=%s\n",
                path.c_str(),
                slug.c_str(),
                username.c_str(),
                userTeam.c_str(),
                resolvedTeam.c_str(),
                hasPage ? "yes" : "no");

        if (resolvedTeam.length() > 0)
        {
          String teamHtml = NodeWebServer::getTeamPage(resolvedTeam);
          String page = buildTeamPageHtml(
            resolvedTeam,
            slug,
            username,
            hasPage,
            NodeWebServer::isUsersSynced(),
            NodeWebServer::isPagesSynced(),
            teamHtml
          );
          request->send(200, "text/html; charset=UTF-8", page);
          return;
        }
      }
    }

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
        bool usersOk = NodeWebServer::isUsersSynced();
        bool pagesOk = NodeWebServer::isPagesSynced();
        bool hasUsers = (User::getUserCount() > 0);
        const unsigned long nowMs = millis();
        if ((!usersOk || !pagesOk) && (nowMs - lastSyncRequestMs > 120000)) {
          if (!usersOk && !LoraNode::isUsersSyncInProgress()) {
            LoraNode::requestUsers();
          } else if (!pagesOk) {
            LoraNode::requestPages();
          }
          lastSyncRequestMs = nowMs;
        }
        String syncStatus;
        if (usersOk && pagesOk)
        {
          syncStatus = "<div style='background:#d4edda;color:#155724;border:1px solid #c3e6cb;padding:10px;border-radius:8px;margin-bottom:20px;border-radius:8px;'>✅ Users en pagina's gesynchroniseerd</div>";
        }
        else
        {
          syncStatus = "<div style='background:#fff3cd;color:#856404;border:1px solid #ffeeba;padding:10px;border-radius:8px;margin-bottom:20px;'>";
          syncStatus += "⚠️ Sync status: ";
          if (!usersOk) syncStatus += "users ontbreken ";
          if (!pagesOk) syncStatus += "pagina's ontbreken ";
          syncStatus += "<br>⏳ Wacht 2 minuten en probeer opnieuw.</div>";
        }
        String loginDisabled = (usersOk && hasUsers) ? "" : " disabled";
        String inputDisabled = (usersOk && hasUsers) ? "" : " disabled";
        String loginPage = buildLoginPageHtml(syncStatus, loginDisabled, inputDisabled);
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", loginPage);
  response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "0");
  request->send(response);
                  });

    httpServer.on("/login", HTTP_POST, [](AsyncWebServerRequest *request)
                  {
        bool usersOk = NodeWebServer::isUsersSynced();
        bool pagesOk = NodeWebServer::isPagesSynced();
        bool hasUsers = (User::getUserCount() > 0);
        const unsigned long nowMs = millis();
        if ((!usersOk || !pagesOk) && (nowMs - lastSyncRequestMs > 120000)) {
          if (!usersOk && !LoraNode::isUsersSyncInProgress()) {
            LoraNode::requestUsers();
          } else if (!pagesOk) {
            LoraNode::requestPages();
          }
          lastSyncRequestMs = nowMs;
        }
        if (!usersOk || !hasUsers)
        {
            String msg = "<h3>⏳ Sync bezig</h3>";
          msg += "<p>Users zijn nog niet geladen. Probeer het opnieuw zodra de sync klaar is.</p>";
            msg += "<p><a href='/'>Terug</a></p>";
            request->send(503, "text/html", msg);
            return;
        }
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

  <script>
    (function () {
      const AP_HOST = "192.168.3.1";
      const AP_URL = "http://192.168.3.1/";
      const TOKEN_KEY = "meshnetSession";

      function getCookie(name) {
        const match = document.cookie.match(new RegExp('(?:^|; )' + name + '=([^;]*)'));
        return match ? decodeURIComponent(match[1]) : "";
      }

      function storeToken(token) {
        if (token && token.length > 0) {
          localStorage.setItem(TOKEN_KEY, token);
        }
      }

      function getStoredToken() {
        return localStorage.getItem(TOKEN_KEY) || "";
      }

      function clearToken() {
        localStorage.removeItem(TOKEN_KEY);
      }

      function redirectToAp(token) {
        const target = token ? (AP_URL + "?token=" + encodeURIComponent(token)) : AP_URL;
        if (window.location.href !== target) {
          window.location.href = target;
        }
      }

      function ensureSession() {
        const url = new URL(window.location.href);
        if (url.searchParams.get('logout') === '1') {
          clearToken();
          url.searchParams.delete('logout');
          history.replaceState({}, '', url.toString());
        }

        const cookieToken = getCookie('session');
        if (cookieToken) {
          storeToken(cookieToken);
        } else {
          const cached = getStoredToken();
          if (cached && !url.searchParams.get('token')) {
            redirectToAp(cached);
          }
        }
      }

      function attemptReconnect() {
        if (window.location.hostname !== AP_HOST) {
          const cached = getStoredToken();
          redirectToAp(cached);
        }
      }

      window.addEventListener('online', attemptReconnect);
      document.addEventListener('visibilitychange', function () {
        if (!document.hidden) {
          attemptReconnect();
        }
      });
      setInterval(attemptReconnect, 15000);
      ensureSession();
    })();
  </script>
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
  response->addHeader("Location", "/?logout=1");
        request->send(response); });
}

void NodeWebServer::setupAdmin()
{
    httpServer.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
        String session = getSessionToken(request);
        if (session == "")
        {
          AsyncWebServerResponse *response = request->beginResponse(302);
          response->addHeader("Location", "/");
          request->send(response);
          return;
        }

        String page = buildAdminPagesHtml(session);
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", page);
        response->addHeader("Set-Cookie", "session=" + session + "; Path=/; Max-Age=86400");
        response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        request->send(response); });
}

void NodeWebServer::setupDebug()
{
    httpServer.on("/debug.html", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
        String page = "<h2>Debug info</h2>";
        page += "<p>Aantal berichten: " + String(MAX_MSGS) + "</p>";
        request->send(200, "text/html", page); });

  httpServer.on("/sync/refresh", HTTP_GET, [](AsyncWebServerRequest *request)
          {
    String session = getSessionToken(request);
    if (session == "")
    {
      request->send(403, "text/plain", "Ongeldig of ontbrekend token.");
      return;
    }

    bool hard = request->hasArg("hard") && request->arg("hard") == "1";
    if (hard)
    {
      Serial.println("[SYNC] Hard refresh requested - clearing users/pages");
      User::clearUsers();
      User::saveUsersNVS();
      clearPages(true);
      LoraNode::setUsersSynced(false);
      NodeWebServer::setUsersSynced(false);
      LoraNode::setPagesSynced(false);
      NodeWebServer::setPagesSynced(false);
    }
    else
    {
      Serial.println("[SYNC] Refresh requested");
      LoraNode::setPagesSynced(false);
      NodeWebServer::setPagesSynced(false);
    }

    LoraNode::requestUsers();


    request->send(200, "text/plain", hard ? "Hard refresh gestart" : "Refresh gestart"); });
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

  loadPagesNVS();
  Serial.println("[DEBUG] loadPagesNVS done");
    
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
    
    setupLogout();
    Serial.println("[DEBUG] setupLogout done");

    setupAdmin();
    Serial.println("[DEBUG] setupAdmin done");
    
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
