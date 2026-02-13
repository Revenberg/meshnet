#include "User.h"
#include "LoraNode.h"
#include "Preferences.h"
#include <HTTPClient.h>
#include <mbedtls/sha256.h>

NodeUser User::users[MAX_USERS];
int User::userCount = 0;
Preferences User::prefs;
bool User::runtimeCacheOnly = false;

void User::saveUsersNVS()
{
    if (User::runtimeCacheOnly)
    {
        Serial.println("[User] Runtime cache only - skipping NVS save");
        return;
    }
    prefs.begin("User::users", false);
    prefs.clear();
    for (int i = 0; i < User::userCount; i++)
    {
        Serial.printf("[User] Saving user: %s\n", User::users[i].username.c_str());
        User::prefs.putString(("user" + String(i) + "_name").c_str(), User::users[i].username);
        User::prefs.putString(("user" + String(i) + "_token").c_str(), User::users[i].token);
        User::prefs.putString(("user" + String(i) + "_hash").c_str(), User::users[i].passwordHash);
        User::prefs.putString(("user" + String(i) + "_team").c_str(), User::users[i].team);
    }
    User::prefs.putInt("userCount", User::userCount);
    User::prefs.end();
}

void User::loadUsersNVS()
{
    Serial.println("\n=== [USER] Loading users from NVS ===");
    User::prefs.begin("User::users", true);
    User::userCount = User::prefs.getInt("userCount", 0);
    Serial.printf("[USER] Found %d stored users\n", User::userCount);
    
    for (int i = 0; i < User::userCount; i++)
    {
        User::users[i].username = User::prefs.getString(("user" + String(i) + "_name").c_str(), "");
        User::users[i].token = User::prefs.getString(("user" + String(i) + "_token").c_str(), "");
        User::users[i].passwordHash = User::prefs.getString(("user" + String(i) + "_hash").c_str(), "");
        User::users[i].team = User::prefs.getString(("user" + String(i) + "_team").c_str(), "");

        Serial.printf("\n[USER] User[%d] loaded:\n", i);
        Serial.printf("  - Username: '%s'\n", User::users[i].username.c_str());
        Serial.printf("  - Team: '%s'\n", User::users[i].team.c_str());
        Serial.printf("  - Token: '%s' (length=%d)\n", User::users[i].token.c_str(), User::users[i].token.length());
        Serial.printf("  - PasswordHash: '%s' (length=%d)\n", User::users[i].passwordHash.c_str(), User::users[i].passwordHash.length());
    }
    User::prefs.end();
    Serial.printf("[USER] Total users loaded: %d\n", User::userCount);
    Serial.println("=== [USER] NVS load complete ===\n");
}

String User::generateToken()
{
    uint64_t chipid = ESP.getEfuseMac();
    String token = String(chipid, HEX) + "_" + String(millis()) + "_" + String(random(100000, 999999));
    return token;
}

bool User::registerUserWithToken(const String &name, const String &pwdHash, const String &team, String token)
{
    Serial.printf("[User] Adding new user: %s\n", name.c_str());
    Serial.printf("[User] New token: %s\n", token.c_str());
    Serial.printf("[User] New password hash: %s\n", pwdHash.c_str());
    Serial.printf("[User] userCount: %d\n", User::userCount);

    User::users[User::userCount].username = name;
    User::users[User::userCount].token = token;
    User::users[User::userCount].passwordHash = pwdHash;
    User::users[User::userCount].team = team;
    User::userCount++;
    User::saveUsersNVS();

    NodeMessage nodeMessage;
    nodeMessage.msgId = String(millis());
    nodeMessage.user = name;
    nodeMessage.TTL = 3;
    nodeMessage.timestamp = millis();
    nodeMessage.object = "USER";
    nodeMessage.function = "ADD";
    nodeMessage.parameters = "name:" + name + ",pwdHash:" + pwdHash + ",token:" + token + ",team:" + team;

    LoraNode::loraSend(nodeMessage);
    return true;
}
bool User::registerUser(const String &name, const String &pwdHash, const String &team)
{
    Serial.println("\n=== [USER] registerUser() called ===");
    Serial.printf("[USER] Name: '%s'\n", name.c_str());
    Serial.printf("[USER] PwdHash: '%s' (length=%d)\n", pwdHash.c_str(), pwdHash.length());
    Serial.printf("[USER] Team: '%s'\n", team.c_str());
    Serial.printf("[USER] Current user count: %d\n", User::userCount);
    
    // Check if user already exists
    for (int i = 0; i < User::userCount; i++)
    {
        if (User::users[i].username == name)
        {
            Serial.printf("[USER] ✗ User '%s' already exists!\n", name.c_str());
            return true;  // User already exists, don't re-register
        }
    }

    String token = User::generateToken();
    if (User::userCount < MAX_USERS)
    {
        Serial.printf("[USER] ✓ Registering new user...\n");
        return User::registerUserWithToken(name, pwdHash, team, token);
    }
    
    Serial.printf("[USER] ✗ Max users exceeded! Cannot register.\n");
    return false;
}

