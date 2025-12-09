#ifndef __USERS_H__
#define __USERS_H__
#include <stdbool.h>
#include <stdlib.h>
#define USERS_MAX_USERNAME_LENGTH 255
#define USERS_MAX_PASSWORD_LENGTH 255


typedef enum user_privilege_level{
    USER_PRIVILEGE_DEFAULT = 0,
    USER_PRIVILEGE_ADMIN = 1
} UserPrivilegeLevel;

typedef struct User {
    char username[USERS_MAX_USERNAME_LENGTH + 1];
    char password[USERS_MAX_PASSWORD_LENGTH + 1];
    UserPrivilegeLevel privilege_level;
} User;

typedef enum user_status {
    USER_OK,
    USER_ALREADYEXISTS,
    USER_WRONGPASSWORD,
    USER_CREDTOOLONG,
    USER_BADUSERNAME,
}UserStatus;

void users_init();

UserStatus user_create(const char *username, const char *password, UserPrivilegeLevel privilege_level);

bool user_exists(const char* username, User * out_user);

/** 
 * @brief checks whether the provided username and password match an existing user.
 * @param username The username to authenticate.
 * @param password The password to authenticate.
 * @return USER_OK if authentication is successful, USER_WRONGPASSWORD if the password is incorrect,
 * or USER_BADUSERNAME if the username does not exist.
 * */
UserStatus user_authenticate(const char* username, const char* password);

int users_get_connected_users_list(char * buffer);

#endif
