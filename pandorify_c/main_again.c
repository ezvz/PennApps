#include "appkey.c"
// #include "mongo.h"

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
char filename[256];
// Instead of mutex's we just busy-wait
int g_track_updated = 0;
int g_playlist_updated = 0;
int g_logged_in = 0;

void tracks_added(sp_playlist *pl, sp_track *const *tracks, int num_tracks, 
				  int position, void *userdata)
{
  printf("tracks added\n");
  track_updated = 1;
}

void playlist_update_in_progress(sp_playlist *pl, bool done, void *userdata)
{
  printf("(playlist updating)");
  playlist_updated = 1;
}

sp_playlist_callbacks playlist_callbacks = {
  .tracks_added = tracks_added,
  .playlist_update_in_progress = playlist_update_in_progress,
};

static void connection_error(sp_session *session, sp_error error)
{
  fprintf(stderr, "Connection error\n");
}

static void logged_in(sp_session *session, sp_error error)
{
    printf("In login.\n");
    if (error != SP_ERROR_OK) {
        fprintf(stderr, "Error: unable to log in: %s\n", sp_error_message(error));
        exit(1);
    }
    printf("Logged in success!\n");
}

static void logged_out(sp_session *session)
{
  printf("Logging out and exiting.\n");
  exit(0);
}

/**
 * This callback is called for log messages.
 *
 */
static void log_message(sp_session *session, const char *data)
{
  fprintf(stderr,"%s",data);
}

static void notify_main_thread(sp_session *session)
{
  printf("main thread notified.\n");
}

static sp_session_callbacks callbacks = {
  .logged_in = &logged_in,
  .logged_out = &logged_out,
  .notify_main_thread = &notify_main_thread,
  .log_message = &log_message,
};

static sp_session_config config = {
    .api_version = SPOTIFY_API_VERSION,
    .cache_location = "tmp",
    .settings_location = "tmp",
    .application_key = g_appkey,
    .application_key_size = g_appkey_size,
    .user_agent = USER_AGENT,
    .callbacks = &callbacks,
};

void list_playlists() {
  sp_playlistcontainer *pc = sp_session_playlistcontainer(g_session);
  int i, j, level = 0;
  sp_playlist *pl;
  char name[200];
  int new = 0;
  
  if (!pc) return;
  printf("%d playlists available\n", sp_playlistcontainer_num_playlists(pc));

  for (i = 0; i < sp_playlistcontainer_num_playlists(pc); ++i) {
    switch (sp_playlistcontainer_playlist_type(pc, i)) {
    case SP_PLAYLIST_TYPE_PLAYLIST:
      printf("%d. ", i);
      for (j = level; j; --j) printf("\t");
      pl = sp_playlistcontainer_playlist(pc, i);
      printf("%s", sp_playlist_name(pl));
      printf("\n");
      break;
    case SP_PLAYLIST_TYPE_START_FOLDER:
      printf("%d. ", i);
      for (j = level; j; --j) printf("\t");
      sp_playlistcontainer_playlist_folder_name(pc, i, name, sizeof(name));
      printf("Folder: %s with id %llu\n", name,
	     sp_playlistcontainer_playlist_folder_id(pc, i));
      level++;
      break;
    case SP_PLAYLIST_TYPE_END_FOLDER:
      level--;
      printf("%d. ", i);
      for (j = level; j; --j) printf("\t");
      printf("End folder with id %llu\n",	sp_playlistcontainer_playlist_folder_id(pc, i));
      break;
    case SP_PLAYLIST_TYPE_PLACEHOLDER:
      printf("%d. Placeholder", i);
      placeholders[current_place_index] = i;
      current_place_index++;
      break;
    }
  }
}

void pandorify_raw() {
  printf("executing pandorify_raw.\n");
  sp_playlistcontainer *pc;
  while (!pc) pc = sp_session_playlistcontainer(g_session);
  int last_slot = sp_playlistcontainer_num_playlists(pc);
  sp_error error;
      
  char buff[1024];
  int make_new_playlist = 0;
  // This is our playlist in between runs
  sp_playlist *pl;
  FILE *fp = fopen(filename, "r");

  int next_timeout = 0;

  //printf("just adding playlist\n");
  //pl = sp_playlistcontainer_add_new_playlist(pc, "foo");
  
  // if (!pl) printf("it was null\n");

  while (fgets(&buff, 1024, fp))
    {
	  printf("Cycle round.\n");
      int len = strnlen(buff, 1024);
      buff[len-1] = '\0';
      if (!strncmp(&buff, "---", 3))
		{
		  //delimiter -- new playlist to make
		  make_new_playlist = 1;
		  printf("Found playlist delimiter.\n");
		}
      else if (make_new_playlist) 
		{
		  printf("Make a new playlist called: %s\n", buff);
		  pl = sp_playlistcontainer_add_new_playlist(pc, buff);
		  make_new_playlist = 0;
		  
		  if (pl)
			printf("Playlist created\n");
		  else 
			printf("Playlist not created\n");
		  // wait until the playlist is updated
		  while (!playlist_updated) {
			sp_session_process_events(g_session, &next_timeout);
			usleep(next_timeout * 1000);
		  }
		}
      else 
		{
		  printf("Make a new song in current playlist called: %s",
				 buff);
		  sp_link *track_link = NULL;
		  track_link = sp_link_create_from_string(buff);
		  if (track_link == NULL || sp_link_type(track_link) != SP_LINKTYPE_TRACK)
			{
			  printf("Not a valid track\n");
			  continue;
			}
		  sp_track *track = sp_link_as_track(track_link);
		  if (track == NULL) {
			printf("invalid track is null\n");
			continue;
		  }
		  while ((error = sp_track_error(track)) == SP_ERROR_IS_LOADING) usleep(100);
		  if (SP_ERROR_OK != error) {
		    fprintf(stderr, "failed to link track: %s\n",
		  	    sp_error_message(error));
		  }
		  printf("track loaded successfully.\n");
		  error = sp_playlist_add_tracks(pl, (const sp_track **) &track, 
										 1, 0, g_session);
		  if (SP_ERROR_OK != error) {
			fprintf(stderr, "failed to add tracks: %s\n",
					sp_error_message(error));
		  }
		  pthread_mutex_lock(&notify_mutex);
		  pthread_cond_wait(&notify_cond, &notify_mutex);
		  pthread_mutex_unlock(&notify_mutex);
		  printf("Track added successfully.\n");
		}
	}
}

int main(int argc, char *argv[])
{
    sp_error error;
    sp_session *session;
    
    config.application_key_size = g_appkey_size;
    error = sp_session_create(&config, &session);
    if (error != SP_ERROR_OK) {
        fprintf(stderr, "Error: unable to create spotify session: %s\n", sp_error_message(error));
        return 1;
    }
    int next_timeout = 0;
    g_logged_in = 0;
    sp_session_login(session, username, password, 0, NULL);
    while (!g_logged_in) {
        sp_session_process_events(session, &next_timeout);
        usleep(next_timeout * 1000);
    }

    return 0;
}
