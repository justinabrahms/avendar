/***************************************************************************
 *
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@efn.org)				   *
*	    Gabrielle Taylor						   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

#define _REENTRANT
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

#if defined(unix)
#include <sys/time.h>
#include <unistd.h>
#else
#include <time.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>
#include <mcheck.h>

#if defined(WIN32)
#include <conio.h>
#include <io.h>
#include <Winsock2.h>
#include <sys/timeb.h>
#else
static const unsigned int INVALID_SOCKET = static_cast<unsigned int>(-1);
#endif

#if defined(MSDOS)
#include <unistd.h>
#endif

#if defined(MSDOS) || defined(WIN32)
#define EWOULDBLOCK EAGAIN
#endif

#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "languages.h"
#include "lookup.h"
#include "olc.h"
#include "dictionary.h"
#include "Titles.h"
#include "Weave.h"
#include "fight.h"
#include "spells_air.h"
#include "NameMaps.h"

/* command procedures needed */
DECLARE_DO_FUN(do_help		);
DECLARE_DO_FUN(do_look		);
DECLARE_DO_FUN(do_skills	);
DECLARE_DO_FUN(do_outfit	);
DECLARE_DO_FUN(do_unread	);
DECLARE_DO_FUN(do_score		);

extern bool    MOBtrigger;
extern void gameserv_process	args((CHAR_DATA *ch));
extern void do_bug( CHAR_DATA *ch, char *argument );
char *ban_message = "Your site has been banned from the MUD.\n\r\n\rAOL Users:\n\r==========\n\rBecause AOL uses dynamic IP addressing across its entire network, we require\n\rusers to have an account before playing the MUD.  If you haven't been banned\n\rfrom Avendar in the past, please e-mail one of our implementors at\n\ravendar@gmail.com and he can help you get setup.  We apologize for the\n\rinconvenience, we don't like it any more than you do.\n\r";


/*
 * Malloc debugging stuff.
 */
#if defined(sun)
#undef MALLOC_DEBUG
#endif

#if defined(MALLOC_DEBUG)
#include <malloc.h>
extern	int	malloc_debug	args( ( int  ) );
extern	int	malloc_verify	args( ( void ) );
#endif

extern bool crumble_test;
extern bool rip_process;

#if defined(unix) || defined(WIN32)
    int control;
#endif

char segv_char[128];
char segv_cmd[MAX_INPUT_LENGTH+1];
int segv_counter;
#define MAX_SEGV 15

/*
 * Signal handling.
 * Apollo has a problem with __attribute(atomic) in signal.h,
 *   I dance around it.
 */
#if defined(apollo)
#define __attribute(x)
#endif

#include <signal.h>

#if defined(apollo)
#undef __attribute
#endif


extern void	fwrite_account  args( ( ACCOUNT_DATA *acct ) );
extern void do_descriptortime(DESCRIPTOR_DATA *d);
void dns_thread_func(void *d);
char confused_string[MAX_STRING_LENGTH]; /*global for confuse_text */
char randword[MAX_STRING_LENGTH]; /*global for confuse_text */

extern bool crumble_process;

/*
 * Socket and TCP/IP stuff.
 */
#if	defined(macintosh) || defined(MSDOS) || defined(WIN32)
const	char	echo_off_str	[] = { '\0' };
const	char	echo_on_str	[] = { '\0' };
const	char 	go_ahead_str	[] = { '\0' };
#endif

#if	defined(unix)
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "telnet.h"
const	char	echo_off_str	[] = { IAC, WILL, TELOPT_ECHO, '\0' };
const	char	echo_on_str	[] = { IAC, WONT, TELOPT_ECHO, '\0' };
const	char 	go_ahead_str	[] = { IAC, GA, '\0' };
const	char	erase_line_str	[] = { IAC, EL, '\0' };
#endif

/*
 * OS-dependent declarations.
 */
#if	defined(_AIX)
#include <sys/select.h>
int	accept		args( ( int s, struct sockaddr *addr, int *addrlen ) );
int	bind		args( ( int s, struct sockaddr *name, int namelen ) );
void	bzero		args( ( char *b, int length ) );
int	getpeername	args( ( int s, struct sockaddr *name, int *namelen ) );
int	getsockname	args( ( int s, struct sockaddr *name, int *namelen ) );
int	gettimeofday	args( ( struct timeval *tp, struct timezone *tzp ) );
int	listen		args( ( int s, int backlog ) );
int	setsockopt	args( ( int s, int level, int optname, void *optval,
			    int optlen ) );
int	socket		args( ( int domain, int type, int protocol ) );
#endif

#if	defined(apollo)
#include <unistd.h>
void	bzero		args( ( char *b, int length ) );
#endif

#if	defined(__hpux)
int	accept		args( ( int s, void *addr, int *addrlen ) );
int	bind		args( ( int s, const void *addr, int addrlen ) );
void	bzero		args( ( char *b, int length ) );
int	getpeername	args( ( int s, void *addr, int *addrlen ) );
int	getsockname	args( ( int s, void *name, int *addrlen ) );
int	gettimeofday	args( ( struct timeval *tp, struct timezone *tzp ) );
int	listen		args( ( int s, int backlog ) );
int	setsockopt	args( ( int s, int level, int optname,
 				const void *optval, int optlen ) );
int	socket		args( ( int domain, int type, int protocol ) );
#endif

#if	defined(interactive)
#include <net/errno.h>
#include <sys/fnctl.h>
#endif

/*
#if	defined(WIN32)
int	close		args( ( int fd ) );
int	write		args( ( int fd, char *buf, int nbyte ) );
int	read		args( ( int fd, char *buf, int nbyte ) );
#endif
*/


#if	defined(linux)
/* 
    Linux shouldn't need these. If you have a problem compiling, try
    uncommenting accept and bind.
int	accept		args( ( int s, struct sockaddr *addr, int *addrlen ) );
int	bind		args( ( int s, struct sockaddr *name, int namelen ) );
*/

int	close		args( ( int fd ) );
/*int	getpeername	args( ( int s, struct sockaddr *name, int *namelen ) );
int	getsockname	args( ( int s, struct sockaddr *name, int *namelen ) );*/
int	gettimeofday	args( ( struct timeval *tp, struct timezone *tzp ) );
/*int	listen		args( ( int s, int backlog ) );*/
// int	read		args( ( int fd, char *buf, int nbyte ) );
int	select		args( ( int width, fd_set *readfds, fd_set *writefds,
			    fd_set *exceptfds, struct timeval *timeout ) );
int	socket		args( ( int domain, int type, int protocol ) );
// int	write		args( ( int fd, char *buf, int nbyte ) );
#endif

#if	defined(macintosh)
#include <console.h>
#include <fcntl.h>
#include <unix.h>
struct	timeval
{
	time_t	tv_sec;
	time_t	tv_usec;
};
#if	!defined(isascii)
#define	isascii(c)		( (c) < 0200 )
#endif
static	long			theKeys	[4];

int	gettimeofday		args( ( struct timeval *tp, void *tzp ) );
#endif

#if	defined(MIPS_OS)
extern	int		errno;
#endif

#if	defined(MSDOS)
// int	gettimeofday	args( ( struct timeval *tp, void *tzp ) );
int	kbhit		args( ( void ) );
#endif

#if	defined(NeXT)
int	close		args( ( int fd ) );
int	fcntl		args( ( int fd, int cmd, int arg ) );
#if	!defined(htons)
u_short	htons		args( ( u_short hostshort ) );
#endif
#if	!defined(ntohl)
u_long	ntohl		args( ( u_long hostlong ) );
#endif
int	read		args( ( int fd, char *buf, int nbyte ) );
int	select		args( ( int width, fd_set *readfds, fd_set *writefds,
			    fd_set *exceptfds, struct timeval *timeout ) );
int	write		args( ( int fd, char *buf, int nbyte ) );
#endif

#if	defined(sequent)
int	accept		args( ( int s, struct sockaddr *addr, int *addrlen ) );
int	bind		args( ( int s, struct sockaddr *name, int namelen ) );
int	close		args( ( int fd ) );
int	fcntl		args( ( int fd, int cmd, int arg ) );
int	getpeername	args( ( int s, struct sockaddr *name, int *namelen ) );
int	getsockname	args( ( int s, struct sockaddr *name, int *namelen ) );
int	gettimeofday	args( ( struct timeval *tp, struct timezone *tzp ) );
#if	!defined(htons)
u_short	htons		args( ( u_short hostshort ) );
#endif
int	listen		args( ( int s, int backlog ) );
#if	!defined(ntohl)
u_long	ntohl		args( ( u_long hostlong ) );
#endif
int	read		args( ( int fd, char *buf, int nbyte ) );
int	select		args( ( int width, fd_set *readfds, fd_set *writefds,
			    fd_set *exceptfds, struct timeval *timeout ) );
int	setsockopt	args( ( int s, int level, int optname, caddr_t optval,
			    int optlen ) );
int	socket		args( ( int domain, int type, int protocol ) );
int	write		args( ( int fd, char *buf, int nbyte ) );
#endif

/* This includes Solaris Sys V as well */
#if defined(sun)
int	accept		args( ( int s, struct sockaddr *addr, int *addrlen ) );
int	bind		args( ( int s, struct sockaddr *name, int namelen ) );
void	bzero		args( ( char *b, int length ) );
int	close		args( ( int fd ) );
int	getpeername	args( ( int s, struct sockaddr *name, int *namelen ) );
int	getsockname	args( ( int s, struct sockaddr *name, int *namelen ) );
int	gettimeofday	args( ( struct timeval *tp, struct timezone *tzp ) );
int	listen		args( ( int s, int backlog ) );
int	read		args( ( int fd, char *buf, int nbyte ) );
int	select		args( ( int width, fd_set *readfds, fd_set *writefds,
			    fd_set *exceptfds, struct timeval *timeout ) );
#if defined(SYSV)
int setsockopt		args( ( int s, int level, int optname,
			    const char *optval, int optlen ) );
#else
int	setsockopt	args( ( int s, int level, int optname, void *optval,
			    int optlen ) );
#endif
int	socket		args( ( int domain, int type, int protocol ) );
int	write		args( ( int fd, char *buf, int nbyte ) );
#endif

#if defined(ultrix)
int	accept		args( ( int s, struct sockaddr *addr, int *addrlen ) );
int	bind		args( ( int s, struct sockaddr *name, int namelen ) );
void	bzero		args( ( char *b, int length ) );
int	close		args( ( int fd ) );
int	getpeername	args( ( int s, struct sockaddr *name, int *namelen ) );
int	getsockname	args( ( int s, struct sockaddr *name, int *namelen ) );
int	gettimeofday	args( ( struct timeval *tp, struct timezone *tzp ) );
int	listen		args( ( int s, int backlog ) );
int	read		args( ( int fd, char *buf, int nbyte ) );
int	select		args( ( int width, fd_set *readfds, fd_set *writefds,
			    fd_set *exceptfds, struct timeval *timeout ) );
int	setsockopt	args( ( int s, int level, int optname, void *optval,
			    int optlen ) );
int	socket		args( ( int domain, int type, int protocol ) );
int	write		args( ( int fd, char *buf, int nbyte ) );
#endif



/*
 * Global variables.
 */
DESCRIPTOR_DATA *   descriptor_list;	/* All open descriptors		*/
DESCRIPTOR_DATA *   d_next;		/* Next descriptor in loop	*/
FILE *		    fpReserve;		/* Reserved file handle		*/
bool		    god;		/* All new chars are gods!	*/
bool		    merc_down;		/* Shutdown			*/
char *		    g_wizlock = NULL;	/* Game is wizlocked		*/
bool		    newlock;		/* Game is newlocked		*/
char		    str_boot_time[MAX_INPUT_LENGTH];
time_t		    current_time;	/* time of this pulse */	
unsigned short	    port;

HOST_DATA *	    host_list = NULL;

void segv_restart(int segv_num);

/*
 * OS-dependent local functions.
 */
#if defined(macintosh) || defined(MSDOS)
void	game_loop_mac_msdos	args( ( void ) );
#endif

#if defined(unix) || defined(WIN32)
void	game_loop_unix		args( ( unsigned int control ) );
int	init_socket		args( ( unsigned short pnum ) );
void	init_descriptor		args( ( int control ) );
#endif

bool	read_from_descriptor	args( ( DESCRIPTOR_DATA *d ) );
bool	write_to_descriptor	args( ( int desc, const char *txt, int length ) );

void	erase_line	args((DESCRIPTOR_DATA *d));

extern	void	do_upcount();
extern	void	lunar_update_char args ((CHAR_DATA *ch) );
extern	void	cleanup();

/*
 * Other local functions (OS-independent).
 */
int 	align_trans		args( (int num) );
int	bit_trans		args( (int num) );
void	good_ok			args( (CHAR_DATA *ch, char *buf) );
void	neutral_ok		args( (CHAR_DATA *ch, char *buf) );
void	evil_ok			args( (CHAR_DATA *ch, char *buf) );
void	lawful_ok		args( (CHAR_DATA *ch, char *buf) );
void	balanced_ok		args( (CHAR_DATA *ch, char *buf) );
void	chaotic_ok		args( (CHAR_DATA *ch, char *buf) );
int	scholar_ok		args( (CHAR_DATA *ch) );
int	templar_ok		args( (CHAR_DATA *ch) );
int	rogue_ok		args( (CHAR_DATA *ch) );
int	warrior_ok		args( (CHAR_DATA *ch) );
int	naturalist_ok		args( (CHAR_DATA *ch) );
int     other_ok		args( (CHAR_DATA *ch) );
int	mentalist_ok		args( (CHAR_DATA *ch) );
void    show_avail_classes      args( (DESCRIPTOR_DATA *d, CHAR_DATA *ch) );
void    show_avail_spheres      args( (DESCRIPTOR_DATA *d, CHAR_DATA *ch, 
                                           char *sphsel));
int     get_avail_aligns        args( (CHAR_DATA *ch) );
int     get_avail_ethos         args( (CHAR_DATA *ch) );
void    show_avail_aligns       args( (DESCRIPTOR_DATA *d, CHAR_DATA *ch) );
void    show_avail_ethos        args( (DESCRIPTOR_DATA *d, CHAR_DATA *ch) );
bool    is_avail_hometown	args( (CHAR_DATA *ch, int hometown) );
void	show_avail_hometown	args( (DESCRIPTOR_DATA *d, CHAR_DATA *ch) );

bool	check_parse_name	args( ( char *name, bool long_name ) );
bool	check_reconnect		args( ( DESCRIPTOR_DATA *d, const char *name, bool fConn ) );
bool	validcolour		args( ( CHAR_DATA *ch, char *argument ) );
bool	check_playing		args( ( DESCRIPTOR_DATA *d, const char *name ) );
int	main			args( ( int argc, char **argv ) );
void	nanny			args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	process_output		args( ( DESCRIPTOR_DATA *d, bool fPrompt ) );
void	read_from_buffer	args( ( DESCRIPTOR_DATA *d ) );
void	stop_idling		args( ( CHAR_DATA *ch ) );
void    bust_a_prompt           args( ( CHAR_DATA *ch ) );
int	roll_stat		args( ( CHAR_DATA *ch, int stat) );
void	add_hostdata		args( ( DESCRIPTOR_DATA *d ) );
void	check_host		args( ( DESCRIPTOR_DATA *d ) );
void	send_host_warning	args( ( DESCRIPTOR_DATA *d ) );
void	add_socket_info		args( ( CHAR_DATA *ch, char *host ) );

ACCOUNT_DATA *find_account	args( ( char *argument ) );

extern	void	save_helptext	args( ( HELP_DATA *pHelp ) );
extern	void	append_note	args( (NOTE_DATA *pnote) );

/* int main( int argc, char **argv ) */
/* { */

/* /\* start tracing allocations and deallocations *\/ */
/* //mtrace(); */
/* //COMMENTED OUT THE ABOVE FOR NOW.  -Kestrel 09 Mar 2009 */

/* object_list = NULL; */
/* //    struct timeval now_time; */

/* #if defined(unix) */
/*     char buf[MAX_STRING_LENGTH]; */
/* #endif */

/*     /\* */
/*      * Memory debugging if needed. */
/*      *\/ */
/* #if defined(MALLOC_DEBUG) */
/*     malloc_debug( 2 ); */
/* #endif */


/*     /\* */
/*      * Init time. */
/*      *\/ */
/* //    gettimeofday( &now_time, NULL ); */
/* //    current_time 	= (time_t) now_time.tv_sec; */

/*     current_time = time(NULL); */
/*     strcpy( str_boot_time, ctime( &current_time ) ); */

/*     /\* */
/*      * Macintosh console initialization. */
/*      *\/ */
/* #if defined(macintosh) */
/*     console_options.nrows = 31; */
/*     cshow( stdout ); */
/*     csetmode( C_RAW, stdin ); */
/*     cecho2file( "log file", 1, stderr ); */
/* #endif */

/* #if defined(unix) */
/*     sprintf(buf, "rm %s", ROOMSAVELIST); */
/*     system(buf); */
/*     sprintf(buf, "ls %s > %s", ROOMSAVE_DIR, ROOMSAVELIST); */
/*     system(buf); */
/* #endif */


/*     /\* */
/*      * Reserve one channel for our use. */
/*      *\/ */
/*     if ( ( fpReserve = fopen( NULL_FILE, "r" ) ) == NULL ) */
/*     { */
/* 	perror( NULL_FILE ); */
/* 	exit( 1 ); */
/*     } */

/*     /\* */
/*      * Get the port number. */
/*      *\/ */
/*     port = 4000; */
/*     if ( argc > 1 ) */
/*     { */
/* 	int i; */

/* 	for (i = 1; i < argc; i++) */
/* 	{ */
/* 	    if (!str_cmp(argv[i], "-no-mysql")) */
/* 		global_option_nomysql = TRUE; */
/* 	    else if ( !is_number( argv[i] ) ) */
/* 	    { */
/* 	        fprintf( stderr, "Usage: %s <options> [port #]\n", argv[0] ); */
/* 	        exit( 1 ); */
/* 	    } */
/* 	    else if ( ( port = atoi( argv[i] ) ) <= 1024 ) */
/* 	    { */
/* 	        fprintf( stderr, "Port number must be above 1024.\n" ); */
/* 	        exit( 1 ); */
/* 	    } */
/* 	} */
/*     } */

/*     /\* */
/*      * Run the game. */
/*      *\/ */
/* #if defined(macintosh) || defined(MSDOS) */
/*     boot_db( ); */
/*     log_string( "Avendar is ready to rock." ); */
/*     game_loop_mac_msdos( ); */
/* #endif */

/* #if defined(unix) || defined(WIN32) */
/*     control = init_socket( port ); */
/*     boot_db( ); */
/*     sprintf( log_buf, "Avendar is ready to rock on port %d.", port); */
/*     log_string( log_buf ); */

/* /\* Frequent crashes: not your friend. We're going to stack SEGVs */
/*    on top of each other til they burst, and instead, we'll */
/*    just restart the loop on a dead command *\/ */

/* //    signal(SIGSEGV, segv_restart); */
/*     segv_counter = 0; */
/*     segv_cmd[0] = (char)0; */
/*     segv_char[0] = (char)0;  */
/*     game_loop_unix( control ); */
/*     close (control); */

/*     // Adding some cleanup here to be a responsible citizen. */
/*     cleanup(); */
    
/* #endif */

/*     /\* */
/*      * That's all, folks. */
/*      *\/ */
/*     log_string( "Normal termination of game." ); */
/*     exit( 0 ); */
/*     return 0; */
/* } */


#ifdef _WIN32
void gettimeofday(struct timeval* t,void* timezone)
{       struct _timeb timebuffer;
        _ftime( &timebuffer );
        t->tv_sec=timebuffer.time;
        t->tv_usec=1000*timebuffer.millitm;
}
#endif

#if defined(unix) || defined(WIN32)
int init_socket( unsigned short pnum )
{
    static struct sockaddr_in sa_zero;
    struct sockaddr_in sa;
    int x = 1;
    int fd;

    if ( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
	perror( "Init_socket: socket" );
	exit( 1 );
    }

    if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
    (char *) &x, sizeof(x) ) < 0 )
    {
	perror( "Init_socket: SO_REUSEADDR" );
	close(fd);
	exit( 1 );
    }

#if defined(SO_DONTLINGER) && !defined(SYSV)
    {
	struct	linger	ld;

	ld.l_onoff  = 1;
	ld.l_linger = 1000;

	if ( setsockopt( fd, SOL_SOCKET, SO_DONTLINGER,
	(char *) &ld, sizeof(ld) ) < 0 )
	{
	    perror( "Init_socket: SO_DONTLINGER" );
	    close(fd);
	    exit( 1 );
	}
    }
#endif

    sa		    = sa_zero;
    sa.sin_family   = AF_INET;
    sa.sin_port	    = htons( pnum );

    if ( bind( fd, (struct sockaddr *) &sa, sizeof(sa) ) < 0 )
    {
	perror("Init socket: bind" );
	close(fd);
	exit(1);
    }


    if ( listen( fd, 3 ) < 0 )
    {
	perror("Init socket: listen");
	close(fd);
	exit(1);
    }

    return fd;
}
#endif



#if defined(macintosh) || defined(MSDOS)
void game_loop_mac_msdos( void )
{
    long int last_time, now_time;
    static DESCRIPTOR_DATA dcon;
    static struct timeval null_time;
    int fdmax;
    fd_set pr_set;
	char buf[MAX_STRING_LENGTH];


//    gettimeofday( &last_time, NULL );
//    current_time = (time_t) last_time.tv_sec;
    last_time = clock();
    current_time = time(NULL);

    /*
     * New_descriptor analogue.
     */
    dcon.descriptor	= 0;
    dcon.connected	= CON_GET_NAME;
    dcon.host		= str_dup( "localhost" );
    dcon.outsize	= 2000;
    dcon.outbuf		= malloc( dcon.outsize );
    g_outbuf_size += d->outsize;
    dcon.next		= descriptor_list;
    dcon.showstr_head	= NULL;
    dcon.showstr_point	= NULL;
    dcon.pEdit		= NULL;			/* OLC */
    dcon.pString	= NULL;			/* OLC */
    dcon.editor		= 0;			/* OLC */
	
    descriptor_list	= &dcon;

    /*
     * Send the greeting.
     */
    show_helpfile(NULL, &dcon, "greeting", 0);

    /* Main loop */
    while ( !merc_down )
    {
	DESCRIPTOR_DATA *d;

//	if (!segv_counter)
//    	  signal(SIGSEGV, segv_restart);
	/*
	 * Process input.
	 */
	for ( d = descriptor_list; d != NULL; d = d_next )
	{
	    d_next	= d->next;
	    d->fcommand	= FALSE;

#if defined(MSDOS)
	    if ( kbhit( ) )
#endif
	    {
		if ( d->character != NULL ) d->character->timer = 0;
        if ( d->original != NULL ) d->original->timer = 0;
		if ( !read_from_descriptor( d ) )
		{
		    if ( d->character != NULL && d->connected == CON_PLAYING)
			save_char_obj( d->character );
		    d->outtop	= 0;
		    close_socket( d );
		    continue;
		}
	    }

	    if (d->character != NULL && d->character->daze > 0)
		--d->character->daze;

	    if ( d->character != NULL && d->character->wait > 0 )
	    {
		--d->character->wait;
		if (IS_NPC(d->character) || !IS_SET(d->character->act, PLR_NOLAG))
		    continue;
	    }

	    read_from_buffer( d );
	    if ( d->incomm[0] != '\0' )
	    {
		d->fcommand	= TRUE;
		stop_idling( d->character );
        stop_idling( d->original);

	        /* OLC */
	        if ( d->showstr_point )
	            show_string( d, d->incomm );
	        else
	        if ( d->pString )
	            string_add( d->character, d->incomm );
	        else
	            switch ( d->connected )
	            {
	                case CON_PLAYING:
			    if ( !run_olc_editor( d ) )
				substitute_alias( d, d->incomm );
			    break;
	                default:
			    nanny( d, d->incomm );
			    break;
	            }

		d->incomm[0]	= '\0';
	    }
	}

	FD_ZERO(&pr_set);
	fdmax = 0;
	
	// Find a set of sockets to poll for PR data... 

	for ( d = descriptor_list; d != NULL; d = d->next )
        {
	    if (d->character && d->character->pcdata
	     && d->character->pcdata->gamesock != INVALID_SOCKET)
	    {
		if (d->character->pcdata->gamesock > fdmax)
		    fdmax = d->character->pcdata->gamesock;

		FD_SET(d->character->pcdata->gamesock, &pr_set);
	    }
	} 

	if (select(fdmax + 1, &pr_set, NULL, NULL, &null_time) > 0)
	{
	    for (d = descriptor_list; d; d = d->next)
	    {
		if (d->character && d->character->pcdata
		 && d->character->pcdata->gamesock != INVALID_SOCKET
		 && FD_ISSET(d->character->pcdata->gamesock, &pr_set))
		    gameserv_process(d->character); // , d->character->pcdata->gamesock);
	    }
	}

	/*
	 * Autonomous game motion.
	 */
	update_handler( );

	/*
	 * Output.
	 */
	for ( d = descriptor_list; d != NULL; d = d_next )
	{
	    d_next = d->next;

	    if ( ( d->fcommand || d->outtop > 0 ) )
	    {
		if ( !process_output( d, TRUE ) )
		{
		    if ( d->character != NULL && d->connected == CON_PLAYING)
			save_char_obj( d->character );
		    d->outtop	= 0;
		    close_socket( d );
		}
	    }
	}



	/*
	 * Synchronize to a clock.
	 * Busy wait (blargh).
	 */
	now_time = last_time;
	for ( ; ; )
	{
//	    int delta;

#if defined(MSDOS) || defined(WIN32)
	    if ( kbhit( ) )
#endif
	    {
		if ( dcon.character != NULL ) dcon.character->timer = 0;
        if ( dcon.original != NULL ) dcon.original->timer = 0;

		if ( !read_from_descriptor( &dcon ) )
		{
		    if ( dcon.character != NULL && dcon.connected == CON_PLAYING)
			save_char_obj( d->character );
		    dcon.outtop	= 0;
		    close_socket( &dcon );
		}
/*
#if defined(MSDOS) || defined(WIN32)
		break;
#endif
*/
	    }

	    now_time = clock();
	    if (((float) (now_time - last_time) / CLOCKS_PER_SEC) > (1.0 / PULSE_PER_SECOND))
		break;
	}
	last_time    = clock();
	current_time = time(NULL);
    }

    return;
}
#endif



#if defined(unix) || defined(WIN32)
void game_loop_unix( unsigned int control )
{
    static struct timeval null_time;
    struct timeval last_time;
    fd_set pr_set;
    unsigned int fdmax;
#if defined(unix)
    signal( SIGPIPE, SIG_IGN );
#endif
    gettimeofday( &last_time, NULL );
    current_time = (time_t) last_time.tv_sec;

    /* Main loop */
    while ( !merc_down )
    {
	fd_set in_set;
	fd_set out_set;
	fd_set exc_set;
	DESCRIPTOR_DATA *d;
	unsigned int maxdesc;
    
#if defined(MALLOC_DEBUG)
	if ( malloc_verify( ) != 1 )
	    abort( );
#endif

	/*
	 * Poll all active descriptors.
	 */

	FD_ZERO( &in_set  );
	FD_ZERO( &out_set );
	FD_ZERO( &exc_set );
	FD_SET( control, &in_set );
	maxdesc	= control;
	for ( d = descriptor_list; d; d = d->next )
	{
	    maxdesc = UMAX( maxdesc, d->descriptor );
	    FD_SET( d->descriptor, &in_set  );
	    FD_SET( d->descriptor, &out_set );
	    FD_SET( d->descriptor, &exc_set );
	}

	if ( select( maxdesc+1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
	{
	    perror( "Game_loop: select: poll" );
	    exit( 1 );
	}

	/*
	 * New connection?
	 */
	/* don't take new connections during the crumbling of items --
	 * could hose up pfiles
	 */
	if ( FD_ISSET( control, &in_set ) && !crumble_process && !rip_process)
	    init_descriptor( control );

	/*
	 * Kick out the freaky folks.
	 */
	for ( d = descriptor_list; d != NULL; d = d_next )
	{
	    d_next = d->next;   
            if (!d->dnsdone) continue; /* still resolving */
	    if ( FD_ISSET( d->descriptor, &exc_set ) )
	    {
		FD_CLR( d->descriptor, &in_set  );
		FD_CLR( d->descriptor, &out_set );
		if ( d->character && d->connected == CON_PLAYING)
		    save_char_obj( d->character );
		d->outtop	= 0;
		close_socket( d );
	    }
	}

	/*
	 * Process input.
	 */
	for ( d = descriptor_list; d != NULL; d = d_next )
	{
	    d_next	= d->next;
	    d->fcommand	= FALSE;
            if (!d->dnsdone) continue; /* still resolving */
	    if ( FD_ISSET( d->descriptor, &in_set ) )
	    {
		if ( d->character != NULL ) d->character->timer = 0;
        if (d->original != NULL) d->original->timer = 0;
		if ( !read_from_descriptor( d ) )
		{
		    FD_CLR( d->descriptor, &out_set );
		    if ( d->character != NULL && d->connected == CON_PLAYING)
			save_char_obj( d->character );
		    d->outtop	= 0;
		    close_socket( d );
		    continue;
		}
	    }

	    if (d->character != NULL && d->character->daze > 0)
		--d->character->daze;
        
		
		if (d->character == NULL)
		{		
		 //  d->newbie = FALSE;
		}
		

	    if ( d->character )
	    {
		if (d->character->shunned_demon)
		{
		    d->character->wait = d->character->shunned_demon->wait;
		    if (d->character->wait > 0)
		        continue;
		}
		else if (d->character->wait > 0)
		{
		    --d->character->wait;

		   if (IS_NPC(d->character) || !IS_SET(d->character->act, PLR_NOLAG))
			continue;
		}
	    }

	    read_from_buffer( d );
	    if ( d->incomm[0] != '\0' )
	    {
		d->fcommand	= TRUE;
		stop_idling( d->character );
        stop_idling(d->original);

		/* OLC */
		if ( d->showstr_point )
	    	    show_string( d, d->incomm );
		else
		    if ( d->pString )
	    		string_add( d->character, d->incomm );
		    else
	    		switch ( d->connected )
	    		{
	        	    case CON_PLAYING:
		    		if ( !run_olc_editor( d ) )
    		        	    substitute_alias( d, d->incomm );
		    		break;
	        	    default:
		    		nanny( d, d->incomm );
		    	    break;
	    		}

		d->incomm[0]	= '\0';
	    }
	}

	FD_ZERO(&pr_set);
	fdmax = 0;
	
	// Find a set of sockets to poll for PR data... 

	for ( d = descriptor_list; d != NULL; d = d->next )
        {
	    if (d->character && d->character->pcdata
	     && d->character->pcdata->gamesock != INVALID_SOCKET)
	    {
		if (d->character->pcdata->gamesock > fdmax)
		    fdmax = d->character->pcdata->gamesock;

		FD_SET(d->character->pcdata->gamesock, &pr_set);
	    }
	} 

	if (select(fdmax + 1, &pr_set, NULL, NULL, &null_time) > 0)
	{
	    for (d = descriptor_list; d; d = d->next)
	    {
		if (d->character && d->character->pcdata
		 && d->character->pcdata->gamesock != INVALID_SOCKET
		 && FD_ISSET(d->character->pcdata->gamesock, &pr_set))
		    gameserv_process(d->character); // , d->character->pcdata->gamesock);
	    }
	}

	/*
	 * Autonomous game motion.
	 */
	update_handler( );


	/*
	 * Output.
	 */
	for ( d = descriptor_list; d != NULL; d = d_next )
	{
	    d_next = d->next;

            if (!d->dnsdone) continue; /* still resolving */
	    if ( ( d->fcommand || d->outtop > 0 )
	    &&   FD_ISSET(d->descriptor, &out_set) )
	    {
		if ( !process_output( d, TRUE ) )
		{
		    if ( d->character != NULL && d->connected == CON_PLAYING)
			save_char_obj( d->character );
		    d->outtop	= 0;
		    close_socket( d );
		}
	    }
	}



	/*
	 * Synchronize to a clock.
	 * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
	 * Careful here of signed versus unsigned arithmetic.
	 */
	{
	    struct timeval now_time;
	    long secDelta;
	    long usecDelta;

	    gettimeofday( &now_time, NULL );
	    usecDelta	= ((int) last_time.tv_usec) - ((int) now_time.tv_usec)
			+ 1000000 / PULSE_PER_SECOND;
	    secDelta	= ((int) last_time.tv_sec ) - ((int) now_time.tv_sec );
	    while ( usecDelta < 0 )
	    {
		usecDelta += 1000000;
		secDelta  -= 1;
	    }

	    while ( usecDelta >= 1000000 )
	    {
		usecDelta -= 1000000;
		secDelta  += 1;
	    }

	    if ( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) )
	    {
		struct timeval stall_time;

		stall_time.tv_usec = usecDelta;
		stall_time.tv_sec  = secDelta;
#if defined(WIN32)
		FD_ZERO(&in_set);
		FD_SET(control, &in_set);
#endif
		if ( select( 0, &in_set, NULL, NULL, &stall_time ) < 0 )
		{
#if !defined(WIN32)
		    perror( "Game_loop: select: stall" );
		    exit( 1 );
#else
		    int i;
		    i = WSAGetLastError();
		    bug("%d", i);
#endif
		}
	    }
	}

	gettimeofday( &last_time, NULL );
	current_time = (time_t) last_time.tv_sec;
    }

    return;
}
#endif



#if defined(unix) || defined(WIN32)
void init_descriptor( int control )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *dnew;
    struct sockaddr_in sock;
//    struct hostent *from;
//    pthread_t tid;
    int desc;
    socklen_t size(sizeof(sock));

    getsockname( control, (struct sockaddr *) &sock, &size );
    if ( ( desc = accept( control, (struct sockaddr *) &sock, &size) ) < 0 )
    {
	perror( "New_descriptor: accept" );
	return;
    }

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

#if defined(unix)
    if ( fcntl( desc, F_SETFL, FNDELAY ) == -1 )
    {
	perror( "New_descriptor: fcntl: FNDELAY" );
	return;
    }
#endif

    /*
     * Cons a new descriptor.
     */
    dnew = new_descriptor();

    dnew->descriptor	= desc;
    dnew->connected	= CON_GET_NAME;
    dnew->showstr_head	= NULL;
    dnew->showstr_point = NULL;
    dnew->outsize	= 2000;
    dnew->pEdit		= NULL;			/* OLC */
    dnew->pString	= NULL;			/* OLC */
    dnew->editor	= 0;			/* OLC */
    dnew->outbuf	= (char*)malloc( dnew->outsize );
    g_outbuf_size += dnew->outsize;
    dnew->dnsdone	= FALSE;
    dnew->warning	= FALSE;

    size = sizeof(sock);
    if ( getpeername( desc, (struct sockaddr *) &sock, &size ) < 0 )
    {
	perror( "New_descriptor: getpeername" );
	dnew->host = str_dup( "(unknown)" );
	dnew->dnsdone = TRUE;
    }
    else
    {
	/*
	 * Would be nice to use inet_ntoa here but it takes a struct arg,
	 * which ain't very compatible between gcc and system libraries.
	 */
	int addr;

	addr = ntohl( sock.sin_addr.s_addr );
	sprintf( buf, "%d.%d.%d.%d",
	    ( addr >> 24 ) & 0xFF, ( addr >> 16 ) & 0xFF,
	    ( addr >>  8 ) & 0xFF, ( addr       ) & 0xFF
	    );
	sprintf( log_buf, "Sock.sinaddr:  %s", buf );
	log_string( log_buf );
//	from = gethostbyaddr( (char *) &sock.sin_addr,
//	sizeof(sock.sin_addr), AF_INET );
//      sprintf(buf2, "%s (%s)", from ? from->h_name : buf, buf);
        sprintf(buf2, "%s (%s)", buf, buf);
        dnew->host = str_dup(buf2);
//	pthread_create(&tid, NULL, (void *)dns_thread_func, (void *)dnew);
	dns_thread_func((void *)dnew);
	check_host(dnew);
    }
	
    /*
     * Swiftest: I added the following to ban sites.  I don't
     * endorse banning of sites, but Copper has few descriptors now
     * and some people from certain sites keep abusing access by
     * using automated 'autodialers' and leaving connections hanging.
     *
     * Furey: added suffix check by request of Nickel of HiddenWorlds.
     */
    if ( check_ban(dnew->host,BAN_ALL))
    {
	write_to_descriptor( desc, ban_message, 0);
	close( desc );
	free_descriptor(dnew);
	return;
    }

    /*
     * Init descriptor data.
     */
    dnew->next			= descriptor_list;
    descriptor_list		= dnew;

    /*
     * Send the greeting.
     */
    {
	show_helpfile(NULL, dnew, "greeting", 0);
	write_to_buffer(dnew, "Under what name shall your deeds be recorded?\n\r",0 );
    }

    return;
}
#endif



