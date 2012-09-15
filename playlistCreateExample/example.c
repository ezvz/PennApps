#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libspotify/api.h>

#include "appkey.c"
#include "login.c"

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

/**
 * Callbacks (only to synchronize some operations).
 */
void cb_logged_in(sp_session *session, sp_error error) // session callback
{
  ASSERT_SP_ERROR_OK(error);
  MESSAGE("Logged in!");
  G_SIGNAL(/*nothing*/);
}

void cb_process_events(sp_session *session) // session callback
{
  /* don’t do this, it might lock up libspotify :P */
  int timeout = 0;
  do { sp_session_process_events(session, &timeout); } while(timeout == 0);
  // MESSAGE("[processed events]");
}

void cb_container_loaded(sp_playlistcontainer *pc, void *userdata)
{
  MESSAGE("(container loaded)");
  G_SIGNAL(/**/);
}

void cb_tracks_added(sp_playlist *pl, sp_track *const *tracks, int num_tracks, int position, void *userdata)
{
  MESSAGE("(tracks added)");
  G_SIGNAL(/**/);
}

void cb_playlist_update_in_progress(sp_playlist *pl, bool done, void *userdata)
{
  MESSAGE("(playlist updating)");
  if (done)
  {
    G_SIGNAL(
      MESSAGE("(playlist done updating!)");
    );
  }
}

int main(void)
{
  MESSAGE("Hi! Example on the way!");
  sp_session *session = NULL;
  
  /* Log in */
  sp_session_callbacks session_callbacks = {
    .logged_in = cb_logged_in,
    .notify_main_thread = cb_process_events,
  };
  sp_session_config session_config =
  {
    .api_version          = SPOTIFY_API_VERSION,
    .cache_location       = "",
    .settings_location    = "/tmp",
    .application_key      = g_appkey,
    .application_key_size = g_appkey_size,
    .user_agent           = "Hallon",
    .callbacks            = &session_callbacks,
    .userdata             = NULL,
    .compress_playlists   = 1,
    .initially_unload_playlists = 0,
    .dont_save_metadata_for_playlists = 0,
  };
  sp_error error = sp_session_create(&session_config, &session);
  ASSERT_SP_ERROR_OK(error);
  MESSAGE("Created session successfully.");
  
  G_WAIT( 
    MESSAGE("Logging in…");
    sp_session_login(session, g_username, g_password);
    MESSAGE("(waiting…)");
  );
  
  INFO("Logged in: %s", (
    sp_session_connectionstate(session) == SP_CONNECTION_STATE_LOGGED_IN ? "yes" : "no"
  ));
  
  /*
    Get container.
    
    NOTE: There is a race condition here. It might be the case that the
    container is loaded *BEFORE* we enter the G_WAIT statement (causing
    a deadlock). This is a result of us processing events directly in
    the callback, having no way to wait before loading the container.
  */
  MESSAGE("Getting container…");
  sp_playlistcontainer_callbacks container_callbacks = {
    .container_loaded = cb_container_loaded,
  };
  sp_playlistcontainer *container = sp_session_playlistcontainer(session);
  G_WAIT(
    sp_playlistcontainer_add_callbacks(container, &container_callbacks, NULL);
    MESSAGE("Added callbacks, waiting for it to load…")
  );
  
  MESSAGE("Give me a name for the new playlist (no spaces, < 255 chars)");
  char buffer[255];
  printf("> ");
  scanf("%255s", buffer);
  
  INFO("Adding “%s” to container…", buffer);
  sp_playlist_callbacks playlist_callbacks = {
    .tracks_added = cb_tracks_added,
    .playlist_update_in_progress = cb_playlist_update_in_progress,
  };
  sp_playlist *playlist = NULL;
  playlist = sp_playlistcontainer_add_new_playlist(container, buffer);
  if (playlist == NULL) FAIL("Failed to create playlist.");
  G_WAIT(
    /* race condition */
    MESSAGE("(waiting for it to load; takes like 30s)");
  );
  sp_playlist_add_callbacks(playlist, &playlist_callbacks, NULL);
  
  sp_link *playlist_link = sp_link_create_from_playlist(playlist);
  if (playlist_link == NULL) FAIL("Failed to create link.");
  int size = sp_link_as_string(playlist_link, buffer, 255);
  if (size > 255 /* buffer size */) FAIL("Link was truncated.");
  INFO("Playlist URL: %s", buffer);
  
  MESSAGE("Time to add tracks (endless loop)!");
  do
  {
    MESSAGE("Give me a track URI!");
    printf("> ");
    scanf("%255s", buffer);
    
    sp_link *track_link = NULL;
    track_link = sp_link_create_from_string(buffer);
    if (track_link == NULL || sp_link_type(track_link) != SP_LINKTYPE_TRACK)
    {
      INFO("Not a track: %s", buffer);
      continue;
    }
    
    sp_track *track = sp_link_as_track(track_link);
    while ((error = sp_track_error(track)) == SP_ERROR_IS_LOADING) usleep(100); /* sleep 100 microseconds */
    ASSERT_SP_ERROR_OK(error);
    
    MESSAGE("Track is loaded (not sure I had to wait, anyway)! On with adding it…");
    error = sp_playlist_add_tracks(playlist, (const sp_track **) &track, 1, 0, session);
    ASSERT_SP_ERROR_OK(error);
    G_WAIT(/* race condition */);
    MESSAGE("Track added (but playlist not necessarily up to date yet)");
  } while(1);
  
  return EXIT_SUCCESS;
}