bool User::addOrUpdateUser(const String &name, const String &pwdHash, String &token, const String &team)
{
    if (token == "")
    {
        return false;
    }

    for (int i = 0; i < User::userCount; i++)
    {
        if (User::users[i].token == token)
        {
            User::users[i].team = team;
            if (pwdHash != "")
            {
                User::users[i].passwordHash = User::hashPassword(pwdHash);
            }
            Serial.printf("[User] Updated user: %s\n", name.c_str());

            NodeMessage nodeMessage;
            nodeMessage.msgId = String(millis());
            nodeMessage.user = name;
            nodeMessage.TTL = 3;
            nodeMessage.timestamp = millis();
            nodeMessage.object = "USER";
            nodeMessage.function = "ADD";
            nodeMessage.parameters = "name:" + name + ", pwdHash:" + pwdHash + ", token:" + token + ", team:" + team;
            
            LoraNode::loraSend(nodeMessage);
            User::saveUsersNVS();
            return true;
        }
    }

    return false;
}

int User::getUserCount()
{
    return User::userCount;
}

String User::getUserName(int index)
{
    if (index >= 0 && index < User::userCount)
    {
        return users[index].username;
    }
    return "Unknown";
}

String User::getUserTeam(int index)
{
    if (index >= 0 && index < User::userCount)
    {
        return users[index].team;
    }
    return "Unknown";
}

String User::getUserTeamByName(const String &name)
{
    for (int i = 0; i < User::userCount; i++)
    {
        if (User::users[i].username == name)
        {
            return users[i].team;
        }
    }
    return "Unknown";
}

bool User::isValidToken(const String &token)
{
    for (int i = 0; i < User::userCount; i++)
    {
        if (User::users[i].token == token)
            return true;
    }
    return false;
}
bool User::isValidLogin(const String &user, const String &pwdHash)
{
    Serial.println("\n=== [LOGIN] Starting validation ===");
    Serial.printf("[LOGIN] Incoming user: '%s'\n", user.c_str());
    Serial.printf("[LOGIN] Incoming hash: '%s' (length=%d)\n", pwdHash.c_str(), pwdHash.length());
    Serial.printf("[LOGIN] Stored users count: %d\n", User::userCount);

    for (int i = 0; i < User::userCount; i++)
    {
        Serial.printf("\n[LOGIN] Checking user[%d]:\n", i);
        Serial.printf("  - Username: '%s'\n", users[i].username.c_str());
        Serial.printf("  - Username match (case-insensitive): %s\n", 
                      users[i].username.equalsIgnoreCase(user) ? "YES" : "NO");
        Serial.printf("  - Stored hash: '%s'\n", users[i].passwordHash.c_str());
        Serial.printf("  - Hash match: %s\n", 
                      (users[i].passwordHash == pwdHash) ? "YES" : "NO");
        
        // Case-insensitive username comparison
        if (users[i].username.equalsIgnoreCase(user)) {
            Serial.printf("[LOGIN] Username matched! Comparing hashes...\n");
            Serial.printf("  - Expected: '%s'\n", users[i].passwordHash.c_str());
            Serial.printf("  - Got:      '%s'\n", pwdHash.c_str());
            
            // DEBUG: Per-character comparison
            Serial.println("[DEBUG] Per-character hash comparison:");
            int minLen = min(users[i].passwordHash.length(), pwdHash.length());
            for (int j = 0; j < minLen; j++) {
                Serial.printf("  [%d] expected='%c'(%d) got='%c'(%d) %s\n", 
                    j, 
                    users[i].passwordHash[j], users[i].passwordHash[j],
                    pwdHash[j], pwdHash[j],
                    (users[i].passwordHash[j] == pwdHash[j]) ? "✓" : "✗");
            }
            Serial.printf("[DEBUG] Length mismatch: expected=%d, got=%d\n", 
                users[i].passwordHash.length(), pwdHash.length());
            
            if (users[i].passwordHash == pwdHash) {
                Serial.println("[LOGIN] ✓ PASSWORD MATCH - LOGIN SUCCESSFUL\n");
                return true;
            } else {
                Serial.println("[LOGIN] ✗ PASSWORD MISMATCH - LOGIN FAILED\n");
            }
        }
    }
    
    Serial.println("[LOGIN] ✗ NO USER MATCH FOUND - LOGIN FAILED\n");
    return false;
}