void close_socket( DESCRIPTOR_DATA *dclose )
{
    CHAR_DATA *ch, *ich;
    DESCRIPTOR_DATA *d;
    int x;

    if ( dclose->outtop > 0 )
	process_output( dclose, FALSE );

    for (x = 0; x < MAX_SNOOP; x++)
	if (dclose->snoop_by[x])
	    write_to_buffer(dclose->snoop_by[x], "Your victim has left the game.\n\r", 0);

    for ( d = descriptor_list; d != NULL; d = d->next )
	for (x = 0; x < MAX_SNOOP; x++)
	    if (d->snoop_by[x] == dclose)
		d->snoop_by[x] = NULL;

    if ((ch = dclose->character) != NULL)
    {
	sprintf( log_buf, "Closing link to %s.", ch->name );
	log_string( log_buf );

	/* cut down on wiznet spam when rebooting */
	if ( dclose->connected == CON_PLAYING && !merc_down)
	{
	    if (!IS_IMMORTAL(ch))
		add_hostdata(dclose);

        for ( ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room )
        {
            if (!((!can_see(ich,ch)) && (IS_IMMORTAL(ch))))
                act_new("$n's eyes glaze over.",ch,ch,ich,TO_VICT,POS_RESTING);
        }

	    /* Losing link while using possession is no way to avoid penalties, dude */
	    if (IS_NPC(ch) && dclose->original && is_affected(ch, gsn_possession))
	    {
		AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

		affect_strip(ch, gsn_possession);
		dclose->original->hit /= 3;
		WAIT_STATE(dclose->original, UMAX(dclose->original->wait, 5*PULSE_VIOLENCE));

		af.where	= TO_AFFECTS;
		af.type		= gsn_possession;
		af.level	= dclose->original->level;
		af.duration	= 8;
		af.modifier	= 0;
		af.location	= 0;
		af.bitvector	= 0;
		affect_to_char(dclose->original, &af);
	    }

	    wiznet("Net death has claimed $N.",ch,NULL,WIZ_LINKS,0,0);
	    ch->desc = NULL;
	}
	else
	{
	    free_char(dclose->original ? dclose->original : 
		dclose->character );
	}
    }

    if ( dclose->editor == ED_HELP && dclose->pEdit )
	save_helptext((HELP_DATA *) dclose->pEdit);

    if ( d_next == dclose )
	d_next = d_next->next;   

    if ( dclose == descriptor_list )
    {
	descriptor_list = descriptor_list->next;
    }
    else
    {
	DESCRIPTOR_DATA *d;

	for ( d = descriptor_list; d && d->next != dclose; d = d->next )
	    ;
	if ( d != NULL )
	    d->next = dclose->next;
	else
	    bug( "Close_socket: dclose not found.", 0 );
    }

#if defined(WIN32)
    closesocket( dclose->descriptor);
#else
    close( dclose->descriptor );
#endif
    free_descriptor(dclose);
#if defined(MSDOS) || defined(macintosh)
    exit(1);
#endif
    return;
}



bool read_from_descriptor( DESCRIPTOR_DATA *d )
{
    /* Hold horses if pending command already. */
    if ( d->incomm[0] != '\0' )
		return TRUE;

    /* Check for overflow. */
    size_t iStart = strlen(d->inbuf);
    if ( iStart >= sizeof(d->inbuf) - 10 )
    {
	sprintf( log_buf, "%s input overflow!", d->host );
	log_string( log_buf );
	write_to_descriptor( d->descriptor,
	    "\n\r*** PUT A LID ON IT!!! ***\n\r", 0 );
	return FALSE;
    }

    /* Snarf input. */
#if defined(macintosh)
    for ( ; ; )
    {
	int c;
	c = getc( stdin );
	if ( c == '\0' || c == EOF )
	    break;
	putc( c, stdout );
	if ( c == '\r' )
	    putc( '\n', stdout );
	d->inbuf[iStart++] = c;
	if ( iStart > sizeof(d->inbuf) - 10 )
	    break;
    }
#endif

    for ( ; ; )
    {
	int nRead;

	nRead = recv( d->descriptor, d->inbuf + iStart,
	    sizeof(d->inbuf) - 10 - iStart, 0 );
	if ( nRead > 0 )
	{
	    iStart += nRead;
	    if ( d->inbuf[iStart-1] == '\n' || d->inbuf[iStart-1] == '\r' )
	    {
		if (iStart >= 5)
		{
		    int i;

		    for (i = 2; ((d->inbuf[iStart - i] == '\r') || (d->inbuf[iStart-i] == '\n')) && ((iStart - i) >= 4); i++)
			;

		    if (((iStart - i) >= 4) && !str_prefix("clear", &d->inbuf[iStart - i - 4])
                     && (((iStart - i) == 4) || d->inbuf[iStart - i - 5] == '\n' || d->inbuf[iStart - i - 5] == '\r'))
		    {
		        iStart = 0;
		        d->incomm[0] = '\0';
		        write_to_descriptor(d->descriptor, "Buffer cleared.\n\r", 0);
			d->fcommand = TRUE;
		    }
		}
		break;
	    }
	}
	else if ( nRead == 0 )
	{
	    log_string( "EOF encountered on read." );
	    return FALSE;
	}
	else if ( errno == EWOULDBLOCK )
	    break;
	else
	{
	    perror( "Read_from_descriptor" );
	    return FALSE;
	}
    }

    d->inbuf[iStart] = '\0';
    return TRUE;
}



/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer( DESCRIPTOR_DATA *d )
{
    int i, j, k;

    /*
     * Hold horses if pending command already.
     */
    if ( d->incomm[0] != '\0' )
	return;

    /*
     * Look for at least one new line.
     */
    for ( i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ )
    {
	if ( d->inbuf[i] == '\0' )
	    return;
    }

    /*
     * Canonical input processing.
     */
    for ( i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r' && i < MAX_INPUT_LENGTH; i++ )
    {
	if ( i >= MAX_INPUT_LENGTH - 2 )
	{
	    write_to_descriptor( d->descriptor, "Line too long.\n\r", 0 );

	    /* skip the rest of the line */
	    for ( ; d->inbuf[i] != '\0'; i++ )
	    {
		if ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
		    break;
	    }
	    d->inbuf[i]   = '\n';
	    d->inbuf[i+1] = '\0';
	    break;
	}

	if ( d->inbuf[i] == '\b' && k > 0 )
	    --k;
	else if ( isascii(d->inbuf[i]) && isprint(d->inbuf[i]) )
	    d->incomm[k++] = d->inbuf[i];
    }

    /*
     * Finish off the line.
     */
    if ( k == 0 )
	d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    /*
     * Deal with bozos with #repeat 1000 ...
     */

    if ( k > 1 || d->incomm[0] == '!' )
    {
    	if ( d->incomm[0] != '!' && strcmp( d->incomm, d->inlast ) )
	{
	    d->repeat = 0;
	}
	else
	{
	    if (++d->repeat >= 25 && d->character
	    &&  d->connected == CON_PLAYING)
	    {
		sprintf( log_buf, "%s input spamming!", d->host );
		log_string( log_buf );
		wiznet("Spam spam spam $N spam spam spam spam spam!",
		       d->character,NULL,WIZ_SPAM,0,get_trust(d->character));
		if (d->incomm[0] == '!')
		    wiznet(d->inlast,d->character,NULL,WIZ_SPAM,0,
			get_trust(d->character));
		else
		    wiznet(d->incomm,d->character,NULL,WIZ_SPAM,0,
			get_trust(d->character));

		d->repeat = 0;
	    }
	}
    }


    /*
     * Do '!' substitution.
     */
    if ( d->incomm[0] == '!' )
	strcpy( d->incomm, d->inlast );
    else
	strcpy( d->inlast, d->incomm );

    /*
     * Shift the input buffer.
     */
    while ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
	i++;
    for ( j = 0; ( d->inbuf[j] = d->inbuf[i+j] ) != '\0'; j++ )
	;
    return;
}


bool validcolour( CHAR_DATA *ch, char *argument )
{
    char *ptr;

	if (ch->level > 51)
		return TRUE;

	if (IS_NPC(ch))
		return TRUE;

 ptr = argument;
 while (1)
	{
	if (*ptr == '\0')
		return TRUE;
	if (*ptr == '{')
		return FALSE;
	ptr++;
	}
}

/*
 * Low level output function.
 */
bool process_output( DESCRIPTOR_DATA *d, bool fPrompt )
{
    extern bool merc_down;
    int x;

    /*
     * Bust a prompt.
     */
    if ( !merc_down && (!d->character || !d->character->shunned_demon) )
	if ( d->showstr_point )
	    write_to_buffer( d, "[Hit Return to continue]\n\r", 0 );
	else if ( fPrompt && d->pString ) // && d->connected == CON_PLAYING )
	    write_to_buffer( d, "> ", 2 );
	else if ( fPrompt && d->connected == CON_PLAYING )
	{
	    CHAR_DATA *ch;
	    CHAR_DATA *victim;

	    ch = d->character;

            /* battle prompt */
            if ((ch != NULL)
	     && (victim = ch->fighting) != NULL
	     && can_see(ch,victim)
	     && (ch->in_room == victim->in_room))
            {
                int percent;
            	char wound[100];
	    	char *pbuff;
	    	char buf[MAX_STRING_LENGTH];
	    	char buffer[MAX_STRING_LENGTH*2];
 
            	if (victim->max_hit > 0)
                    percent = victim->hit * 100 / victim->max_hit;
            	else
                    percent = -1;
 
            	if (percent >= 100)
                    sprintf(wound,"is in excellent condition.");
            	else if (percent >= 90)
                    sprintf(wound,"has a few scratches.");
            	else if (percent >= 75)
                    sprintf(wound,"has some small wounds and bruises.");
            	else if (percent >= 50)
                    sprintf(wound,"has quite a few wounds.");
            	else if (percent >= 30)
                    sprintf(wound,"has some big nasty wounds and scratches.");
            	else if (percent >= 15)
                    sprintf(wound,"looks pretty hurt.");
            	else if (percent >= 0)
                    sprintf(wound,"is in awful condition.");
            	else
                    sprintf(wound,"is bleeding to death.");
 
                sprintf(buf,"%s %s \n\r", 
	            IS_NPC(victim) ? victim->short_descr : 
                    is_affected(victim, gsn_hooded) ? victim->short_descr :
                    victim->name,wound);
	        buf[0]	= UPPER( buf[0] );
	        pbuff	= buffer;
	        colourconv( pbuff, buf, d->character );
                write_to_buffer( d, buffer, 0);
            }


	    ch = d->original ? d->original : d->character;
	    if (!IS_SET(ch->comm, COMM_COMPACT) )
	        write_to_buffer( d, "\n\r", 2 );

            if ( IS_SET(ch->comm, COMM_PROMPT) )
                bust_a_prompt( d->character );

	    if (IS_SET(ch->comm,COMM_TELNET_GA))
	        write_to_buffer(d,go_ahead_str,0);
        }

    /*
     * Short-circuit if nothing to write.
     */
    if ( d->outtop == 0 )
	return TRUE;

    /*
     * Snoop-o-rama.
     */
    if (d->snooped)
    {
        for (x = 0; x < MAX_SNOOP; x++)
        {
	    if (!d->snoop_by[x] || d->snoop_type[x] == SNOOP_COMM
	     || ((d->snoop_type[x] == SNOOP_BRIEF) && (global_bool_sending_brief == TRUE)))
	        continue;
	    if (d && d->character != NULL)
	        write_to_buffer( d->snoop_by[x], d->character->name,0);
	    write_to_buffer( d->snoop_by[x], "> ", 2 );
	    write_to_buffer( d->snoop_by[x], d->outbuf, d->outtop );
	}
    }

    if (d->character && IS_NPC(d->character)
     && (d->character->pIndexData->vnum == MOB_VNUM_SHUNNED_DEMON))
    {
	DESCRIPTOR_DATA *td;

	for (td = descriptor_list; td; td = td->next)
	    if ((td != d) && td->character
	     && (td->character->shunned_demon == d->character))
	     {
		write_to_descriptor(td->descriptor, d->outbuf, d->outtop);
		td->outtop = 0;
	     }
    }	

    if (d->character && IS_OAFFECTED(d->character, AFF_SYM_TARGET))
    {
	AFFECT_DATA *paf;

	for (paf = d->character->affected; paf; paf = paf->next)
	  if (paf->type == gsn_symbiont)
	    write_to_buffer((DESCRIPTOR_DATA *) paf->point, d->outbuf, d->outtop);
    }
    
    if (d->character && d->character->shunned_demon)
	return TRUE;

    /*
     * OS-dependent output.
     */
    if ( !write_to_descriptor( d->descriptor, d->outbuf, d->outtop ) )
    {
	d->outtop = 0;
	return FALSE;
    }
    else
    {
	d->outtop = 0;
	return TRUE;
    }
}

/*
 * Bust a prompt (player settable prompt)
 * coded by Morgenes for Aldara Mud
 */
void bust_a_prompt( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    const char *str;
    const char *i;
    char *point;
    char *pbuff;
//    char *pl;
    char buffer[ MAX_STRING_LENGTH*2 ];
    char doors[MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    bool found, vertigo;
    const char *dir_name[] = {"N","E","S","W","U","D"};
    int door;
 
    point = buf;

    if (!ch->prompt || ch->prompt[0] == '\0')
    {
	if (ch->prompt)
	    free_string(ch->prompt);

	ch->prompt = str_dup("{c<%hhp %mm %vmv>{x ");
    }

    str = ch->prompt;
   if (IS_SET(ch->comm,COMM_AFK))
   {
	send_to_char("<AFK> ",ch);
	return;
   }

   while( *str != '\0' )
   {
      if( *str != '%' )
      {
         *point++ = *str++;
         continue;
      }
      ++str;
      switch( *str )
      {
         default :
            i = " "; break;
	 case 'e':
	    if ( check_blind( ch ) || !ch->in_room || !can_see_in_room(ch, ch->in_room) 
        || is_affected(ch, gsn_visions) 
        || (area_is_affected(ch->in_room->area, gsn_brume) && number_percent() > get_skill(ch, gsn_stormmastery)))
	    {
		strcpy(buf2, "???");
		i = buf2;
		break;
	    }

	    doors[0] = '\0';

	    vertigo = is_affected(ch, gsn_vertigo); 
        {
            AFFECT_DATA * fugue(get_affect(ch, gsn_fugue));
            if (fugue != NULL && fugue->location != APPLY_NONE && number_bits(6) == 0)
                vertigo = true;
        }

	    found = FALSE;
	    for ( door = 0; door <= 5; door++ )
	    {
		if ( ( pexit = ch->in_room->exit[door] ) != NULL
		&&   pexit->u1.to_room != NULL
		&&   can_see_room(ch,pexit->u1.to_room) 
		&&   !IS_SET(pexit->exit_info, EX_CLOSED)
		&&   !IS_SET(pexit->exit_info, EX_SECRET) )
		{
		    found = TRUE;
		    if (vertigo && number_bits(2) == 0) 
    			strcat( doors, dir_name[number_range(0, 5)] );
		    else
	    		strcat( doors, dir_name[door] );
		}
	    }
	
	    if (!found)
	 	strcat(doors,"none");
	    
            sprintf(buf2,"%s",doors);
	    i = buf2; break;
 	 case 'c' :
	    sprintf(buf2,"%s","\n\r");
	    i = buf2; break;
         case 'h' :
            sprintf( buf2, "%d", ((IS_NAFFECTED(ch, AFF_ASHURMADNESS) || IS_AFFECTED(ch, AFF_BERSERK) || is_loc_not_affected(ch, gsn_anesthetize, APPLY_NONE)) ? 0 : (ch->hit + ch->fake_hit) ) );
            i = buf2; break;
         case 'H' :
            sprintf( buf2, "%d", ch->max_hit );
            i = buf2; break;
         case 'm' :
            sprintf( buf2, "%d", ch->mana );
            i = buf2; break;
         case 'M' :
            sprintf( buf2, "%d", ch->max_mana );
            i = buf2; break;
         case 'v' :
            sprintf( buf2, "%d", ch->move );
            i = buf2; break;
         case 'V' :
            sprintf( buf2, "%d", ch->max_move );
            i = buf2; break;
         case 'x' :
            sprintf( buf2, "%d", ch->exp );
            i = buf2; break;
	 case 'X' :
	    sprintf(buf2, "%d", IS_NPC(ch) ? 0 :
            exp_on_level(ch,ch->level+1)-ch->exp);
	    i = buf2; break;
         case 'a' :
            if( ch->level > 9 )
               sprintf( buf2, "%d", ch->alignment );
            else
               sprintf( buf2, "%s", IS_GOOD(ch) ? "good" : IS_EVIL(ch) ?
                "evil" : "neutral" );
            i = buf2; break;
         case 'r' :
            if( ch->in_room != NULL )
               sprintf( buf2, "%s", 
		((!IS_NPC(ch) && IS_SET(ch->act,PLR_HOLYLIGHT)) ||
		 (!check_blind(ch) && !room_is_dark( ch->in_room )))
		? (is_affected(ch, gsn_visions) ? "Visions" : ch->in_room->name) : "darkness");
            else
               sprintf( buf2, " " );
            i = buf2; break;
         case 'R' :
            if( IS_IMMORTAL( ch ) && ch->in_room != NULL )
               sprintf( buf2, "%d", ch->in_room->vnum );
            else
               sprintf( buf2, " " );
            i = buf2; break;
         case 'z' :
            if( IS_IMMORTAL( ch ) && ch->in_room != NULL )
               sprintf( buf2, "%s", ch->in_room->area->name );
            else
               sprintf( buf2, " " );
            i = buf2; break;
         case '%' :
            sprintf( buf2, "%%" );
            i = buf2; break;
         case 'o' :
            sprintf( buf2, "%s", olc_ed_name(ch) );
            i = buf2; break;
	 case 'O' :
	    sprintf( buf2, "%s", olc_ed_vnum(ch) );
	    i = buf2; break;
	 case 'p' :
	    sprintf( buf2, "%d", ch->ep );
	    i = buf2; break;
	 case 'P' :
	    sprintf( buf2, "%d", ch->ep < ep_table[ch->level+1] ?
		ep_table[ch->level+1] - ch->ep : 0 );
	    i = buf2; break;
	 case 'l' :
	    sprintf( buf2, "%s", lang_data[ch->speaking].name);
	    i = buf2; break;
	 case 't' :
	    sprintf( buf2, "%d:%s", time_info.hour, time_info.half ? "30" : "00");
	    i = buf2; break;
         case 's' :
	    switch(ch->position)
	    {
	    	case POS_DEAD: sprintf(buf2, "dead"); break;
		case POS_MORTAL:
		case POS_INCAP:
		case POS_STUNNED: sprintf(buf2, "near death"); break;
		case POS_SLEEPING: sprintf(buf2, "sleeping"); break;
		case POS_RESTING: sprintf(buf2, "resting"); break;
		case POS_SITTING: sprintf(buf2, "sitting"); break;
		case POS_FIGHTING:
		case POS_STANDING: if(is_flying(ch)) sprintf(buf2, "flying");
		            	   else sprintf(buf2, "standing");
				   break;
	    }
	    i = buf2; break;
	  case 'T' :
	  {
	    if (ch->in_room)
	    	switch (ch->in_room->sector_type)
		{
		    case SECT_INSIDE: sprintf(buf2,"inside"); break;
		    case SECT_CITY: sprintf(buf2,"city"); break;
		    case SECT_FIELD: sprintf(buf2,"field"); break;
		    case SECT_FOREST: sprintf(buf2,"forest"); break;
		    case SECT_HILLS: sprintf(buf2,"hills"); break;
		    case SECT_MOUNTAIN: sprintf(buf2,"mountain"); break;
		    case SECT_WATER_SWIM:
		    case SECT_WATER_NOSWIM: sprintf(buf2,"on water"); break;
		    case SECT_AIR: sprintf(buf2,"in air"); break;
		    case SECT_DESERT: sprintf(buf2,"desert"); break;
		    case SECT_UNDERWATER: sprintf(buf2,"underwater"); break;
		    case SECT_UNDERGROUND: sprintf(buf2,"underground"); break;
		    case SECT_ROAD: sprintf(buf2,"road"); break;
		    case SECT_SWAMP: sprintf(buf2,"swamp"); break;
		    default: sprintf(buf2,"???"); break;
		}
	    i = buf2; break;
	  }
      }
      ++str;
      while( (*point = *i) != '\0' )
         ++point, ++i;
   }
   *point	= '\0';

    pbuff	= buffer;
    colourconv( pbuff, buf, ch );
    write_to_buffer( ch->desc, buffer, 0 );

    if (ch->desc)
	ch->desc->dPrompt = TRUE;

    if (ch->prefix[0] != '\0')
        write_to_buffer(ch->desc,ch->prefix,0);
    return;
}



/*
 * Append onto an output buffer.
 */
void write_to_buffer( DESCRIPTOR_DATA *d, const char *txt, int length )
{
    /*
     * Find length in case caller didn't.
     */
    if ( length <= 0 )
	length = strlen(txt);

    /*
     * Initial \n\r if needed.
     */
    if ( d->outtop == 0 && !d->fcommand )
    {
	d->outbuf[0]	= '\n';
	d->outbuf[1]	= '\r';
	d->outtop	= 2;
    }

    /*
     * Expand the buffer as needed.
     */
    while ( d->outtop + length >= d->outsize )
    {
	char *outbuf;

        if (d->outsize >= 32000)
	{
	    close_socket(d);
	    bug("Buffer overflow. Closing.\n\r",0);
	    return;
 	}
	outbuf      = (char*)malloc( 2 * d->outsize );
	g_outbuf_size += d->outsize;
	strncpy( outbuf, d->outbuf, d->outtop );
	free( d->outbuf);
	d->outbuf   = outbuf;
	d->outsize *= 2;
    }

    /*
     * Copy.
     */
    strcpy( d->outbuf + d->outtop, txt );
    d->outtop += length;
    return;
}



/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_descriptor( int desc, const char *txt, int length )
{
    int iStart;
    int nWrite;
    int nBlock;

#if defined(macintosh) || defined(MSDOS)
    if ( desc == 0 )
	desc = 1;
#endif

    if ( length <= 0 )
	length = strlen(txt);

    for ( iStart = 0; iStart < length; iStart += nWrite )
    {
	nBlock = UMIN( length - iStart, 4096 );
	if ( ( nWrite = send( desc, txt + iStart, nBlock, 0 ) ) < 0 )
	    { perror( "Write_to_descriptor" ); return FALSE; }
    } 

    return TRUE;
}


void show_avail_classes(DESCRIPTOR_DATA *d, CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    int i;

    write_to_buffer(d, "\n\rThe professions available to your character are:\n\r", 0);
    sprintf(buf, "[");
    for (i = 0; i < MAX_CLASS; i++)
        if ((class_table[i].races_allowed[ch->race] == 1) && (ch->class_num == class_table[i].class_group))
	    sprintf(buf, "%s %s", buf, class_table[i].name);
    sprintf(buf, "%s ] (or 'back' to choose a new profession) \n\r", buf);
    write_to_buffer(d, buf, 0);
    if (d->newbie && ch->class_num == PROF_OTHER)
	write_to_buffer(d, "(Please note that alchemists can be very difficult for beginners.)\n\r", 0);
    write_to_buffer(d, "Which of these professions do you wish to follow? ", 0);
    return;
}

void show_avail_spheres(DESCRIPTOR_DATA *d, CHAR_DATA *ch, char *sphsel)
{
    write_to_buffer(d, "\n\r",0);
    write_to_buffer(d, "You must now pick your ", 0);
    write_to_buffer(d, sphsel, 0);
    write_to_buffer(d, " sphere.\n\r", 0);
    write_to_buffer(d, "Valid choices are: [",0);
    if (pc_race_table[ch->race].allowed_aligns != 4
     && pc_race_table[ch->race].allowed_ethos  != 4)
        write_to_buffer(d, " Water", 0);
    if (IS_SET(pc_race_table[ch->race].allowed_aligns, BIT_GOOD)
      || ((ch->race == global_int_race_nefortu) 
        && ((ch->class_num == PROF_SCHOLAR) || (ch->class_num == global_int_class_spiritscholar)))
      && pc_race_table[ch->race].allowed_ethos  != 1)
        write_to_buffer(d, " Spirit", 0);
    if (pc_race_table[ch->race].allowed_ethos  != 4)
        write_to_buffer(d, " Earth", 0);
    if (pc_race_table[ch->race].allowed_ethos  != 1)
        write_to_buffer(d, " Air", 0);
    if (pc_race_table[ch->race].allowed_aligns != 1 
     && pc_race_table[ch->race].allowed_ethos  != 4)
        write_to_buffer(d, " Void", 0);
    if (pc_race_table[ch->race].allowed_aligns != 1
     && pc_race_table[ch->race].allowed_ethos  != 1)
        write_to_buffer(d, " Fire", 0);
    write_to_buffer(d, " ]\n\r", 0);
    if (d->newbie && ch->class_num == PROF_SCHOLAR
      && (pc_race_table[ch->race].allowed_aligns != 1
	&& pc_race_table[ch->race].allowed_ethos != 1))
	write_to_buffer(d, "(Please note that Void scholars are very difficult for beginners.)\n\r", 0);
    if (d->newbie && ch->class_num < 6
      && !strcmp(sphsel,"minor"))
	write_to_buffer(d, "(Please note that scholars with mixed spheres can be somewhat more difficult for beginners.)\n\r", 0);
    write_to_buffer(d, "Select a sphere: (or 'back' to choose a new profession) ", 0);

    return;
}

int get_avail_aligns(CHAR_DATA *ch)
{
    int alignnum;

    alignnum = pc_race_table[ch->race].allowed_aligns;
    alignnum &= class_table[ch->class_num].align_allowed;
    if (ch->class_num < 6)
    {
        if ((ch->pcdata->minor_sphere == SPH_WATER) || (ch->pcdata->minor_sphere == SPH_SPIRIT))
	    REMOVE_BIT(alignnum, BIT_EVIL);
	if ((ch->pcdata->minor_sphere == SPH_VOID) || (ch->pcdata->minor_sphere == SPH_FIRE))
	    REMOVE_BIT(alignnum, BIT_GOOD);
    }
    if (ch->class_num == global_int_class_druid)
    {
	if (ch->pcdata->minor_sphere == SPH_GAMALOTH)
	    REMOVE_BIT(alignnum, BIT_GOOD);
	if (ch->pcdata->minor_sphere == SPH_FIRIEL)
	    REMOVE_BIT(alignnum, BIT_EVIL);
    }
    return alignnum;
}

int get_avail_ethos(CHAR_DATA *ch)
{
    int ethosnum;

    ethosnum = pc_race_table[ch->race].allowed_ethos;
    ethosnum &= class_table[ch->class_num].ethos_allowed;
    if (ch->class_num < 6)
        if ((ch->pcdata->minor_sphere == SPH_WATER) || (ch->pcdata->minor_sphere == SPH_VOID) || (ch->pcdata->minor_sphere == SPH_EARTH))
	    REMOVE_BIT(ethosnum, BIT_CHAOTIC);
	if ((ch->pcdata->minor_sphere == SPH_SPIRIT) || (ch->pcdata->minor_sphere == SPH_FIRE) || (ch->pcdata->minor_sphere == SPH_AIR))
	    REMOVE_BIT(ethosnum, BIT_LAWFUL);

    return ethosnum;
}

void show_avail_aligns(DESCRIPTOR_DATA *d, CHAR_DATA *ch)
{
    int aligns;

    aligns = get_avail_aligns(ch);
 
    write_to_buffer(d, "The following alignments are available to your character:\n\r [", 0);
    if (IS_SET(aligns, BIT_GOOD))
	write_to_buffer(d, " Good", 0);
    if (IS_SET(aligns, BIT_NEUTRAL))
	write_to_buffer(d, " Neutral", 0);
    if (IS_SET(aligns, BIT_EVIL))
	write_to_buffer(d, " Evil", 0);

    write_to_buffer(d, " ]\n\r", 0);
    write_to_buffer(d, "Which alignment? ", 0);

    return;
}

void show_avail_ethos(DESCRIPTOR_DATA *d, CHAR_DATA *ch) 
{
    int ethos;

    ethos = get_avail_ethos(ch);
    write_to_buffer(d, "The following ethos choices are available to your character:\n\r [", 0);
    if (IS_SET(ethos, BIT_LAWFUL))
	write_to_buffer(d, " Lawful", 0);
    if (IS_SET(ethos, BIT_BALANCED))
	write_to_buffer(d, " Neutral", 0);
    if (IS_SET(ethos, BIT_CHAOTIC))
	write_to_buffer(d, " Chaotic", 0);

    write_to_buffer(d, " ]\n\r", 0);
    write_to_buffer(d, "Which ethos? ", 0);
    return;
}

bool is_avail_hometown(CHAR_DATA *ch, int hometown)
{
    if (pc_race_table[ch->race].avail_hometowns[hometown]
     && class_table[ch->class_num].avail_hometowns[hometown]
     && ((IS_GOOD(ch) && IS_SET(home_table[hometown].align, BIT_GOOD))
      || (IS_EVIL(ch) && IS_SET(home_table[hometown].align, BIT_EVIL))
      || (IS_NEUTRAL(ch) && IS_SET(home_table[hometown].align, BIT_NEUTRAL)))
     && (((ch->pcdata->ethos == ETH_LAWFUL) && IS_SET(home_table[hometown].ethos, BIT_LAWFUL))
      || ((ch->pcdata->ethos == ETH_CHAOTIC) && IS_SET(home_table[hometown].ethos, BIT_CHAOTIC))
      || ((ch->pcdata->ethos == ETH_NEUTRAL) && IS_SET(home_table[hometown].ethos, BIT_BALANCED))))
	return TRUE;

    return FALSE;
}

bool firiel_ok(CHAR_DATA *ch)
{
   if (ch->race == global_int_race_ethron
     || ch->race == global_int_race_caladaran
     || ch->race == global_int_race_kankoran
     || ch->race == global_int_race_human)
	return TRUE;
   return FALSE;
}

bool lunar_ok(CHAR_DATA *ch)
{
   if (ch->race == global_int_race_ethron
     || ch->race == global_int_race_srryn
     || ch->race == global_int_race_kankoran
     || ch->race == global_int_race_human)
	return TRUE;
   return FALSE;
}

bool gamaloth_ok(CHAR_DATA *ch)
{
   if (ch->race == global_int_race_srryn
     || ch->race == global_int_race_kankoran
     || ch->race == global_int_race_caladaran
     || ch->race == global_int_race_human)
	return TRUE;
   return FALSE;
}

void heirloom_notice(OBJ_DATA *obj)
{
    NOTE_DATA *pnote;
    char *strtime;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    pnote = new_note();

    pnote->sender	= str_dup("Elvis");
    pnote->to_list	= str_dup("immortal");
    pnote->subject  = str_dup("Heirloom created");
    pnote->type	= NOTE_NOTE;

    switch (obj->item_type)
    {
	case ITEM_WEAPON:
	    if (obj->value[0] == WEAPON_SWORD)
	    {
		if (obj->value[3] == DAM_PIERCE)
		    strcpy(buf, "shortsword");
		else if (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS))
		    strcpy(buf, "greatsword");
		else
		    strcpy(buf, "longsword");
	    }
	    else if (obj->value[0] == WEAPON_SPEAR)
	    {
		if (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS))
		    strcpy(buf, "long spear");
		else
		    strcpy(buf, "short spear");
	    }
	    else if (obj->value[0] == WEAPON_AXE)
	    {
		if (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS))
		    strcpy(buf, "battle axe");
		else
		    strcpy(buf, "hand axe");
	    }
	    else if (obj->value[0] == WEAPON_EXOTIC)
	    {
		if (obj->value[3] == DAM_SLASH)
		    strcpy(buf, "slashing");
		else if (obj->value[3] == DAM_PIERCE)
		    strcpy(buf, "piercing");
		else if (obj->value[3] == DAM_BASH)
		    strcpy(buf, "bashing");
	    }
	    else
		strcpy(buf, "");

	    if (buf[0] != '\0')
		sprintf(buf2, "%s (%s)", flag_string(weapon_class, obj->value[0]), buf);
	    else
		strcpy(buf2, flag_string(weapon_class, obj->value[0]));

	    break;

	case ITEM_ARMOR:
	    if (IS_SET(obj->wear_flags, ITEM_WEAR_BODY))
		sprintf(buf2, "torso (%s)", obj->value[0] == 11 ? "armor shirt" : "jerkin");
	    else if (IS_SET(obj->wear_flags, ITEM_WEAR_HEAD))
		sprintf(buf2, "head (%s)", obj->value[1] == 11 ? "helmet" : "cap");
	    else if (IS_SET(obj->wear_flags, ITEM_WEAR_LEGS))
		strcpy(buf2, "legs");
	    else if (IS_SET(obj->wear_flags, ITEM_WEAR_FEET))
		strcpy(buf2, "feet");
	    else if (IS_SET(obj->wear_flags, ITEM_WEAR_HANDS))
		sprintf(buf2, "hands (%s)", obj->value[1] == 11 ? "gauntlets" : "gloves");
	    else if (IS_SET(obj->wear_flags, ITEM_WEAR_SHIELD))
		strcpy(buf2, "shield");
	    else if (IS_SET(obj->wear_flags, ITEM_WEAR_ABOUT))
		strcpy(buf2, "about body");
	    else if (IS_SET(obj->wear_flags, ITEM_WEAR_WAIST))
		strcpy(buf2, "waist");
	    else if (IS_SET(obj->wear_flags, ITEM_WEAR_WRIST))
		strcpy(buf2, "wrist");

	break;

	case ITEM_JEWELRY:
	    if (IS_SET(obj->wear_flags, ITEM_WEAR_FINGER))
		strcpy(buf2, "finger");
	    else if (IS_SET(obj->wear_flags, ITEM_WEAR_NECK))
		strcpy(buf2, "neck");
	    else if (IS_SET(obj->wear_flags, ITEM_WEAR_HEAD))
		strcpy(buf2, "head");
	    else if (IS_SET(obj->wear_flags, ITEM_WEAR_ARMS))
		strcpy(buf2, "arms");
	    else if (IS_SET(obj->wear_flags, ITEM_WEAR_WAIST))
		strcpy(buf2, "waist");
	    else if (IS_SET(obj->wear_flags, ITEM_HOLD))
		strcpy(buf2, "held");

	break;
    }

    sprintf(buf, "%s created the following heirloom:\n\r\n\rType: %s\n\rSub-Type: %s\n\rMaterial: %s\n\r\n\rShort: %s\n\rKeywords: %s\n\r\n\rDescription:\n\r%s",
	    obj->carried_by->name, flag_string(type_flags, obj->item_type), buf2, material_table[obj->material].name, obj->short_descr, obj->name, obj->extra_descr->description);

    pnote->text	= str_dup(buf);

    strtime				= ctime( &current_time );
    strtime[strlen(strtime)-1]	= '\0';
    pnote->date			= str_dup(strtime);
    pnote->date_stamp		= current_time;

    append_note(pnote);

    for (d = descriptor_list; d; d = d->next)
	if ((d->connected == CON_PLAYING) && d->character && IS_IMMORTAL(d->character))
	    send_to_char("New heirloom note waiting.\n\r", d->character);

    return;
}





