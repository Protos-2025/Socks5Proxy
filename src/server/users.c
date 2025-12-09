#include "include/users.h"
#include <string.h>
#include "../shared/include/list.h"

static List usersList = NULL;

static int user_cmp(void * a, void * b) {
    User aa = *(User *)a;
    User bb = *(User *)b;
    return strcmp(aa.username, bb.username);
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

bool user_exists(const char *username, User * out_user) {
  bool found = false; 
  list_begin_iter(usersList);
  
  while(list_has_next(usersList) && !found) {
    User user;
    list_get_next(usersList, &user);

    if(0 == strcmp(user.username, username)) {
      found = true;
      if (out_user != NULL) {
        *out_user = user;
      }
    }  
  }
  return found;
}


UserStatus user_create(const char *username, const char *password, UserPrivilegeLevel pl) {
    if(strlen(username) >= USERS_MAX_USERNAME_LENGTH) {
        return USER_CREDTOOLONG;
    }
    if(strlen(password) >= USERS_MAX_PASSWORD_LENGTH) {
        return USER_CREDTOOLONG;
    }
    if (!user_exists(username, NULL)) {
        User user;
        strncpy(user.username, username, USERS_MAX_USERNAME_LENGTH - 1);
        user.username[USERS_MAX_USERNAME_LENGTH - 1] = '\0';
        strncpy(user.password, password, USERS_MAX_PASSWORD_LENGTH - 1);
        user.password[USERS_MAX_PASSWORD_LENGTH - 1] = '\0';
        user.privilege_level = pl;
        list_add(usersList, &user);
        return USER_OK;
    }
    return USER_ALREADYEXISTS;
}



UserStatus user_authenticate(const char *username, const char *password) {
  User user;
  if(!user_exists(username, &user)) {
    return USER_BADUSERNAME;
  }
  if(0 != strcmp(user.password, password)) {
    return USER_WRONGPASSWORD;
  }
  return USER_OK;
}
