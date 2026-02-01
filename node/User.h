#ifndef USER_H
#define USER_H
#include <Arduino.h>
#include <Preferences.h>
#define MAX_USERS 50

struct NodeUser
{
    String username;
    String passwordHash;
    String token;
    String team;
};

struct User
{
    String username;
    String token;
    String passwordHash;
    // static methods below
    static void loadUsersNVS();
    static bool registerUserWithToken(const String &name, const String &pwdHash, const String &team, String userToken);
    static bool registerUser (const String &name, const String &pwdHash, const String &team);
    static bool addOrUpdateUser(const String &name, const String &pwdHash, String &token, const String &team);
    static bool isValidToken(const String &token);
    static bool isValidLogin(const String &user, const String &pwdHash);
    static String createSession(const String &username);
    static String getNameBySession(const String &token);
    static String getUserTeamBySession(const String &token);
    static String hashPassword(const String &pwd);
    static String getUsers();
    static int getUserCount();
    static void saveUsersNVS();
    static String generateToken();
    static String getUserName(int index);
    static String getUserTeam(int index);
    static String getUserTeamByName(const String &name);

public:
    static NodeUser users[MAX_USERS];
    static int userCount;

    static Preferences prefs;
};
#endif // USER_H