void show_avail_hometown(DESCRIPTOR_DATA *d, CHAR_DATA *ch)
{
    int i;

    write_to_buffer(d, "You may pick one of the following hometowns:\n\r", 0);
    for (i = 0; i < MAX_HOMETOWN; i++)
	if (is_avail_hometown(ch, i))
	{
	    write_to_buffer(d, home_table[i].name, 0);
	    write_to_buffer(d, "\n\r", 0);
	}
    write_to_buffer(d, "From which location do you hail? ", 0);
    return;
}

char *name_to_keywords(char *argument, char *outstr)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    char *a = argument, *b = buf;
    bool found;
    unsigned int x;

    const char *banned_words[] = { "the", "an", "of", "in", "onto", "under", "around", "a", "called",
			     "named", "one", "crafted", "forged", "made", "from", "with", "out",
			     "many", NULL };

    while (*a != '\0')
    {
        if (*a == ' ' || *a == '-' || isalpha(*a))
	{
	    *b = *a;
	    b++;
	}

	a++;
    }
    *b = '\0';

    strcpy(outstr, "");

    b = buf;
    while (*b != '\0')
    {
	b = one_argument(b, arg);

	found = FALSE;
	for (x = 0; banned_words[x]; x++)
	    if (!str_cmp(banned_words[x], arg))
	    {
		found = TRUE;
		break;
	    }

	if (!found)
	{
	    if (outstr[0] != '\0')
		strcat(outstr, " ");
	    
	    strcat(outstr, arg);
	}
    }
	    
    return outstr;
}

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny( DESCRIPTOR_DATA *d, char *argument )
{
    DESCRIPTOR_DATA *d_old, *d_next, *d_find;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char strArea[MAX_INPUT_LENGTH];
//    FILE *fp;
    CHAR_DATA *ch, *ich;
    char *pwdnew;
    char *p;
    int pp;
    int race,i,weapon,j;
    bool fOld, cFound;
    struct stat s;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    short attr_count = 0;
    
    while ( isspace(*argument) )
	argument++;

    ch = d->character;

    switch ( d->connected )
    {

    default:
	bug( "Nanny: bad d->connected %d.", d->connected );
	close_socket( d );
	return;

    case CON_GET_NAME:
 
	if ( argument[0] == '\0' )
	{
	    close_socket( d );
	    return;
	}

	/* web based time -- woo? woo. */
	if (!strcmp(argument, "webgettime"))
        {
          do_descriptortime(d);
          close_socket(d);
          return;
	}

	for (pp = 1; argument[pp] != '\0'; pp++)
	  argument[pp] = LOWER(argument[pp]);

	argument[0] = UPPER(argument[0]);

	if (!d->acct && ((d->acct = find_account(argument)) != NULL))
	{
	    for (d_find = descriptor_list; d_find; d_find = d_find->next)
	    {
		if ((d_find != d) && d_find->acct && !str_cmp(d->acct->name, d_find->acct->name))
		{
		    sprintf(buf, "Account '%s' already logged in.  Continuing to connect will disconnect existing session.\n\rEnter password to continue: ", d->acct->name);
		    write_to_buffer(d, buf, 0);
		    write_to_buffer(d, echo_off_str, 0);
		    d->connected = CON_GET_ACCT_PASSWORD;
		    return;
		}
	    }

	    sprintf(buf, "Account '%s' logging in.  Password: ", d->acct->name);
	    write_to_buffer( d, buf, 0 );
	    write_to_buffer( d, echo_off_str, 0 );
	    d->connected = CON_GET_ACCT_PASSWORD;
	    return;

	}

	if (!check_parse_name( argument, (bool) ((d->acct && d->acct->award_points > 0) ? TRUE : FALSE)))
	{
	    write_to_buffer( d, "Illegal (or denied) name, try another.\n\rName: ", 0 );
	    return;
	}

	if (dict_lookup(g_denied_names, argument))
	{
	    sprintf(log_buf, "Denying access to %s@%s.", argument, d->host);
	    log_string(log_buf);
	    write_to_buffer(d, "That character profile has been denied.\n\r", 0);
	    close_socket(d);
	    return;
	}

	ch = new_char();
	fOld = load_char_obj( argument, ch );
	d->character = ch;
	ch->desc = d;

	if ((ch->prompt[0] == '\0') && (d->acct) && (d->acct->def_prompt[0] != '\0'))
	{
	    free_string(ch->prompt);
	    ch->prompt = str_dup( d->acct->def_prompt );
	}

	if (fOld && d->acct && !is_name(argument, d->acct->chars))
	{
	    write_to_buffer(d, "Sorry, that character is not part of this account.  Please reconnect and login again.\n\r", 0);
	    close_socket(d);
	    return;
	}

	if (IS_SET(ch->act, PLR_DENY))
	{
	    sprintf( log_buf, "Denying access to %s@%s.", argument, d->host );
	    log_string( log_buf );
	    write_to_buffer( d, "You are denied access.\n\r", 0 );
	    close_socket( d );
	    return;
	}

	if (check_ban(d->host,BAN_PERMIT) && !IS_SET(ch->act,PLR_PERMIT)
         && (!d->acct || (!IS_SET(d->acct->flags, ACCT_NO_BAN) || (fOld && !is_name(ch->name, d->acct->chars)))))
	{
	    write_to_buffer(d,ban_message,0);
	    close_socket(d);
	    return;
	}

	if (IS_SET(ch->act, PLR_ACCOUNT) && !d->acct)
	{
	    write_to_buffer(d, "That character is part of a player account.  Please login using that account.", 0);
	    close_socket(d);
	    return;
	}

	if (check_ban(d->host, BAN_WARNING) && !IS_SET(ch->act, PLR_PERMIT)
         && (!d->acct || (!IS_SET(d->acct->flags, ACCT_NO_BAN) || (fOld && !is_name(ch->name, d->acct->chars)))))
	{
	    write_to_buffer(d, "**************************** WARNING *****************************\n\r\n\r", 0);
	    write_to_buffer(d, "The site you are playing from is currently being watched carefully\n\r", 0);
	    write_to_buffer(d, "due to a recent history of cheating and rules avoidance.  Be aware\n\r", 0);
	    write_to_buffer(d, "that if this continues, your site WILL be banned.  This may not be\n\r", 0);
	    write_to_buffer(d, "a result of your own actions, but others playing Avendar from the\n\r", 0);
	    write_to_buffer(d, "same domain as yourself.  You know who you are.\n\r\n\r", 0);
	    write_to_buffer(d, "******************************************************************\n\r\n\r", 0);
	}


	if ( check_reconnect( d, argument, FALSE ) )
	{
	    fOld = TRUE;
	}
	else
	{
	    if ( g_wizlock && !IS_IMMORTAL(ch)) 
	    {
		write_to_buffer( d, "The game is currently locked.\n\rReason: ", 0 );
		write_to_buffer( d, g_wizlock, 0);
		write_to_buffer( d, ".\n\r", 0);
		close_socket( d );
		return;
	    }
	}

	if (d->warning)
	    send_host_warning(d);	    

	if ( fOld )
	{
	    /* Old player */
	    write_to_buffer( d, "Password: ", 0 );
	    write_to_buffer( d, echo_off_str, 0 );
	    d->connected = CON_GET_OLD_PASSWORD;
	    return;
	}
	else
	{
	    /* New player */
 	    if (newlock)
	    {
                write_to_buffer( d, "The game is newlocked.\n\r", 0 );
                close_socket( d );
                return;
            }

	    if (check_ban(d->host,BAN_NEWBIES))
	    {
		write_to_buffer(d, "New players are not allowed from your site.\n\r",0);
		close_socket(d);
		return;
	    }

	    if (!d->acct)	
	        sprintf( buf, "We don't have a character named %s.\n\rWould you like to create a character named %s (Y/N)? ", argument, argument );
	    else
		sprintf( buf, "We don't have a character named %s.\n\rAttach new character %s to account %s (Y/N)? ", argument, argument, d->acct->name);
	    write_to_buffer( d, buf, 0 );
	    d->connected = CON_CONFIRM_NEW_NAME;
	    return;
	}
	break;


    case CON_GET_ACCT_PASSWORD:
    {
        char aname[MAX_INPUT_LENGTH], *ap;

#if defined(unix)
	write_to_buffer( d, "\n\r", 2 );
#endif

	if ( strcmp( crypt( argument, d->acct->pwd ), d->acct->pwd ))
	{
	    write_to_buffer(d, "Wrong password.\n\r", 0 );
	    close_socket( d );
	    return;
	}

	write_to_buffer(d, echo_on_str, 0);

	for (d_find = descriptor_list; d_find; d_find = d_find->next)
	    if ((d_find != d) && d_find->acct && !str_cmp(d_find->acct->name, d->acct->name))
	    {
		close_socket(d_find);
		break;
	    }

        write_to_buffer(d, "\n\rThe following characters are currently associated with this account:\n\r\n\r", 0);

	ap = one_argument(d->acct->chars, aname);
	memset((void *) buf, 0, MAX_STRING_LENGTH);
	while (aname[0] != '\0')
	{
	    sprintf( strArea, "%s%s", PLAYER_DIR, capitalize( aname ) );
	    if (stat(strArea, &s) == 0)
	    {
	        write_to_buffer(d, capitalize(aname), 0);
	        write_to_buffer(d, "\n\r", 0);
		strcat(buf, " ");
		strcat(buf, capitalize(aname));
	    }
	    else
	    {
		char buf2[MAX_STRING_LENGTH];

		sprintf(buf2, "%s%s (Unknown -- Immortal Intervention or Inactivity Deletion)\n\r", d->acct->deleted, capitalize(aname));
		free_string(d->acct->deleted);
		d->acct->deleted = str_dup(buf2);
	    }
		
	    ap = one_argument(ap, aname);
	}
	free_string(d->acct->chars);
	d->acct->chars = str_dup(buf);
	fwrite_account(d->acct);

	write_to_buffer(d,"\n\rTo create a new character, enter an original name at the following prompt.\n\r",0);
	write_to_buffer(d, "Enter character name to use: ", 0);
	d->connected = CON_GET_NAME;
	return;
    }
    
	break;


    case CON_GET_OLD_PASSWORD:
#if defined(unix)
	write_to_buffer( d, "\n\r", 2 );
#endif

	if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd ))
	{
	    write_to_buffer( d, "Wrong password.\n\r", 0 );
	    close_socket( d );
	    return;
	}
 
	write_to_buffer( d, echo_on_str, 0 );

	if (check_playing(d,ch->name))
	    return;

	if ( check_reconnect( d, ch->name, TRUE ) )
	    return;

	sprintf( log_buf, "%s@%s has connected.", ch->name, d->host );
	log_string( log_buf );
	wiznet(log_buf,NULL,NULL,WIZ_SITES,0,get_trust(ch));

        if (IS_SET(ch->act, PLR_SLOG))
        {
	    sprintf(log_buf, "M: %s has connected.", ch->name);
	    write_slog(ch, log_buf);
	}

	add_socket_info(ch, d->host);

	if ( IS_IMMORTAL(ch) )
	{
	    sprintf(buf, "imotd%d", port);
	    do_help( ch, buf );
	    d->connected = CON_READ_IMOTD;
 	}
	else
	{
	    sprintf(buf, "motd%d", port);
	    do_help( ch, buf );
	    d->connected = CON_READ_MOTD;
	}
	
	break;

/* RT code for breaking link */
 
    case CON_BREAK_CONNECT:
	switch( *argument )
	{
	case 'y' : case 'Y':
            for ( d_old = descriptor_list; d_old != NULL; d_old = d_next )
	    {
		d_next = d_old->next;
		if (d_old == d || d_old->character == NULL)
		    continue;

		if (str_cmp(ch->name,d_old->original ?
		    d_old->original->name : d_old->character->name))
		    continue;

		close_socket(d_old);
	    }
	    if (check_reconnect(d,ch->name,TRUE))
	    	return;
	    write_to_buffer(d,"Reconnect attempt failed.\n\rName: ",0);
            if ( d->character != NULL )
            {
                free_char( d->character );
                d->character = NULL;
            }
	    d->connected = CON_GET_NAME;
	    break;

	case 'n' : case 'N':
	    write_to_buffer(d,"Name: ",0);
            if ( d->character != NULL )
            {
                free_char( d->character );
                d->character = NULL;
            }
	    d->connected = CON_GET_NAME;
	    break;

	default:
	    write_to_buffer(d,"Please type Y or N? ",0);
	    break;
	}
	break;

    case CON_CONFIRM_NEW_NAME:
	switch ( *argument )
	{
	case 'y': case 'Y':
	    sprintf(buf, "New Character.\n\r");
	    write_to_buffer( d, buf, 0 );
	    sprintf(buf, "\n\r************* Name Guidelines *************\n\r");
	    write_to_buffer( d, buf, 0 );
	
	    sprintf(buf, "\n\rAvendar is a medieval-era fantasy role playing game.\n\r");
	    write_to_buffer( d, buf, 0 );
	    sprintf(buf, "All names must fit the following guidelines:\n\r");
	    write_to_buffer( d, buf, 0 );
	    sprintf(buf, "\n\r** The name must be original. Do not use\n\r   dictionary words in any language, names\n\r   borrowed from other fantasy settings,\n\r   including books.\n\r");
	    write_to_buffer( d, buf, 0 );
	    sprintf(buf, "\n\r** Do not use names with capitals mid-name, \n\r   such as MacDugal.\n\r");
	    write_to_buffer( d, buf, 0 );
	    sprintf(buf, "\n\r** The name must be a birth name.\n\r   Do not use a name such as TrollKiller, \n\r   which describes something you might\n\r   do later in life.\n\r");
	    write_to_buffer( d, buf, 0 );
	    sprintf(buf, "\n\rDoes your name fit these conditions? ");
	    write_to_buffer( d, buf, 0 );
	    d->connected = CON_ACCEPT_CONDITIONS;
	    break;

	case 'n': case 'N':
	    write_to_buffer( d, "Ok, what IS it, then? ", 0 );
	    free_char( d->character );
	    d->character = NULL;
	    d->connected = CON_GET_NAME;
	    break;

	default:
	    write_to_buffer( d, "Please type Yes or No? ", 0 );
	    break;
	}
	break;


     case CON_ACCEPT_CONDITIONS:
	switch ( UPPER(*argument) )
	{
	    case 'Y':
#ifdef HERODAY
		if (TRUE)
#else
		if (d->acct && (d->acct->award_points >= 0))
#endif
		{
		    write_to_buffer(d, "\n\rAs an experienced and respected player of Avendar, you have garnered a\n\r", 0);
		    write_to_buffer(d, "sufficient number of 'roleplaying points' to allow you to decide upon your\n\r", 0);
		    write_to_buffer(d, "character's surname during creation.  Fewer restrictions are placed upon this\n\r", 0);
		    write_to_buffer(d, "name, but please use the common sense that you have been judged to have when\n\r", 0);
		    write_to_buffer(d, "choosing a name.  This is a privilege, not a right.\n\r\n\r", 0);
		    write_to_buffer(d, "Please enter a surname (enter for none): ", 0);
		    d->connected = CON_GET_SURNAME;
		}
		else
		{
	   	    write_to_buffer( d, "\n\rDo you wish to use ANSI colour? ", 0 );
		    d->connected = CON_CHOOSE_COLOUR;
		}
		break;

	    case 'N':
	   	free_char(d->character);
	   	close_socket(d);
	   	return;

	    default:
		write_to_buffer( d, "Please enter Yes or No: ", 0);
		break;
	}
	
	break;

     case CON_GET_SURNAME:
	if (argument[0] != '\0')
	{	
	    if (strlen(argument) < 4)
	    {
	        write_to_buffer( d, "Surnames must be at least four characters long.\n\r\n\rSurname: ",		0 );
	        return;
	    }

	    if (ch->pcdata->surname)
	        free_string(ch->pcdata->surname);

	    ch->pcdata->surname = str_dup(argument);

	    sprintf(buf, "\n\r\n\r\"%s %s\", is this correct? ", ch->name, ch->pcdata->surname);
	    write_to_buffer(d, buf, 0);
	    d->connected = CON_CONFIRM_SURNAME;
	    break;
	}

	write_to_buffer(d, "\n\rDo you wish to use ANSI colour? ", 0);
	d->connected = CON_CHOOSE_COLOUR;
	break;

     case CON_CONFIRM_SURNAME:
	switch (UPPER(*argument))
	{
	    case 'Y':
		write_to_buffer(d, "\n\rDo you wish to use ANSI colour? (recommended) ", 0);
		d->connected = CON_CHOOSE_COLOUR;
		break;

	    case 'N':
		write_to_buffer( d, "Ok, what is it, then (enter for none)? ", 0 );
	        free_string( ch->pcdata->surname );
	        ch->pcdata->surname = str_dup("");
		d->connected = CON_GET_SURNAME;
		break;

	    default:
		write_to_buffer(d, "Please enter Yes or No: ", 0);
		break;
	}
	break;
	
     case CON_CHOOSE_COLOUR:
	switch ( UPPER(*argument) )
	{
	    default:
		sprintf(buf, "Please enter 'Y' or 'N'.\n\rDo you wish to use ANSI colour? ");
		write_to_buffer( d, buf, 0 );
		break;

	    case 'Y':
		SET_BIT( ch->act, PLR_COLOUR );
		write_to_buffer( d, "ANSI colour will be used.  You may toggle this anytime in-game with the 'colour' command.\n\r\n\r", 0);
		d->connected = CON_GET_NEW_PASSWORD;
		break;

	    case 'N':
		write_to_buffer( d, "ANSI colour will not be used.  You may toggle this anytime in-game with the 'colour' command.\n\r\n\r", 0);
		d->connected = CON_GET_NEW_PASSWORD;
		break;
	}

	if (d->connected == CON_GET_NEW_PASSWORD)
	{
	    write_to_buffer( d, "\n\rYou will now be prompted to enter a password for your character.\n\r\n\r", 0);

	    write_to_buffer( d, "It is recommended that you avoid using passwords that are easily\n\r", 0);
	    write_to_buffer( d, "guessed by those who know you either in real life or in character.\n\r", 0);
	    write_to_buffer( d, "\"Easy to guess\" includes things like your real life name, your\n\r", 0);
	    write_to_buffer( d, "birthday, your character's name, or any permutation of the above or\n\r", 0);
	    write_to_buffer( d, "similar things. Also try to avoid choosing a dictionary word as\n\r", 0);
	    write_to_buffer( d, "a password. If you absolutely MUST choose a dictionary word, try\n\r", 0);
	    write_to_buffer( d, "to choose something obscure, and to append special characters or\n\r", 0);
	    write_to_buffer( d, "numbers to the password.\n\r\n\r", 0);

	    write_to_buffer(d , "And most importantly, do not forget your password.\n\r", 0);

	    sprintf( buf, "\n\r\n\rEnter a password for %s: %s", ch->name, echo_off_str );
	    write_to_buffer( d, buf, 0 );
	    d->connected = CON_GET_NEW_PASSWORD;
	    break;

	}
	break;

    case CON_GET_NEW_PASSWORD:
#if defined(unix)
	write_to_buffer( d, "\n\r", 2 );
#endif

	if ( strlen(argument) < 5 )
	{
	    write_to_buffer( d,
		"Password must be at least five characters long.\n\rPassword: ",
		0 );
	    return;
	}
//	bug(argument,0);

	pwdnew = crypt( argument, ch->name );
	for ( p = pwdnew; *p != '\0'; p++ )
	{
	    if ( *p == '~' )
	    {
		write_to_buffer( d,
		    "New password not acceptable, try again.\n\rPassword: ",
		    0 );
		return;
	    }
	}

	free_string( ch->pcdata->pwd );
	ch->pcdata->pwd	= str_dup( pwdnew );
	write_to_buffer( d, "Please retype password: ", 0 );
	d->connected = CON_CONFIRM_NEW_PASSWORD;
	break;

    case CON_CONFIRM_NEW_PASSWORD:
#if defined(unix)
	write_to_buffer( d, "\n\r", 2 );
#endif

//	bug(argument,0);
	if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd ) )
	{
	    write_to_buffer( d, "Passwords don't match.\n\rRetype password: ",
		0 );
	    d->connected = CON_GET_NEW_PASSWORD;
	    return;
	}

	write_to_buffer( d, echo_on_str, 0 );

	write_to_buffer(d, "\n\r\n\r[===== Online Help =====]\n\r\n\rHelp is now available to you while you decide\n\rthings about your character.\n\r\n\r",0);
	write_to_buffer(d, "** To access help, type HELP or HELP <topic>.\n\r",0);
	write_to_buffer(d, "\n\rOf particular importance, if you are new,\n\ryou may want to examine 'help race' and\n\r'help class', as you are asked about them.\n\r\n\r", 0);
	write_to_buffer(d, "Are you new to Avendar: The Crucible of Legends? ", 0);
	d->connected = CON_IS_NEWBIE;
	break;


    case CON_ACCOUNT_INFO:
#if defined(unix)
	write_to_buffer( d, "\n\r", 2 );
#endif

	write_to_buffer(d, "Avendar offers an account system so that you may group your characters\n\r", 0);
	write_to_buffer(d, "together, and build up a standing record of your accomplishments.\n\r", 0);
	write_to_buffer(d, "Characters on accounts which have earned good standing through past\n\r", 0);
	write_to_buffer(d, "actions will be eligible for special rewards and benefits that those\n\r", 0);
	write_to_buffer(d, "who choose not to take advantage of our account system may not be.\n\r\n\r", 0);

	write_to_buffer(d, "Accounts are to be used by one person only.  Shared accounts are not\n\r", 0);
	write_to_buffer(d, "acceptible under any circumstances, no excuses.  Therefore, any rules\n\r", 0);
	write_to_buffer(d, "infractions committed by one character affects the standing of the\n\r", 0);
	write_to_buffer(d, "account as a whole.\n\r\n\r", 0);

	write_to_buffer(d, "Note: if you already have an account, please disconnect and re-login\n\r", 0);
      	write_to_buffer(d, "      using your account name at the name prompt.\n\r\n\r", 0);

	write_to_buffer(d, "Do you wish to create a new account (y/n)? ", 0);
#ifdef HERODAY
	write_to_buffer(d, "\n\r************YAY HERODAY, Don't bother ************",0);
#endif
	d->connected = CON_CONFIRM_ACCOUNT;
	break;


    case CON_CONFIRM_ACCOUNT:
	switch ( *argument )
	{
	    case 'y': case 'Y':

		d->acct = new_acct();

		free_string(d->acct->pwd);
		d->acct->pwd = str_dup(ch->pcdata->pwd);
		free_string(d->acct->name);
		d->acct->name = str_dup(ch->name);

		sprintf(buf, "\n\rAccount '%s' created.\n\r\n\r", ch->name);
		write_to_buffer(d, buf, 0);
		write_to_buffer(d, "The password for this account will be the same password used for creating\n\r", 0);
		write_to_buffer(d, "this character.  This may be changed in-game with the acctpwd command.\n\r\n\r", 0);
		write_to_buffer(d, "In the future, always use this account name for logging into Avendar.\n\r", 0);
		write_to_buffer(d, "You will then be able to create new characters under the account as per\n\r", 0);
		write_to_buffer(d, "normal.\n\r\n\r", 0);

		write_to_buffer(d, "We will now prompt you for an e-mail address to associate with the account.\n\r", 0);
		write_to_buffer(d, "This is for your protection, as we can verify future requests for lost\n\r", 0);
		write_to_buffer(d, "passwords and similar requests by the information provided here to us.  Any\n\r", 0);
		write_to_buffer(d, "e-mail address provided will otherwise be kept private.\n\r\n\r", 0);
		
		write_to_buffer(d, "Enter e-mail address to use, or enter for none: ", 0);
		d->connected = CON_GET_ACCOUNT_EMAIL;
		break;

	    case 'n': case 'N':
		write_to_buffer(d, "\n\r[ Press Enter to continue ]", 0);
		d->connected = CON_RACE_INFO;
		break;

	    default:
		write_to_buffer(d, "Please enter Yes or No: ", 0);
		break;

	}
	break;

case CON_GET_ACCOUNT_EMAIL:

	d->acct->email = str_dup(argument);
	write_to_buffer(d, "\n\r[ Press Enter to continue ]", 0);
	d->connected = CON_RACE_INFO;
	break;


     case CON_IS_NEWBIE:
	switch ( UPPER(*argument) )
	{
	    default:
		sprintf(buf, "Please enter 'Y' or 'N'.\n\rAre you new to Avendar: The Crucible of Legends? ");
		write_to_buffer( d, buf, 0 );
		break;

	    case 'Y':
		d->newbie = TRUE;
		write_to_buffer(d, "Then welcome, new player!\n\r\n\r", 0);
		write_to_buffer(d, "We will now ask you a number of questions which will help shape your initial\n\r", 0);
		write_to_buffer(d, "character.  In most cases, you will be able to type 'help <word>' to gain\n\r", 0);
		write_to_buffer(d, "further information on the choices available to you.  Once you have entered\n\r", 0);
		write_to_buffer(d, "the game itself, you can either use the new players channel (using the\n\r", 0);
		write_to_buffer(d, "'newbie' command) or 'pray' to the immortals if you have questions.  You\n\r", 0);
		write_to_buffer(d, "should also read 'help starting' once you are done with character creation.\n\r\n\r", 0);
		write_to_buffer(d, "We hope you enjoy your travels in Avendar!\n\r\n\r[ Press Enter to continue ]", 0);

		if (!d->acct)
		    d->connected = CON_ACCOUNT_INFO;
		else
		    d->connected = CON_RACE_INFO;
		break;

	    case 'N':
		SET_BIT(d->character->comm, COMM_NONEWBIE);
		write_to_buffer(d, "[ Press Enter to continue ]", 0);
		
		if (!d->acct)
		    d->connected = CON_ACCOUNT_INFO;
		else
		    d->connected = CON_RACE_INFO;
		break;
	}
	break;

    case CON_RACE_INFO:
	do_help(ch, "racelist");
	d->connected = CON_GET_NEW_RACE;
	break;

    case CON_GET_NEW_RACE:
	one_argument(argument,arg);

	if (!strcmp(arg,"help"))
	{
	    argument = one_argument(argument,arg);
	    if (argument[0] == '\0')
		do_help(ch,"races");
	    else
		do_help(ch,argument);
            write_to_buffer(d, "\n\rWhat is your race (help for more information)? ",0);
	    break;
  	}

	race = race_lookup(argument);

	if (race == 0 || race == 12 || !race_table[race].pc_race)
	{
	    write_to_buffer(d,"That is not a valid race.\n\r",0);
            write_to_buffer(d,"The following races are available:\n\r  ",0);
            for ( race = 1; race_table[race].name != NULL; race++ )
            {
            	if (!race_table[race].pc_race)
                    break;
            	if (race != 12)
		{
		    write_to_buffer(d,race_table[race].name,0);
            	    write_to_buffer(d," ",1);
		}
            }
            write_to_buffer(d,"\n\r",0);
            write_to_buffer(d,
		"Select your race (help for more information): ",0);
	    break;
	}

        ch->race = race;

	write_to_buffer(d, "\n\r", 0);
	show_helpfile(ch, ch->desc, race_table[race].name, HELP_NOTITLE);

	write_to_buffer(d, "\n\rAre you sure this is the race you wish to choose (y/n)? ", 0);
	d->connected = CON_CONFIRM_RACE;
	break;

case CON_CONFIRM_RACE:
	switch ( *argument )
	{
	    case 'y': case 'Y':

		/* initialize stats */
		for (i = 0; i < MAX_STATS; i++)
		    ch->perm_stat[i] = pc_race_table[ch->race].stats[i];

		if (number_percent() < 50)
		    ch->pcdata->max_age = number_range(pc_race_table[ch->race].age_values[1], pc_race_table[ch->race].age_values[0]);
		else
		{
		    if (number_percent() < 80)
			ch->pcdata->max_age = number_range(pc_race_table[ch->race].age_values[0], (pc_race_table[ch->race].age_values[0] * 2) - pc_race_table[ch->race].age_values[1]);
		    else
			ch->pcdata->max_age = number_range((pc_race_table[ch->race].age_values[0] * 2) - pc_race_table[ch->race].age_values[1], pc_race_table[ch->race].age_values[2]);
		}

		ch->pcdata->max_deaths = number_range(111, 118) - pc_race_table[ch->race].deaths_mod;

		ch->affected_by = ch->affected_by|race_table[ch->race].aff;
		REMOVE_BIT(ch->affected_by, AFF_FLY_NATURAL);

		ch->imm_flags	= ch->imm_flags|race_table[ch->race].imm;
		for (i = 0; i < MAX_RESIST; i++)
 		    ch->resist[i] += race_table[ch->race].resist[i];
		ch->form	= race_table[ch->race].form;
		ch->parts	= race_table[ch->race].parts;
		ch->train	= 17;
		ch->practice = 8;

		if (ch->race != global_int_race_kankoran)
		{
		    ch->dam_type = DAM_BASH; /*punch */
		    ch->dam_verb = str_dup("punch");
		}
		else
		{
		    ch->dam_type = DAM_SLASH;
		    ch->dam_verb = str_dup("claw");
		}
	
		if (ch->race != global_int_race_alatharya)
		    ch->pcdata->learned[*lang_data[race_table[ch->race].native_tongue].sn] = 100;
		else
		    ch->pcdata->learned[*lang_data[race_table[ch->race].native_tongue].sn] = 75;

		if (ch->race == global_int_race_chtaren)
            modify_karma(ch, -500);

		ch->pcdata->death_count = 0;
		/* add skills */
		for (i = 0; i < 5; i++)
		{
		    if (pc_race_table[ch->race].skills[i] == NULL)
	 		break;
		    group_add(ch,pc_race_table[ch->race].skills[i],FALSE);
		}

		if (ch->race == global_int_race_chaja)
		    BIT_SET(ch->pcdata->traits, TRAIT_IRONSTOMACH);
		
		/* add cost */
		ch->pcdata->points = 0; /* New point system */
		ch->size = pc_race_table[ch->race].size;

		ch->speaking = LANG_COMMON;
		if (ch->race == global_int_race_human)
		{   
		    write_to_buffer(d, "\n\rAs a human, you have the option of learning a second language, in exchange\n\rfor one of your character's initial training sessions.  If you are\n\rnew to the lands of Avendar, or do not wish to learn an additional\n\rlanguage, type 'none' at the prompt.  The following languages are available\n\rto you:\n\r\n\r", 0);
		    for (i = (LANG_COMMON + 1); i < LANG_ARCANE; i++)
		    {
			if (i != LANG_ALATHARYA)
			{
			    sprintf(buf, "%s ", lang_data[i].name);
			    write_to_buffer(d, buf, 0);
			}
		    }
		    write_to_buffer(d, "none", 0);
		    write_to_buffer(d, "\n\r\n\rWhich language do you wish to learn? ", 0);
		    d->connected = CON_GET_HUMAN_LANGUAGE;
		}
		else
		{
#ifdef HERODAY
		    if (TRUE)
#else
		    if (d->acct && d->acct->award_points > 0)
#endif
		    {
		      sprintf(buf, "\n\rAs a%s %s, you have the option of %s the common language, in exchange\n\rfor a number of practices.  How many practice sessions do you wish to use in this way?\n\r\n\r** Note: It is recommended that new players spend 3 practices learning common.\n\r\n\rSpend how many practices (0-3)? ",
		      (IS_VOWEL(race_table[ch->race].name[0]) ? "n" : ""), race_table[ch->race].name,
		      ((ch->race == global_int_race_alatharya) ? "improving in" : "learning"));
		      write_to_buffer(d, buf, 0);
		    }
		    else write_to_buffer(d, "[ Press Enter to continue ]", 0);
		    d->connected = CON_GET_LANG_TRAINS;
		}
		break;

	    case 'n': case 'N':
		ch->race = 0;
		do_help(ch, "racelist");
		d->connected = CON_GET_NEW_RACE;
		break;

	    default:
		write_to_buffer(d, "Please enter Yes or No: ", 0);
		break;

	}
	break;

    case CON_GET_HUMAN_LANGUAGE:
	argument = one_argument(argument, arg);
	if (!strcmp(arg, "help"))
	{
	    if (argument[0] == '\0')
		do_help(ch,"summary");
	    else
		do_help(ch,argument);
            write_to_buffer(d, "Which language do you wish to learn? ", 0);
	    break;
  	}
	if (arg[0] != '\0')
	{
	    if (!str_cmp(arg, "none"))
	    {
		write_to_buffer(d, "\n\rWhat is your sex (M/F)? ", 0);
		d->connected = CON_GET_NEW_SEX;
		break;
	    }

	    for (i = (LANG_COMMON + 1); i < LANG_ARCANE; i++)
		if (i != LANG_ALATHARYA)
	            if (!strcmp(arg, lang_data[i].name))
	            {
		        ch->speaking = i;
		        write_to_buffer(d, "\n\rHow many initial trains do you wish to spend (0-1)? ", 0);
		        d->connected = CON_GET_LANG_TRAINS;
		        return;
	            }
	}

	write_to_buffer( d, "Invalid language.  Which language do you wish to learn? ", 0);
	break;

    case CON_GET_ATTR:
	argument = one_argument(argument, arg);

        for (i = 0; i < MAX_STATS; i++)
	    attr_count += ch->attr_prime[i];
	
	if (!str_cmp(arg, "default"))
	{
	    ch->attr_prime[class_table[ch->class_num].attr_prime] += (3 - attr_count);
	    attr_count = 3;

	    sprintf(buf, "\n\rYour %s has been incremented.  Maximum %s is now %d.\n\r\n\r", lname_stat[class_table[ch->class_num].attr_prime], lname_stat[class_table[ch->class_num].attr_prime], 20 + ch->attr_prime[class_table[ch->class_num].attr_prime]);
	    write_to_buffer(d, buf, 0);
	}
	else
	{
    	    for (i = 0; i < MAX_STATS; i++)
	    {	
		if ((!str_cmp(arg, lname_stat[i]) || !str_cmp (arg, sname_stat[i])) && (i != STAT_CON))
		{
		    ch->attr_prime[i]++;
		    attr_count++;
		    sprintf(buf, "\n\rYour %s has been incremented.  Maximum %s is now %d.\n\r\n\r", lname_stat[i], lname_stat[i], 20+ch->attr_prime[i]);
		    write_to_buffer(d, buf, 0);
		    break;
		}		    
	    }
	}
	

	if (attr_count == 3)
	{   
	    show_avail_aligns(d, ch);
	    d->connected = CON_PICK_ALIGNMENT;
	    break;
	}

	if (i == MAX_STATS)
	{
	    write_to_buffer(d, "\n\rInvalid option.  Valid attributes are: str, int, wis, dex, chr, default", 0);
	    write_to_buffer(d, "\n\rChoose primary attribute: ", 0);
	}
	else
	{
	    sprintf(buf, "You have %d points remaining.", 3 - attr_count);
	    write_to_buffer(d, buf, 0);
	    write_to_buffer(d, "\n\rChoose primary attribute [str dex wis int chr default]: ", 0);
	}

	break;
											
    case CON_GET_LANG_TRAINS:
#ifdef HERODAY
	if (TRUE)
#else
	if (d->acct && (d->acct->award_points >= 1))
#endif
          argument = one_argument(argument, arg);		
#ifdef HERODAY
	if (is_number(arg))
#else
	if (is_number(arg) || !d->acct || d->acct->award_points < 1)
