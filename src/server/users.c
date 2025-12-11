#include "include/users.h"
#include <string.h>
#include "../shared/include/list.h"

static List usersList = NULL;

static int user_cmp(void * a, void * b) {
    User aa = *(User *)a;
    User bb = *(User *)b;
    return strcmp((char*)aa.username, (char*)bb.username);
}


void users_init() {
    if (usersList == NULL) {
        usersList = list_create(&user_cmp, sizeof(User));

        // Create a default admin user
        User adminUser = {
            .username = "admin",
            .password = "admin",
            .privilege_level = USER_PRIVILEGE_ADMIN
        };
        list_add(usersList, &adminUser);
    }
}

User* user_get_if_exists(const uint8_t *username) {
	list_begin_iter(usersList);
	
	while(list_has_next(usersList)) {
		User *user = (User *)list_get_next(usersList, NULL);
		if(0 == strcmp((char*)user->username, (char*)username)) {
			return user;  // Return pointer to actual user in list
		}
	}
	return NULL;  // Not found
}

bool user_is_admin(const uint8_t* username) {
    User * user = user_get_if_exists(username);
    if (user != NULL) {
        return user->privilege_level == USER_PRIVILEGE_ADMIN;
    }
    return false;
}


UserStatus user_create(const uint8_t *username, const uint8_t *password, UserPrivilegeLevel pl) {
    if(strlen((char*)username) >= USERS_MAX_USERNAME_LENGTH) {
        return USER_CREDTOOLONG;
    }
    if(strlen((char*)password) >= USERS_MAX_PASSWORD_LENGTH) {
        return USER_CREDTOOLONG;
    }
    User * user = user_get_if_exists(username);
    if (user == NULL) {
        User user;
        strncpy((char*)user.username, (char*)username, USERS_MAX_USERNAME_LENGTH - 1);
        user.username[USERS_MAX_USERNAME_LENGTH - 1] = '\0';
        strncpy((char*)user.password, (char*)password, USERS_MAX_PASSWORD_LENGTH - 1);
        user.password[USERS_MAX_PASSWORD_LENGTH - 1] = '\0';
        user.privilege_level = pl;
        list_add(usersList, &user);
        return USER_OK;
    }
    return USER_ALREADYEXISTS;
}

UserStatus user_remove(const uint8_t *username) {
    User *user = user_get_if_exists(username);

    if(user != NULL) {
        list_remove(usersList, &user);
        return USER_OK;
    }
    return USER_BADUSERNAME;
}


UserStatus user_change_password(const uint8_t *username, const uint8_t *new_password) {
    User * user = user_get_if_exists(username);
    if(user != NULL) {
        memcpy(user->password, new_password, strlen((char*)new_password));
        return USER_OK;
    }
    return USER_BADUSERNAME;
}


UserStatus user_change_role(const uint8_t *username, UserPrivilegeLevel new_role) {
    User *user = user_get_if_exists(username);
    if(user != NULL) {
        user->privilege_level = new_role;
        return USER_OK;
    }
    return USER_BADUSERNAME;
}


UserStatus user_authenticate(const uint8_t *username, const uint8_t *password) {
  User * user = user_get_if_exists(username);
  if(user == NULL) {
    return USER_BADUSERNAME;
  }
  if(0 != strcmp((char *)user->password, (char *)password)) {
    return USER_WRONGPASSWORD;
  }
  return USER_OK;
}


int users_get_connected_users_list(uint8_t * buffer, int from) {
    size_t copied = from; 
    list_begin_iter(usersList);
    
    while(list_has_next(usersList)) {
        User user;
        list_get_next(usersList, &user);
        UserPrivilegeLevel pl = user.privilege_level;
        buffer[copied++] = pl == USER_PRIVILEGE_ADMIN ? '@' : '#';
        
        // append user.username to the result string
        uint8_t * username = user.username;
        size_t usernameLength = strlen((char *)username);
        memcpy(buffer + copied, username, usernameLength);
        copied += usernameLength;
        
        buffer[copied++] = '\n';
    }
    
    buffer[copied] = '\0';
    return copied;
}

void users_free() {
    list_free(usersList);
}