String User::createSession(const String &username)
{
    for (int i = 0; i < User::userCount; i++)
    {
        if (User::users[i].username == username)
            return User::users[i].token;
    }
    return "";
}

String User::getNameBySession(const String &token)
{
    for (int i = 0; i < User::userCount; i++)
    {
        if (User::users[i].token == token)
            return User::users[i].username;
    }
    return "Unknown";
}

String User::getUserTeamBySession(const String &token)
{
    for (int i = 0; i < User::userCount; i++)
    {
        if (User::users[i].token == token)
            return User::users[i].team;
    }
    return "Unknown";
}

String User::hashPassword(const String &pwd)
{
    unsigned char hash[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, (const unsigned char *)pwd.c_str(), pwd.length());
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);

    char hex[65];
    for (int i = 0; i < 32; i++)
    {
        sprintf(hex + (i * 2), "%02x", hash[i]);
    }
    hex[64] = '\0';

    String result = String(hex);
    Serial.printf("[USER] hashPassword('%s') -> '%s'\n", pwd.c_str(), result.c_str());
    return result;
}

String User::getUsers()
{
    String userList;
    for (int i = 0; i < User::userCount; i++)
    {
        userList += User::users[i].username + ",";
    }
    return userList;
}

void User::clearUsers()
{
    User::userCount = 0;
    for (int i = 0; i < MAX_USERS; i++)
    {
        User::users[i].username = "";
        User::users[i].passwordHash = "";
        User::users[i].token = "";
        User::users[i].team = "";
    }
}

void User::setRuntimeCacheOnly(bool enabled)
{
    User::runtimeCacheOnly = enabled;
}

bool User::setUsersFromSyncPayload(const String &payload)
{
    Serial.println("[USER-SYNC] Parsing users payload");
    if (payload.length() == 0)
    {
        Serial.println("[USER-SYNC] Empty payload");
        return false;
    }

    // Preserve existing tokens so sessions survive sync refreshes
    NodeUser oldUsers[MAX_USERS];
    int oldCount = User::userCount;
    for (int i = 0; i < oldCount && i < MAX_USERS; i++)
    {
        oldUsers[i] = User::users[i];
    }

    User::clearUsers();

    int start = 0;
    while (start < payload.length() && User::userCount < MAX_USERS)
    {
        int end = payload.indexOf(';', start);
        if (end == -1) end = payload.length();
        String entry = payload.substring(start, end);
        entry.trim();
        if (entry.length() > 0)
        {
            int p1 = entry.indexOf('|');
            int p2 = entry.indexOf('|', p1 + 1);

            if (p1 > 0 && p2 > p1)
            {
                String username = entry.substring(0, p1);
                String pwdHash = entry.substring(p1 + 1, p2);
                String team = entry.substring(p2 + 1);

                User::users[User::userCount].username = username;
                User::users[User::userCount].passwordHash = pwdHash;
                User::users[User::userCount].team = team;

                // Reuse token if user existed before, otherwise generate a new one
                String preservedToken = "";
                for (int i = 0; i < oldCount; i++)
                {
                    if (oldUsers[i].username.equalsIgnoreCase(username) && oldUsers[i].token.length() > 0)
                    {
                        preservedToken = oldUsers[i].token;
                        break;
                    }
                }
                User::users[User::userCount].token = preservedToken.length() > 0 ? preservedToken : User::generateToken();
                Serial.printf("[USER-SYNC] Added user: %s | Team: %s\n", username.c_str(), team.c_str());
                User::userCount++;
            }
            else
            {
                Serial.printf("[USER-SYNC] Skipping invalid entry: %s\n", entry.c_str());
            }
        }

        start = end + 1;
    }

    Serial.printf("[USER-SYNC] Total users loaded: %d\n", User::userCount);
    User::saveUsersNVS();
    return User::userCount > 0;
}