#endif
	{
		// Set up language defaults
		int maxCount = 1;
		int * pracOrTrain(&ch->train);
		int skill_levels[] = {0, 100, 100, 100};

		// Adjust for common
		if (ch->speaking == LANG_COMMON)
		{
			maxCount = 3;
			pracOrTrain = &ch->practice;
			skill_levels[1] = 33;
			skill_levels[2] = 66;
			skill_levels[3] = 100;
		}

    	if (!d->acct || d->acct->award_points < 1) 
			i = maxCount;
	    else 
			i = atoi(arg);

	    if ((i >= 0) && (i <= maxCount))
	    {
	    	ch->pcdata->learned[*lang_data[ch->speaking].sn] = skill_levels[i];
			*pracOrTrain -= i;

			if (d->character->train >= 1 && d->acct && d->acct->award_points > 0)
			{
		    	write_to_buffer(d, "Would you like to train another language (y/n)? ", 0);
		    	d->connected = CON_SECOND_LANGUAGE;
			}
			else
			{   
	            ch->speaking = race_table[ch->race].native_tongue;
		    	write_to_buffer(d, "\n\rWhat is your sex (M/F)? ", 0);
			    d->connected = CON_GET_NEW_SEX;
			}
			break;
		}
	}
	if (ch->speaking == LANG_COMMON)
	{
	    write_to_buffer(d, "Invalid number.  Must be between 0 and 3.\n\r", 0);
	    write_to_buffer(d, "How many initial practices do you wish to spend (0-3)? ", 0);
	}
	else
	{
	    write_to_buffer(d, "Invalid number.  Must be between 0 and 1.\n\r",0);
	    write_to_buffer(d, "How many initial trains do you wish to spend (0-1)? ", 0);
	}
	break;	

    case CON_SECOND_LANGUAGE:
	switch (UPPER(*argument))
	{
	    default:
		write_to_buffer(d, "Please enter 'Y' or 'N': ", 0);
		break;

	    case 'N':
	        write_to_buffer(d, "\n\rWhat is your sex (M/F)? ", 0);
	        d->connected = CON_GET_NEW_SEX;
		break;
															    
	    case 'Y':
		for (i = (LANG_COMMON + 1); i < LANG_ARCANE; i++)
		{
		    if (i != LANG_ALATHARYA)
		        if (d->character->pcdata->learned[*lang_data[i].sn] == 0)
		        {
			    sprintf(buf, "%s ", lang_data[i].name);
			    write_to_buffer(d, buf, 0);
		        }
		}
		write_to_buffer(d, "none", 0);
		write_to_buffer(d, "\n\r\n\rWhich language do you wish to learn? ", 0);
		d->connected = CON_GET_HUMAN_LANGUAGE;
		break;
	}
	break;

    case CON_GET_NEW_SEX:
	argument = one_argument(argument,arg);
	if (!strcmp(arg,"help"))
	{
	    if (argument[0] == '\0')
		do_help(ch,"summary");
	    else
		do_help(ch,argument);
            write_to_buffer(d,
		"What is your sex (M/F)? ",0);
	    break;
  	}

	switch ( arg[0] )
	{
	case 'm': case 'M': ch->sex = SEX_MALE;    
			    ch->pcdata->true_sex = SEX_MALE;
			    break;
	case 'f': case 'F': ch->sex = SEX_FEMALE; 
			    ch->pcdata->true_sex = SEX_FEMALE;
			    break;
	default:
	    write_to_buffer( d, "That's not a sex.\n\rWhat IS your sex? ", 0 );
	    return;
	}
	write_to_buffer(d, "\n\r", 0);
write_to_buffer(d, "                            The Classes of Avendar\n\r", 0);
write_to_buffer(d, "                            [====================]\n\r", 0);
write_to_buffer(d, "Warriors:      Use brute force or combat expertise to their advantage.\n\r", 0);
write_to_buffer(d, "               Includes fighters, gladiators, barbarians, and swordmasters.\n\r", 0);
write_to_buffer(d, "Rogues:        Those classes who rely on cunning and dexterity to make\n\r", 0);
write_to_buffer(d, "               their way in the world. Includes thieves, watchers, bards,\n\r", 0);
write_to_buffer(d, "               and bandits.\n\r", 0);
write_to_buffer(d, "Scholars:      Sages who are schooled in the ways of elemental magic.\n\r", 0);
write_to_buffer(d, "               Includes six spheres of study: Water, Spirit, Earth, Air,\n\r", 0);
write_to_buffer(d, "               Fire, and Void.\n\r", 0);
write_to_buffer(d, "Templars:      Warrior-mages who train in both combat and magic, foregoing\n\r", 0);
write_to_buffer(d, "               expertise in either field. Can study in one of the six\n\r", 0);
write_to_buffer(d, "               elemental spheres.\n\r", 0);
write_to_buffer(d, "Naturalists:   Devotees of the forests, the naturalists utilize the powers\n\r", 0);
write_to_buffer(d, "               of the wild. Includes rangers and druids.\n\r", 0);
write_to_buffer(d, "Mentalists:    Disciples of the mind who use psionic abilities to augment\n\r", 0);
write_to_buffer(d, "               their strength or hinder others. Includes assassins and\n\r",0);
write_to_buffer(d, "               psionicists.\n\r",0);
	strcpy( buf, "\n\rSelect a class [" );
	write_to_buffer( d, buf, 0 );

	sprintf(buf, " %s%s%s%s%s%s%s",
		scholar_ok(ch)     ? "Scholar " : "",
		templar_ok(ch)     ? "Templar " : "",
		rogue_ok(ch)       ? "Rogue "   : "",
		warrior_ok(ch)     ? "Warrior " : "",
		naturalist_ok(ch)  ? "Naturalist " : "",
		mentalist_ok(ch)   ? "Mentalist " : "",
		other_ok(ch)	   ? "Other " : "");
	strcat( buf, "]: " );
	write_to_buffer( d, buf, 0 );
	d->connected = CON_GET_NEW_CLASS;
	break;

    case CON_GET_NEW_CLASS:
	argument = one_argument(argument,arg);
	if (!strcmp(arg,"help"))
	{
	    if (argument[0] == '\0')
		do_help(ch,"summary");
	    else
		do_help(ch,argument);

	  write_to_buffer( d,
		"\n\rValid classes are: [", 0 );
	sprintf(buf, " %s%s%s%s%s%s%s",
		scholar_ok(ch) ? "Scholar " : "",
		templar_ok(ch) ? "Templar " : "",
		rogue_ok(ch)   ? "Rogue "   : "",
		warrior_ok(ch) ? "Warrior " : "",
		naturalist_ok(ch) ? "Naturalist " : "",
		mentalist_ok(ch) ? "Mentalist " : "",
		other_ok(ch) ? "Other " : "");
	strcat( buf, "]: " );
	strcat(buf, "\n\rWhich class do you wish to be? ");
	write_to_buffer( d, buf, 0 );
	    break;
  	}

	cFound = FALSE;

	switch(arg[0])
	{

	case 'S':case 's':
	    if (scholar_ok(ch))
	    {
		ch->class_num = PROF_SCHOLAR;
		ch->pcdata->learned[*lang_data[LANG_ARCANE].sn] = 100;
		cFound = TRUE;
	        BIT_SET(ch->pcdata->traits, TRAIT_MAGAPT);
	    }
	    break;

	case 'T':case 't':
	    if (templar_ok(ch))
	    {
		ch->class_num = PROF_TEMPLAR;
		ch->pcdata->learned[*lang_data[LANG_ARCANE].sn] = 100;
		cFound = TRUE;
	    }
	    break;

	case 'R':case 'r':
	    if (rogue_ok(ch))
	    {
		ch->class_num = PROF_ROGUE;
		cFound = TRUE;
	    }
	    break;

	case 'W':case 'w':
	    if (warrior_ok(ch))
	    {
		ch->class_num = PROF_WARRIOR;
		cFound = TRUE;
	    }
	    break;

	case 'M':case 'm':
	    if (mentalist_ok(ch))
	    {
		ch->class_num = PROF_MENTALIST;
		cFound = TRUE;
	    }
	    break;

	case 'N':case 'n':
	    if (naturalist_ok(ch))
	    {
		ch->class_num = PROF_NATURALIST;
		cFound = TRUE;
	    }
	    break;

	case 'O':case 'o':
	    if (other_ok(ch))
	    {
		ch->class_num = PROF_OTHER;
		cFound = TRUE;
	    }
	    break;

	}

        if (!cFound)
        {	
             write_to_buffer( d,
		"That's not a class.\n\rValid classes are: [", 0 );
	     sprintf(buf, " %s%s%s%s%s%s%s",
		scholar_ok(ch) ? "Scholar " : "",
		templar_ok(ch) ? "Templar " : "",
		rogue_ok(ch)   ? "Rogue "   : "",
		warrior_ok(ch) ? "Warrior " : "",
		naturalist_ok(ch) ? "Naturalist " : "",
		mentalist_ok(ch) ? "Mentalist " : "",
		other_ok(ch) ? "Other " : "");
	    strcat( buf, "] " );
	    strcat(buf, "\n\rWhich class do you wish to be? ");
	    write_to_buffer(d, buf, 0);

	    return;
	}

	if (ch->class_num < 2)
  	    d->connected = CON_PICK_MAJOR;
	else
	{
	    switch(ch->class_num)
	    {
		case PROF_ROGUE:
		    write_to_buffer(d, "\n\r", 0);
write_to_buffer(d, "                               Rogue Professions\n\r", 0);
write_to_buffer(d, "                               [===============]\n\r", 0);
write_to_buffer(d, "Thieves:   Wily and quick, thieves have mastered the art of not being seen\n\r", 0);
write_to_buffer(d, "           They specialize in taking advantage of their invisibility to\n\r", 0);
write_to_buffer(d, "           the untrained eye.\n\r", 0);
write_to_buffer(d, "Watchers:  Watchers are cunning hunters trained in the art of tracking and\n\r", 0);
write_to_buffer(d, "           capturing the more-than-elusive.\n\r", 0);
write_to_buffer(d, "Bandits:   The most unrefined of the rogues, bandits are known for their\n\r", 0);
write_to_buffer(d, "           brutal hand-to-hand techniques.\n\r", 0);
write_to_buffer(d, "Bards:     Whether wandering minstrel or court herald, bards are trained\n\r", 0);
write_to_buffer(d, "           in a variety of serviceable talents and songs that evoke\n\r", 0);
write_to_buffer(d, "           powerful effects.\n\r", 0);
		break;
		case PROF_WARRIOR:
		    write_to_buffer(d, "\n\r", 0);
write_to_buffer(d, "                          Warrior Professions\n\r", 0);
write_to_buffer(d, "                          [=================]\n\r", 0);
write_to_buffer(d, "Barbarians:   Fierce warriors who disdain subtlety and rely on instinct,\n\r", 0);
write_to_buffer(d, "              rage, and power.\n\r", 0);
write_to_buffer(d, "Fighters:     Soldiers of fortune who utilize tactics, formations, and\n\r", 0);
write_to_buffer(d, "              a variety of weapon skills.\n\r", 0);
write_to_buffer(d, "Gladiators:   Pit-fighters trained in showy combat techniques, gladiators\n\r", 0);
write_to_buffer(d, "              often win through unexpected maneuvers.\n\r", 0);
write_to_buffer(d, "Swordmasters: Warriors who devote themselves solely to the sword, becoming\n\r", 0);
write_to_buffer(d, "              supreme combat machines.\n\r", 0);
		break;
		case PROF_NATURALIST:
		    write_to_buffer(d, "\n\r", 0);
write_to_buffer(d, "                          Naturalist Professions\n\r", 0);
write_to_buffer(d, "                          [====================]\n\r", 0);
write_to_buffer(d, "Rangers:    Warriors of the forests, rangers are experts in making use of\n\r", 0);
write_to_buffer(d, "            wild terrain, taming beasts, and hunting.\n\r", 0);
write_to_buffer(d, "Druids:     Priests of nature, druids tap into the mystical forces of the\n\r", 0);
write_to_buffer(d, "            wild, and seek to protect the balance of nature.\n\r", 0);
		break;
		case PROF_MENTALIST:
write_to_buffer(d,"\n\r", 0);
write_to_buffer(d, "                          Mentalist Professions\n\r", 0);
write_to_buffer(d, "                          [===================]\n\r",0);
write_to_buffer(d, "Psionicists: Devotees of the mind who achieve supreme command over both\n\r", 0);
write_to_buffer(d, "             the mental and physical.\n\r",0);
write_to_buffer(d, "Assassins:   Trained killers who combine mental techniques, stealth, and\n\r", 0);
write_to_buffer(d, "             combat skill. Perhaps most feared for their use of poisons.\n\r", 0);
	    }
	    show_avail_classes(d, ch);
	    d->connected = CON_PICK_NON_SCHOLAR;
	    return;
 	}
	write_to_buffer(d, "\n\r", 0);
write_to_buffer(d, "                          The Spheres of Magic\n\r", 0);
write_to_buffer(d, "                          [==================]\n\r", 0);
write_to_buffer(d, "Water:      The magic of healing and life, and the conjuration/manipulation\n\r", 0);
write_to_buffer(d, "            of water and ice. Opposes Chaos and Evil.\n\r", 0);
write_to_buffer(d, "Spirit:     The magic of the mind and spirit, including the manipulation of\n\r", 0);
write_to_buffer(d, "            mana. Opposes Law and Evil.\n\r", 0);
write_to_buffer(d, "Earth:      The magic of protection, order, and strength, and the\n\r", 0);
write_to_buffer(d, "            manipulation of stone and earth. Opposes Chaos.\n\r", 0);
write_to_buffer(d, "Air:        The magic of kinetics, randomness, deception, and the\n\r", 0);
write_to_buffer(d, "            manipulation of air and electricity. Opposes Law.\n\r", 0);
write_to_buffer(d, "Fire:       The magic of destruction, chaos, and thermal energy, including\n\r", 0);
write_to_buffer(d, "            the manipulation of fire and smoke. Opposes Law and Good.\n\r", 0);
write_to_buffer(d, "Void:       The magic of negative energy, extraplanar manipulation, and\n\r", 0);
write_to_buffer(d, "            maledictions. Opposes Chaos and Good.\n\r", 0);
	show_avail_spheres(d, ch, "major");
	break;

case CON_PICK_NON_SCHOLAR:
 	argument = one_argument(argument,arg);

	if (!strcmp(arg,"help"))
	{
	    if (argument[0] == '\0')
		do_help(ch,"summary");
	    else
		do_help(ch,argument);

	    show_avail_classes(d, ch);
	    return;
 	}
	if (!strcmp(arg,"back"))
	{
	    strcpy( buf, "\n\rSelect a class [" );
	    write_to_buffer( d, buf, 0 );

	    sprintf(buf, " %s%s%s%s%s%s%s",
		scholar_ok(ch)     ? "Scholar " : "",
		templar_ok(ch)     ? "Templar " : "",
		rogue_ok(ch)       ? "Rogue "   : "",
		warrior_ok(ch)     ? "Warrior " : "",
		naturalist_ok(ch)  ? "Naturalist " : "",
		mentalist_ok(ch)   ? "Mentalist " : "",
		other_ok(ch)	   ? "Other " : "");
	    strcat( buf, "]: " );
	    write_to_buffer( d, buf, 0 );
	    d->connected = CON_GET_NEW_CLASS;
	    break;
	}

	cFound = FALSE;

        for (i = 0; i < MAX_CLASS; i++)
	{
	    if ((UPPER(arg[0]) == UPPER(class_table[i].name[0])) && (class_table[i].races_allowed[ch->race] == 1) && !str_cmp(arg, class_table[i].name))
	    {
	        ch->class_num = i;
		ch->pcdata->major_sphere = SPH_SWORDMASTER;
		group_add(ch, class_table[i].base_group, FALSE);
		group_add(ch, class_table[i].default_group, FALSE);
		cFound = TRUE;
	    }
	}

	if (!cFound)
	{
	    write_to_buffer(d, "\n\rInvalid choice.\n\r", 0);
	    show_avail_classes(d, ch);
	    return;
	}
	
	if (ch->class_num == global_int_class_gladiator)
	{
	    BIT_SET(ch->pcdata->traits, TRAIT_AMBIDEXTROUS);
	    BIT_SET(ch->pcdata->traits, TRAIT_EXOTICMASTERY);
	}
	if (ch->class_num == global_int_class_barbarian)
	    BIT_SET(ch->pcdata->traits, TRAIT_HOLLOWLEG);
	
	if (ch->class_num == global_int_class_watcher)
	{
	    BIT_SET(ch->pcdata->traits, TRAIT_LIGHTSLEEPER);
	    BIT_SET(ch->pcdata->traits, TRAIT_EAGLE_EYED);
	}

	if (ch->class_num == global_int_class_thief)
	{
	    BIT_SET(ch->pcdata->traits, TRAIT_FLEET);
	    BIT_SET(ch->pcdata->traits, TRAIT_COWARD);
	    BIT_SET(ch->pcdata->traits, TRAIT_THIEVESCANT);
	}
	
	if (ch->class_num == global_int_class_swordmaster)
	    BIT_SET(ch->pcdata->traits, TRAIT_AMBIDEXTROUS);

	if (ch->class_num == global_int_class_bandit)
	    BIT_SET(ch->pcdata->traits, TRAIT_PACK_HORSE);

	/*if (ch->class_num == global_int_class_druid)
	{
	    write_to_buffer(d,"\n\rDruids gain some of their power from nature in a broad sense, but the",0);
	    write_to_buffer(d,"\n\rrest is drawn from one of three sources.",0);
	    write_to_buffer(d,"\n\rFiriel:   Druids who call upon Firiel tap into the ancient arts of",0);
	    write_to_buffer(d,"\n\r          healing, mobility, and protection, as well as gain a deeper",0);
	    write_to_buffer(d,"\n\r	     connection to the plant world.",0);
	    write_to_buffer(d,"\n\rLunar:    Some druids call upon the moons, Lunus and Rhos, to tap ",0);
	    write_to_buffer(d,"\n\r	     into their fickle whims. These druids grow closer to the",0);
	    write_to_buffer(d,"\n\r	     animal world, and are most powerful at night.",0);
	    write_to_buffer(d,"\n\rGamaloth: Embracing the power of decay, infection, and death, those",0);
	    write_to_buffer(d,"\n\r          who embrace Gamaloth bind themselves to the darker",0);
	    write_to_buffer(d,"\n\r	     aspects of the primal world.",0);
	    sprintf(buf,"\n\rChoose a focus: %s%s%s",//MARKER
		firiel_ok(ch) ? "firiel " : "",
		lunar_ok(ch) ? "lunar " : "",
		gamaloth_ok(ch) ? "gamaloth " : "");
	    write_to_buffer(d,buf,0);
	    d->connected = CON_GET_DRUID_POWER;
	}
	else*/ if (ch->race == global_int_race_human)
	{
	    write_to_buffer(d, "\n\rAs a human, you may customize your character by choosing your 'primary", 0);
	    write_to_buffer(d, "\n\rattributes'.  For example, you may decide to have a very dextrous character,", 0);
	    write_to_buffer(d, "\n\ror a somewhat dextrous but also somewhat strong character.  You have three", 0);
	    write_to_buffer(d, "\n\rpoints to spend, and they may be distributed in multiple attributes.  If you", 0);
	    write_to_buffer(d, "\n\rwish to use the class default (recommended for new players), type 'default'.\n\r", 0);
	    write_to_buffer(d, "\n\rChoose primary attribute [str dex wis int chr default]: ", 0);
		     
	    d->connected = CON_GET_ATTR;
	}
	else
	{
	    show_avail_aligns(d, ch);
	    d->connected = CON_PICK_ALIGNMENT;
	}
	break;

case CON_GET_DRUID_POWER:
	argument = one_argument(argument,arg);
	if (!strcmp(arg,"help"))
	{
	    if (argument[0]=='\0')
		do_help(ch,"summary");
	    else
		do_help(ch,argument);

	    write_to_buffer(d,"\n\rDruids gain some of their power from nature in a broad sense, but the",0);
	    write_to_buffer(d,"\n\rrest is drawn from one of three sources.",0);
	    write_to_buffer(d,"\n\rFiriel:   ",0);
	    write_to_buffer(d,"\n\rLunar:    ",0);
	    write_to_buffer(d,"\n\rGamaloth:  ",0);
	    sprintf(buf,"\n\rChoose a focus: %s%s%s",//MARKER
		firiel_ok(ch) ? "firiel " : "",
		lunar_ok(ch) ? "lunar " : "",
		gamaloth_ok(ch) ? "gamaloth " : "");
	    write_to_buffer(d,buf,0);
	    return;
	}
	    
	if (!str_prefix(arg,"firiel"))
	{
	    if (firiel_ok(ch))
	    {
		ch->pcdata->minor_sphere = SPH_FIRIEL;
		group_add(ch, "firiel", FALSE);
		write_to_buffer(d,"\n\rFiriel selected.",0);
	    }
	    else
	    {
		write_to_buffer(d,"\n\rFiriel is not available to this race.",0);
		sprintf(buf,"\n\rChoose a focus: %s%s%s",
		    firiel_ok(ch) ? "firiel " : "",
		    lunar_ok(ch) ? "lunar " : "",
		    gamaloth_ok(ch) ? "gamaloth " : "");
		write_to_buffer(d,buf,0);
		return;
	    }
	}
	if (!str_prefix(arg,"lunar"))
	{
	    if (lunar_ok(ch))
	    {
		ch->pcdata->minor_sphere = SPH_LUNAR;
		group_add(ch, "lunar", FALSE);
		write_to_buffer(d,"\n\rLunar selected.",0);
	    }
	    else
	    {
		write_to_buffer(d,"\n\rLunar is not available to this race.",0);
		sprintf(buf,"\n\rChoose a focus: %s%s%s",
		    firiel_ok(ch) ? "firiel " : "",
		    lunar_ok(ch) ? "lunar " : "",
		    gamaloth_ok(ch) ? "gamaloth " : "");
		write_to_buffer(d,buf,0);
		return;
	    }
	}

	if (!str_prefix(arg,"gamaloth"))
	{
	    if (gamaloth_ok(ch))
	    {
		ch->pcdata->minor_sphere = SPH_GAMALOTH;
		group_add(ch, "gamaloth", FALSE);
		write_to_buffer(d,"\n\rGamaloth selected.",0);
	    }
	    else
	    {
		write_to_buffer(d,"\n\rGamaloth is not available to this race.",0);
		sprintf(buf,"\n\rChoose a focus: %s%s%s",
		    firiel_ok(ch) ? "firiel " : "",
		    lunar_ok(ch) ? "lunar " : "",
		    gamaloth_ok(ch) ? "gamaloth " : "");
		write_to_buffer(d,buf,0);
		return;
	    }
	}
	
	if (ch->race == global_int_race_human)
	{
	    write_to_buffer(d, "\n\rAs a human, you may customize your character by choosing your 'primary", 0);
	    write_to_buffer(d, "\n\rattributes'.  For example, you may decide to have a very dextrous character,", 0);
	    write_to_buffer(d, "\n\ror a somewhat dextrous but also somewhat strong character.  You have three", 0);
	    write_to_buffer(d, "\n\rpoints to spend, and they may be distributed in multiple attributes.  If you", 0);
	    write_to_buffer(d, "\n\rwish to use the class default (recommended for new players), type 'default'.\n\r", 0);
	    write_to_buffer(d, "\n\rChoose primary attribute [str dex wis int chr default]: ", 0);
		     
	    d->connected = CON_GET_ATTR;
	}
	else
	{
	    show_avail_aligns(d, ch);
	    d->connected = CON_PICK_ALIGNMENT;
	}
	break;
case CON_PICK_MAJOR:
 	argument = one_argument(argument,arg);

	if (!strcmp(arg,"help"))
	{
	    if (argument[0] == '\0')
		do_help(ch,"summary");
	    else
		do_help(ch, argument);

	    show_avail_spheres(d, ch, "major");
	    return;
	}
	if (!strcmp(arg,"back"))
	{
	    strcpy( buf, "\n\rSelect a class [" );
	    write_to_buffer( d, buf, 0 );

	    sprintf(buf, " %s%s%s%s%s%s%s",
		scholar_ok(ch)     ? "Scholar " : "",
		templar_ok(ch)     ? "Templar " : "",
		rogue_ok(ch)       ? "Rogue "   : "",
		warrior_ok(ch)     ? "Warrior " : "",
		naturalist_ok(ch)  ? "Naturalist " : "",
		mentalist_ok(ch)   ? "Mentalist " : "",
		other_ok(ch)	   ? "Other " : "");
	    strcat( buf, "]: " );
	    write_to_buffer( d, buf, 0 );
	    d->connected = CON_GET_NEW_CLASS;
	    break;
	}

	cFound = FALSE;

	switch(sphere_lookup(arg))
	{
	    case SPH_WATER:
		if ((pc_race_table[ch->race].allowed_aligns != 4)
		 && (pc_race_table[ch->race].allowed_ethos  != 4))
		{
		    group_add(ch, "water default", FALSE);
		    cFound = TRUE;
		}
		break;
	    case SPH_EARTH:
		if (pc_race_table[ch->race].allowed_ethos != 1)
		{
		    group_add(ch, "earth default", FALSE);
		    cFound = TRUE;
		}
		break;
	    case SPH_VOID:
		if ((pc_race_table[ch->race].allowed_aligns != 1)
		 && (pc_race_table[ch->race].allowed_ethos  != 4))
		{
		    group_add(ch, "void default", FALSE);
		    cFound = TRUE;
		}
		break;
	    case SPH_SPIRIT:
		if ((pc_race_table[ch->race].allowed_aligns != 4)
		 && (pc_race_table[ch->race].allowed_ethos  != 1))
		{
		    group_add(ch, "spirit default", FALSE);
		    cFound = TRUE;
		}
		break;
	    case SPH_AIR:
		if (pc_race_table[ch->race].allowed_ethos  != 1)
		{
		    group_add(ch, "air default", FALSE);
		    cFound = TRUE;
		}
		break;
	    case SPH_FIRE:
		if ((pc_race_table[ch->race].allowed_aligns != 1)
		 && (pc_race_table[ch->race].allowed_ethos  != 1))
		{
		    group_add(ch, "fire default", FALSE);
		    cFound = TRUE;
		}
		break;
	}

	if (cFound == TRUE)
	{
	    if (ch->class_num == 0)
	    {
		ch->class_num = sphere_lookup(arg);
		d->connected = CON_PICK_MINOR;
	        show_avail_spheres(d, ch, "minor");
	    }
	    else
	    {
		ch->class_num = (sphere_lookup(arg) + 6);
	    
		if (ch->race == global_int_race_human)
		{
		    write_to_buffer(d, "\n\rAs a human, you may customize your character by choosing your 'primary", 0);
		    write_to_buffer(d, "\n\rattributes'.  For example, you may decide to have a very dextrous character,", 0);
		    write_to_buffer(d, "\n\ror a somewhat dextrous but also somewhat strong character.  You have three", 0);
		    write_to_buffer(d, "\n\rpoints to spend, and they may be distributed in multiple attributes.  If you", 0);
		    write_to_buffer(d, "\n\rwish to use the class default (recommended for new players), type 'default'.\n\r", 0);
		    write_to_buffer(d, "\n\rChoose primary attribute [str dex wis int chr default]: ", 0);
		     
		    d->connected = CON_GET_ATTR;
		}	    
		else
		{
		    show_avail_aligns(d, ch);
		    d->connected = CON_PICK_ALIGNMENT;
		}
	    }
	    ch->pcdata->major_sphere = sphere_lookup(arg);
	    group_add(ch, class_table[ch->class_num].base_group, FALSE);
	    group_add(ch, class_table[ch->class_num].default_group, FALSE);
	}
	else
	{
	    write_to_buffer(d, "\n\rInvalid selection.\n\r", 0);
	    show_avail_spheres(d, ch, "major");
	    return;
	}
	break;

case CON_PICK_MINOR:
 	argument = one_argument(argument,arg);

	if (!strcmp(arg,"help"))
	{
	    if (argument[0] == '\0')
		do_help(ch,"summary");
	    else
		do_help(ch, argument);

	    show_avail_spheres(d, ch, "minor");
	    return;
	}

	cFound = FALSE;

	switch(sphere_lookup(arg))
	{
	    case SPH_WATER:
		if (pc_race_table[ch->race].allowed_aligns != 4 && pc_race_table[ch->race].allowed_ethos  != 4)
		{
			group_add(ch, "minor water v2", FALSE);
            switch (ch->pcdata->major_sphere)
            {
                case SPH_FIRE:      group_add(ch, "mixed fire water", false);   break;
                case SPH_SPIRIT:    group_add(ch, "mixed spirit water", false); break;
                case SPH_WATER:                                                 break;
                case SPH_AIR:       group_add(ch, "mixed air water", false);    break;
                case SPH_EARTH:     group_add(ch, "mixed earth water", false);  break;
                case SPH_VOID:      group_add(ch, "mixed void water", false);   break;
            }
		    cFound = TRUE;
		}
		break;
	    case SPH_EARTH:
		if (pc_race_table[ch->race].allowed_ethos != 1)
		{
            group_add(ch, "minor earth v2", FALSE);
            switch (ch->pcdata->major_sphere)
            {
                case SPH_FIRE:      group_add(ch, "mixed fire earth", false);   break;
                case SPH_SPIRIT:    group_add(ch, "mixed spirit earth", false); break;
                case SPH_WATER:     group_add(ch, "mixed water earth", false);  break;
                case SPH_AIR:       group_add(ch, "mixed air earth", false);    break;
                case SPH_EARTH:                                                 break;
                case SPH_VOID:      group_add(ch, "mixed void earth", false);   break;
            }
		    cFound = TRUE;
		}
		break;
	    case SPH_VOID:
		if (pc_race_table[ch->race].allowed_aligns != 1 && pc_race_table[ch->race].allowed_ethos  != 4)
		{
            group_add(ch, "minor void v2", FALSE);
            switch (ch->pcdata->major_sphere)
            {
                case SPH_FIRE:      group_add(ch, "mixed fire void", false);   break;
                case SPH_SPIRIT:    group_add(ch, "mixed spirit void", false); break;
                case SPH_WATER:     group_add(ch, "mixed water void", false);  break;
                case SPH_AIR:       group_add(ch, "mixed air void", false);    break;
                case SPH_EARTH:     group_add(ch, "mixed earth void", false);  break;
                case SPH_VOID:                                                 break;
            }
		    cFound = TRUE;
		}
		break;
	    case SPH_SPIRIT:
		if (pc_race_table[ch->race].allowed_aligns != 4 && pc_race_table[ch->race].allowed_ethos  != 1)
		{
            group_add(ch, "minor spirit v2", FALSE);
            switch (ch->pcdata->major_sphere)
            {
                case SPH_FIRE:      group_add(ch, "mixed fire spirit", false);  break;
                case SPH_SPIRIT:                                                break;
                case SPH_WATER:     group_add(ch, "mixed water spirit", false); break;
                case SPH_AIR:       group_add(ch, "mixed air spirit", false);   break;
                case SPH_EARTH:     group_add(ch, "mixed earth spirit", false); break;
                case SPH_VOID:      group_add(ch, "mixed void spirit", false);  break;
            }
		    cFound = TRUE;
		}
		break;
	    case SPH_AIR:
		if (pc_race_table[ch->race].allowed_ethos  != 1)
		{
            group_add(ch, "minor air v2", false);
            switch (ch->pcdata->major_sphere)
            {
                case SPH_FIRE:      group_add(ch, "mixed fire air", false);   break;
                case SPH_SPIRIT:    group_add(ch, "mixed spirit air", false); break;
                case SPH_WATER:     group_add(ch, "mixed water air", false);  break;
                case SPH_AIR:                                                 break;
                case SPH_EARTH:     group_add(ch, "mixed earth air", false);  break;
                case SPH_VOID:      group_add(ch, "mixed void air", false);   break;
            }
		    cFound = TRUE;
		}
		break;
	    case SPH_FIRE:
		if (pc_race_table[ch->race].allowed_aligns != 1 && pc_race_table[ch->race].allowed_ethos  != 1)
		{
			group_add(ch, "minor fire v2", FALSE);
            switch (ch->pcdata->major_sphere)
            {
                case SPH_FIRE:                                                  break;
                case SPH_SPIRIT:    group_add(ch, "mixed spirit fire", false);  break;
                case SPH_WATER:     group_add(ch, "mixed water fire", false);   break;
                case SPH_AIR:       group_add(ch, "mixed air fire", false);     break;
                case SPH_EARTH:     group_add(ch, "mixed earth fire", false);   break;
                case SPH_VOID:      group_add(ch, "mixed void fire", false);    break;
            }
		    cFound = TRUE;
		}
		break;
	}	

	if (cFound)
	    ch->pcdata->minor_sphere = sphere_lookup(arg);
	else
	{
	    write_to_buffer(d, "\n\rInvalid selection.\n\r", 0);
	    show_avail_spheres(d, ch, "minor");
	    return;
	}

	if (ch->race == global_int_race_human)
	{
	    write_to_buffer(d, "\n\rAs a human, you may customize your character by choosing your 'primary", 0);
	    write_to_buffer(d, "\n\rattributes'.  For example, you may decide to have a very dextrous character,", 0);
	    write_to_buffer(d, "\n\ror a somewhat dextrous but also somewhat strong character.  You have three", 0);
	    write_to_buffer(d, "\n\rpoints to spend, and they may be distributed in multiple attributes.  If you", 0);
	    write_to_buffer(d, "\n\rwish to use the class default (recommended for new players), type 'default'.\n\r", 0);
	    write_to_buffer(d, "\n\rChoose primary attribute [str dex wis int chr default]: ", 0);
		     
	    d->connected = CON_GET_ATTR;
	}
	else
	{
	    show_avail_aligns(d, ch);
	    d->connected = CON_PICK_ALIGNMENT;
	}
	break;
	    
case CON_PICK_ALIGNMENT:
 	argument = one_argument(argument,arg);

	if (!strcmp(arg,"help"))
	{
	    if (argument[0] == '\0')
		do_help(ch,"summary");
	    else
		do_help(ch, argument);

	    show_avail_aligns(d, ch);
	    return;
	}

        cFound = FALSE;

	switch (arg[0])
	{
	    case 'G':
	    case 'g':
		if (IS_SET(get_avail_aligns(ch), BIT_GOOD))
		{
		    ch->alignment = 750;
            modify_karma(ch, PALEGOLDENAURA);
		    cFound = TRUE;
		}
		break;
	    case 'N':
	    case 'n':
		if (IS_SET(get_avail_aligns(ch), BIT_NEUTRAL))
		{
		    ch->alignment = 0;
		    cFound = TRUE;
		}
		break;
	    case 'E':
	    case 'e':
		if (IS_SET(get_avail_aligns(ch), BIT_EVIL))
		{
		    ch->alignment = -750;
            modify_karma(ch, FAINTREDAURA);
		    cFound = TRUE;
		}
		break;
	}

	if (!cFound)
	{
	    write_to_buffer(d, "\n\rInvalid choice.\n\r", 0);
	    show_avail_aligns(d, ch);
	    return;
        }

	show_avail_ethos(d, ch);
	d->connected = CON_PICK_ETHOS;
	break;

