#include "User.h"
#include "LoraNode.h"
#include "Preferences.h"

NodeUser User::users[MAX_USERS];
int User::userCount = 0;
Preferences User::prefs;

void User::saveUsersNVS()
{
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
    User::prefs.begin("User::users", true);
    User::userCount = User::prefs.getInt("userCount", 0);
    for (int i = 0; i < User::userCount; i++)
    {
        User::users[i].username = User::prefs.getString(("user" + String(i) + "_name").c_str(), "");
        User::users[i].token = User::prefs.getString(("user" + String(i) + "_token").c_str(), "");
        User::users[i].passwordHash = User::prefs.getString(("user" + String(i) + "_hash").c_str(), "");
        User::users[i].team = User::prefs.getString(("user" + String(i) + "_team").c_str(), "");

        Serial.printf("[User] Loaded user: %s\n", User::users[i].username.c_str());
        Serial.printf("[User] Loaded token: %s\n", User::users[i].token.c_str());
    }
    User::prefs.end();
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
    for (int i = 0; i < User::userCount; i++)
    {
        if (User::users[i].username == name)
        {
            return true;
        }
    }

    String token = User::generateToken();
    if (User::userCount < MAX_USERS)
    {
        return User::registerUserWithToken(name, pwdHash, team, token);
    }
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
    Serial.printf("[User] Validating password for user: %s\n", user.c_str());
    Serial.printf("[User] Expected hash: %s\n", pwdHash.c_str());
    Serial.printf("[User] Stored users count: %d\n", User::userCount);

    for (int i = 0; i < User::userCount; i++)
    {
        Serial.printf("[User] Comparing with stored user: %s\n", users[i].username.c_str());
        if (users[i].username == user && users[i].passwordHash == pwdHash)
            return true;
    }
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
    unsigned long h = 0;
    for (size_t i = 0; i < pwd.length(); i++)
        h += pwd[i] * 31;
    return String(h);
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