bool User::syncUsersFromDatabase(const String &apiUrl)
{
    Serial.println("\n[USER-SYNC] Starting synchronization from database...");
    
    HTTPClient http;
    
    String fullUrl = apiUrl + "/api/sync/users";
    Serial.printf("[USER-SYNC] Fetching from: %s\n", fullUrl.c_str());
    
    http.begin(fullUrl);
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[USER-SYNC] Failed to fetch users! HTTP Code: %d\n", httpCode);
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    Serial.printf("[USER-SYNC] Received payload (%d bytes):\n", payload.length());
    Serial.println(payload);
    
    // Parse JSON response
    // Format: {"status":"success","user_count":2,"users":[{"username":"test","team":"Red Team","password_hash":"abc123..."},{"username":"sander","team":"Blue Team","password_hash":"def456..."}]}
    
    // Simple JSON parsing (no external library)
    int user_count_pos = payload.indexOf("\"user_count\":");
    if (user_count_pos == -1) {
        Serial.println("[USER-SYNC] Invalid response format!");
        return false;
    }
    
    // Preserve existing tokens so sessions survive sync refreshes
    NodeUser oldUsers[MAX_USERS];
    int oldCount = User::userCount;
    for (int i = 0; i < oldCount && i < MAX_USERS; i++)
    {
        oldUsers[i] = User::users[i];
    }

    // Clear existing users
    User::userCount = 0;
    
    // Extract users array
    int users_array_start = payload.indexOf("\"users\":[");
    int users_array_end = payload.lastIndexOf("]");
    
    if (users_array_start == -1 || users_array_end == -1) {
        Serial.println("[USER-SYNC] Could not find users array in response!");
        return false;
    }
    
    String usersJson = payload.substring(users_array_start + 9, users_array_end);
    Serial.printf("[USER-SYNC] Users JSON: %s\n", usersJson.c_str());
    
    // Parse each user object
    int userStartPos = 0;
    while (userStartPos < usersJson.length() && User::userCount < MAX_USERS) {
        int objectStart = usersJson.indexOf("{", userStartPos);
        int objectEnd = usersJson.indexOf("}", objectStart);
        
        if (objectStart == -1 || objectEnd == -1) break;
        
        String userObj = usersJson.substring(objectStart + 1, objectEnd);
        Serial.printf("[USER-SYNC] Parsing user object: %s\n", userObj.c_str());
        
        // Extract username
        int usernamePos = userObj.indexOf("\"username\":\"");
        int usernameEnd = userObj.indexOf("\"", usernamePos + 12);
        String username = userObj.substring(usernamePos + 12, usernameEnd);
        
        // Extract team
        int teamPos = userObj.indexOf("\"team\":\"");
        int teamEnd = userObj.indexOf("\"", teamPos + 8);
        String team = userObj.substring(teamPos + 8, teamEnd);
        
        // Extract password_hash
        int hashPos = userObj.indexOf("\"password_hash\":\"");
        int hashEnd = userObj.indexOf("\"", hashPos + 17);
        String pwdHash = userObj.substring(hashPos + 17, hashEnd);
        
        Serial.printf("[USER-SYNC] User[%d]: %s | Team: %s | Hash: %s\n", 
                      User::userCount, username.c_str(), team.c_str(), pwdHash.c_str());
        
        // Add user
        User::users[User::userCount].username = username;
        User::users[User::userCount].team = team;
        User::users[User::userCount].passwordHash = pwdHash;

        // Reuse token if user existed before, otherwise generate a new one
        String preservedToken = "";
        for (int i = 0; i < oldCount; i++)
        {
            if (oldUsers[i].username.equalsIgnoreCase(username) && oldUsers[i].token.length() > 0)
            {
                preservedToken = oldUsers[i].token;
                break;
            }
        }
        User::users[User::userCount].token = preservedToken.length() > 0 ? preservedToken : User::generateToken();
        
        User::userCount++;
        userStartPos = objectEnd + 1;
    }
    
    // Save to NVS
    User::saveUsersNVS();
    
    Serial.printf("[USER-SYNC] Successfully synced %d users from database!\n", User::userCount);
    return true;
}