case CON_PICK_ETHOS:
 	argument = one_argument(argument,arg);

	if (!strcmp(arg,"help"))
	{
	    if (argument[0] == '\0')
		do_help(ch,"summary");
	    else
		do_help(ch, argument);

	    show_avail_ethos(d, ch);
	    return;
	}

	cFound = FALSE;

	switch (arg[0])
	{
	    case 'L':
	    case 'l':
		if (IS_SET(get_avail_ethos(ch), BIT_LAWFUL))
		{
		    ch->pcdata->ethos = ETH_LAWFUL;
		    cFound = TRUE;
		}
		break;
	    case 'N':
	    case 'n':
		if (IS_SET(get_avail_ethos(ch), BIT_BALANCED))
		{
		    ch->pcdata->ethos = ETH_BALANCED;
		    cFound = TRUE;
		}
		break;
	    case 'C':
	    case 'c':
		if (IS_SET(get_avail_ethos(ch), BIT_CHAOTIC))
		{
		    ch->pcdata->ethos = ETH_CHAOTIC;
		    cFound = TRUE;
		}
		break;
	}

	if (!cFound)
	{
	    write_to_buffer(d, "\n\rInvalid choice.\n\r", 0);
	    show_avail_ethos(d, ch);
	    return;
	}

    if (ch->class_num == global_int_class_alchemist)
        BIT_SET(ch->pcdata->traits, TRAIT_MORTICIAN);
	if (ch->class_num == global_int_class_watertemplar)
    {
	    BIT_SET(ch->pcdata->traits, TRAIT_ENDURANCE);
        BIT_SET(ch->pcdata->traits, TRAIT_NIMBLE);
    }
    if (ch->class_num == global_int_class_earthtemplar)
        BIT_SET(ch->pcdata->traits, TRAIT_NIMBLE);
    if (ch->class_num == global_int_class_airtemplar)
        BIT_SET(ch->pcdata->traits, TRAIT_NIMBLE);
	if (ch->class_num == global_int_class_spirittemplar)
	    BIT_SET(ch->pcdata->traits, TRAIT_BRAVE);
	if (ch->class_num == global_int_class_voidtemplar)
	    BIT_SET(ch->pcdata->traits, TRAIT_OBSCURE);
	if (ch->class_num == global_int_class_voidscholar)
    {
	    BIT_SET(ch->pcdata->traits, TRAIT_OBSCURE);
        BIT_SET(ch->pcdata->traits, TRAIT_MORTICIAN);
    }
	if (ch->class_num == global_int_class_bandit)
	    BIT_SET(ch->pcdata->traits, TRAIT_PACK_HORSE);
	if (ch->class_num == global_int_class_firetemplar)
    {
	    BIT_SET(ch->pcdata->traits, TRAIT_SURVIVOR);
        BIT_SET(ch->pcdata->traits, TRAIT_NIMBLE);
    }
	if (ch->class_num == global_int_class_assassin)
	    BIT_SET(ch->pcdata->traits, TRAIT_POISONRES);
	if (ch->class_num == global_int_class_swordmaster)
	    BIT_SET(ch->pcdata->traits, TRAIT_SWORDMASTERY);
	if (get_skill(ch,gsn_peek))
	    SET_BIT(ch->nact,PLR_AUTOPEEK);
	if (get_skill(ch,gsn_trackless_step) || get_skill(ch,gsn_covertracks))
	    SET_BIT(ch->nact,PLR_AUTOTRACKS);
	REMOVE_BIT(ch->nact,PLR_EXTRASPAM);
	
    if (!d->newbie) 
    	show_avail_hometown(d, ch);
	else
	{		
		write_to_buffer(d, "\n\rAs a new player, your starting city will be Var Bandor.", 0);
		write_to_buffer(d, "\n\r[ Press Enter to Continue ]", 0);
	} 
	d->connected = CON_PICK_HOMETOWN;
	break;

case CON_PICK_HOMETOWN:
	
	if (d->newbie)
	{	
	    for (i = 0; i < MAX_HOMETOWN; i++)
		{		
			if (is_avail_hometown(ch, i))
			{	
	            ch->recall_to = get_room_index(home_table[i].vnum);
				write_to_buffer( d, "\n\rEnjoy your stay in Avendar!", 0);
				write_to_buffer( d, "\n\r[ Press Enter to continue ]", 0);
		        d->connected = CON_INIT_CHAR;
	 	        break;
			}	
		}
		break;
	}
    
	one_argument(argument, arg);
	if (!strcmp(arg,"help"))
	{
	    argument = one_argument(argument, arg);
	    if (argument[0] == '\0')
		do_help(ch,"summary");
	    else
		do_help(ch, argument);

	    show_avail_hometown(d, ch);
	    return;
	}

	cFound = FALSE;
	for (i = 0; i < MAX_HOMETOWN; i++)
	{		
	    if (!str_prefix(argument, home_table[i].name) && is_avail_hometown(ch, i))
	    {
		ch->recall_to = get_room_index(home_table[i].vnum);
		cFound = TRUE;
		break;
	    }
    }
	
	if (!cFound)
	{
	    write_to_buffer(d, "\n\rInvalid choice.\n\r", 0);
	    show_avail_hometown(d, ch);
	    return;
	}

	write_to_buffer(d, "\n\r\n\r", 2);

#ifdef HERODAY	
	if (TRUE)
#else
	if (d->acct && (d->acct->award_points > 0))
