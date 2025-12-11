#ifndef __USERS_H__
#define __USERS_H__
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#define USERS_MAX_USERNAME_LENGTH 255
#define USERS_MAX_PASSWORD_LENGTH 255


typedef enum user_privilege_level{
    USER_PRIVILEGE_ADMIN = 0, 
    USER_PRIVILEGE_DEFAULT = 1,
} UserPrivilegeLevel;

typedef struct User {
    uint8_t username[USERS_MAX_USERNAME_LENGTH + 1];
    uint8_t password[USERS_MAX_PASSWORD_LENGTH + 1];
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

UserStatus user_create(const uint8_t *username, const uint8_t *password, UserPrivilegeLevel privilege_level);

UserStatus user_remove(const uint8_t *username);

User* user_get_if_exists(const uint8_t *username);

UserStatus user_change_password(const uint8_t *username, const uint8_t *new_password);

UserStatus user_change_role(const uint8_t *username, UserPrivilegeLevel new_role);

bool user_is_admin(const uint8_t* username);

/** 
 * @brief checks whether the provided username and password match an existing user.
 * @param username The username to authenticate.
 * @param password The password to authenticate.
 * @return USER_OK if authentication is successful, USER_WRONGPASSWORD if the password is incorrect,
 * or USER_BADUSERNAME if the username does not exist.
 * */
UserStatus user_authenticate(const uint8_t* username, const uint8_t* password);

void users_free();

int users_get_connected_users_list(uint8_t * buffer, int from);

#endif
