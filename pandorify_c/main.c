#include "appkey.c"

#define USER_AGENT "pandorify"

#include <pthread.h>
#include <sys/time.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <libspotify/api.h>
#include <strings.h>

sp_session *g_session;

static int notify_events;
static pthread_mutex_t notify_mutex;
static pthread_cond_t notify_cond;

// Tells us our playlists are ready
static void container_loaded(sp_playlistcontainer *pc, void *userdata)
{
  printf("Container loaded with %d playlists available\n",
	  sp_playlistcontainer_num_playlists(pc));
}

// Registers playlist container callbacks
static sp_playlistcontainer_callbacks pc_callbacks = {
  .container_loaded = &container_loaded,
};

static void connection_error(sp_session *session, sp_error error)
{
  fprintf(stderr, "Connection error\n");
}

static void logged_in(sp_session *session, sp_error error)
{
  printf("Logged in success!\n");
  // Register container callbacks here
  sp_playlistcontainer *pc = sp_session_playlistcontainer(session);
  sp_playlistcontainer_add_callbacks(pc, &pc_callbacks, NULL);				     
}

static void logged_out(sp_session *session)
{
  exit(0);
}


/**
 * This callback is called for log messages.
 *
 * @sa sp_session_callbacks#log_message
 */
static void log_message(sp_session *session, const char *data)
{
  fprintf(stderr,"%s",data);
}

void notify_main_thread(sp_session *session)
{
  pthread_mutex_lock(&notify_mutex);
  notify_events = 1;
  pthread_cond_signal(&notify_cond);
  pthread_mutex_unlock(&notify_mutex);
}

static sp_session_callbacks callbacks = {
  &logged_in,
  &logged_out,
  NULL,
  &connection_error,
  NULL,
  &notify_main_thread,
  NULL,
  NULL,
  &log_message
};

int spotify_init(const char *username,const char *password)
{
  sp_session_config config;
  sp_error error;
  sp_session *session;

  bzero(&config, sizeof(config));

  // Always do this. It allows libspotify to check for
  // header/library inconsistencies.
  config.api_version = SPOTIFY_API_VERSION;

  // The path of the directory to store the cache. This must be specified.
  // Please read the documentation on preferred values.
  config.cache_location = "tmp";

  // The path of the directory to store the settings. 
  // This must be specified.
  // Please read the documentation on preferred values.
  config.settings_location = "tmp";

  // The key of the application. They are generated by Spotify,
  // and are specific to each application using libspotify.
  config.application_key = g_appkey;
  config.application_key_size = g_appkey_size;

  // This identifies the application using some
  // free-text string [1, 255] characters.
  config.user_agent = USER_AGENT;

  // Register the callbacks.
  config.callbacks = &callbacks;

  error = sp_session_create(&config, &session);
  if (SP_ERROR_OK != error) {
    fprintf(stderr, "failed to create session: %s\n",
	    sp_error_message(error));
    return 2;
  }

  // Login using the credentials given on the command line.
  error = sp_session_login(session, username, password, 0, 0);

  if (SP_ERROR_OK != error) {
    fprintf(stderr, "failed to login: %s\n",
	    sp_error_message(error));
    return 3;
  }

  g_session = session;
  return 0;
}

int main(int argc, char **argv)
{
  int next_timeout = 0;
  if(argc < 3) {
    fprintf(stderr,"Usage: %s <username> <password>\n",argv[0]);
  }
  pthread_mutex_init(&notify_mutex, NULL);
  pthread_cond_init(&notify_cond, NULL);
  
  if(spotify_init(argv[1],argv[2]) != 0) {
    fprintf(stderr,"Spotify failed to initialize\n");
    exit(-1);
  }
  pthread_mutex_lock(&notify_mutex);
  for (;;) {
    if (next_timeout == 0) {
      while(!notify_events)
	pthread_cond_wait(&notify_cond, &notify_mutex);
    } else {
      struct timespec ts;
      
#if _POSIX_TIMERS > 0
      clock_gettime(CLOCK_REALTIME, &ts);
#else
      struct timeval tv;
      gettimeofday(&tv, NULL);
      TIMEVAL_TO_TIMESPEC(&tv, &ts);
#endif
      
      ts.tv_sec += next_timeout / 1000;
      ts.tv_nsec += (next_timeout % 1000) * 1000000;
      
      while(!notify_events) {
	if(pthread_cond_timedwait(&notify_cond, &notify_mutex, &ts))
	  break;
      }
    }
    
    // Process libspotify events
    notify_events = 0;
    pthread_mutex_unlock(&notify_mutex);
    
    do {
      sp_session_process_events(g_session, &next_timeout);
    } while (next_timeout == 0);
    
    pthread_mutex_lock(&notify_mutex);
  }
  return 0;
}