#endif
	{
	    write_to_buffer(d, "As an Avendar player using an account in good standing, you have the option\n\r", 0);
	    write_to_buffer(d, "to create 'hardcore' characters, that may only suffer a single death before\n\r", 0);
	    write_to_buffer(d, "being removed from the world.  Characters who select this option will gain\n\r", 0);
	    write_to_buffer(d, "experience at an increased rate, and gain in skills slightly more quickly.\n\r", 0);
	    write_to_buffer(d, "Please understand that Avendar is a dangerous place, and such characters are\n\r", 0);
	    write_to_buffer(d, "not expected to survive long, and therefore this option is generally only\n\r", 0);
	    write_to_buffer(d, "encouraged to be used by the insane.\n\r\n\r", 0);

	    write_to_buffer(d, "Do you wish this to be a hardcore character [y/N]? ", 0);
	    d->connected = CON_GET_HARDCORE;
	}
	else
	{
	    write_to_buffer( d, "[ Press Enter to continue ]", 0);
	    d->connected = CON_INIT_CHAR;
	}
	break;

    case CON_GET_HARDCORE:
	switch ( UPPER(*argument) )
	{
	    default:
		sprintf(buf, "Please enter 'Y' or 'N'.\n\rDo you wish this to be a hardcore character [y/N]? ");
		write_to_buffer( d, buf, 0 );
		break;

	    case 'Y':
		SET_BIT( ch->nact, PLR_HARDCORE );
		write_to_buffer( d, "Character WILL be created in hardcore mode.  Be careful!\n\r\n\r", 0);

#ifdef HERODAY
		if (TRUE)
#else
		if (d->acct->award_points >= 2)
#endif
		{
		    write_to_buffer( d, "As a player in good standing, you also have the option available to choose\n\r", 0);
		    write_to_buffer( d, "beneficial traits for your character, in exchange for a number of trains\n\r", 0);
		    write_to_buffer( d, "taken from your initial allocation.  The number of trains is dependant on\n\r", 0);
		    write_to_buffer( d, "the trait(s) selected.  Would you like to enter the trait selector [Y/n]? ", 0);
		    d->connected = CON_TRAIT_OPTION;
		}
		else
		{
		    write_to_buffer(d, "[ Press Enter to continue ]", 0);
		    d->connected = CON_INIT_CHAR;
		}
		break;

	    case '\0':
	    case 'N':
		write_to_buffer( d, "Character will not be created in hardcore mode.\n\r\n\r", 0);
		
#ifdef HERODAY
		if (TRUE)
#else
		if (d->acct->award_points >= 2)
#endif
		{
		    write_to_buffer( d, "As a player in good standing, you also have the option available to choose\n\r", 0);
		    write_to_buffer( d, "beneficial traits for your character, in exchange for a number of trains\n\r", 0);
		    write_to_buffer( d, "taken from your initial allocation.  The number of trains is dependant on\n\r", 0);
		    write_to_buffer( d, "the trait(s) selected.  Would you like to enter the trait selector [Y/n]? ", 0);
		    d->connected = CON_TRAIT_OPTION;
		}
		else
		{
		    write_to_buffer(d, "[ Press Enter to continue ]", 0);
		    d->connected = CON_INIT_CHAR;
		}
		break;
	}
	break;

    case CON_TRAIT_OPTION:
	switch (UPPER(*argument))
	{
	    default:
		write_to_buffer(d, "Please enter 'Y' or 'N': ", 0);
		break;

	    case 'N':
#ifdef HERODAY
		if(TRUE)
#else
		if (d->acct && d->acct->award_points > 0)
#endif
		{
		    write_to_buffer(d, "\n\r\n\rIf you wish, you may declare your character as a follower of a specific god\n\r", 0);
		    write_to_buffer(d, "upon creation.  This information will be known to the gods, although it has\n\r", 0);
		    write_to_buffer(d, "no effect beyond letting this detail about your character be known.  Do you\n\r", 0);
		    write_to_buffer(d, "wish to select a god to follow (y/N)? ", 0);
		    d->connected = CON_GOD_QUESTION;
		}
		else
		{
		    write_to_buffer(d, "[ Press Enter to continue ]", 0);
		    d->connected = CON_INIT_CHAR;
		}
		break;

	    case '\0':
	    case 'Y':
	        write_to_buffer(d, "\n\rEntering the trait selector...\n\rSelect from the following groups of traits:\n\r\n\r", 0);
		for (i = 0; trait_groups[i].name; i++)
		{
		    sprintf(buf, "%s\n\r", trait_groups[i].name);
		    write_to_buffer(d, buf, 0);
		}
		write_to_buffer(d, "\n\rPlease enter a group name, or 'done' to exit: ", 0);
		d->connected = CON_TRAIT_GROUPSEL;
		break;
	}
	break;

    case CON_TRAIT_GROUPSEL:
	if (!str_cmp(argument, "done"))
	{
#ifdef HERODAY
	    if (TRUE)
#else
	    if (d->acct && d->acct->award_points > 0)
#endif
	    {
		write_to_buffer(d, "\n\r\n\rIf you wish, you may declare your character as a follower of a specific god\n\r", 0);
		write_to_buffer(d, "upon creation.  This information will be known to the gods, although it has\n\r", 0);
		write_to_buffer(d, "no effect beyond letting this detail about your character be known.  Do you\n\r", 0);
		write_to_buffer(d, "wish to select a god to follow (y/N)? ", 0);
		d->connected = CON_GOD_QUESTION;
	    }
	    else
	    {
		write_to_buffer(d, "[ Press Enter to continue ]", 0);
		d->connected = CON_INIT_CHAR;
	    }
	    break;
	}

	for (i = 0; trait_groups[i].name; i++)
	    if (!str_prefix(argument, trait_groups[i].name))
	    {
		sprintf(buf, "\n\rThe following %s traits are available:\n\r\n\r", trait_groups[i].name);
		write_to_buffer(d, buf, 0);

		for (j = 0; trait_table[j].name; j++)
#ifdef HERODAY
		    if (trait_table[j].trait_group == trait_groups[i].group_num
		      && d->character->train >= trait_table[j].trains
		      && !BIT_GET(d->character->pcdata->traits, trait_table[j].bit)
#else
		    if (trait_table[j].trait_group == trait_groups[i].group_num
		      && d->character->train >= trait_table[j].trains
		      && d->acct->award_points >= trait_table[j].acct_pts
		      && !BIT_GET(d->character->pcdata->traits, trait_table[j].bit)
#endif
//brazen: farewell, resistant, we knew ye all too well
		      && str_cmp(trait_table[j].name,"resistant"))
		    {
			sprintf(buf, "{W%s{D: {w%s ({c%d train%s{w){x\n\r",
			    trait_table[j].name, trait_table[j].desc, trait_table[j].trains,
			    trait_table[j].trains == 1 ? "" : "s");
			send_to_char(buf, d->character);
			// write_to_buffer(d, buf, 0);
		    }

		write_to_buffer(d, "\n\rPlease select a trait, or 'back' to return to group select: ", 0);
		d->connected = trait_groups[i].group_num;
	    }
	
	if (d->connected == CON_TRAIT_GROUPSEL)
	    write_to_buffer(d, "Please enter a group name, or 'done' to exit: ", 0);

	break;

    case CON_TRAIT_GROUP1:
    case CON_TRAIT_GROUP2:
    case CON_TRAIT_GROUP3:
    case CON_TRAIT_GROUP4:
	if (!str_cmp(argument, "back"))
	{
	    write_to_buffer(d, "\n\rSelect from the following groups of traits:\n\r\n\r", 0);
	    for (i = 0; trait_groups[i].name; i++)
	    {
		sprintf(buf, "%s\n\r", trait_groups[i].name);
		write_to_buffer(d, buf, 0);
	    }
	    write_to_buffer(d, "\n\rPlease enter a group name, or 'done' to exit: ", 0);
	    d->connected = CON_TRAIT_GROUPSEL;
	    break;
	}

	for (i = 0; trait_table[i].name; i++)

#ifdef HERODAY
	   if (trait_table[i].trait_group == d->connected
	     && d->character->train >= trait_table[i].trains
	     && !BIT_GET(d->character->pcdata->traits, trait_table[i].bit)
	     && !str_cmp(argument, trait_table[i].name)
	     && str_cmp(trait_table[i].name,"resistant"))
#else
	   if (trait_table[i].trait_group == d->connected
	     && d->acct->award_points >= trait_table[i].acct_pts
	     && d->character->train >= trait_table[i].trains
	     && !BIT_GET(d->character->pcdata->traits, trait_table[i].bit)
	     && !str_cmp(argument, trait_table[i].name)
// brazen: farewell, resistant, we knew ye all too well
	     && str_cmp(trait_table[i].name,"resistant"))
#endif
	    {
		sprintf(buf, "Trait '%s' selected.  This trait costs %d train%s.\n\rThis would reduce your starting trains to %d.  Are you sure (y/n)? ",
		    trait_table[i].name,
		    trait_table[i].trains,
		    trait_table[i].trains == 1 ? "" : "s",
		    d->character->train - trait_table[i].trains);
		write_to_buffer(d, buf, 0);
		d->temp = i;
		d->connected = CON_TRAIT_CONFIRM;
		break;
	    }

	if (d->connected >= CON_TRAIT_GROUP1
	 && d->connected <= CON_TRAIT_GROUP4)
	    write_to_buffer(d, "Please select a trait, or 'back' to return to group select: ", 0);

	break;

    case CON_TRAIT_CONFIRM:
	switch (UPPER(*argument))
	{
	    default:
		write_to_buffer(d, "Please enter 'Y' or 'N': ", 0);
		break;

	    case 'N':
		write_to_buffer(d, "\n\rPlease select a trait, or 'back' to return to group select: ", 0);
		d->connected = trait_table[d->temp].trait_group;
		break;

	    case 'Y':
		BIT_SET(d->character->pcdata->traits, trait_table[d->temp].bit);
		d->character->train -= trait_table[d->temp].trains;
		sprintf(buf, "Trait '%s' added to character.\n\r", trait_table[d->temp].name);
		write_to_buffer(d, buf, 0);
		if (d->character->train > 0)
		{
		    write_to_buffer(d, "Do you wish to continue selecting traits (y/n)? ", 0);
		    d->connected = CON_TRAIT_CONTINUE;
		}
		else
		{
#ifdef HERODAY
		    if (TRUE)
#else
		    if (d->acct && d->acct->award_points > 0)
#endif
		    {
			write_to_buffer(d, "\n\r\n\rIf you wish, you may declare your character as a follower of a specific god\n\r", 0);
			write_to_buffer(d, "upon creation.  This information will be known to the gods, although it has\n\r", 0);
			write_to_buffer(d, "no effect beyond letting this detail about your character be known.  Do you\n\r", 0);
			write_to_buffer(d, "wish to select a god to follow (y/N)? ", 0);
			d->connected = CON_GOD_QUESTION;
		    }
		    else
		    {
			write_to_buffer(d, "[ Press Enter to continue ]", 0);
			d->connected = CON_INIT_CHAR;
		    }
		}
	}
	break;
		
    case CON_TRAIT_CONTINUE:
	switch (UPPER(*argument))
	{
	    default:
		write_to_buffer(d, "Please enter 'Y' or 'N': ", 0);
		break;

	    case 'N':
		if (d->acct && d->acct->award_points > 0)
		{
		    write_to_buffer(d, "\n\r\n\rIf you wish, you may declare your character as a follower of a specific god\n\r", 0);
		    write_to_buffer(d, "upon creation.  This information will be known to the gods, although it has\n\r", 0);
		    write_to_buffer(d, "no effect beyond letting this detail about your character be known.  Do you\n\r", 0);
		    write_to_buffer(d, "wish to select a god to follow (y/N)? ", 0);
		    d->connected = CON_GOD_QUESTION;
		}
		else
		{
		    write_to_buffer(d, "[ Press Enter to continue ]", 0);
		    d->connected = CON_INIT_CHAR;
		}
		break;

	    case 'Y':
	        write_to_buffer(d, "\n\r\n\rSelect from the following groups of traits:\n\r\n\r", 0);
		for (i = 0; trait_groups[i].name; i++)
		{
		    sprintf(buf, "%s\n\r", trait_groups[i].name);
		    write_to_buffer(d, buf, 0);
		}
		write_to_buffer(d, "\n\rPlease enter a group name, or 'done' to exit: ", 0);
		d->connected = CON_TRAIT_GROUPSEL;
		break;
	}
	break;

    case CON_GOD_QUESTION:
	switch (UPPER(*argument))
	{
	    default:
		write_to_buffer(d, "Please enter 'Y' or 'N': ", 0);
		break;

	    case '\0':
	    case 'N':
		if (BIT_GET(d->character->pcdata->traits, TRAIT_HEIRLOOM))
		{
		    write_to_buffer(d, "\n\rHeirloom Creation\n\r=================\n\r", 0);
		    write_to_buffer(d, "The first step in creating an heirloom is choosing a type.  Options are:\n\r", 0);
		    write_to_buffer(d, "armor\n\rjewelry\n\rweapon\n\r\n\rPlease select a type: ", 0);
		    d->connected = CON_HEIRLOOM_TYPE;
		}
		else
		{
		    write_to_buffer(d, "[ Press Enter to continue ]", 0);
		    d->connected = CON_INIT_CHAR;
		}
		break;

	    case 'Y':
		write_to_buffer(d, "\n\rYou may pick from the following list of gods:\n\r", 0);
		for (i = 0; i < MAX_GODS; i++)
		    if (god_table[i].can_follow)
		    {
			sprintf(buf, "%s\n\r", god_table[i].name);
			write_to_buffer(d, buf, 0);
		    }
		write_to_buffer(d, "\n\rSelect a god, or type 'cancel' to continue: ", 0);
		d->connected = CON_GOD_SELECT;
		break;
	}
	break;

    case CON_GOD_SELECT:
	if (!str_cmp(argument, "cancel"))
	{
	    if (BIT_GET(d->character->pcdata->traits, TRAIT_HEIRLOOM))
	    {
		write_to_buffer(d, "\n\rHeirloom Creation\n\r=================\n\r", 0);
		write_to_buffer(d, "The first step in creating an heirloom is choosing a type.  Options are:\n\r", 0);
		write_to_buffer(d, "armor\n\rjewelry\n\rweapon\n\r\n\rPlease select a type: ", 0);
		d->connected = CON_HEIRLOOM_TYPE;
	    }
	    else
	    {
		write_to_buffer(d, "[ Press Enter to continue ]", 0);
		d->connected = CON_INIT_CHAR;
	    }
	}

	for (i = 0; i < MAX_GODS; i++)
	    if (!str_cmp(argument, god_table[i].name) && god_table[i].can_follow)
	    {
		ch->religion = i;

#ifdef HERODAY
		if (d->character->train >= 3)
#else
		if (d->acct->award_points >= 5 && d->character->train >= 3 && strcmp(god_table[i].marking,""))
#endif
		{
		    sprintf(buf, "In exchange for {W3 trains{x, you may select the bonus trait '%s of %s',\n\r",
				    (god_table[i].align == ALIGN_EVIL) ? "Taint" :
				    (god_table[i].align == ALIGN_NEUTRAL) ? "Boon" : "Blessing",
				    god_table[i].name);
		    send_to_char(buf, d->character);
		    write_to_buffer(d, "which bestows upon you a minor benefit from your god.  Do you wish to take\n\r", 0);
		    write_to_buffer(d, "this (y/N)? ", 0);
		    d->connected = CON_BLESSING_OPTION;
		}
		else
		{
    		    if (BIT_GET(d->character->pcdata->traits, TRAIT_HEIRLOOM))
		    {
			write_to_buffer(d, "\n\rHeirloom Creation\n\r=================\n\r", 0);
			write_to_buffer(d, "The first step in creating an heirloom is choosing a type.  Options are:\n\r", 0);
			write_to_buffer(d, "armor\n\rjewelry\n\rweapon\n\r\n\rPlease select a type: ", 0);
			d->connected = CON_HEIRLOOM_TYPE;
		    }
		    else
		    {
			write_to_buffer(d, "\n\r[ Press Enter to continue ]", 0);
			d->connected = CON_INIT_CHAR;
		    }
		}
		break;
	    }

	if (d->connected == CON_GOD_SELECT)
	    write_to_buffer(d, "\n\rSelect a god, or type 'cancel' to continue: ", 0);
    
	break;

    case CON_BLESSING_OPTION:
	switch(UPPER(*argument))
	{
	    default:
		write_to_buffer(d, "Please enter 'Y' or 'N': ", 0);
		break;

	    case '\0':
	    case 'N':
		if (BIT_GET(d->character->pcdata->traits, TRAIT_HEIRLOOM))
		{
		    write_to_buffer(d, "\n\rHeirloom Creation\n\r=================\n\r", 0);
		    write_to_buffer(d, "The first step in creating an heirloom is choosing a type.  Options are:\n\r", 0);
		    write_to_buffer(d, "armor\n\rjewelry\n\rweapon\n\r\n\rPlease select a type: ", 0);
		    d->connected = CON_HEIRLOOM_TYPE;
		}
		else
		{
		    write_to_buffer(d, "[ Press Enter to continue ]", 0);
		    d->connected = CON_INIT_CHAR;
		}
		break;

	    case 'Y':
		d->character->train -= 3;
		BIT_SET(d->character->pcdata->traits, TRAIT_MARKED);
		if (god_table[d->character->religion].align == ALIGN_GOOD)
		    sprintf(buf, "\n\rYou were given a blessing at the time of your birth by the god %s.\n\r", god_table[d->character->religion].name);
		else if (god_table[d->character->religion].align == ALIGN_NEUTRAL)
		    sprintf(buf, "\n\rYou were given a boon at the time of your birth by the god %s.\n\r", god_table[d->character->religion].name);
		else if (god_table[d->character->religion].align == ALIGN_EVIL)
		    sprintf(buf, "\n\rYou were given a taint at the time of your birth by the god %s.\n\r", god_table[d->character->religion].name);

		write_to_buffer(d, buf, 0);

		if (god_table[d->character->religion].marking[0] != '\0')
		{
		    write_to_buffer(d, "When others look at you, they will see an outward manifestation of this.\n\r", 0);
		    write_to_buffer(d, "Observe yourself when you enter the game to view it, and feel free to\n\r", 0);
		    write_to_buffer(d, "incorporate it into your description if able.", 0);
		}
    
		write_to_buffer(d, "\n\r\n\r", 0);

		if (BIT_GET(d->character->pcdata->traits, TRAIT_HEIRLOOM))
		{
		    write_to_buffer(d, "\n\rHeirloom Creation\n\r=================\n\r", 0);
		    write_to_buffer(d, "The first step in creating an heirloom is choosing a type.  Options are:\n\r", 0);
		    write_to_buffer(d, "armor\n\rjewelry\n\rweapon\n\r\n\rPlease select a type: ", 0);
		    d->connected = CON_HEIRLOOM_TYPE;
		}
		else
		{
		    write_to_buffer(d, "[ Press Enter to continue ]", 0);
		    d->connected = CON_INIT_CHAR;
		}
		break;
	}
	break;

    case CON_HEIRLOOM_TYPE:
	if (!str_cmp(argument, "weapon"))
	{
	    obj_to_char(create_object(get_obj_index(OBJ_VNUM_HEIRLOOM), 10), d->character);

	    d->character->carrying->level = 10;
	    d->character->carrying->item_type = ITEM_WEAPON;
	    SET_BIT(d->character->carrying->wear_flags, ITEM_WIELD);
	    d->character->carrying->value[1] = 4;

	    af.where	 = TO_OBJECT;
	    af.type	 = gsn_heirloom;    
	    af.level     = 10;
	    af.duration  = -1;
	    af.location  = ((number_bits(1) == 0) ? APPLY_DAMROLL : APPLY_HITROLL);
	    af.modifier  = 1;
	    af.bitvector = 0;
	    obj_affect_join(d->character->carrying, &af);
/*
	    af.location  = ((number_bits(1) == 0) ? APPLY_STR : APPLY_DEX);
	    obj_affect_join(d->character->carrying, &af);
*/
	    write_to_buffer(d, "\n\rNext you must select the type of weapon.  Your options are:\n\r", 0);
	    send_to_char("{Wexotic{w, {Wsword{w, {Wdagger{w, {Wspear{w, {Wmace{w, {Waxe{w, {Wflail{w, {Wwhip{w, {Wpolearm{w, {Wstaff{x\n\r\n\r", d->character);
	    write_to_buffer(d, "Enter a weapon type to continue: ", 0);
	    d->connected = CON_HEIRLOOM_TYPE2;
	    break;
	}

	if (!str_cmp(argument, "armor"))
	{
	    obj_to_char(create_object(get_obj_index(OBJ_VNUM_HEIRLOOM), 10), d->character);
	    d->character->carrying->level = 10;
	    d->character->carrying->item_type = ITEM_ARMOR;
	    d->character->carrying->size = SIZE_MEDIUM;

	    d->character->carrying->value[0] = 10;
	    d->character->carrying->value[1] = 10;
	    d->character->carrying->value[2] = 10;
	    d->character->carrying->value[3] = 4;


	    af.where	 = TO_OBJECT;
	    af.type	 = gsn_heirloom;    
	    af.level     = 10;
	    af.duration  = -1;
	    af.location  = APPLY_DAMROLL;
	    af.modifier  = 1;
	    af.bitvector = 0;
	    obj_affect_join(d->character->carrying, &af);

	    af.modifier  = 1;
	    af.level     = 10;
	    af.location  = APPLY_HITROLL;
	    obj_affect_join(d->character->carrying, &af);

	    i = number_range(1, 5);
	    switch (i)
	    {
		case 1:	af.location = APPLY_STR;    break;
		case 2: af.location = APPLY_DEX;    break;
		case 3: af.location = APPLY_CON;    break;
		case 4: af.location = APPLY_HITROLL;break;
		case 5: af.location = APPLY_DAMROLL;break;
	    }
    
	    obj_affect_join(d->character->carrying, &af);

	    write_to_buffer(d, "\n\rNext you must select wear the armor is worn.  Your options are:\n\r", 0);
	    send_to_char("{Wtorso{w, {Whead{w, {Wlegs{w, {Wfeet{w, {Whands{w, {Wshield{w, {Wabout body{w, {Wwaist{w, {Wwrist{x\n\r\n\r", d->character);
	    write_to_buffer(d, "Enter an armor location to continue: ", 0);
	    d->connected = CON_HEIRLOOM_TYPE2;
	    break;
	}

	if (!str_cmp(argument, "jewelry"))
	{
	    obj_to_char(create_object(get_obj_index(OBJ_VNUM_HEIRLOOM), 10), d->character);
	    d->character->carrying->level = 10;
	    d->character->carrying->item_type = ITEM_JEWELRY;

	    af.where	 = TO_OBJECT;
	    af.type	 = gsn_heirloom;    
	    af.level     = 10;
	    af.duration  = -1;
	    af.location  = APPLY_HIT;
	    af.modifier  = 10;
	    af.bitvector = 0;
	    obj_affect_join(d->character->carrying, &af);

	    af.location  = APPLY_MANA;
	    obj_affect_join(d->character->carrying, &af);

	    i = number_range(1, 5);

	    switch (i)
	    {
		case 1:
		    af.location = APPLY_DAMROLL;
		    af.modifier = 1;
		    break;

		case 2:
		    af.location = APPLY_HITROLL;
		    af.modifier = 1;
		    break;

		case 3:
		    af.location = APPLY_INT;
		    af.modifier = 1;
		    break;

		case 4:
		    af.location = APPLY_WIS;
		    af.modifier = 1;
		    break;

		case 5:
		    af.location = APPLY_HIT;
		    af.modifier = 5;
		    break;

		case 6:
		    af.location = APPLY_MANA;
		    af.modifier = 5;
		    break;

		case 7:
		    af.location = APPLY_SAVES;
		    af.modifier = -2;
		    break;

	    }
    
	    obj_affect_join(d->character->carrying, &af);

	    write_to_buffer(d, "\n\rNext you must select where the jewelry is worn.  Your options are:\n\r", 0);
	    send_to_char("{Warm    {D- {warmbands\n\r", d->character);
	    send_to_char("{Wfinger {D- {wrings\n\r", d->character);
	    send_to_char("{Whead   {D- {wcirclets, tiaras, crowns, earrings\n\r", d->character);
	    send_to_char("{Wheld   {D- {wvarious baubles and held treasures\n\r", d->character);
	    send_to_char("{Wneck   {D- {wmedallions, pendants, necklaces, chokers\n\r", d->character);
	    send_to_char("{Wwaist  {D- {wbuckles\n\r\n\r", d->character);
	    write_to_buffer(d, "Enter a jewelry type to continue: ", 0);
	    d->connected = CON_HEIRLOOM_TYPE2;
	    break;
	}

	write_to_buffer(d, "Please enter 'weapon', 'armor', or 'jewelry': ", 0);
	break;

    case CON_HEIRLOOM_TYPE2:
	switch (d->character->carrying->item_type)
	{
	    case ITEM_WEAPON:
		if (!str_cmp(argument, "exotic"))
		{
		    d->character->carrying->value[0] = WEAPON_EXOTIC;
		    d->character->carrying->value[2] = 6;
		    d->character->carrying->size = SIZE_MEDIUM;
		    d->character->carrying->weight = 80;

		    write_to_buffer(d, "\n\rSelect the type of damage which most accurately fits your design:\n\r", 0);
		    write_to_buffer(d, "piercing\n\rslashing\n\rbashing\n\r\n\r", 0);
		    write_to_buffer(d, "Please select a type of damage: ", 0);
		    d->connected = CON_HEIRLOOM_TYPE3;
		}
		else if (!str_cmp(argument, "sword"))
		{
		    d->character->carrying->value[0] = WEAPON_SWORD;
		    d->character->carrying->size = SIZE_MEDIUM;

		    write_to_buffer(d, "\n\rSelect the category of sword which most accurately fits your design:\n\r\n\r", 0);
		    send_to_char("{Wshortsword {D- {wlighter, piercing, less damaging.  category includes rapiers.\n\r", d->character);
		    send_to_char("{Wlongsword  {D- {wnormal type of sword.  most swords fall into this category.\n\r", d->character);
		    send_to_char("{Wgreatsword {D- {wenhanced damage, but requires two hands.\n\r\n\r", d->character);
		    write_to_buffer(d, "Please select a type of sword: ", 0);
		    d->connected = CON_HEIRLOOM_TYPE3;
		}
		else if (!str_cmp(argument, "dagger"))
		{
		    d->character->carrying->value[0] = WEAPON_DAGGER;
		    d->character->carrying->value[2] = 6;
		    d->character->carrying->value[3] = DAM_PIERCE;
		    free_string(d->character->carrying->obj_str);
		    d->character->carrying->obj_str = str_dup("stab");
		    d->character->carrying->size = SIZE_SMALL;
		    d->character->carrying->weight = 60;

		    write_to_buffer(d, "bone\n\rbronze\n\rcopper\n\rdiamond\n\riron\n\rplatinum\n\rsilver\n\rsteel\n\r\n\r", 0);
		    write_to_buffer(d, "Select from the above list of materials: ", 0);
		    d->connected = CON_HEIRLOOM_MATERIAL;
		}
		else if (!str_cmp(argument, "spear"))
		{
		    d->character->carrying->value[0] = WEAPON_SPEAR;
		    d->character->carrying->value[3] = DAM_PIERCE;
		    d->character->carrying->size = SIZE_MEDIUM;
		    free_string(d->character->carrying->obj_str);
		    d->character->carrying->obj_str = str_dup("stab");

		    write_to_buffer(d, "\n\rSelect the category of spear which most accurately fits your design:\n\r\n\r", 0);
		    send_to_char("{Wshort spear {D- {wlighter, only requires one hand, but less damaging.\n\r", d->character);
		    send_to_char("{Wlong spear  {D- {wany normal type of spear, requires two hands.\n\r\n\r", d->character);
		    write_to_buffer(d, "Please select a type of spear: ", 0);
		    d->connected = CON_HEIRLOOM_TYPE3;
		}
		else if (!str_cmp(argument, "mace"))
		{
		    d->character->carrying->value[0] = WEAPON_MACE;
		    d->character->carrying->value[2] = 6;
		    d->character->carrying->value[3] = DAM_BASH;
		    free_string(d->character->carrying->obj_str);
		    d->character->carrying->obj_str = str_dup("smash");
		    d->character->carrying->size = SIZE_MEDIUM;
		    d->character->carrying->weight = 220;

		    write_to_buffer(d, "bone\n\rbronze\n\rcopper\n\riron\n\rplatinum\n\rsilver\n\rsteel\n\r\n\r", 0);
		    write_to_buffer(d, "Select from the above list of materials: ", 0);
		    d->connected = CON_HEIRLOOM_MATERIAL;
		}
		else if (!str_cmp(argument, "axe"))
		{
		    d->character->carrying->value[0] = WEAPON_AXE;
		    d->character->carrying->value[3] = DAM_SLASH;
		    d->character->carrying->size = SIZE_MEDIUM;
		    free_string(d->character->carrying->obj_str);
		    d->character->carrying->obj_str = str_dup("slash");

		    write_to_buffer(d, "\n\rSelect the category of axe which most accurately fits your design:\n\r\n\r", 0);

		    send_to_char("{Wbattle axe {D- {wdouble-bladed axes, require two hands.\n\r", d->character);
		    send_to_char("{Whand axe   {D- {wsingle-bladed axe, requires one hand.\n\r\n\r", d->character);
		    write_to_buffer(d, "Please select a type of axe: ", 0);
		    d->connected = CON_HEIRLOOM_TYPE3;
		}
		else if (!str_cmp(argument, "flail"))
		{
		    d->character->carrying->value[0] = WEAPON_FLAIL;
		    d->character->carrying->value[2] = 6;
		    d->character->carrying->value[3] = DAM_SLASH;
		    d->character->carrying->size = SIZE_MEDIUM;
		    d->character->carrying->weight = 110;
		    free_string(d->character->carrying->obj_str);
		    d->character->carrying->obj_str = str_dup("flailing");
		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An old iron flail lies here.");
		    d->character->carrying->material = material_lookup("iron");
            setName(*d->character->carrying, "old iron flail");
		}
		else if (!str_cmp(argument, "whip"))
		{
		    d->character->carrying->value[0] = WEAPON_WHIP;
		    d->character->carrying->value[2] = 6;
		    d->character->carrying->value[3] = DAM_SLASH;
		    d->character->carrying->size = SIZE_MEDIUM;
		    d->character->carrying->weight = 60;
		    free_string(d->character->carrying->obj_str);
		    d->character->carrying->obj_str = str_dup("whip");
		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An old leather whip lies here.");
		    d->character->carrying->material = material_lookup("leather");
            setName(*d->character->carrying, "old leather whip");
		}
		else if (!str_cmp(argument, "polearm"))
		{
		    d->character->carrying->value[0] = WEAPON_POLEARM;
		    d->character->carrying->value[2] = 6;
		    d->character->carrying->value[3] = DAM_PIERCE;
		    d->character->carrying->size = SIZE_LARGE;
		    d->character->carrying->weight = 140;
		    free_string(d->character->carrying->obj_str);
		    d->character->carrying->obj_str = str_dup("pierce");
		    SET_BIT(d->character->carrying->value[4], WEAPON_TWO_HANDS);

		    write_to_buffer(d, "bronze\n\riron\n\rplatinum\n\rsilver\n\rsteel\n\r\n\r", 0);
		    write_to_buffer(d, "Select from the above list of materials: ", 0);
		    d->connected = CON_HEIRLOOM_MATERIAL;
		}
		else if (!str_cmp(argument, "staff"))
		{
		    d->character->carrying->value[0] = WEAPON_STAFF;
		    d->character->carrying->value[2] = 6;
		    d->character->carrying->value[3] = DAM_BASH;
		    d->character->carrying->size = SIZE_LARGE;
		    d->character->carrying->weight = 120;
		    SET_BIT(d->character->carrying->value[4], WEAPON_TWO_HANDS);
		    free_string(d->character->carrying->obj_str);
		    d->character->carrying->obj_str = str_dup("smash");
		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An old wooden staff lies here.");
		    d->character->carrying->material = material_lookup("wood");
            setName(*d->character->carrying, "old wooden staff");
		}
		else
		{
		    write_to_buffer(d, "Enter a weapon type to continue: ", 0);
		    break;
		}

		if (d->connected == CON_HEIRLOOM_TYPE2)
		{
		    write_to_buffer(d, "\n\rYou now need to provide a short description for your heirloom.  Examples:\n\r", 0);
		    write_to_buffer(d, "the silver rapier of the Narith family\n\r", 0);
		    write_to_buffer(d, "a massive iron claymore\n\r", 0);
		    write_to_buffer(d, "an elegant steel dagger\n\r\n\r", 0);

		    write_to_buffer(d, "(10-50 characters): ", 0);
		    d->connected = CON_HEIRLOOM_SHORT;
		}
	    
		break;

	    case ITEM_ARMOR:
		if (!str_cmp(argument, "torso"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_BODY);

		    write_to_buffer(d, "Select the type of bodywear that most accurately fits your design:\n\r", 0);
		    send_to_char("{Warmor shirt {D- {wheavier, but more protective.  category includes breastplates.\n\r", d->character);
		    send_to_char("{Wjerkin {D- {wnormal shirt, made out of cloth.\n\r\n\r", d->character);

		    write_to_buffer(d, "Select a type of bodywear: ", 0);
		    d->connected = CON_HEIRLOOM_TYPE3;
		}
		else if (!str_cmp(argument, "head"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_HEAD);
		    write_to_buffer(d, "Select the type of headwear that most accurately fits your design:\n\r", 0);
		    send_to_char("{Whelmet {D- {wa metal helmet of any type.  heavier, but more protective.\n\r", d->character);
		    send_to_char("{Wcap {D- {wany sort of cloth or leather headgear.\n\r\n\r", d->character);
		    write_to_buffer(d, "Select a type of headgear: ", 0);
		    d->connected = CON_HEIRLOOM_TYPE3;
		}
		else if (!str_cmp(argument, "legs"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_LEGS);
		    d->character->carrying->weight = 100;

		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An old pair of leggings lie here.");

            setName(*d->character->carrying, "old leggings");
		    
		    write_to_buffer(d, "\n\rThe following materials are available:\n\r", 0);
		    send_to_char("{Wbrass{D, {Wbronze{D, {Wcloth{D, {Wcopper{D, {Wdragonhide{D, {Wdragonscale{D, {Whide{D, {Wiron{D, {Wleather{D,{x\n\r", d->character);
		    send_to_char("{Wplatinum{D, {Wsilk{D, {Wsilver{D, {Wsteel{x\n\r\n\r", d->character);
		    write_to_buffer(d, "Select from the above list of materials: ", 0);
		    d->connected = CON_HEIRLOOM_MATERIAL;
		}
		else if (!str_cmp(argument, "feet"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_FEET);
		    d->character->carrying->weight = 60;

		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An old pair of boots stand here.");

            setName(*d->character->carrying, "old boots");

		    write_to_buffer(d, "\n\rThe following materials are available:\n\r", 0);
		    send_to_char("{Wbrass{D, {Wbronze{D, {Wcloth{D, {Wcopper{D, {Wdragonhide{D, {Wdragonscale{D, {Whide{D, {Wiron{D, {Wleather{D,{x\n\r", d->character);
		    send_to_char("{Wplatinum{D, {Wsilk{D, {Wsilver{D, {Wsteel{D, {Wwood{x\n\r\n\r", d->character);
		    write_to_buffer(d, "Select from the above list of materials: ", 0);
		    d->connected = CON_HEIRLOOM_MATERIAL;
		}
		else if (!str_cmp(argument, "hands"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_HANDS);

		    write_to_buffer(d, "Select the type of headwear that most accurately fits your design:\n\r", 0);
		    send_to_char("{Wgauntlets {D- {wmetal gauntlets of any type.  heavier, but more protective.\n\r", d->character);
		    send_to_char("{Wcap {D- {wlighter gloves, more appropriate for spellcasting.\n\r\n\r", d->character);
		    write_to_buffer(d, "Select a type of handwear: ", 0);
		    d->connected = CON_HEIRLOOM_TYPE3;
		}
		else if (!str_cmp(argument, "shield"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_SHIELD);
		    d->character->carrying->weight = 80;

		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An old shield lies here.");

            setName(*d->character->carrying, "old shield");

		    write_to_buffer(d, "\n\rThe following materials are available:\n\r", 0);
		    send_to_char("{Wbrass{D, {Wbronze{D, {Wcopper{D, {Wdragonhide{D, {Wdragonscale{D, {Whide{D, {Wiron{D, {Wleather{D,{x\n\r", d->character);
		    send_to_char("{Wplatinum{D, {Wsilver{D, {Wsteel{D, {Wwood{x\n\r\n\r", d->character);
		    write_to_buffer(d, "Select from the above list of materials: ", 0);
		    d->connected = CON_HEIRLOOM_MATERIAL;
		}
		else if (!str_cmp(argument, "about"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_ABOUT);
		    d->character->carrying->weight = 30;

		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An old pile of clothing is here.");

            setName(*d->character->carrying, "old clothing");

		    write_to_buffer(d, "\n\rAbout body items typically come in the form of robes, tabards, and cloaks,\n\r", 0);
		    send_to_char("so the following material choices are available: {Wcloth{w, {Wsilk\n\r\n\r", d->character);
		    write_to_buffer(d, "Select from the above list of materials: ", 0);
		    d->connected = CON_HEIRLOOM_MATERIAL;
		}
		else if (!str_cmp(argument, "waist"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_WAIST);
		    d->character->carrying->weight = 10;

		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An old belt lies here.");

            setName(*d->character->carrying, "old belt");

		    write_to_buffer(d, "\n\rThe following materials are available:\n\r", 0);
		    send_to_char("{Wcloth{D, {Wleather{D, {Wsilk{x\n\r\n\r", d->character);
		    write_to_buffer(d, "Select from the above list of materials: ", 0);
		    d->connected = CON_HEIRLOOM_MATERIAL;
		}
		else if (!str_cmp(argument, "wrist"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_WRIST);
		    d->character->carrying->weight = 20;

		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An old bracer lies here.");

            setName(*d->character->carrying, "old bracer");

		    write_to_buffer(d, "\n\rThe following materials are available:\n\r", 0);
		    send_to_char("{Wbrass{D, {Wbronze{D, {Wcopper{D, {Wdragonhide{D, {Wdragonscale{D, {Whide{D, {Wiron{D, {Wleather{D,{x\n\r", d->character);
		    send_to_char("{Wplatinum{D, {Wsilver{D, {Wsteel{D, {Wwood{x\n\r\n\r", d->character);
		    write_to_buffer(d, "Select from the above list of materials: ", 0);
		    d->connected = CON_HEIRLOOM_MATERIAL;
		}
		else
		    write_to_buffer(d, "Enter an armor location to continue: ", 0);		    

		break;

	    case ITEM_JEWELRY:
		if (!str_cmp(argument, "finger"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_FINGER);

		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An elegant ring glints on the ground.");

            setName(*d->character->carrying, "elegant ring");
		}
		else if (!str_cmp(argument, "neck"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_NECK);

		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An old piece of jewelry gleams on the ground.");

            setName(*d->character->carrying, "old jewelry");
		}
		else if (!str_cmp(argument, "head"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_HEAD);

		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An old piece of jewelry gleams on the ground.");

            setName(*d->character->carrying, "old jewelry");
		}
		else if (!str_cmp(argument, "arm"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_ARMS);

		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An elegant armband gleams on the ground.");

            setName(*d->character->carrying, "elegant armband");
		}
		else if (!str_cmp(argument, "waist"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_WEAR_WAIST);

		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("A gleaming belt buckle lies on the ground.");

            setName(*d->character->carrying, "gleaming buckle");
		}
		else if (!str_cmp(argument, "held"))
		{
		    SET_BIT(d->character->carrying->wear_flags, ITEM_HOLD);

		    free_string(d->character->carrying->description);
		    d->character->carrying->description = str_dup("An old family treasure lies on the ground.");

            setName(*d->character->carrying, "old treasure");
		}
		else
		{
		    write_to_buffer(d, "Enter a jewelry type to continue: ", 0);
		    break;
		}

		write_to_buffer(d, "\n\rThe following materials are available:\n\r", 0);
		write_to_buffer(d, "amber, amethyst, aquamarine, bloodstone, bone, brass, bronze, copper,\n\r", 0);
		write_to_buffer(d, "crystal, diamond, electrum, ebony, emerald, gem, glass, gold, ivory, jade,\n\r", 0);
		write_to_buffer(d, "obsidian, onyx, opal, pearl, platinum, ruby, sapphire, silver, stone, topaz\n\r\n\r", 0);
		write_to_buffer(d, "Select from the above list of materials: ", 0);
    		d->connected = CON_HEIRLOOM_MATERIAL;

		break;
	}
	break;

    case CON_HEIRLOOM_TYPE3:
	switch(d->character->carrying->item_type)
	{
	    case ITEM_WEAPON:
	    {
		switch(d->character->carrying->value[0])
		{
		    case WEAPON_EXOTIC:
			if (!str_cmp(argument, "slashing"))
			{
			    d->character->carrying->value[3] = DAM_SLASH;
			    free_string(d->character->carrying->obj_str);
			    d->character->carrying->obj_str = str_dup("slash");
			}
			else if (!str_cmp(argument, "piercing"))
			{
			    d->character->carrying->value[3] = DAM_PIERCE;
			    free_string(d->character->carrying->obj_str);
			    d->character->carrying->obj_str = str_dup("pierce");
			}
			else if (!str_cmp(argument, "bashing"))
			{
			    d->character->carrying->value[3] = DAM_BASH;
			    free_string(d->character->carrying->obj_str);
			    d->character->carrying->obj_str = str_dup("bash");
			}
			else
			{
			    write_to_buffer(d, "Please select from the list above: ", 0);
			    break;
			}

			write_to_buffer(d, "bone\n\rbronze\n\rcopper\n\riron\n\rplatinum\n\rsilver\n\rsteel\n\rwood\n\r\n\r", 0);
			write_to_buffer(d, "Select from the above list of materials: ", 0);
			d->connected = CON_HEIRLOOM_MATERIAL;

			break;

		    case WEAPON_SWORD:
			if (!str_cmp(argument, "shortsword"))
			{
			    d->character->carrying->value[2] = 5;
			    free_string(d->character->carrying->obj_str);
			    d->character->carrying->obj_str = str_dup("pierce");
			    d->character->carrying->value[3] = DAM_PIERCE;
			    d->character->carrying->weight = 100;
			}
			else if (!str_cmp(argument, "longsword"))
			{
			    free_string(d->character->carrying->obj_str);
			    d->character->carrying->obj_str = str_dup("slash");
			    d->character->carrying->value[3] = DAM_SLASH;
			    d->character->carrying->value[2] = 6;
			    d->character->carrying->weight = 170;
	    		}
			else if (!str_cmp(argument, "greatsword"))
			{
			    free_string(d->character->carrying->obj_str);
			    d->character->carrying->obj_str = str_dup("slash");
			    d->character->carrying->value[3] = DAM_SLASH;
			    d->character->carrying->value[2] = 7;
			    d->character->carrying->weight = 220;
			    SET_BIT(d->character->carrying->value[4], WEAPON_TWO_HANDS);
			}
			else
			{
			    write_to_buffer(d, "Please select from the list above: ", 0);
			    break;
			}

			write_to_buffer(d, "bone\n\rbronze\n\rcopper\n\riron\n\rplatinum\n\rsilver\n\rsteel\n\r\n\r", 0);
			write_to_buffer(d, "Select from the above list of materials: ", 0);
			d->connected = CON_HEIRLOOM_MATERIAL;

			break;

		    case WEAPON_SPEAR:
			if (!str_cmp(argument, "short spear"))
			{
			    d->character->carrying->value[2] = 5;
			    d->character->carrying->weight = 140;
			}
			else if (!str_cmp(argument, "long spear"))
			{
			    d->character->carrying->value[2] = 6;
			    d->character->carrying->weight = 200;
			    SET_BIT(d->character->carrying->value[4], WEAPON_TWO_HANDS);
			}
			else
			{
			    write_to_buffer(d, "Please select from the list above: ", 0);
			    break;
			}	    
		    
			write_to_buffer(d, "bone\n\rbronze\n\rcopper\n\riron\n\rplatinum\n\rsilver\n\rsteel\n\rwood\n\r\n\r", 0);
			write_to_buffer(d, "Select from the above list of materials: ", 0);
			d->connected = CON_HEIRLOOM_MATERIAL;

			break;

		    case WEAPON_AXE:
			if (!str_cmp(argument, "battle axe"))
			{
			    d->character->carrying->value[2] = 7;
			    d->character->carrying->weight = 260;
			    SET_BIT(d->character->carrying->value[4], WEAPON_TWO_HANDS);
			}
			else if (!str_cmp(argument, "hand axe"))
			{
			    d->character->carrying->value[2] = 6;
			    d->character->carrying->weight = 150;
			}
			else
			{
			    write_to_buffer(d, "Please select from the list above: ", 0);
			    break;
			}

			write_to_buffer(d, "bronze\n\rcopper\n\riron\n\rplatinum\n\rsilver\n\rsteel\n\r\n\r", 0);
			write_to_buffer(d, "Select from the above list of materials: ", 0);
			d->connected = CON_HEIRLOOM_MATERIAL;

			break;
		}
	    }
	    break;

	    case ITEM_ARMOR:
		if (IS_SET(d->character->carrying->wear_flags, ITEM_WEAR_BODY))
		{
		    if (!str_cmp(argument, "armor shirt"))
		    {
			d->character->carrying->value[0] = 11;
			d->character->carrying->value[1] = 11;
			d->character->carrying->value[2] = 11;
			d->character->carrying->value[3] = 5;
			d->character->carrying->weight = 200;

			free_string(d->character->carrying->description);
			d->character->carrying->description = str_dup("An old armored chestpiece lies here.");

            setName(*d->character->carrying, "old chestpiece");

			write_to_buffer(d, "\n\rThe following materials are available:\n\r", 0);
			send_to_char("{Wbrass{D, {Wbronze{D, {Wcopper{D, {Wdragonhide{D, {Wdragonscale{D, {Whide{D, {Wiron{D, {Wleather{D,{x\n\r", d->character);
			send_to_char("{Wplatinum{D, {Wsilver{D, {Wsteel{D, {Wwood{x\n\r\n\r", d->character);
			write_to_buffer(d, "Select from the above list of materials: ", 0);
			d->connected = CON_HEIRLOOM_MATERIAL;
		    }
		    else if (!str_cmp(argument, "jerkin"))
		    {
			d->character->carrying->value[0] = 9;
			d->character->carrying->value[1] = 9;
			d->character->carrying->value[2] = 9;
			d->character->carrying->value[3] = 3;
			d->character->carrying->weight = 30;

			free_string(d->character->carrying->description);
			d->character->carrying->description = str_dup("An old jerkin lies here.");

            setName(*d->character->carrying, "old jerkin");

			write_to_buffer(d, "\n\rThe following materials are available:\n\r", 0);
			send_to_char("{Wcloth{D, {Wsilk{x\n\r\n\r", d->character);
			write_to_buffer(d, "Select from the above list of materials: ", 0);
			d->connected = CON_HEIRLOOM_MATERIAL;
		    }
		    else
		    {
			write_to_buffer(d, "Please select from the list above: ", 0);
			break;
		    }
		}
		else if (IS_SET(d->character->carrying->wear_flags, ITEM_WEAR_HEAD))
		{
		    if (!str_cmp(argument, "helmet"))
		    {
			d->character->carrying->value[0] = 11;
			d->character->carrying->value[1] = 11;
			d->character->carrying->value[2] = 11;
			d->character->carrying->value[3] = 5;
			d->character->carrying->weight = 50;

			free_string(d->character->carrying->description);
			d->character->carrying->description = str_dup("An old helmet lies here.");

            setName(*d->character->carrying, "old helmet");

			write_to_buffer(d, "\n\rThe following materials are available:\n\r", 0);
			send_to_char("{Wbrass{D, {Wbronze{D, {Wcopper{D, {Wdragonhide{D, {Wdragonscale{D, {Whide{D, {Wiron{D, {Wleather{D,{x\n\r", d->character);
			send_to_char("{Wplatinum{D, {Wsilver{D, {Wsteel{D, {Wwood{x\n\r\n\r", d->character);
			write_to_buffer(d, "Select from the above list of materials: ", 0);
			d->connected = CON_HEIRLOOM_MATERIAL;
		    }
		    else if (!str_cmp(argument, "cap"))
		    {
			d->character->carrying->value[0] = 9;
			d->character->carrying->value[1] = 9;
			d->character->carrying->value[2] = 9;
			d->character->carrying->value[3] = 3;
			d->character->carrying->weight = 10;

			free_string(d->character->carrying->description);
			d->character->carrying->description = str_dup("An old cap lies here.");

            setName(*d->character->carrying, "old cap");

			write_to_buffer(d, "\n\rThe following materials are available:\n\r", 0);
			send_to_char("{Wcloth{D, {Wleather{D, {Wsilk{x\n\r\n\r", d->character);
			write_to_buffer(d, "Select from the above list of materials: ", 0);
			d->connected = CON_HEIRLOOM_MATERIAL;
		    }
		    else
		    {
			write_to_buffer(d, "Please select from the list above: ", 0);
			break;
		    }
		}
		else if (IS_SET(d->character->carrying->wear_flags, ITEM_WEAR_HANDS))
		{
		    if (!str_cmp(argument, "gauntlets"))
		    {
			d->character->carrying->value[0] = 11;
			d->character->carrying->value[1] = 11;
			d->character->carrying->value[2] = 11;
			d->character->carrying->value[3] = 5;
			d->character->carrying->weight = 70;

			free_string(d->character->carrying->description);
			d->character->carrying->description = str_dup("An old pair of gauntlets lies here.");

            setName(*d->character->carrying, "old gauntlets");

			write_to_buffer(d, "\n\rThe following materials are available:\n\r", 0);
			send_to_char("{Wbrass{D, {Wbronze{D, {Wcopper{D, {Wdragonhide{D, {Wdragonscale{D, {Whide{D, {Wiron{D, {Wleather{D,{x\n\r", d->character);
			send_to_char("{Wplatinum{D, {Wsilver{D, {Wsteel{D, {Wwood{x\n\r\n\r", d->character);
			write_to_buffer(d, "Select from the above list of materials: ", 0);
			d->connected = CON_HEIRLOOM_MATERIAL;
		    }
		    else if (!str_cmp(argument, "gloves"))
		    {
			d->character->carrying->value[0] = 9;
			d->character->carrying->value[1] = 9;
			d->character->carrying->value[2] = 9;
			d->character->carrying->value[3] = 3;
			d->character->carrying->weight = 5;

			free_string(d->character->carrying->description);
			d->character->carrying->description = str_dup("An old pair of gloves lies here.");

            setName(*d->character->carrying, "old gloves");

			write_to_buffer(d, "\n\rThe following materials are available:\n\r", 0);
			send_to_char("{Wcloth{D, {Wsilk{D, {Wvelvet{x\n\r\n\r", d->character);
			write_to_buffer(d, "Select from the above list of materials: ", 0);
			d->connected = CON_HEIRLOOM_MATERIAL;
		    }
		    else
		    {
			write_to_buffer(d, "Please select from the list above: ", 0);
			break;
		    }
		}
	}		
	break;

    case CON_HEIRLOOM_MATERIAL:
	switch (d->character->carrying->item_type)
	{
	    case ITEM_WEAPON:
	    {
		if (!str_cmp(argument, "bone")
		 && (d->character->carrying->value[0] != WEAPON_AXE
		  && d->character->carrying->value[0] != WEAPON_POLEARM))
		    d->character->carrying->material = material_lookup("bone");
		else if (!str_cmp(argument, "bronze"))
		    d->character->carrying->material = material_lookup("bronze");
		else if (!str_cmp(argument, "copper")
		 && (d->character->carrying->value[0] != WEAPON_POLEARM))
		    d->character->carrying->material = material_lookup("copper");
		else if (!str_cmp(argument, "diamond")
		 && (d->character->carrying->value[0] == WEAPON_DAGGER))
		    d->character->carrying->material = material_lookup("diamond");
		else if (!str_cmp(argument, "iron"))
		    d->character->carrying->material = material_lookup("iron");
		else if (!str_cmp(argument, "platinum"))
		    d->character->carrying->material = material_lookup("platinum");
		else if (!str_cmp(argument, "silver"))
		    d->character->carrying->material = material_lookup("silver");
		else if (!str_cmp(argument, "steel"))
		    d->character->carrying->material = material_lookup("steel");
		else
		{
		    write_to_buffer(d, "Select from the above list of materials: ", 0);
		    break;
		}

		sprintf(buf, "An old %s %s lies here.",
		    material_table[d->character->carrying->material].name,
		    flag_string( weapon_class, d->character->carrying->value[0] ));

		free_string(d->character->carrying->description);
		d->character->carrying->description = str_dup(buf);

		sprintf(buf, "old %s %s",
		    material_table[d->character->carrying->material].name,
		    flag_string( weapon_class, d->character->carrying->value[0] ));

        setName(*d->character->carrying, buf);

		write_to_buffer(d, "\n\rYou now need to provide a short description for your heirloom.  Examples:\n\r", 0);
		write_to_buffer(d, "the silver rapier of the Narith family\n\r", 0);
		write_to_buffer(d, "a massive iron claymore\n\r", 0);
		write_to_buffer(d, "an elegant steel dagger\n\r\n\r", 0);

		write_to_buffer(d, "(10-50 characters): ", 0);
		d->connected = CON_HEIRLOOM_SHORT;
	    }
	    break;

	    case ITEM_ARMOR:
	    {
		if ((!str_cmp(argument, "brass")
		  || !str_cmp(argument, "bronze")
		  || !str_cmp(argument, "copper")
		  || !str_cmp(argument, "dragonhide")
		  || !str_cmp(argument, "dragonscale")
		  || !str_cmp(argument, "hide")
		  || !str_cmp(argument, "iron")
		  || !str_cmp(argument, "platinum")
		  || !str_cmp(argument, "silver")
		  || !str_cmp(argument, "steel")) 
		 && d->character->carrying->value[0] != 9
		 && !IS_SET(d->character->carrying->wear_flags, ITEM_WEAR_ABOUT)
		 && !IS_SET(d->character->carrying->wear_flags, ITEM_WEAR_WAIST))
		    d->character->carrying->material = material_lookup(argument);
		else if ((!str_cmp(argument, "cloth")
		  || !str_cmp(argument, "silk"))
		 && d->character->carrying->value[0] != 11
		 && !IS_SET(d->character->carrying->wear_flags, ITEM_WEAR_SHIELD)
		 && !IS_SET(d->character->carrying->wear_flags, ITEM_WEAR_WRIST))
		    d->character->carrying->material = material_lookup(argument);
		else if (!str_cmp(argument, "leather")
		 && (IS_SET(d->character->carrying->wear_flags, ITEM_WEAR_HEAD)
		  || d->character->carrying->value[0] != 9)
		 && !IS_SET(d->character->carrying->wear_flags, ITEM_WEAR_ABOUT))
		    d->character->carrying->material = material_lookup(argument);
		else if (!str_cmp(argument, "velvet")
		 && d->character->carrying->value[0] == 9
		 && IS_SET(d->character->carrying->wear_flags, ITEM_WEAR_HANDS))
		    d->character->carrying->material = material_lookup("velvet");
		else
		{
		    write_to_buffer(d, "Select from the above list of materials: ", 0);
		    break;
		}

		write_to_buffer(d, "\n\rYou now need to provide a short description for your heirloom.  Examples:\n\r", 0);
		write_to_buffer(d, "an iron breastplate\n\r", 0);
		write_to_buffer(d, "the golden armor of the Crellik family\n\r", 0);
		write_to_buffer(d, "an old chainmail vest\n\r\n\r", 0);

		write_to_buffer(d, "(10-50 characters): ", 0);
		d->connected = CON_HEIRLOOM_SHORT;
	    }
	    break;

	    case ITEM_JEWELRY:
	    {
		if (!str_cmp(argument, "amber")
		 || !str_cmp(argument, "amethyst")
		 || !str_cmp(argument, "aquamarine")
		 || !str_cmp(argument, "bloodstone")
		 || !str_cmp(argument, "bone")
		 || !str_cmp(argument, "brass")
		 || !str_cmp(argument, "bronze")
		 || !str_cmp(argument, "copper")
		 || !str_cmp(argument, "crystal")
		 || !str_cmp(argument, "diamond")
		 || !str_cmp(argument, "electrum")
		 || !str_cmp(argument, "ebony")
		 || !str_cmp(argument, "emerald")
		 || !str_cmp(argument, "gem")
		 || !str_cmp(argument, "glass")
		 || !str_cmp(argument, "gold")
		 || !str_cmp(argument, "ivory")
		 || !str_cmp(argument, "jade")
		 || !str_cmp(argument, "obsidian")
		 || !str_cmp(argument, "onyx")
		 || !str_cmp(argument, "opal")
		 || !str_cmp(argument, "pearl")
		 || !str_cmp(argument, "platinum")
		 || !str_cmp(argument, "ruby")
		 || !str_cmp(argument, "sapphire")
		 || !str_cmp(argument, "silver")
		 || !str_cmp(argument, "stone")
		 || !str_cmp(argument, "topaz"))
		    d->character->carrying->material = material_lookup(argument);
		else
		{
		    write_to_buffer(d, "Select from the above list of materials: ", 0);
		    break;
		}

		write_to_buffer(d, "\n\rYou now need to provide a short description for your heirloom.  Examples:\n\r", 0);
		write_to_buffer(d, "a simple golden ring\n\r", 0);
		write_to_buffer(d, "the crested ring of the Ghakunda family\n\r", 0);
		write_to_buffer(d, "a necklace of thick gold\n\r\n\r", 0);

		write_to_buffer(d, "(10-50 characters): ", 0);
		d->connected = CON_HEIRLOOM_SHORT;
	    }
	}
	break;

    case CON_HEIRLOOM_SHORT:
	if (strlen(argument) < 10 || strlen(argument) > 50)
	{
	    write_to_buffer(d, "Short description must be between 10 and 50 characters long!\n\r", 0);
	    write_to_buffer(d, "Please enter description: ", 0);
	    break;
	}

	free_string(d->character->carrying->short_descr);
	d->character->carrying->short_descr = str_dup(argument);

	name_to_keywords(argument, buf);
	strcat(buf, " ");
	strcat(buf, d->character->carrying->name);

    setName(*d->character->carrying, buf);

	write_to_buffer(d, "\n\rThe next step is to enter a description that others will see when\n\r", 0);
	write_to_buffer(d, "examining the object.  Press Enter to enter the editor...", 0);
	d->connected = CON_HEIRLOOM_ED;

	break;	

    case CON_HEIRLOOM_ED:
	d->character->carrying->extra_descr = new_extra_descr();

	d->character->carrying->extra_descr->next = NULL;
	d->character->carrying->extra_descr->keyword = str_dup(d->character->carrying->name);
	d->character->carrying->extra_descr->description = str_dup("");

	string_append(d->character, &d->character->carrying->extra_descr->description);
	d->connected = CON_INIT_CHAR;

	break;

    case CON_INIT_CHAR:
//	if (BIT_GET(d->character->pcdata->traits, TRAIT_HEIRLOOM))
//	    heirloom_notice(d->character->carrying);

	write_to_buffer(d,"\n\r",2);
        sprintf( log_buf, "%s@%s new player.", ch->name, d->host );
        log_string( log_buf );
        wiznet("Newbie alert!  $N sighted.",ch,NULL,WIZ_NEWBIE,0,0);
        wiznet(log_buf,NULL,NULL,WIZ_SITES,0,get_trust(ch));
	set_perm_stat(ch, STAT_STR, pc_race_table[ch->race].stats[STAT_STR]);
	set_perm_stat(ch, STAT_INT, pc_race_table[ch->race].stats[STAT_INT]);
	set_perm_stat(ch, STAT_WIS, pc_race_table[ch->race].stats[STAT_WIS]);
	set_perm_stat(ch, STAT_DEX, pc_race_table[ch->race].stats[STAT_DEX]);
	set_perm_stat(ch, STAT_CON, pc_race_table[ch->race].stats[STAT_CON]);
	set_perm_stat(ch, STAT_CHR, pc_race_table[ch->race].stats[STAT_CHR]);
        ch->speaking = race_table[ch->race].native_tongue;

	if (BIT_GET(ch->pcdata->traits, TRAIT_LONGLIFE))
	    ch->pcdata->max_age = UMIN(pc_race_table[ch->race].age_values[2], ch->pcdata->max_age + number_range(5, 10));

	if (BIT_GET(ch->pcdata->traits, TRAIT_SURVIVOR))
	    ch->pcdata->max_deaths += number_range(5, 10);

 	weapon = weapon_lookup("dagger");

	if (ch->class_num == global_int_class_swordmaster
	|| ch->class_num == global_int_class_gladiator
	|| ch->class_num == global_int_class_fighter
	|| ch->class_num == global_int_class_ranger
	|| ch->class_num == global_int_class_watertemplar
	|| ch->class_num == global_int_class_spirittemplar
	|| ch->class_num == global_int_class_earthtemplar
	|| ch->class_num == global_int_class_airtemplar
	|| ch->class_num == global_int_class_voidtemplar
	|| ch->class_num == global_int_class_firetemplar)
		ch->pcdata->learned[gsn_sword] = 40; 
        else if (ch->class_num == global_int_class_barbarian)
	        ch->pcdata->learned[gsn_axe] = 40; 
	else if (ch->class_num == global_int_class_waterscholar
	|| ch->class_num == global_int_class_spiritscholar
	|| ch->class_num == global_int_class_voidscholar
	|| ch->class_num == global_int_class_druid
	|| ch->class_num == global_int_class_watcher)
	        ch->pcdata->learned[gsn_staff] = 40;
   	else if (ch->class_num == global_int_class_earthscholar
 	|| ch->class_num == global_int_class_bandit)
	        ch->pcdata->learned[gsn_mace] = 40;
	else if (ch->class_num == global_int_class_airscholar
	|| ch->class_num == global_int_class_firescholar
	|| ch->class_num == global_int_class_thief
	|| ch->class_num == global_int_class_assassin)
	        ch->pcdata->learned[gsn_dagger] = 40; 
	else 
		ch->pcdata->learned[*weapon_table[weapon].gsn] = 40;

        if (ch->class_num == global_int_class_spirittemplar)
            modify_karma(ch, -500);
	
	if (ch->race == global_int_race_human)
            for (i = 0; i < 6; i++)
		ch->perm_stat[i] += ch->attr_prime[i];
	if (BIT_GET(ch->pcdata->traits,TRAIT_CHARMING))
	    set_perm_stat(ch,STAT_CHR,ch->perm_stat[STAT_CHR]+1);

	ch->coins[coin_lookup("copper")] = 100;
	ch->exp     = 1800;
	ch->ep     = 0;
	ch->level = 1; // This is just for display purposes
	if (d->newbie)
	    SET_BIT(ch->nact,PLR_NEWBIE);

	int god;
	for (god=0;god<MAX_GODS;god++)
	{
	    ch->pcdata->sacrifices[god].level=0;
	    ch->pcdata->sacrifices[god].number=0;
	}
	
        sprintf( buf, "%s", Titles::LookupTitle(*ch));
	set_title( ch, buf );
        ch->pcdata->extitle = str_dup( "" );

	write_to_buffer(d,"\n\r",2);
	do_score(ch,"");
	ch->level = 0; // We don't want to break item limits with this
	write_to_buffer(d,"Are you satisfied with this character (Y/n)? ",0);
	d->connected = CON_ACCEPT_CHAR;
	break;

    case CON_ACCEPT_CHAR:
	switch (UPPER(argument[0]))
	{
	    default:
		write_to_buffer(d,"Please enter Y or N.\n\rAre you satisfied with this character (y/n)? ", 0);
		d->connected = CON_ACCEPT_CHAR;
		break;
	    case 'Y':
		write_to_buffer(d,"Character accepted.\n\r",0);
		d->connected = CON_READ_MOTD;	
		sprintf(buf, "motd%d", port);
		do_help(ch, buf);
		d->connected = CON_READ_MOTD;
		break;
	    case 'N':
		write_to_buffer(d, "Restarting the creation process.\n\r",0);
		free_char(d->character);
		d->character = NULL;
		d->connected = CON_GET_NAME;
		write_to_buffer(d, "Under what name shall your deeds be recorded?\n\r",0 );
		break;
	}
	break;

    case CON_READ_IMOTD:
        write_to_buffer(d,"\n\r",2);
	sprintf(buf, "motd%d", port);
        do_help( ch, buf );
        d->connected = CON_READ_MOTD;
	break;

    case CON_READ_MOTD:
        if ( ch->pcdata == NULL || ch->pcdata->pwd[0] == '\0')
        {
            write_to_buffer( d, "Warning! Null password!\n\r",0 );
            write_to_buffer( d, "Please report old password with bug.\n\r",0);
            write_to_buffer( d, "Type 'password null <new password>' to fix.\n\r",0);
        }

	write_to_buffer(d, "\n\rYou open your eyes and cast your gaze upon the land.\n\r", 0);
	ch->next	= char_list;
	char_list	= ch;

	d->connected	= CON_PLAYING;
	reset_char(ch);

	if ( ch->level == 0 )
	{
	    ch->level = 1;
	    player_levels++;
	    ch->hit	= ch->max_hit;
	    ch->mana	= ch->max_mana;
	    ch->move	= ch->max_move;
	    ch->familiar_type = number_range(50, 59);
	    ch->id	= get_pc_id();
	    ch->pcdata->age_group = AGE_YOUTHFUL;

	    SET_BIT(ch->act, PLR_AUTOATTACK);
	    SET_BIT(ch->act, PLR_AUTODEFEND);
	    SET_BIT(ch->act, PLR_AUTOASSIST);
	    SET_BIT(ch->act, PLR_AUTOEXIT);
	    SET_BIT(ch->act, PLR_AUTOLOOT);
	    SET_BIT(ch->act, PLR_AUTODES);
	    SET_BIT(ch->act, PLR_AUTOGOLD);
	    SET_BIT(ch->act, PLR_AUTOSPLIT);
	    SET_BIT(ch->act, PLR_CANLOOT);
	    SET_BIT(ch->act, PLR_AUTODATE);
	    SET_BIT(ch->act, PLR_SHOWLINES);
	    SET_BIT(ch->act, PLR_EPNOTE);

	    do_outfit(ch,"");

	    char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
	    send_to_char("\n\r",ch);
	    do_help(ch,"NEWBIE INFO");
	    send_to_char("\n\r",ch);
	}
	else if ( ch->in_room != NULL )
	{
	    char_to_room( ch, ch->in_room );
	}
	else if ( IS_IMMORTAL(ch) )
	{
	    char_to_room( ch, get_room_index( ROOM_VNUM_CHAT ) );
	}
	else
	{
	    char_to_room( ch, get_room_index( ROOM_VNUM_TEMPLE ) );
	}

	// The player has logged back on; clear the delete_requested field
	if (ch->pcdata->delete_requested != 0)
	{
		send_to_char("{RDelete request cleared.{x\n\r", ch);
		wiznet("$N has logged back in, clearing their delete request", ch, 0, 0, 0, 0);
		ch->pcdata->delete_requested = 0;
	}

	if (ch->pcdata->recycle_log[0] != '\0')
	{
	    NOTE_DATA *pnote;
	    char strtime[80];

	    pnote = new_note();
    
	    pnote->sender	= str_dup("Immortal");
	    pnote->to_list	= str_dup(ch->name);
	    pnote->subject  	= str_dup("Equipment recycled");
	    pnote->type		= NOTE_NOTE;

	    strcpy(strtime, ctime(&current_time));
	    strtime[strlen(strtime)-1] = '\0';
	    pnote->date		= str_dup(strtime);
	    pnote->date_stamp	= current_time;

	    sprintf(buf, "During your long absence from the realm, the following limited items\n\rwere taken from your possession:\n\r\n\r%s\n\rSee help 'LIMITED EQUIPMENT' for more details.\n\r", ch->pcdata->recycle_log);

	    pnote->text	= str_dup(buf);

	    append_note(pnote);

	    free_string(ch->pcdata->recycle_log);
	    ch->pcdata->recycle_log = str_dup("");
	}

    // Update any attunements
    Weave::UpdateAttunements();

   	for ( ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room )
            if (!((!can_see(ich,ch)) && (IS_IMMORTAL(ch))))
                act_new("$n enters the land and takes stock of $s surroundings.",ch,ch,ich,TO_VICT,POS_RESTING);

	if (!IS_IMMORTAL(ch) && ch->level > 9)
            ch->pcdata->adrenaline = 6; /* everyone stays 3 minutes */

	if (ch->pcdata->dailyxp == 0 && ch->level < LEVEL_HERO && ch->level > 1)
	{
	    int xpadd = (exp_on_level(ch,ch->level+1) - exp_on_level(ch,ch->level))/20;
   	    sprintf(log_buf,"You gained %u experience today.\n\r",xpadd);
	    send_to_char(log_buf,ch);
	    sprintf(log_buf,"%s gained %u experience today.\n\r",ch->name,xpadd);
	    log_string(log_buf);
	    gain_exp(ch,xpadd);
	    ch->pcdata->dailyxp = 1;
	}
	
	if (ch->class_num == global_int_class_druid)
	    lunar_update_char(ch);
	
        do_look( ch,"auto" );

	if (ch->level == 1)
	{
	    if (IS_SET(ch->comm, COMM_NONEWBIE))
		send_to_char("{cType \"newbie\" if you wish to enable the global new players channel.{x\n\r", ch);
	    else
	    {
		send_to_char("\n\r{cJoining new players channel.  Type 'newbie <text>' to talk.{x\n\r", ch);
		sprintf(buf, "{c[{gNewbie{c] {CNew player '%s' has joined the channel!{x\n\r", ch->name);
		for (d_find = descriptor_list; d_find; d_find = d_find->next)
		{
		    CHAR_DATA *victim;
		    victim = d_find->original ? d_find->original : d_find->character;

		    if ( d_find->connected == CON_PLAYING
		    && (IS_IMMORTAL(victim)
		     || victim->level < 10
		     || (d_find->acct && IS_SET(d_find->acct->flags, ACCT_MENTOR)))
		    && !IS_SET(victim->comm, COMM_NONEWBIE))
		    send_to_char(buf, d_find->character);
		}
	    }
	}

	do_upcount();

        sprintf(buf, "%s has left real life behind. [Room %d]",
		ch->name, ch->in_room ? ch->in_room->vnum : -1);
	wiznet(buf,NULL,NULL,
		WIZ_LOGINS,WIZ_SITES,get_trust(ch));

    // Add in all pets
    for (CHAR_DATA * pet(char_list); pet != NULL; pet = pet->next)
    {
        if (pet->in_room == NULL && pet->leader == ch && pet->master == ch)
        {
	        char_to_room(pet, ch->in_room);
            act("$n follows $N closely behind.",pet,NULL,ch,TO_ROOM);
	    }
    }

	/*if (ch->pet != NULL)
	{
	    char_to_room(ch->pet,ch->in_room);
            act("$n follows $N closely behind.",ch->pet,NULL,ch,TO_ROOM);
	}

	if (ch->familiar != NULL)
	{
	    char_to_room(ch->familiar, ch->in_room);
	    act("$n follows $N closely behind.", ch->familiar, NULL, ch, TO_ROOM);
	}*/

	do_unread(ch, "");
	break;
    }
}

