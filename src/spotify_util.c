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


/*** Globals ***/

#define PF_USER_AGENT "pandorify"

/// Sync mutex for main thread
static pthread_mutex_t g_notify_mutex;
/// Sync condition var for main thread
static pthread_cond_t g_notify_cond;
/// Sync var telling main thread to process events
static int g_notify_do;
/// The global session handle
static sp_session *g_session;
/// Handle to the playlist currently being manipulated
static sp_playlist *g_playlist;
/// Name of the playlist currently being manipulated
static char *g_playlistname;

/*** End Globals ***/

/*** Playlist callbacks ***/

static void tracks_added(sp_playlist *pl, sp_track * const *tracks,
                         int num_tracks, int position, void *userdata)
{
    if (pl != g_playlist)
        return;
    INFO("Added %d tracks to playlist %s", num_tracks, g_playlistname);
}

static sp_playlist_callbacks pl_callbacks = {
    .tracks_added = &tracks_added,
};

/*** End playlist callbacks ***/

/*** Playlist Container Callbacks ***/

/**
 * Callback from libspotify, telling us a playlist was added to the playlist container.
 *
 * We add our playlist callbacks to the newly added playlist. We set the global
 * playlist handler to the newly added playlist.
 *
 * @param  pc            The playlist container handle
 * @param  pl            The playlist handle
 * @param  position      Index of the added playlist
 * @param  userdata      The opaque pointer
 */
static void playlist_added(sp_playlistcontainer *pc, sp_playlist *pl,
                           int position, void *userdata)
{
    sp_playlist_add_callbacks(pl, &pl_callbacks, NULL);
    
    if (!strcasecmp(sp_playlist_name(pl), g_playlistname)) {
        g_playlist = pl;
        g_playlistname = sp_playlist_name(pl);
    }
}

// playlist remove? to remove callbacks??

/**
 * Callback from libspotify, telling us the rootlist is fully synchronized
 * We just print an informational message
 *
 * @param  pc            The playlist container handle
 * @param  userdata      The opaque pointer
 */
static void container_loaded(sp_playlistcontainer *pc, void *userdata)
{
	INFO(stderr, "jukebox: Rootlist synchronized (%d playlists)\n",
	    sp_playlistcontainer_num_playlists(pc));
}

/**
 * The playlist container callbacks
 */
static sp_playlistcontainer_callbacks pc_callbacks = {
	.playlist_added = &playlist_added,
	// .playlist_removed = &playlist_removed,
	.container_loaded = &container_loaded,
};

/*** End Playlist Container Callbacks ***/

/*** Session Callbacks ***/

static void logged_in(sp_session *session, sp_error error) {
    ASSERT_SP_ERROR_OK(error);
    
    sp_playlistcontainer *pc = sp_session_playerlistcontainer(session);
    int i;

    sp_playlistcontainer_add_callbacks(pc,
                                       &pc_callbacks,
                                       NULL);
    
    INFO("Looking at %d playlists", sp_playlistcontainer_num_playlists(pc));
    
}

/**
 * This callback is called from an internal libspotify thread to ask us to
 * reiterate the main loop.
 *
 * We notify the main thread using a condition variable and a protected variable.
 *
 * @sa sp_session_callbacks#notify_main_thread
 */
static void notify_main_thread(sp_session *session)
{
	pthread_mutex_lock(&g_notify_mutex);
	g_notify_do = 1;
	pthread_cond_signal(&g_notify_cond);
	pthread_mutex_unlock(&g_notify_mutex);
}

static sp_session_callbacks session_callbacks = {
    .logged_in = logged_in,
    .notify_main_thread = notify_main_thread,
};

/*** End Session Callbacks ***/


/** 
 * Initialize and login to create a session.
 * 
 * @param username 
 * @param password 
 * 
 * @return 0 on success, error otherwise
 */
int pf_spotify_init(const char *username, const char* password) {
    INFO("Init with uname %s and password %s", username, password);
    sp_session *session;
    sp_session_config config;
    sp_error error;
    
    config.api_version = SPOTIFY_API_VERSION;
    config.cache_location = "tmp";
    config.settings_location = "tmp";
    config.application_key = g_appkey;
    config.application_key_size = g_appkey_size;
    config.user_agent = PF_USER_AGENT;
    config.callbacks = &session_callbacks;

    error = sp_session_create(&config, &session);
    ASSERT_SP_ERROR_OK(error);
    MESSAGE("Created session successfully.");

    MESSAGE("Logging in...");
    error = sp_session_login(session, username, password, 0, NULL);
    pthread_mutex_lock(&g_notify_mutex);

    INFO("Logged in: %s", (
                           sp_session_connectionstate(session) == SP_CONNECTION_STATE_LOGGED_IN ? "yes" : "no"
                           ));
    g_session = session;
    return 0;
}

int main(int argc, char *argv[])
{
    MESSAGE("Running spotify_util");
 
    int next_timeout = 0;
   
    pthread_mutex_init(&g_notify_mutex, NULL);
    pthread_cond_init(&g_notify_cond, NULL);
    
    if (argc < 3) {
        FAIL("Not enough arguments");
    }
    printf("%d\n", argc);
    if (pf_spotify_init(argv[1], argv[2]) != 0) {
        FAIL("Failed to initialize");
    }
    
    while (1) 
        {
            if (next_timeout == 0) {
                while (!g_notify_do)
                    pthread_cond_wait(&g_notify_cond, &g_notify_mutex);
            }
            else {
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
                
                pthread_cond_timedwait(&g_notify_cond, &g_notify_mutex, &ts);
            }
            
            g_notify_do = 0;
            pthread_mutex_unlock(&g_notify_mutex);
            
            do {
                sp_session_process_events(g_session, &next_timeout);
            } while (next_timeout == 0);

        }

    return 0;
}
