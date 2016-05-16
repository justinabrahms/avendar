#include "merc.h"

// Majoritively pulled from comm.c to circumvent an issue in our
// compilation where we don't have clear dependencies, which was
// causing errors around multiple main methods. Instead of slaying
// that beast (for now), we'll just extern those vars.
extern int control;
extern char str_boot_time[];
extern int init_socket(unsigned short);
extern void game_loop_unix(unsigned int);
extern void cleanup();

int main( int argc, char **argv )
{

/* start tracing allocations and deallocations */
//mtrace();
//COMMENTED OUT THE ABOVE FOR NOW.  -Kestrel 09 Mar 2009

object_list = NULL;
//    struct timeval now_time;

#if defined(unix)
    char buf[MAX_STRING_LENGTH];
#endif

    /*
     * Memory debugging if needed.
     */
#if defined(MALLOC_DEBUG)
    malloc_debug( 2 );
#endif


    /*
     * Init time.
     */
//    gettimeofday( &now_time, NULL );
//    current_time 	= (time_t) now_time.tv_sec;

    current_time = time(NULL);
    strcpy( str_boot_time, ctime( &current_time ) );

    /*
     * Macintosh console initialization.
     */
#if defined(macintosh)
    console_options.nrows = 31;
    cshow( stdout );
    csetmode( C_RAW, stdin );
    cecho2file( "log file", 1, stderr );
#endif

#if defined(unix)
    sprintf(buf, "rm %s", ROOMSAVELIST);
    system(buf);
    sprintf(buf, "ls %s > %s", ROOMSAVE_DIR, ROOMSAVELIST);
    system(buf);
#endif


    /*
     * Reserve one channel for our use.
     */
    if ( ( fpReserve = fopen( NULL_FILE, "r" ) ) == NULL )
    {
	perror( NULL_FILE );
	exit( 1 );
    }

    /*
     * Get the port number.
     */
    port = 4000;
    if ( argc > 1 )
    {
	int i;

	for (i = 1; i < argc; i++)
	{
	    if (!str_cmp(argv[i], "-no-mysql"))
		global_option_nomysql = TRUE;
	    else if ( !is_number( argv[i] ) )
	    {
	        fprintf( stderr, "Usage: %s <options> [port #]\n", argv[0] );
	        exit( 1 );
	    }
	    else if ( ( port = atoi( argv[i] ) ) <= 1024 )
	    {
	        fprintf( stderr, "Port number must be above 1024.\n" );
	        exit( 1 );
	    }
	}
    }

    /*
     * Run the game.
     */
#if defined(macintosh) || defined(MSDOS)
    boot_db( );
    log_string( "Avendar is ready to rock." );
    game_loop_mac_msdos( );
#endif

#if defined(unix) || defined(WIN32)
    control = init_socket( port );
    boot_db( );
    sprintf( log_buf, "Avendar is ready to rock on port %d.", port);
    log_string( log_buf );

/* Frequent crashes: not your friend. We're going to stack SEGVs
   on top of each other til they burst, and instead, we'll
   just restart the loop on a dead command */

//    signal(SIGSEGV, segv_restart);
    segv_counter = 0;
    segv_cmd[0] = (char)0;
    segv_char[0] = (char)0;
    game_loop_unix( control );
    close (control);

    // Adding some cleanup here to be a responsible citizen.
    cleanup();
    
#endif

    /*
     * That's all, folks.
     */
    log_string( "Normal termination of game." );
    exit( 0 );
    return 0;
}