int roll_stat( CHAR_DATA *ch, int stat)
{
return (number_range(UMIN(pc_race_table[ch->race].stats[stat]+2, pc_race_table[ch->race].max_stats[stat]-2), UMAX(pc_race_table[ch->race].stats[stat]+2, pc_race_table[ch->race].max_stats[stat]-1)));
}

int align_trans(int num)
{
if (num < -700)
	return 4;
if (num == 0)
	return 2;
if (num > 700)
	return 1;

return -1;
}

int bit_trans(int num)
{
	switch(num)
	{
	case 0:
		return 1;
	case 1:
		return 2;
	case 2:
		return 4;
	default:
		return 0;
	}
}



void good_ok(CHAR_DATA *ch, char *buf)
{
if (pc_race_table[ch->race].allowed_aligns & BIT_GOOD)
	{
	strcpy(buf, "good");
	}
	else
	{
	strcpy(buf, "");
	}
}

void neutral_ok(CHAR_DATA *ch, char *buf)
{
if (pc_race_table[ch->race].allowed_aligns & BIT_NEUTRAL)
	{
	strcpy(buf, "neutral");
	}
	else
	{
	strcpy(buf, "");
	}
}

void evil_ok(CHAR_DATA *ch, char *buf)
{
if (pc_race_table[ch->race].allowed_aligns & BIT_EVIL)
	{
	strcpy(buf, "evil");
	}
	else
	{
	strcpy(buf, "");
	}
}

void lawful_ok(CHAR_DATA *ch, char *buf)
{
if (pc_race_table[ch->race].allowed_ethos & BIT_LAWFUL)
	{
	strcpy(buf, "lawful");
	}
	else
	{
	strcpy(buf, "");
	}
}

void balanced_ok(CHAR_DATA *ch, char *buf)
{
if (pc_race_table[ch->race].allowed_ethos & BIT_BALANCED)
	{
	strcpy(buf, "neutral");
	}
	else
	{
	strcpy(buf, "");
	}
}

void chaotic_ok(CHAR_DATA *ch, char *buf)
{
if (pc_race_table[ch->race].allowed_ethos & BIT_CHAOTIC)
	{
	strcpy(buf, "chaotic");
	}
	else
	{
	strcpy(buf, "");
	}
}

int scholar_ok(CHAR_DATA *ch)
{
if (class_table[0].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[1].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[2].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[3].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[4].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[5].races_allowed[ch->race] == 1)
return TRUE;

return FALSE;
}


int templar_ok(CHAR_DATA *ch)
{
if (class_table[6].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[7].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[8].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[9].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[10].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[11].races_allowed[ch->race] == 1)
return TRUE;

return FALSE;
}

int rogue_ok(CHAR_DATA *ch)
{
if (class_table[12].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[13].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[15].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[24].races_allowed[ch->race] == 1)
return TRUE;

return FALSE;
}

int warrior_ok(CHAR_DATA *ch)
{
if (class_table[18].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[19].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[20].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[21].races_allowed[ch->race] == 1)
return TRUE;

return FALSE;
}

int naturalist_ok(CHAR_DATA *ch)
{
if (class_table[23].races_allowed[ch->race] == 1)
return TRUE;

if (class_table[29].races_allowed[ch->race] == 1)
return TRUE;

return FALSE;
}

int other_ok(CHAR_DATA *ch)
{
    if (class_table[25].races_allowed[ch->race] == 1)
	return TRUE;

    return FALSE;
}

int mentalist_ok(CHAR_DATA *ch)
{
    if (class_table[14].races_allowed[ch->race] == 1)
	return TRUE;

    if (class_table[28].races_allowed[ch->race] == 1)
	return TRUE;

    return FALSE;
}

int entertainer_ok(CHAR_DATA *ch)
{
    if (class_table[25].races_allowed[ch->race] == 1)
	return TRUE;

    return FALSE;
}


/*
 * Parse a name for acceptability.
 */
bool check_parse_name( char *name, bool long_name )
{
    FILE *fp;
    char buf[MAX_INPUT_LENGTH];
    char strArea[MAX_INPUT_LENGTH];

    /*
     * Reserved words.
     */
    if ( is_name( name, 
	"all auto immortal self someone something the you demise balance circle loner honor shunned champion guardian raider conquest wanderer") )
	return FALSE;
	
    if (strcmp(capitalize(name),"Ashur") && (!str_prefix("Ashur",name)
    || !str_suffix("Ashur",name)))
	return FALSE;

   if (is_name(name, dictwords))
	return FALSE;
/*
    for (x = 0; x < strlen(name); x++)
    {
      buf[x] = name[x];
      buf[x+1] = (char)0;

    if (x>3 && is_name(buf, dictwords))
	{
	bug ("goat1", 0);
	return FALSE;
	}
    }
*/

    /*
     * Length restrictions.
     */
     
    if ( strlen(name) <  2 )
	return FALSE;

#if defined(MSDOS)
    if ( strlen(name) >  8 )
	return FALSE;
#endif

#if defined(macintosh) || defined(unix) || defined(WIN32)
    if ((!long_name && strlen(name) > 10) || (strlen(name) > 12))
	return FALSE;
#endif

    /*
     * Alphanumerics only.
     * Lock out IllIll twits.
     */
    {
	char *pc;
	bool fIll,adjcaps = FALSE,cleancaps = FALSE;
 	unsigned int total_caps = 0;

	fIll = TRUE;
	for ( pc = name; *pc != '\0'; pc++ )
	{
	    if ( !isalpha(*pc) )
		return FALSE;

	    if ( isupper(*pc)) /* ugly anti-caps hack */
	    {
		if (adjcaps)
		    cleancaps = TRUE;
		total_caps++;
		adjcaps = TRUE;
	    }
	    else
		adjcaps = FALSE;

	    if ( LOWER(*pc) != 'i' && LOWER(*pc) != 'l' )
		fIll = FALSE;
	}

	if ( fIll )
	    return FALSE;

	if (cleancaps || (total_caps > (strlen(name)) / 2 && strlen(name) < 3))
	    return FALSE;
    }

    /*
     * Prevent players from naming themselves after mobs.
     */
    {
	extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
	MOB_INDEX_DATA *pMobIndex;
	int iHash;

	for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
	{
	    for ( pMobIndex  = mob_index_hash[iHash];
		  pMobIndex != NULL;
		  pMobIndex  = pMobIndex->next )
	    {
		if ( is_name( name, pMobIndex->player_name ) )
		    return FALSE;
	    }
	}
    }

    sprintf(buf, "%s%s", PLAYER_DIR, BADNAME_FILE);
    if ((fp = fopen(buf, "r")))
    {
	while ( !feof(fp) )
	{
            if (!fread_dyn( fp, strArea, MAX_INPUT_LENGTH))
	    {
		sprintf(log_buf, "check_parse_name: error reading %s", buf);
		bug(log_buf, 0);
	    }

	    if (!str_cmp(name, strArea))
	    {
		fclose(fp);
	        return FALSE;
	    }
	}
	fclose(fp);
    }

    return TRUE;
}



/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect( DESCRIPTOR_DATA *d, const char *name, bool fConn )
{
    CHAR_DATA *ch,*ich;

    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
	if ( !IS_NPC(ch)
	&&   (!fConn || ch->desc == NULL)
	&&   !str_cmp( d->character->name, ch->name ) )
	{
	    if ( fConn == FALSE )
	    {
		free_string( d->character->pcdata->pwd );
		d->character->pcdata->pwd = str_dup( ch->pcdata->pwd );
	    }
	    else
	    {
		free_char( d->character );
		d->character = ch;
		ch->desc	 = d;
		ch->timer	 = 0;

		if (buf_string(ch->pcdata->buffer)[0] == '\0')
		    send_to_char("Reconnecting.  You have no new tells waiting.\n\r", ch);
		else
		    send_to_char("Reconnecting.  Type replay to see missed tells.\n\r", ch );

		do_unread(ch, "new");

    		for ( ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room )
    		{
      		    if (can_see(ich,ch) || !IS_IMMORTAL(ch))
        		act_new("$n's eyes become clear again.",ch,ch,ich,TO_VICT,POS_RESTING);
    		}

		sprintf( log_buf, "%s@%s reconnected.", ch->name, d->host );
		log_string( log_buf );
		wiznet("$N groks the fullness of $S link.", ch,NULL,WIZ_LINKS,0,0);
		d->connected = CON_PLAYING;
	    }
	    return TRUE;
	}
    }

    return FALSE;
}



/*
 * Check if already playing.
 */
bool check_playing( DESCRIPTOR_DATA *d, const char *name )
{
    DESCRIPTOR_DATA *dold;

    for ( dold = descriptor_list; dold; dold = dold->next )
    {
	if ( dold != d
	&&   dold->character != NULL
	&&   dold->connected != CON_GET_NAME
	&&   dold->connected != CON_GET_OLD_PASSWORD
	&&   !str_cmp( name, dold->original
	         ? dold->original->name : dold->character->name ) )
	{
	    write_to_buffer( d, "That character is already playing.\n\r",0);
	    write_to_buffer( d, "Do you wish to connect anyway (Y/N)?",0);
	    d->connected = CON_BREAK_CONNECT;
	    return TRUE;
	}
    }

    return FALSE;
}



void stop_idling( CHAR_DATA *ch )
{
    if ( ch == NULL
    ||   ch->desc == NULL
    ||   ch->desc->connected != CON_PLAYING
    ||   ch->was_in_room == NULL 
    ||   ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
	return;

    ch->timer = 0;
    char_from_room( ch );
    char_to_room( ch, ch->was_in_room );
    ch->was_in_room	= NULL;
    act( "$n has returned from the void.", ch, NULL, NULL, TO_ROOM );
    return;
}

void printf_to_char (CHAR_DATA *ch, char *fmt, ...)
{
    char buf [MAX_STRING_LENGTH];
    va_list args;

    va_start (args, fmt);
    vsprintf (buf, fmt, args);
    va_end (args);
	
    send_to_char (buf, ch);

    return;
}

/*
 * Write to one char.
 */
void send_to_char_bw( const char *txt, CHAR_DATA *ch )
{
    if ( txt != NULL && ch->desc != NULL )
        write_to_buffer( ch->desc, txt, strlen(txt) );
    return;
}

char	* garble_text( char *str )
{
    static char strfix[MAX_STRING_LENGTH];
    int i;

    if ( str == NULL )
        return '\0';

    for ( i = 0; str[i] != '\0'; i++ )
    {
        if (str[i] >= 'a' && str[i] <= 'z')
            str[i] = number_range('a', 'z');
	if (str[i] >= 'A' && str[i] <= 'Z')
	    str[i] = number_range('A', 'Z');
        strfix[i] = str[i];
    }
    strfix[i] = '\0';
    return strfix;
}

char * get_random_word(void)
{
    int x;

    memset((void *)randword, 0, MAX_STRING_LENGTH);
    x = number_range(0, MAX_CONFUSED_WORDS-1);
    randword[0] = '\0';
    sprintf(randword, "%s", confused_words[x]);

    return randword;
}

char	* confuse_text( char *str )
{
    char arg[MAX_STRING_LENGTH];
    bool meep = FALSE;

    if ( str == NULL )
        return '\0';

    memset((void *)confused_string, 0, MAX_STRING_LENGTH);

    confused_string[0] = '\0';
    while (1)
	{
	str = one_argument(str, arg);
	if (arg[0] == '\0')
		break;
	if (meep)
		strcat(confused_string, " ");
	else
		meep = TRUE;
	if (number_bits(2) == 0)
		{
		get_random_word();
		strcat(confused_string, randword);
		}
	else
		strcat(confused_string, arg);
	}


confused_string[0] = UPPER(confused_string[0]);
return confused_string;
}
   
/*
 * Write to one char, new colour version, by Lope.
 */
void send_to_char( const char *txt, CHAR_DATA *ch )
{
    char buf[ MAX_STRING_LENGTH*4 ];

    if (!ch || !ch->desc)
	return;

    colourconv(buf, txt, ch);
    write_to_buffer( ch->desc, buf, strlen(buf) );

/*
    buf[0] = '\0';
    point2 = buf;
    if( txt && ch->desc )
	{
	if( (ch->desc->original && IS_SET(ch->desc->original->act, PLR_COLOUR)) || (!ch->desc->original && IS_SET(ch->act, PLR_COLOUR)) )
	    {
		for( point = txt ; *point ; point++ )
	        {
		    if( *point == '{' )
		    {
			point++;
			skip = colour( *point, ch, point2 );
			while( skip-- > 0 )
			    ++point2;
			continue;
		    }
		    *point2 = *point;
		    *++point2 = '\0';
		}			
		*point2 = '\0';
        	write_to_buffer( ch->desc, buf, point2 - buf );
	    }
	    else
	    {
		for( point = txt ; *point ; point++ )
	        {
		    if( *point == '{' )
		    {
			point++;
			continue;
		    }
		    *point2 = *point;
		    *++point2 = '\0';
		}
		*point2 = '\0';
        	write_to_buffer( ch->desc, buf, point2 - buf );
	    }
	}
*/

    return;
}

/*
 * Send a page to one char.
 */
void page_to_char_bw( const char *txt, CHAR_DATA *ch )
{
    char *newmem;

    if ( txt == NULL || ch->desc == NULL)


    if (ch->lines == 0 )
    {
	send_to_char(txt,ch);
	return;
    }
	
#if defined(macintosh)
	send_to_char(txt,ch);
#else
    if (ch->desc->showstr_head)
    {
	newmem = (char*)malloc(strlen(ch->desc->showstr_head) + strlen(txt) + 1);
	strcpy(newmem, ch->desc->showstr_head);
	strcat(newmem, txt);
	free(ch->desc->showstr_head);
	ch->desc->showstr_head = newmem;
	g_size_pagebuf += strlen(txt);
    }
    else
    {
	ch->desc->showstr_head  = (char*)malloc(strlen(txt) + 1);
	g_size_pagebuf += (strlen(txt) + 1);
	strcpy( ch->desc->showstr_head, txt );
    }
    ch->desc->showstr_point = ch->desc->showstr_head;
    show_string(ch->desc,"");
#endif
}

/*
 * Page to one char, new colour version, by Lope.
 */
void page_to_char( const char *txt, CHAR_DATA *ch )
{
    char	buf[ MAX_STRING_LENGTH * 4 ];
    char	*newmem;

    if (!ch || !ch->desc)
	return;

    colourconv(buf, txt, ch);

    if (ch->desc->showstr_head)
    {
	newmem = (char*)malloc(strlen(ch->desc->showstr_head) + strlen(buf) + 1);
	strcpy(newmem, ch->desc->showstr_head);
	strcat(newmem, buf);
	free(ch->desc->showstr_head);
	ch->desc->showstr_head = newmem;
	g_size_pagebuf += strlen(buf);
    }
    else
    {
	ch->desc->showstr_head  = (char*)malloc(strlen(buf) + 1);
	g_size_pagebuf += (strlen(buf) + 1);
	strcpy( ch->desc->showstr_head, buf );
    }

    ch->desc->showstr_point = ch->desc->showstr_head;
    show_string( ch->desc, "" );
}


/* string pager */
void show_string(struct descriptor_data *d, char *input)
{
    char buffer[4*MAX_STRING_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    register char *scan, *chk;
    int lines = 0, toggle = 1;
    int show_lines;

    one_argument(input,buf);
    if (buf[0] != '\0')
    {
	if (d->showstr_head)
	{
	    g_size_pagebuf -= (strlen(d->showstr_head) + 1);
	    free(d->showstr_head);
	    d->showstr_head = 0;
	}
    	d->showstr_point  = 0;
	return;
    }

    if (d->character)
	show_lines = d->character->lines;
    else
	show_lines = 0;

    for (scan = buffer; ; scan++, d->showstr_point++)
    {
	if (((*scan = *d->showstr_point) == '\n' || *scan == '\r')
	    && (toggle = -toggle) < 0)
	    lines++;

	else if (!*scan || (show_lines > 0 && lines >= show_lines))
	{
	    *scan = '\0';
	    write_to_buffer(d,buffer,strlen(buffer));
	    for (chk = d->showstr_point; isspace(*chk); chk++);
	    {
		if (!*chk)
		{
		    if (d->showstr_head)
        	    {
			g_size_pagebuf -= (strlen(d->showstr_head) + 1);
            		free(d->showstr_head);
            		d->showstr_head = 0;
        	    }
        	    d->showstr_point  = 0;
    		}
	    }
	    return;
	}
    }
    return;
}
	

/* quick sex fixer */
void fix_sex(CHAR_DATA *ch)
{
    if (ch->sex < 0 || ch->sex > 2)
    	ch->sex = IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
}

void emote_act (const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2,
	  int type)
{
    /* to be compatible with older code */
    emote_act_new(format,ch,arg1,arg2,type,POS_RESTING);
}

void tc_act (const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2,
	  int type)
{
    /* to be compatible with older code */
    tc_act_new(format,ch,arg1,arg2,type,POS_RESTING);
}

void c_act (const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2,
	  int type)
{
    /* to be compatible with older code */
    c_act_new(format,ch,arg1,arg2,type,POS_RESTING);
}

void wizact (const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2,
	  int type)
{
    /* to be compatible with older code */
    wizact_new(format,ch,arg1,arg2,type,POS_RESTING);
}

void act (const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2,
	  int type)
{
    /* to be compatible with older code */
    act_new(format,ch,arg1,arg2,type,POS_RESTING);
}

void emote_act_new( const char *format, CHAR_DATA *ch, const void *arg1, 
	      const void *arg2, int type, int min_pos )
{
    static char * const he_she  [] = { "it",  "he",  "she" };
    static char * const him_her [] = { "it",  "him", "her" };
    static char * const his_her [] = { "its", "his", "her" };
 
    CHAR_DATA 		*to;
    CHAR_DATA 		*vch = ( CHAR_DATA * ) arg2;
    OBJ_DATA 		*obj1 = ( OBJ_DATA  * ) arg1;
    OBJ_DATA 		*obj2 = ( OBJ_DATA  * ) arg2;
    const 	char 	*str;
    const char 		*i = NULL;
    char 		*point;
    char 		*pbuff;
    char 		buffer[ MAX_STRING_LENGTH*2 ];
    char 		buf[ MAX_STRING_LENGTH   ];
    char 		fname[ MAX_INPUT_LENGTH  ];
    bool		fColour = FALSE;

    /*
     * Discard null and zero-length messages.
     */
    if( !format || !*format )
        return;

    /* discard null rooms and chars */
    if( !ch || !ch->in_room )
	return;

    to = ch->in_room->people;
    if( type == TO_VICT || type == TO_VICTROOM )
    {
        if( !vch )
        {
            bug( "Act: null vch with TO_VICT.", 0 );
            return;
        }

	if( !vch->in_room )
	    return;

        to = vch->in_room->people;
    }
 
    for( ; to ; to = to->next_in_room )
    {
        if( !to->desc || to->position < min_pos )
            continue;
 
        if( ( type == TO_CHAR ) && to != ch )
            continue;
        if( type == TO_VICT && ( to != vch || to == ch ) )
            continue;
        if( type == TO_ROOM && to == ch )
            continue;
        if( type == TO_NOTVICT && (to == ch || to == vch) )
            continue;
	if( type == TO_VICTROOM && ((to == vch) || (to == ch)) )
	    continue;
 
        point   = buf;
        str     = format;
        while( *str != '\0' )
        {
            if( *str != '$' )
            {
                *point++ = *str++;
                continue;
            }
 
	    fColour = TRUE;
	    ++str;
	    i = " <@@@> ";
            if( !arg2 && *str >= 'A' && *str <= 'Z' )
            {
                bug( "Act: missing arg2 for code %d.", *str );
                i = " <@@@> ";
            }
            else
            {
                switch ( *str )
                {
                default:  bug( "Act: bad code %d.", *str );
                          i = " <@@@> ";                                break;
                /* Thx alex for 't' idea */
                case 't': i = (char *) arg1;                            break;
                case 'T': i = (char *) arg2;                            break;
                case 'n': i = PERS( ch,  to  );				break;
                case 'N': i = PERS( vch,  to );				break;
                case 'e': i = can_see(to, ch)  ? he_she[URANGE(0, ch->sex, 2)]   : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch)  ? "it"  : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? he_she[URANGE(0, ch->sex, 2)]   : "their")); break;
                case 'E': i = can_see(to, vch) ? he_she[URANGE(0, vch->sex, 2)]  : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? he_she[URANGE(0, vch->sex, 2)]  : "their")); break;
                case 'm': i = can_see(to, ch)  ? him_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "it"  : "them")  : (IS_AFFECTED(ch, AFF_WIZI)  ? him_her[URANGE(0, ch->sex, 2)]  : "them"));  break;
                case 'M': i = can_see(to, vch) ? him_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "them")  : (IS_AFFECTED(vch, AFF_WIZI) ? him_her[URANGE(0, vch->sex, 2)] : "them"));  break;
                case 's': i = can_see(to, ch)  ? his_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "its" : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? his_her[URANGE(0, ch->sex, 2)]  : "their")); break;
                case 'S': i = can_see(to, vch) ? his_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "its" : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? his_her[URANGE(0, vch->sex, 2)] : "their")); break;
                case 'c': i = clan_table[ch->clan].who_name; break; /* XUR */
                

                case 'p':
                    i = can_see_obj( to, obj1 )
                            ? obj1->short_descr
                            : "something";
                    break;
 
                case 'P':
                    i = can_see_obj( to, obj2 )
                            ? obj2->short_descr
                            : "something";
                    break;
 
                case 'd':
                    if ( arg2 == NULL || ((char *) arg2)[0] == '\0' )
                    {
                        i = "door";
                    }
                    else
                    {
                        one_argument( (char *) arg2, fname );
                        i = fname;
                    }
                    break;
                }
            }
 
            ++str;
            while ( ( *point = *i ) != '\0' )
                ++point, ++i;
        }
 
        *point++ = '\n';
        *point++ = '\r';
	*point	 = '\0';
        buf[0]   = UPPER(buf[0]);
	if (buf[0] == '{')
		buf[2] = UPPER( buf[2] );
	pbuff	 = buffer;
	check_comm(to, buf);
	colourconv( pbuff, buf, to );
        write_to_buffer( to->desc, front_capitalize(buffer), 0 );
    }
 
    MOBtrigger = TRUE;
}

/*
 * The colour version of the act_new( ) function, -Lope
 */
void act_mprog_activate( const char *format, CHAR_DATA *ch, const void *arg1, 
	      const void *arg2, int type, int min_pos )
{
    static char * const he_she  [] = { "it",  "he",  "she" };
    static char * const him_her [] = { "it",  "him", "her" };
    static char * const his_her [] = { "its", "his", "her" };
 
    CHAR_DATA 		*to, *fch, *fch_next, *next_to;
    ROOM_INDEX_DATA	*proom;
    CHAR_DATA 		*vch = ( CHAR_DATA * ) arg2;
    OBJ_DATA 		*obj1 = ( OBJ_DATA  * ) arg1;
    OBJ_DATA 		*obj2 = ( OBJ_DATA  * ) arg2;
    OBJ_DATA            *vobj = NULL;
    const 	char 	*str;
    const char 		*i = NULL;
    char 		*point;
    char 		*pbuff;
    char 		buffer[ MAX_STRING_LENGTH*2 ];
    char 		buf[ MAX_STRING_LENGTH   ];
    char 		fname[ MAX_INPUT_LENGTH  ];
    bool		fColour = FALSE;

    /*
     * Discard null and zero-length messages.
     */
    if( !format || !*format )
        return;

    /* discard null rooms and chars */
    if( !ch || !ch->in_room )
	return;

    proom = ch->in_room;
    to = ch->in_room->people;
    if( type == TO_VICT )
    {
        if( !vch )
        {
            bug( "Act: null vch with TO_VICT.", 0 );
            return;
        }

	if( !vch->in_room )
	    return;

        to = vch->in_room->people;
    }
 
    for( ; to ; to = next_to )
    {
    next_to = to->next_in_room;
	/*to->desc removed because we want act progs to work always */
        if(/* !to->desc ||*/ to->position < min_pos )
            continue;
 
        if( ( type == TO_CHAR ) && to != ch )
            continue;
        if( type == TO_VICT && ( to != vch || to == ch ) )
            continue;
        if( type == TO_ROOM && to == ch )
            continue;
        if( type == TO_NOTVICT && (to == ch || to == vch) )
            continue;
 
        point   = buf;
        str     = format;
        while( *str != '\0' )
        {
            if( *str != '$' )
            {
                *point++ = *str++;
                continue;
            }
 
	    fColour = TRUE;
	    ++str;
	    i = " <@@@> ";
            if( !arg2 && *str >= 'A' && *str <= 'Z' )
            {
                bug( "Act: missing arg2 for code %d.", *str );
                i = " <@@@> ";
            }
            else
            {
                switch ( *str )
                {
                default:  bug( "Act: bad code %d.", *str );
                          i = " <@@@> ";                                break;
                /* Thx alex for 't' idea */
                case 't': i = (char *) arg1;                            break;
                case 'T': i = (char *) arg2;                            break;
                case 'n': i = PERS( ch,  to  );				break;
                case 'N': i = PERS( vch,  to );				break;
                case 'e': i = can_see(to, ch)  ? he_she[URANGE(0, ch->sex, 2)]   : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch)  ? "it"  : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? he_she[URANGE(0, ch->sex, 2)]   : "their")); break;
                case 'E': i = can_see(to, vch) ? he_she[URANGE(0, vch->sex, 2)]  : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? he_she[URANGE(0, vch->sex, 2)]  : "their")); break;
                case 'm': i = can_see(to, ch)  ? him_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "it"  : "them")  : (IS_AFFECTED(ch, AFF_WIZI)  ? him_her[URANGE(0, ch->sex, 2)]  : "them"));  break;
                case 'M': i = can_see(to, vch) ? him_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "them")  : (IS_AFFECTED(vch, AFF_WIZI) ? him_her[URANGE(0, vch->sex, 2)] : "them"));  break;
                case 's': i = can_see(to, ch)  ? his_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "its" : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? his_her[URANGE(0, ch->sex, 2)]  : "their")); break;
                case 'S': i = can_see(to, vch) ? his_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "its" : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? his_her[URANGE(0, vch->sex, 2)] : "their")); break;
                case 'c': i = clan_table[ch->clan].who_name; break; /* XUR */
                

                case 'p':
                    i = can_see_obj( to, obj1 )
                            ? obj1->short_descr
                            : "something";
                    break;
 
                case 'P':
                    i = can_see_obj( to, obj2 )
                            ? obj2->short_descr
                            : "something";
                    break;
 
                case 'd':
                    if ( arg2 == NULL || ((char *) arg2)[0] == '\0' )
                    {
                        i = "door";
                    }
                    else
                    {
                        one_argument( (char *) arg2, fname );
                        i = fname;
                    }
                    break;
                }
            }
 
            ++str;
            while (i != NULL && ((*point = *i) != '\0' ))
                ++point, ++i;
        }
 
        *point++ = '\n';
        *point++ = '\r';
	*point	 = '\0';
        buf[0]   = UPPER(buf[0]);
	if (buf[0] == '{')
		buf[2] = UPPER( buf[2] );
	pbuff	 = buffer;
	colourconv( pbuff, buf, to );
    	      mprog_act_trigger( buf, to, ch, obj1, vch );
	      if (ch && ch->in_room)
    		for ( fch = ch->in_room->people; fch != NULL; fch = fch_next )
    		{
        	fch_next = fch->next_in_room;
        		for ( vobj = fch->carrying; vobj != NULL; vobj = vobj->next_content )
        		{
             		oprog_act_trigger( buf, vobj, ch, obj1, vch );
        		}
    		}

	      if (ch && ch->in_room)
          	for ( vobj = ch->in_room->contents; vobj != NULL; vobj = vobj->next_content )
          	{
             	oprog_act_trigger( buf, vobj, ch, obj1, vch );
          	}
    }
 
    MOBtrigger = TRUE;
}

/*tc -- tell variant so you can avoid revealing people with
 *alter self and chameleon. Damn this chunk of code is
 *annoying me
 */
void tc_act_new( const char *format, CHAR_DATA *ch, const void *arg1, 
	      const void *arg2, int type, int min_pos )
{
    static char * const he_she  [] = { "it",  "he",  "she" };
    static char * const him_her [] = { "it",  "him", "her" };
    static char * const his_her [] = { "its", "his", "her" };
 
    CHAR_DATA 		*to, *next_to;
    ROOM_INDEX_DATA	*proom;
    CHAR_DATA 		*vch = ( CHAR_DATA * ) arg2;
    OBJ_DATA 		*obj1 = ( OBJ_DATA  * ) arg1;
    OBJ_DATA 		*obj2 = ( OBJ_DATA  * ) arg2;
    const 	char 	*str;
    const char 		*i = NULL;
    char 		*point;
    char 		*pbuff;
    char 		buffer[ MAX_STRING_LENGTH*2 ];
    char 		buf[ MAX_STRING_LENGTH   ];
    char 		fname[ MAX_INPUT_LENGTH  ];
    bool		fColour = FALSE;

    /*
     * Discard null and zero-length messages.
     */
    if( !format || !*format )
        return;

    /* discard null rooms and chars */
    if( !ch || !ch->in_room )
	return;

    proom = ch->in_room;
    to = ch->in_room->people;
    if( type == TO_VICT )
    {
        if( !vch )
        {
            bug( "Act: null vch with TO_VICT.", 0 );
            return;
        }

	if( !vch->in_room )
	    return;

        to = vch->in_room->people;
    }
 
    for( ; to ; to = next_to )
    {
    next_to = to->next_in_room;
/* code for passing sleeping people/min pos people WOULD be here, but
 * no, because we want everyone to see this wacked shit, really. That's
 * the glory of c_act_new */
 
        if( ( type == TO_CHAR ) && to != ch )
            continue;
        if( type == TO_VICT && ( to != vch || to == ch ) )
            continue;
        if( type == TO_ROOM && to == ch )
            continue;
        if( type == TO_NOTVICT && (to == ch || to == vch) )
            continue;
 
        point   = buf;
        str     = format;
        while( *str != '\0' )
        {
            if( *str != '$' )
            {
                *point++ = *str++;
                continue;
            }
 
	    fColour = TRUE;
	    ++str;
	    i = " <@@@> ";
            if( !arg2 && *str >= 'A' && *str <= 'Z' )
            {
                bug( "Act: missing arg2 for code %d.", *str );
                i = " <@@@> ";
            }
            else
            {
                switch ( *str )
                {
                default:  bug( "Act: bad code %d.", *str );
                          i = " <@@@> ";                                break;
                /* Thx alex for 't' idea */
                case 't': i = (char *) arg1;                            break;
                case 'T': i = (char *) arg2;                            break;
                case 'n': i = APERS( ch,  to  );			break;
                case 'N': i = APERS( vch,  to );			break;
                case 'e': i = can_see_comm(to, ch)  ? he_she[URANGE(0, ch->sex, 2)]   : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch)  ? "it"  : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? he_she[URANGE(0, ch->sex, 2)]   : "their")); break;
                case 'E': i = can_see_comm(to, vch) ? he_she[URANGE(0, vch->sex, 2)]  : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? he_she[URANGE(0, vch->sex, 2)]  : "their")); break;
                case 'm': i = can_see_comm(to, ch)  ? him_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "it"  : "them")  : (IS_AFFECTED(ch, AFF_WIZI)  ? him_her[URANGE(0, ch->sex, 2)]  : "them"));  break;
                case 'M': i = can_see_comm(to, vch) ? him_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "them")  : (IS_AFFECTED(vch, AFF_WIZI) ? him_her[URANGE(0, vch->sex, 2)] : "them"));  break;
                case 's': i = can_see_comm(to, ch)  ? his_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "its" : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? his_her[URANGE(0, ch->sex, 2)]  : "their")); break;
                case 'S': i = can_see_comm(to, vch) ? his_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "its" : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? his_her[URANGE(0, vch->sex, 2)] : "their")); break;
                case 'c': i = clan_table[ch->clan].who_name; break; /* XUR */
                

                case 'p':
                    i = can_see_obj( to, obj1 )
                            ? obj1->short_descr
                            : "something";
                    break;
 
                case 'P':
                    i = can_see_obj( to, obj2 )
                            ? obj2->short_descr
                            : "something";
                    break;
 
                case 'd':
                    if ( arg2 == NULL || ((char *) arg2)[0] == '\0' )
                    {
                        i = "door";
                    }
                    else
                    {
                        one_argument( (char *) arg2, fname );
                        i = fname;
                    }
                    break;
                }
            }
 
            ++str;
            while ( ( *point = *i ) != '\0' )
                ++point, ++i;
        }
 
        *point++ = '\n';
        *point++ = '\r';
	*point	 = '\0';
        buf[0]   = UPPER(buf[0]);
	if (buf[0] == '{')
		buf[2] = UPPER( buf[2] );
	pbuff	 = buffer;

        check_comm(to, buf);
	colourconv( pbuff, buf, to );
	if (to->desc)
        	write_to_buffer( to->desc, front_capitalize(buffer), 0 );
    }
}

