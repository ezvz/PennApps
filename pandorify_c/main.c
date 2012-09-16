#include "appkey.c"
#include "mongo.h"

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

// How many times to loop to allow libspotify to process its own
// fucking events.
int num_wait_loops = 500000;

int placeholders[50] = {0}; // List of placeholder indices
int current_place_index = 0;

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

void clean_placeholders() {
  int i;
  sp_error error;
  sp_playlistcontainer *pc = sp_session_playlistcontainer(g_session);
  for (i = 0; i < current_place_index; i++) {
    error = sp_playlistcontainer_remove_playlist(pc, placeholders[i]);
    if (SP_ERROR_OK != error) {
      fprintf(stderr, "failed to remove playlist: %s\n",
	      sp_error_message(error));
    }
  }
}

/* void find_pandora() { */
/*   sp_playlistcontainer *pc = sp_session_playlistcontainer(g_session); */
/*   int i = 0; */
/*   sp_playlist *pl; */
/*   char name[200]; */
/*   int in_pandora = 0; */
  
/*   if (!pc) return; */
/*   printf("%d playlists available\n", sp_playlistcontainer_num_playlists(pc)); */

/*   for (i = 0; i < sp_playlistcontainer_num_playlists(pc); ++i) { */
/*     switch (sp_playlistcontainer_playlist_type(pc, i)) { */
/*     case SP_PLAYLIST_TYPE_PLAYLIST: */
/*       if (in_pandora) { */
/* 	// do the playlist sync here */
/*       } */
/*       break; */
/*     case SP_PLAYLIST_TYPE_START_FOLDER: */
/*       sp_playlistcontainer_playlist_folder_name(pc, i, name, sizeof(name)); */
/*       printf("Folder: %s with id %llu\n", name, */
/* 	     sp_playlistcontainer_playlist_folder_id(pc, i)); */
/*       // check if pandora here */
/*       break; */
/*     case SP_PLAYLIST_TYPE_END_FOLDER: */
/*       printf("End folder with id %llu\n",	sp_playlistcontainer_playlist_folder_id(pc, i)); */
/*       break; */
/*     case SP_PLAYLIST_TYPE_PLACEHOLDER: */
/*       break; */
/*     } */
/*   } */
/* } */

/**
 * Creates a new folder named pandora and creates 
 * all station playlists within. 
 */ 
void pandorify_raw() {
  sp_playlistcontainer *pc = sp_session_playlistcontainer(g_session);
  int last_slot = sp_playlistcontainer_num_playlists(pc);
  sp_error error;
  
  // printf("Creating folder.\n");
  // Create folder
  /* error = sp_playlistcontainer_add_folder(pc, last_slot, "Pandorify"); */
  /* if (SP_ERROR_OK != error) { */
  /*   fprintf(stderr, "failed to create folder: %s\n", */
  /* 	    sp_error_message(error)); */
  /* } */
  /* int pandorify_index = last_slot; */
  /* int pandorify_end_index = last_slot+1; */

  // Setup mongodb
  /* mongo conn; */
  /* mongo_cursor cursor; */
  /* bson query; */
  
  /* mongo_init(&conn); */
  /* if (mongo_connect(&conn, "127.0.0.1", 27017) != MONGO_OK) { */
  /*   printf("mongo error\n"); */
  /*   exit(1); */
  /* } */
  /* bson_init(&query); */
  /* bson_append_int(&query, "has_uri", 1); */
  /* bson_finish(&query); */

  /* mongo_cursor_init(&cursor, &conn, "prod.songs"); */
  /* // mongo_cursor_set_query(&cursor, &query); */
  
  /* while (mongo_cursor_next(&cursor) == MONGO_OK) { */
  /*   bson_print(&cursor.current); */
  /*   /\* bson_iterator iterator; *\/ */
  /*   /\* if (bson_find(&iterator, mongo_cursor_bson(&cursor), "uri")) { *\/ */
  /*   /\*   printf("name: %s\n", bson_iterator_string(&iterator)); *\/ */
  /*   /\* } *\/ */
  /* } */
  
  /* bson_destroy(&query); */
  /* mongo_cursor_destroy(&cursor); */
    
  char buff[1024];
  int make_new_playlist = 0;
  // This is our playlist in between runs
  sp_playlist *pl;
  FILE *fp = fopen("stations_songs.txt", "r");
  
  //printf("just adding playlist\n");
  //pl = sp_playlistcontainer_add_new_playlist(pc, "foo");
  
  // if (!pl) printf("it was null\n");

  while (fgets(&buff, 1024, fp))
    {
      int len = strnlen(buff, 1024);
      buff[len-1] = '\0';
      if (!strncmp(&buff, "---", 3))
	{
	  //delimiter -- new playlist to make
	  make_new_playlist = 1;
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
	  // while ((error = sp_track_error(track)) == SP_ERROR_IS_LOADING) usleep(100);
	  /* if (SP_ERROR_OK != error) { */
	  /*   fprintf(stderr, "failed to link track: %s\n",  */
	  /* 	    sp_error_message(error)); */
	  /* } */
	  error = sp_playlist_add_tracks(pl, (const sp_track **) &track, 
					 1, 0, g_session);
	  if (SP_ERROR_OK != error) {
	    fprintf(stderr, "failed to add tracks: %s\n",
	  	    sp_error_message(error));
	  }
	}
    }
  
  // add playlist
  // sp_playlist *pl = sp_playlistcontainer_add_new_playlist(pc, "Station1");
  /* error = sp_playlist_add_tracks(pl, tracks, num_tracks, 0, g_session); */
  /* if (SP_ERROR_OK != error) { */
  /*   fprintf(stderr, "failed to add tracks: %s\n", */
  /* 	    sp_error_message(error)); */
  /* } */

  // move into folder
  // int new_last = sp_playlistcontainer_num_playlists(pc);
  // printf("Adding to folder.\n");
  // error = sp_playlistcontainer_move_playlist(pc, pandorify_end_index, new_last, 1);
  /* if (SP_ERROR_OK != error) { */
  /*   fprintf(stderr, "failed to move playlist: %s\n", */
  /* 	    sp_error_message(error)); */
  /* } */
  
    // mongo_destroy(&conn);
}

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
  printf("Logging out and exiting.\n");
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
  int finished = 0;
  
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

    // Do our thing
    if (!num_wait_loops && !finished) {
      // pthread_mutex_unlock(&notify_mutex);
      list_playlists();
      int i;
      /* for (i = 0; i < 50; i++) { */
      /* 	printf("index %d is %d.\n", i, placeholders[i]); */
      /* } */
      // clean_placeholders();
      pthread_mutex_unlock(&notify_mutex);
      pandorify_raw();
      finished = 1;

      sp_error error = sp_session_logout(g_session);
      // To reloop if needed 
      /* num_wait_loops = 1000000; */
      /* finished = 1; */
      pthread_mutex_lock(&notify_mutex);
    }
    else {
      num_wait_loops--;
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
