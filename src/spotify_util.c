/**
 * @file   spotify_util.c
 * @author Mishal Awadah <a.mamish@gmail.com>
 * @date   Sat Sep 15 14:06:46 2012
 * 
 * @brief  The spotify utility process. This gets called by our
 *         python scripts to perform functions we need.
 * 
 * 
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libspotify/api.h>

#include "appkey.c"

#define INFO(fmt, xargs...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, xargs)
#define ABORT(fmt, xargs...) do { INFO(fmt, xargs); exit(EXIT_FAILURE); } while(0)

#define MESSAGE(msg) INFO("%s", (msg))
#define FAIL(msg) ABORT("%s", (msg))

#define ASSERT_SP_ERROR_OK(error) do {\
  if ((error) != SP_ERROR_OK) ABORT("%s", sp_error_message(error));\
} while(0)

/**
 * Global variables.
 */
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_cond  = PTHREAD_COND_INITIALIZER;

#define PF_USER_AGENT "pandorify"

// Convenience macros:
#define G_MUTEX_SYNCHRONIZE(code) do {\
  pthread_mutex_lock(&(g_mutex));\
  code;\
  pthread_mutex_unlock(&(g_mutex));\
} while(0)

#define G_WAIT(code) do {\
  G_MUTEX_SYNCHRONIZE(code; pthread_cond_wait(&g_cond, &g_mutex));\
} while(0)

#define G_SIGNAL(code) do {\
  G_MUTEX_SYNCHRONIZE(code; pthread_cond_signal(&g_cond));\
} while(0)

static void pf_logged_in(sp_session *session, sp_error error) {

}

static void pf_notify_main_thread(sp_session *session) {
    
}

static sp_session_callbacks callbacks = {
    .logged_in = pf_logged_in,
    .notify_main_thread = pf_notify_main_thread,
};

int spotify_init(const char *username, const char* password) {
    sp_session *session;
    sp_session_config config;
    sp_error error;
    
    config.api_version = SPOTIFY_API_VERSION;
    config.cache_location = "tmp";
    config.settings_location = "tmp";
    config.application_key = g_appkey;
    config.application_key_size = g_appkey_size;
    config.user_agent = PF_USER_AGENT;
    config.callbacks = &callbacks;
}

int main(int argc, char *argv[])
{
    MESSAGE("Running spotify_util");
    
    
    
    return 0;
}