/* c_act_new is for communciations only. It's so people hear yells and
 * what not while asleep, and see people normally, whereas most acts
 * they wouldn't see, and wouldn't know "who" did it even if they did
 */
void c_act_new( const char *format, CHAR_DATA *ch, const void *arg1, 
	      const void *arg2, int type, int min_pos )
{
    static char * const he_she  [] = { "it",  "he",  "she" };
    static char * const him_her [] = { "it",  "him", "her" };
    static char * const his_her [] = { "its", "his", "her" };
 
    CHAR_DATA 		*to, *next_to;
    ROOM_INDEX_DATA	*proom;
    CHAR_DATA 		*vch = ( CHAR_DATA * ) arg2;
    OBJ_DATA 		*obj1 = ( OBJ_DATA  * ) arg1;
    OBJ_DATA 		*obj2 = ( OBJ_DATA  * ) arg2;
    const 	char 	*str;
    const char 		*i = NULL;
    char 		*point;
    char 		*pbuff;
    char 		buffer[ MAX_STRING_LENGTH*2 ];
    char 		buf[ MAX_STRING_LENGTH   ];
    char 		fname[ MAX_INPUT_LENGTH  ];
    bool		fColour = FALSE;

    /*
     * Discard null and zero-length messages.
     */
    if( !format || !*format )
        return;

    /* discard null rooms and chars */
    if( !ch || !ch->in_room )
	return;

    proom = ch->in_room;
    to = ch->in_room->people;
    if( type == TO_VICT )
    {
        if( !vch )
        {
            bug( "Act: null vch with TO_VICT.", 0 );
            return;
        }

	if( !vch->in_room )
	    return;

        to = vch->in_room->people;
    }
 
    for( ; to ; to = next_to )
    {
    next_to = to->next_in_room;
/* code for passing sleeping people/min pos people WOULD be here, but
 * no, because we want everyone to see this wacked shit, really. That's
 * the glory of c_act_new */
 
        if( ( type == TO_CHAR ) && to != ch )
            continue;
        if( type == TO_VICT && ( to != vch || to == ch ) )
            continue;
        if( type == TO_ROOM && to == ch )
            continue;
        if( type == TO_NOTVICT && (to == ch || to == vch) )
            continue;
 
        point   = buf;
        str     = format;
        while( *str != '\0' )
        {
            if( *str != '$' )
            {
                *point++ = *str++;
                continue;
            }
 
	    fColour = TRUE;
	    ++str;
	    i = " <@@@> ";
            if( !arg2 && *str >= 'A' && *str <= 'Z' )
            {
                bug( "Act: missing arg2 for code %d.", *str );
                i = " <@@@> ";
            }
            else
            {
                switch ( *str )
                {
                default:  bug( "Act: bad code %d.", *str );
                          i = " <@@@> ";                                break;
                /* Thx alex for 't' idea */
                case 't': i = (char *) arg1;                            break;
                case 'T': i = (char *) arg2;                            break;
                case 'n': i = CPERS( ch,  to  );			break;
                case 'N': i = CPERS( vch,  to );			break;
                case 'e': i = can_see_comm(to, ch)  ? he_she[URANGE(0, ch->sex, 2)]   : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch)  ? "it"  : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? he_she[URANGE(0, ch->sex, 2)]   : "their")); break;
                case 'E': i = can_see_comm(to, vch) ? he_she[URANGE(0, vch->sex, 2)]  : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? he_she[URANGE(0, vch->sex, 2)]  : "their")); break;
                case 'm': i = can_see_comm(to, ch)  ? him_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "it"  : "them")  : (IS_AFFECTED(ch, AFF_WIZI)  ? him_her[URANGE(0, ch->sex, 2)]  : "them"));  break;
                case 'M': i = can_see_comm(to, vch) ? him_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "them")  : (IS_AFFECTED(vch, AFF_WIZI) ? him_her[URANGE(0, vch->sex, 2)] : "them"));  break;
                case 's': i = can_see_comm(to, ch)  ? his_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "its" : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? his_her[URANGE(0, ch->sex, 2)]  : "their")); break;
                case 'S': i = can_see_comm(to, vch) ? his_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "its" : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? his_her[URANGE(0, vch->sex, 2)] : "their")); break;
                case 'c': i = clan_table[ch->clan].who_name; break; /* XUR */
                

                case 'p':
                    i = can_see_obj( to, obj1 )
                            ? obj1->short_descr
                            : "something";
                    break;
 
                case 'P':
                    i = can_see_obj( to, obj2 )
                            ? obj2->short_descr
                            : "something";
                    break;
 
                case 'd':
                    if ( arg2 == NULL || ((char *) arg2)[0] == '\0' )
                    {
                        i = "door";
                    }
                    else
                    {
                        one_argument( (char *) arg2, fname );
                        i = fname;
                    }
                    break;
                }
            }
 
            ++str;
            while ( ( *point = *i ) != '\0' )
                ++point, ++i;
        }
 
        *point++ = '\n';
        *point++ = '\r';
	*point	 = '\0';
        buf[0]   = UPPER(buf[0]);
	if (buf[0] == '{')
		buf[2] = UPPER( buf[2] );
	pbuff	 = buffer;

        check_comm(to, buf);
	colourconv( pbuff, buf, to );
	if (to->desc)
        	write_to_buffer( to->desc, front_capitalize(buffer), 0 );

/* We really want everyone to see the action BEFORE the trigger. Ouch
 * for calling ANOTHER function with all this shit. At some point,
 * it should be inlined back into the normal function, but I'm lazy, since
 * it requires a retranslation anyhow. -mw
        if (1)
        {
    	      mprog_act_trigger( buf, to, ch, obj1, vch );
    		for ( fch = ch->in_room->people; fch != NULL; fch = fch_next )
    		{
        	fch_next = fch->next_in_room;
        		for ( vobj = fch->carrying; vobj != NULL; vobj = vobj->next_content )
        		{
             		oprog_act_trigger( buf, vobj, ch, obj1, vch );
        		}
    		}

          	for ( vobj = ch->in_room->contents; vobj != NULL; vobj = vobj->next_content )
          	{
             	oprog_act_trigger( buf, vobj, ch, obj1, vch );
          	}
        }
*/
    }

    act_mprog_activate(format, ch, arg1, arg2, type, min_pos);
 
    MOBtrigger = TRUE;
}

void wizact_new( const char *format, CHAR_DATA *ch, const void *arg1, 
	      const void *arg2, int type, int min_pos )
{
    static char * const he_she  [] = { "it",  "he",  "she" };
    static char * const him_her [] = { "it",  "him", "her" };
    static char * const his_her [] = { "its", "his", "her" };
 
    CHAR_DATA 		*to, *next_to;
    ROOM_INDEX_DATA	*proom;
    CHAR_DATA 		*vch = ( CHAR_DATA * ) arg2;
    OBJ_DATA 		*obj1 = ( OBJ_DATA  * ) arg1;
    OBJ_DATA 		*obj2 = ( OBJ_DATA  * ) arg2;
    const 	char 	*str;
    const char 		*i = NULL;
    char 		*point;
    char 		*pbuff;
    char 		buffer[ MAX_STRING_LENGTH*2 ];
    char 		buf[ MAX_STRING_LENGTH   ];
    char 		fname[ MAX_INPUT_LENGTH  ];
    bool		fColour = FALSE;

    /*
     * Discard null and zero-length messages.
     */
    if( !format || !*format )
        return;

    /* discard null rooms and chars */
    if( !ch || !ch->in_room )
	return;

    proom = ch->in_room;
    to = ch->in_room->people;
    if( type == TO_VICT )
    {
        if( !vch )
        {
            bug( "Act: null vch with TO_VICT.", 0 );
            return;
        }

	if( !vch->in_room )
	    return;

        to = vch->in_room->people;
    }
 
    for( ; to ; to = next_to )
    {
    next_to = to->next_in_room;
	/*to->desc removed because we want act progs to work always */
        if(/* !to->desc ||*/ to->position < min_pos )
            continue;

	if (to->trust < 52)
	 continue;
 
        if( ( type == TO_CHAR ) && to != ch )
            continue;
        if( type == TO_VICT && ( to != vch || to == ch ) )
            continue;
        if( type == TO_ROOM && to == ch )
            continue;
        if( type == TO_NOTVICT && (to == ch || to == vch) )
            continue;
 
        point   = buf;
        str     = format;
        while( *str != '\0' )
        {
            if( *str != '$' )
            {
                *point++ = *str++;
                continue;
            }
 
	    fColour = TRUE;
	    ++str;
	    i = " <@@@> ";
            if( !arg2 && *str >= 'A' && *str <= 'Z' )
            {
                bug( "Act: missing arg2 for code %d.", *str );
                i = " <@@@> ";
            }
            else
            {
                switch ( *str )
                {
                default:  bug( "Act: bad code %d.", *str );
                          i = " <@@@> ";                                break;
                /* Thx alex for 't' idea */
                case 't': i = (char *) arg1;                            break;
                case 'T': i = (char *) arg2;                            break;
                case 'n': i = PERS( ch,  to  );				break;
                case 'N': i = PERS( vch,  to );				break;
                case 'e': i = can_see(to, ch)  ? he_she[URANGE(0, ch->sex, 2)]   : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch)  ? "it"  : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? he_she[URANGE(0, ch->sex, 2)]   : "their")); break;
                case 'E': i = can_see(to, vch) ? he_she[URANGE(0, vch->sex, 2)]  : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? he_she[URANGE(0, vch->sex, 2)]  : "their")); break;
                case 'm': i = can_see(to, ch)  ? him_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "it"  : "them")  : (IS_AFFECTED(ch, AFF_WIZI)  ? him_her[URANGE(0, ch->sex, 2)]  : "them"));  break;
                case 'M': i = can_see(to, vch) ? him_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "them")  : (IS_AFFECTED(vch, AFF_WIZI) ? him_her[URANGE(0, vch->sex, 2)] : "them"));  break;
                case 's': i = can_see(to, ch)  ? his_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "its" : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? his_her[URANGE(0, ch->sex, 2)]  : "their")); break;
                case 'S': i = can_see(to, vch) ? his_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "its" : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? his_her[URANGE(0, vch->sex, 2)] : "their")); break;
                case 'c': i = clan_table[ch->clan].who_name; break; /* XUR */
                

                case 'p':
                    i = can_see_obj( to, obj1 )
                            ? obj1->short_descr
                            : "something";
                    break;
 
                case 'P':
                    i = can_see_obj( to, obj2 )
                            ? obj2->short_descr
                            : "something";
                    break;
 
                case 'd':
                    if ( arg2 == NULL || ((char *) arg2)[0] == '\0' )
                    {
                        i = "door";
                    }
                    else
                    {
                        one_argument( (char *) arg2, fname );
                        i = fname;
                    }
                    break;
                }
            }
 
            ++str;
            while ( ( *point = *i ) != '\0' )
                ++point, ++i;
        }
 
        *point++ = '\n';
        *point++ = '\r';
	*point	 = '\0';

        buf[0]   = UPPER(buf[0]);
	if (buf[0] == '{')
		buf[2] = UPPER( buf[2] );
	pbuff	 = buffer;
	colourconv( pbuff, buf, to );
	if (to->desc)
        	write_to_buffer( to->desc, front_capitalize(buffer), 0 );
/* We really want everyone to see the action BEFORE the trigger. Ouch
 * for calling ANOTHER function with all this shit. At some point,
 * it should be inlined back into the normal function, but I'm lazy, since
 * it requires a retranslation anyhow. -mw
        if (1)
        {
    	      mprog_act_trigger( buf, to, ch, obj1, vch );
    		for ( fch = ch->in_room->people; fch != NULL; fch = fch_next )
    		{
        	fch_next = fch->next_in_room;
        		for ( vobj = fch->carrying; vobj != NULL; vobj = vobj->next_content )
        		{
             		oprog_act_trigger( buf, vobj, ch, obj1, vch );
        		}
    		}

          	for ( vobj = ch->in_room->contents; vobj != NULL; vobj = vobj->next_content )
          	{
             	oprog_act_trigger( buf, vobj, ch, obj1, vch );
          	}
        }
*/
    }
act_mprog_activate(format, ch, arg1, arg2, type, min_pos);
 
    MOBtrigger = TRUE;
}

const char * show_person(CHAR_DATA * ch, CHAR_DATA * looker, bool checkDisguise)
{
    static std::string unrealIncursionDesc;

    if (can_see_comm(looker, ch))
    {
        // Check for unreal incursion
        if (is_affected(looker, gsn_unrealincursion))
        {
            if (number_percent() <= 5)
            {
                unrealIncursionDesc = generate_unreal_entity();
                return unrealIncursionDesc.c_str();
            }
        }

        // Check for masquerade
        if (ch->in_room != NULL)
        {
            AFFECT_DATA * masquerade(get_area_affect(ch->in_room->area, gsn_masquerade));
            if (masquerade != NULL && masquerade->modifier != looker->id && number_percent() > get_skill(looker, gsn_masquerade))
                return "a grinning skull";
        }

        if (IS_NPC(ch) || IS_OAFFECTED(ch, AFF_DISGUISE)) return ch->short_descr;
        return ch->name;
    }

    if (IS_NPC(ch))
    {
        if (IS_AFFECTED(ch, AFF_WIZI)) return ch->short_descr;
        return "someone";
    }

    if (IS_IMMORTAL(ch)) return "A Divine Presence";
    return "someone";
}

void act_new( const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type, int min_pos )
{
    static char * const he_she  [] = { "it",  "he",  "she" };
    static char * const him_her [] = { "it",  "him", "her" };
    static char * const his_her [] = { "its", "his", "her" };
 
    CHAR_DATA 		*to, *next_to;
    ROOM_INDEX_DATA	*proom;
    CHAR_DATA 		*vch = ( CHAR_DATA * ) arg2;
    OBJ_DATA 		*obj1 = ( OBJ_DATA  * ) arg1;
    OBJ_DATA 		*obj2 = ( OBJ_DATA  * ) arg2;
    const 	char 	*str;
    const char 		*i = NULL;
    char 		*point;
    char 		*pbuff;
    char 		buffer[ MAX_STRING_LENGTH*2 ];
    char 		buf[ MAX_STRING_LENGTH   ];
//  char 		fname[ MAX_INPUT_LENGTH  ];
    bool		fColour = FALSE;

    /*
     * Discard null and zero-length messages.
     */
    if( !format || !*format )
        return;

    /* discard null rooms and chars */
    if( !ch || !ch->in_room )
	return;

    proom = ch->in_room;
    to = ch->in_room->people;
    if( (type == TO_VICT) || (type == TO_VICTROOM) )
    {
        if( !vch )
        {
            bug( "Act: null vch with TO_VICT or TO_VICTROOM.", 0 );
            return;
        }

	if( !vch->in_room )
	    return;

        to = vch->in_room->people;
    }

    for( ; to ; to = next_to )
    {
    next_to = to->next_in_room;
	/*to->desc removed because we want act progs to work always */
        if(/* !to->desc ||*/ to->position < min_pos )
            continue;
 
        if( ( type == TO_CHAR ) && to != ch )
            continue;
        if( type == TO_VICT && ( to != vch || to == ch ) )
            continue;
        if( type == TO_ROOM && (to == ch))
            continue;
	if( type == TO_VICTROOM && ((to == vch) || (to == ch)) )
	    continue;
        if( type == TO_NOTVICT && (to == ch || to == vch) )
            continue;

        point   = buf;
        str     = format;

        while( *str != '\0' )
        {
            if( *str != '$' )
            {
                *point++ = *str++;
                continue;
            }
 
	    fColour = TRUE;
	    ++str;
	    i = " <@@@> ";
            if( !arg2 && *str >= 'A' && *str <= 'Z' )
            {
                bug( "Act: missing arg2 for code %d.", *str );
                i = " <@@@> ";
            }
            else
            {
                switch ( *str )
                {
                default:  bug( "Act: bad code %d.", *str );
                          i = " <@@@> ";                                break;
                /* Thx alex for 't' idea */
                case 't': i = (char *) arg1;                            break;
                case 'T': i = (char *) arg2;                            break;
                case 'n': i = PERS( ch,  to  );				break;
                case 'N': i = PERS( vch,  to );				break;
                case 'e': i = can_see(to, ch)  ? he_she[URANGE(0, ch->sex, 2)]   : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch)  ? "it"  : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? he_she[URANGE(0, ch->sex, 2)]   : "their")); break;
                case 'E': i = can_see(to, vch) ? he_she[URANGE(0, vch->sex, 2)]  : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? he_she[URANGE(0, vch->sex, 2)]  : "their")); break;
                case 'm': i = can_see(to, ch)  ? him_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "it"  : "them")  : (IS_AFFECTED(ch, AFF_WIZI)  ? him_her[URANGE(0, ch->sex, 2)]  : "them"));  break;
                case 'M': i = can_see(to, vch) ? him_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "them")  : (IS_AFFECTED(vch, AFF_WIZI) ? him_her[URANGE(0, vch->sex, 2)] : "them"));  break;
                case 's': i = can_see(to, ch)  ? his_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "its" : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? his_her[URANGE(0, ch->sex, 2)]  : "their")); break;
                case 'S': i = can_see(to, vch) ? his_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "its" : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? his_her[URANGE(0, vch->sex, 2)] : "their")); break;
                case 'c': i = clan_table[ch->clan].who_name; break; /* XUR */
                

                case 'p':
                    i = can_see_obj( to, obj1 )
                            ? obj1->short_descr
                            : "something";
                    break;
 
                case 'P':
                    i = can_see_obj( to, obj2 )
                            ? obj2->short_descr
                            : "something";
                    break;
 
                case 'd':
                    if ( arg2 == NULL || ((char *) arg2)[0] == '\0' )
                    {
                        i = "door";
                    }
                    else
                    {
//                      one_argument( (char *) arg2, fname );
//                      i = fname; 
			i = (char *) arg2;
                    }
                    break;
                }
            }
 
            ++str;
            while (i != NULL && ((*point = *i) != '\0'))
                ++point, ++i;
        }

	if (IS_SET(to->act, PLR_SHOWDAM) && !global_showdam.empty())
	{
        std::ostringstream mess;
        mess << " ";
        int total(0);
        for (size_t j(0); j < global_showdam.size(); ++j)
        {
            total += global_showdam[j].amount;
            mess << " {x({C" << global_showdam[j].amount << ", " << damtype_table[global_showdam[j].type].name << "{x)";
        }
        
        if (global_showdam.size() > 1)
            mess << " {x[{C" << total << " total{x]";

        std::string dammess(mess.str());
        for (size_t j(0); j < dammess.size(); ++j)
            *point++ = dammess[j];
	}

        *point++ = '\n';  
        *point++ = '\r';  
	*point	 = '\0';

        buf[0]   = UPPER(buf[0]);
	if (buf[0] == '{')
		buf[2] = UPPER( buf[2] );
	pbuff	 = buffer;
	colourconv( pbuff, buf, to );

	if (to->desc)
        	write_to_buffer( to->desc, front_capitalize(buffer), 0 );
    }
act_mprog_activate(format, ch, arg1, arg2, type, min_pos);
 
    MOBtrigger = TRUE;
}

/* act nnew is only used for wiznet. No triggers. Woot. */
void act_nnew( const char *format, CHAR_DATA *ch, const void *arg1, 
	      const void *arg2, int type, int min_pos )
{
    static char * const he_she  [] = { "it",  "he",  "she" };
    static char * const him_her [] = { "it",  "him", "her" };
    static char * const his_her [] = { "its", "his", "her" };
 
    CHAR_DATA 		*to, *to_next;
    CHAR_DATA 		*vch = ( CHAR_DATA * ) arg2;
    OBJ_DATA 		*obj1 = ( OBJ_DATA  * ) arg1;
    OBJ_DATA 		*obj2 = ( OBJ_DATA  * ) arg2;
    const 	char 	*str;
    const char 		*i = NULL;
    char 		*point;
    char 		*pbuff;
    char 		buffer[ MAX_STRING_LENGTH*2 ];
    char 		buf[ MAX_STRING_LENGTH   ];
    char 		fname[ MAX_INPUT_LENGTH  ];
    bool		fColour = FALSE;

    /*
     * Discard null and zero-length messages.
     */
    if( !format || !*format )
        return;

    /* discard null rooms and chars */
    if( !ch || !ch->in_room )
        return;

    to = ch->in_room->people;
    if( type == TO_VICT )
    {
        if( !vch )
        {
            bug( "Act: null vch with TO_VICT.", 0 );
            return;
        }

        if( !vch->in_room )
            return;

        to = vch->in_room->people;
    }
 
    for( ; to ; to = to_next )
    {
        to_next = to->next_in_room;
        /*to->desc removed because we want act progs to work always */
        if( /*!to->desc ||*/ to->position < min_pos )
            continue;
 
        if( ( type == TO_CHAR ) && to != ch )
            continue;
        if( type == TO_VICT && ( to != vch || to == ch ) )
            continue;
        if( type == TO_ROOM && to == ch )
            continue;
        if( type == TO_NOTVICT && (to == ch || to == vch) )
            continue;
 
        point   = buf;
        str     = format;
        while( *str != '\0' )
        {
            if( *str != '$' )
            {
                *point++ = *str++;
                continue;
            }
 
	    fColour = TRUE;
	    ++str;
	    i = " <@@@> ";
            if( !arg2 && *str >= 'A' && *str <= 'Z' )
            {
                bug( "Act: missing arg2 for code %d.", *str );
                i = " <@@@> ";
            }
            else
            {
                switch ( *str )
                {
                default:  bug( "Act: bad code %d.", *str );
                          i = " <@@@> ";
		break;
                /* Thx alex for 't' idea */
                case 't': i = (char *) arg1; break;
                case 'T': i = (char *) arg2; break;
                case 'n': i = NPERS( ch,  to  ); break;
                case 'N': i = NPERS( vch, to  ); break;
                case 'e': i = can_see(to, ch)  ? he_she[URANGE(0, ch->sex, 2)]   : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch)  ? "it"  : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? he_she[URANGE(0, ch->sex, 2)]   : "their")); break;
                case 'E': i = can_see(to, vch) ? he_she[URANGE(0, vch->sex, 2)]  : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? he_she[URANGE(0, vch->sex, 2)]  : "their")); break;
                case 'm': i = can_see(to, ch)  ? him_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "it"  : "them")  : (IS_AFFECTED(ch, AFF_WIZI)  ? him_her[URANGE(0, ch->sex, 2)]  : "them"));  break;
                case 'M': i = can_see(to, vch) ? him_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "it"  : "them")  : (IS_AFFECTED(vch, AFF_WIZI) ? him_her[URANGE(0, vch->sex, 2)] : "them"));  break;
                case 's': i = can_see(to, ch)  ? his_her[URANGE(0, ch->sex, 2)]  : (!IS_NPC(ch)  ? (IS_IMMORTAL(ch) ?  "its" : "their") : (IS_AFFECTED(ch, AFF_WIZI)  ? his_her[URANGE(0, ch->sex, 2)]  : "their")); break;
                case 'S': i = can_see(to, vch) ? his_her[URANGE(0, vch->sex, 2)] : (!IS_NPC(vch) ? (IS_IMMORTAL(vch) ? "its" : "their") : (IS_AFFECTED(vch, AFF_WIZI) ? his_her[URANGE(0, vch->sex, 2)] : "their")); break;
                case 'c': i = clan_table[ch->clan].who_name; break; /* XUR
*/
                

                case 'p':
                    i = can_see_obj( to, obj1 )
                            ? obj1->short_descr
                            : "something";
                    break;
 
                case 'P':
                    i = can_see_obj( to, obj2 )
                            ? obj2->short_descr
                            : "something";
                    break;
 
                case 'd':
                    if ( arg2 == NULL || ((char *) arg2)[0] == '\0' )
                    {
                        i = "door";
                    }
                    else
                    {
                        one_argument( (char *) arg2, fname );
                        i = fname;
                    }
                    break;
                }
            }
 
            ++str;
            while ( ( *point = *i ) != '\0' )
                ++point, ++i;
        }
 
        *point++ = '\n';
        *point++ = '\r';
	*point	 = '\0';
        buf[0]   = UPPER(buf[0]);
	if (buf[0] == '{')
		buf[2] = UPPER( buf[2] );
	pbuff	 = buffer;
	colourconv( pbuff, buf, to );
	if (to->desc)
        	write_to_buffer( to->desc, front_capitalize(buffer), 0 );

    }
 
    MOBtrigger = TRUE;
}

/*
 * Macintosh support functions.
 */
#if defined(macintosh)
int gettimeofday( struct timeval *tp, void *tzp )
{
    tp->tv_sec  = time( NULL );
    tp->tv_usec = 0;
}
#endif

int colour( char type, CHAR_DATA *ch, char *string )
{
    char	code[ 20 ];
    char	*p = '\0';

    if( IS_NPC( ch ) && (!ch->desc || !ch->desc->original))
	return( 0 );

    switch( type )
    {
	default:
	    sprintf( code, CLEAR );
	    break;
	case 'x':
	    sprintf( code, CLEAR );
	    break;
	case 'b':
	    sprintf( code, C_BLUE );
	    break;
	case 'c':
	    sprintf( code, C_CYAN );
	    break;
	case 'g':
	    sprintf( code, C_GREEN );
	    break;
	case 'm':
	    sprintf( code, C_MAGENTA );
	    break;
	case 'r':
	    sprintf( code, C_RED );
	    break;
	case 'w':
	    sprintf( code, C_WHITE );
	    break;
	case 'y':
	    sprintf( code, C_YELLOW );
	    break;
	case 'B':
	    sprintf( code, C_B_BLUE );
	    break;
	case 'C':
	    sprintf( code, C_B_CYAN );
	    break;
	case 'G':
	    sprintf( code, C_B_GREEN );
	    break;
	case 'M':
	    sprintf( code, C_B_MAGENTA );
	    break;
	case 'R':
	    sprintf( code, C_B_RED );
	    break;
	case 'W':
	    sprintf( code, C_B_WHITE );
	    break;
	case 'Y':
	    sprintf( code, C_B_YELLOW );
	    break;
	case 'D':
	    sprintf( code, C_D_GREY );
	    break;
	case '.':
	    sprintf( code, C_BLINKING );
	    break;
	case '*':
	    sprintf( code, "%c", 007 );
	    break;
	case '/':
	    sprintf( code, "%c", 012 );
	    break;
	case '{':
	    sprintf( code, "%c", '{' );
	    break;
	case '}':
	    sprintf( code, "%c", '}' );
	    break;
    }

    p = code;
    while( *p != '\0' )
    {
	*string = *p++;
	*++string = '\0';
    }

    return( strlen( code ) );
}

void colourconv( char *buffer, const char *txt, CHAR_DATA *ch )
{
    const	char	*point;
		int	skip = 0;

    if( ch->desc && txt )
    {
	if( (ch->desc->original && IS_SET(ch->desc->original->act, PLR_COLOUR)) || (!ch->desc->original && IS_SET(ch->act, PLR_COLOUR)) )
	{
	    for( point = txt ; *point ; point++ )
	    {
		if( *point == '{' || *point == '}')
		{
		    if (*point == '}' && ((ch->desc->original && !IS_SET(ch->desc->original->nact, PLR_EXCOLOUR)) || (!ch->desc->original && !IS_SET(ch->nact, PLR_EXCOLOUR))))
		    {
			point++;
			continue;
		    }

		    point++;
		    skip = colour( *point, ch, buffer );
		    while( skip-- > 0 )
			++buffer;
		    continue;
		}
		*buffer = *point;
		*++buffer = '\0';
	    }			
	    *buffer = '\0';
	}
	else
	    stripcolor(buffer, txt);
    }
}

char *stripcolor( char *buffer, const char *txt )
{
    const char *point;
    char *orig = buffer;

    for (point = txt; *point; point++)
    {
	if (*point == '{' || *point == '}')
	{
	    point++;

	    if ((*point != '{') && (*point != '}'))
	        continue;
	}
	*buffer = *point;
	*++buffer = '\0';
    }
    *buffer = '\0';

    return orig;
}

void segv_restart(int segv_num) {
  char buf[MAX_INPUT_LENGTH*2];
  segv_counter++; /* increment counter */
  if (segv_counter > MAX_SEGV)
	signal(SIGSEGV, SIG_DFL); /* restore core dump behavior */
  sprintf(buf, "SEGV: %s --> %s", segv_char, segv_cmd);
  bug(buf,0); /* log it */
//  wiznet(buf,NULL,NULL,WIZ_BUGS,0,0); /* notify the imms */

#if defined(unix) || defined(WIN32)
  game_loop_unix(control); /* keep on chuggin */
#else
  game_loop_mac_msdos();
#endif

}

#if defined(unix) || defined(WIN32)
void dns_thread_func(void *d) 
{
    char buf[1024];
    char word1[512], word2[512];
    char *oldhost;
    struct hostent *from;
    struct sockaddr_in sock;
    socklen_t n(sizeof(sock));

    oldhost = ((DESCRIPTOR_DATA *)d)->host;

    if ((getpeername(((DESCRIPTOR_DATA *)d)->descriptor, (struct sockaddr *) &sock, &n))<0)
    {
	((DESCRIPTOR_DATA *)d)->dnsdone = TRUE;
	return;
    }

    from = gethostbyaddr((char *)&sock.sin_addr, sizeof(sock.sin_addr), AF_INET );
 
    if (!from)
    {
	bug("failed resolve", 0);
	((DESCRIPTOR_DATA *)d)->dnsdone = TRUE;
	return;
    }

    sscanf(oldhost, "%s (%s)", word1, word2);
    sprintf(buf, "%s (%s)", from->h_name, word1);
    ((DESCRIPTOR_DATA *)d)->host = str_dup(buf);
    free_string(oldhost);
    ((DESCRIPTOR_DATA *)d)->dnsdone = TRUE;
}
#endif

void send_host_warning(DESCRIPTOR_DATA *d)
{
    HOST_DATA *pHost;
    char buf[MAX_STRING_LENGTH];

    if (!d->character)
	return;

    for (pHost = host_list; pHost; pHost = pHost->next)
	if (!str_cmp(d->host, pHost->host_name))
	{
	    sprintf(buf, "%s reconnection time not met, previously on as %s.", d->character->name, pHost->char_name);
	    wiznet(buf, NULL, NULL, WIZ_CHEATING, 0, 0);
	    log_string(buf);
	    d->warning = FALSE;
	    break;
	}

    return;
}

void check_host(DESCRIPTOR_DATA *d)
{
    HOST_DATA *pHost, *last_host = NULL, *host_next;

    for (pHost = host_list; pHost; pHost = host_next)
    {
	host_next = pHost->next;

	if ((current_time - pHost->lastoff) < HOST_TIME_WARNING)
	{
	    if (!str_cmp(d->host, pHost->host_name))
	    {
		d->warning = TRUE;
/*
		sprintf(buf, "%s reconnection time not met, previously on as %s.", d->host, pHost->char_name);
		wiznet(buf, NULL, NULL, WIZ_CHEATING, 0, 0);
		log_string(buf);
*/
	    }

	    if (last_host)
	        last_host->next = pHost;
	    else
		host_list = pHost;

	    last_host = pHost;
	}
	else
	{
	    if (last_host)
		last_host->next = NULL;

	    free_string(pHost->host_name);
	    free_string(pHost->char_name);
	    free(pHost);
	    g_num_hostdata--;
	}
    }

    if (!last_host)
	host_list = NULL;

    return;
}

void add_hostdata(DESCRIPTOR_DATA *d)
{
    HOST_DATA *pHost;

    if (!d->character)
	return;

    for (pHost = host_list; pHost; pHost = pHost->next)
    {
	if (!str_cmp(d->host, pHost->host_name))
	{
	    pHost->lastoff = current_time;
	    return;
	}
    }

    pHost = (HOST_DATA*)malloc(sizeof(struct host_data));
    g_num_hostdata++;
    pHost->next = host_list;
    host_list = pHost;
    pHost->lastoff = current_time;
    pHost->host_name = str_dup(d->host);
    pHost->char_name = str_dup(d->character->name);
}

void add_socket_info(CHAR_DATA *ch, char *host)
{
    char buf[640];
    char *spoint, *ddec, *sdec;
    int snum;
    char shost[512];
    char dnshost[512], iphost[512];
    char new_socket_info[MAX_STRING_LENGTH];
    bool found = FALSE;

    spoint = ch->pcdata->socket_info;

    sscanf(host, "%s (%s)", dnshost, iphost);

    memset((void *) new_socket_info, 0, MAX_STRING_LENGTH);

    ddec = strchr(dnshost, '.');

    if (spoint[0] != '\0')
    {
	while (spoint)
	{
    	    spoint = str_linecopy(buf, spoint);

	    if (!found)
	    {
	        sscanf(buf, "%d %s", &snum, shost);
	    
	        if (!str_cmp(dnshost, shost))
	        {
		    snum++;
		    sprintf(buf, "%d %s", snum, shost);
		    found = TRUE;
		}
		else
		{
		    if (!str_cmp(dnshost, iphost))
		    {
			sdec = strrchr(shost, '.');

			if (!strncmp(dnshost, shost, sdec - shost))
			{
			    char buf2[512];

			    memset((void *) buf2, 0, 512);
		
			    strncpy(buf2, dnshost, sdec - shost);

			    snum++;
			    sprintf(buf, "%d %s*", snum, buf2);
			    found = TRUE;
			}
		    }
		    else if (ddec)
		    {
			sdec = strchr(shost, '.');
			if (sdec && !str_cmp(ddec, sdec))
			{
			    snum++;
			    sprintf(buf, "%d *%s", snum, ddec);
			    found = TRUE;
			}
		    }
		}
	    }

	    strcat(new_socket_info, buf);
	    strcat(new_socket_info, "\n\r");
	}
    }

    if (!found)
    {
	sprintf(buf, "1 %s\n\r", dnshost);
	strcat(new_socket_info, buf);
    }

    free_string(ch->pcdata->socket_info);
    ch->pcdata->socket_info = str_dup(new_socket_info);

    if (!ch->desc || !ch->desc->acct)
	return;

// horrible hack.. just put acct socket record here.

    found = FALSE;
    spoint = ch->desc->acct->socket_info;

    sscanf(host, "%s (%s)", dnshost, iphost);

    memset((void *) new_socket_info, 0, MAX_STRING_LENGTH);

    ddec = strchr(dnshost, '.');

    if (spoint[0] != '\0')
    {
	while (spoint)
	{
    	    spoint = str_linecopy(buf, spoint);

	    if (!found)
	    {
	        sscanf(buf, "%d %s", &snum, shost);
	    
	        if (!str_cmp(dnshost, shost))
	        {
		    snum++;
		    sprintf(buf, "%d %s", snum, shost);
		    found = TRUE;
		}
		else
		{
		    if (!str_cmp(dnshost, iphost))
		    {
			sdec = strrchr(shost, '.');

			if (!strncmp(dnshost, shost, sdec - shost))
			{
			    char buf2[512];

			    memset((void *) buf2, 0, 512);
		
			    strncpy(buf2, dnshost, sdec - shost);

			    snum++;
			    sprintf(buf, "%d %s*", snum, buf2);
			    found = TRUE;
			}
		    }
		    else if (ddec)
		    {
			sdec = strchr(shost, '.');
			if (sdec && !str_cmp(ddec, sdec))
			{
			    snum++;
			    sprintf(buf, "%d *%s", snum, ddec);
			    found = TRUE;
			}
		    }
		}
	    }

	    strcat(new_socket_info, buf);
	    strcat(new_socket_info, "\n\r");
	}
    }

    if (!found)
    {
	sprintf(buf, "1 %s\n\r", dnshost);
	strcat(new_socket_info, buf);
    }

    free_string(ch->desc->acct->socket_info);
    ch->desc->acct->socket_info = str_dup(new_socket_info);

    return;
}

void erase_line(DESCRIPTOR_DATA *d)
{
#if defined(unix)
    write_to_buffer(d, erase_line_str, 0);
#endif
    return;
}
