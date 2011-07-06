
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: status.c,v 1.8 2007/03/28 18:22:29 paulf Exp $
 *
 *    Revision history:
 *     $Log: status.c,v $
 *     Revision 1.8  2007/03/28 18:22:29  paulf
 *     added _MACOSX flag
 *
 *     Revision 1.7  2006/04/04 19:02:25  stefan
 *     removing deendency on old .h file
 *
 *     Revision 1.6  2005/07/27 17:27:15  friberg
 *     added _LINUX directive for appropriate startstop.h
 *
 *     Revision 1.5  2005/04/04 21:37:55  dietz
 *     *** empty log message ***
 *
 *     Revision 1.4  2005/04/04 21:34:15  dietz
 *     Change strnicmp to strncasecmp and added #define strncasecmp _strnicmp
 *     in the #ifdef _WINNT section.
 *
 *     Revision 1.3  2004/08/03 18:46:08  mark
 *     Added command-line param to redirect stdout to a file
 *
 *     Revision 1.2  2000/07/24 21:13:59  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 19:40:54  lucky
 *     Initial revision
 *
 *
 */

/**********************************************************
 *                       status                           *
 *    Program to ask startstop for a status report on     *
 *    the currently running Earthworm modules             *
 **********************************************************/

/* 5/21/98: changed to allow config file to be specified on the command line.
 * If none is specified, then a default file is used depending on the OS.
 * PNL, UW Geophysics. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* include the appropriate definition of DEF_CONFIG for the OS */
#if defined(_SOLARIS) || defined(_LINUX)  || defined(_MACOSX)
#include <startstop_unix_generic.h>
#endif

#ifdef _WINNT
#include <startstop_winlib.h>
#define strncasecmp _strnicmp
#endif
#ifdef _OS2
#include <startstop_os2.h>
#endif

void ReadRingName( char*, char* );
void SendStatusRequest( void );
void GetStatusReport( char* );
void SendRestartRequest( char* );
void GetChild( void );
void SocketLoop(void);

SHM_INFO   region;                /* The shared memory region   */
MSG_LOGO   logo;
static unsigned char InstId;      /* local installation id      */
static unsigned char MyModId;
static unsigned char Port;
static unsigned char TypeReqStatus;
static unsigned char TypeStatus;
static unsigned char TypeRestart;

static char targetMod[50];
static char targetModLine[50];
static int ringKey;
static int redirect = 0;

#define MAX_BYTES_STATUS MAX_BYTES_PER_EQ

main( int argc, char *argv[] )
{

	char *runPath;
	char  ringname[MAX_RING_STR];
	char configFile[FILENAME_MAX] = DEF_CONFIG;
	FILE *stream;

	/* Check the number of arguments.
	 ******************************/
	if ( argc == 3 )
	{
		strcpy ( configFile, argv[1] );
		if (strncasecmp(argv[2], "-r", 2) == 0 || strncasecmp(argv[2], "/r", 2) == 0)
		{
			redirect = 1;
		}
	}
	else if ( argc == 2 )
	{
		if (strncasecmp(argv[1], "-r", 2) == 0 || strncasecmp(argv[1], "/r", 2) == 0)
			redirect = 1;
		else
			strcpy ( configFile, argv[1] );
	}
	else
	{
		fprintf ( stderr, "using default config file %s\n", configFile );

	}

	/* Change working directory to environment variable EW_PARAMS value
	 ****************************************************************/
	runPath = getenv( "EW_PARAMS" );

	if ( runPath == NULL )
	{
		fprintf(stderr, "status: Environment variable EW_PARAMS not defined." );
		fprintf(stderr, " Exiting.\n" );
		return -1;
	}

	if ( *runPath == '\0' )
	{
		fprintf(stderr, "status: Environment variable EW_PARAMS " );
		fprintf(stderr, "defined, but has no value. Exiting.\n" );
		return -1;
	}

	if ( chdir_ew( runPath ) == -1 )
	{
		fprintf(stderr, "status: Params directory not found: %s\n", runPath );
		fprintf(stderr, "status: Reset environment variable EW_PARAMS." );
		fprintf(stderr, " Exiting.\n" );
		return -1;
	}

	/* Look up ids in earthworm.d tables
	 ***********************************/
	if ( GetLocalInst( &InstId ) != 0 )
	{
		fprintf( stderr,
				"status: error getting local installation id; exiting!\n" );
		return -1;
	}

	if ( GetModId( "MOD_STATUS", &MyModId ) != 0 )
	{
		fprintf( stderr,
				"status: Invalid module id <MOD_STATUS> exiting!\n" );
		return -1;
	}

	if ( GetType( "TYPE_REQSTATUS", &TypeReqStatus ) != 0 )
	{
		fprintf( stderr,
				"status: Invalid message type <TYPE_REQSTATUS> exiting!\n" );
		return -1;
	}
	if ( GetType( "TYPE_STATUS", &TypeStatus ) != 0 )
	{
		fprintf( stderr,
				"status: Invalid message type <TYPE_STATUS> exiting!\n" );
		return -1;
	}
	if ( GetType( "TYPE_RESTART", &TypeRestart ) != 0 )
	{
		fprintf( stderr,
				"restart: Invalid message type <TYPE_RESTART> exiting!\n" );
		return -1;
	}
/*
	if ( GetType( "Port", &Port ) != 0 )
	{
		fprintf( stderr,
				"status: Invalid module id <MOD_STATUS> exiting!\n" );
		return -1;
	}

*/
	/* Read startstop's config file to find a valid transport ring name
	 ******************************************************************/
	ReadRingName( ringname, configFile );
	if( (ringKey = GetKey(ringname)) == -1 )
	{
		printf( "status: Invalid ring name <%s>. Exiting.\n",
				ringname );
		return -1;
	}

	if (redirect != 0)
	{
		stream = freopen("status_output.log", "w", stdout);
		if (stream == NULL)
		{
			fprintf( stderr, "Unable to redirect output to 'status_output.log'\n");
		}
	}

	/* Attach to shared memory ring
	 *****************************/
	fprintf( stdout, "NOTE: If next line reads \"ERROR: tport_attach...\","
			" Earthworm is not running.\n" );
	tport_attach( &region, ringKey );
	SocketLoop();

	if (redirect != 0)
		fclose(stream);

	tport_detach( &region );
	return 0;
}

void SocketLoop(void )
{
	int sockfd, newsockfd, portno;

	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)        error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi("6666");


	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);


	if (bind(sockfd, (struct sockaddr *) &serv_addr,
				sizeof(serv_addr)) < 0)
		error("ERROR on binding");
	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	while(1){
		newsockfd = accept(sockfd,
				(struct sockaddr *) &cli_addr,
				&clilen);
		if (newsockfd < 0)
			error("ERROR on accept");
		bzero(buffer,256);
		n = read(newsockfd,buffer,255);
		if (n < 0) error("ERROR reading from socket");

		printf("Restarting %s ....\n ",buffer);

		SendStatusRequest();
		GetStatusReport(buffer);
		char *key, *pid;
		key = strtok(targetModLine, " ");
		pid = strtok(NULL, " ");


		SendRestartRequest(pid);
		n = write(newsockfd,"DONE",18);
		if (n < 0) error("ERROR writing to socket");
	}

	close(newsockfd);
	close(sockfd);




}
void GetChild( void )
{
	printf("getting child\n");
	//printf("The line is %s\n", wavemas2disk);
	char *key, *pid;
	//key = strtok(wavemas2disk, " ");
	pid = strtok(NULL, " ");
	
	
	SendRestartRequest(pid);	
	printf("getting child end\n");
	return;
}
void SendRestartRequest( char*  modPid ) /* pid of module as ascii string*/
{
	MSG_LOGO logo;
	char message[512];

	/* Build message
	 *    *************/
	strcpy (message,modPid);
	strcat( message, "\n\0" );

	/* Set logo values of pager message
	 *    ********************************/
	logo.type   = TypeRestart;
	logo.mod    = MyModId;
	logo.instid = InstId;

	/* Send restart message to transport ring
	 *    **************************************/
	if ( tport_putmsg( &region, &logo, strlen(message), message ) != PUT_OK )
		fprintf(stderr, "restart: Error sending message to transport region.\n" );

	return;

}

/************************************************************
 *                    PrintStatusReport                     *
 *   Get the Earthworm status message from shared memory.   *
 *   Print it to the screen.                                *
 ************************************************************/
void GetStatusReport( char* targetMod )
{
	char     msg[MAX_BYTES_STATUS];
	MSG_LOGO rlogo;
	long     rlen;
	int      timeout=10;
	int      rc;
	int      i;

	/* Set logo values of message
	 **************************/
	logo.type   = TypeStatus;
	logo.mod    = 0;           /* zero is the wildcard */
	logo.instid = InstId;
	char * myline;
	for(i=1; i<timeout; i++)
	{
		rc = tport_getmsg( &region, &logo, 1,
				&rlogo, &rlen, msg, (long)MAX_BYTES_STATUS-1 );
		if( rc == GET_NONE   )  {
			sleep_ew( 1000 );
			continue;
		}
		if( rc == GET_TOOBIG ) {
			fprintf( stdout,
					"Earthworm is running, but the status msg is too big for me!\n" );
			return;
		}
		else {
			msg[rlen]='\0';  /* null terminate the message */
			myline = strtok(msg,"\n");
			while(myline !=NULL)
			{
				if(strstr(myline,targetMod))
				{
					strcpy(targetModLine,myline);
					break;
				}
				myline = strtok(NULL,"\n");
			}
			return;
		}
	}

	/* If you got here, you timed out
	 ********************************/
	fprintf( stdout,
			"Earthworm may be hung; no response from startstop in %d seconds.\n",
			timeout );
	return;
}

void SendStatusRequest( void )
{
	char message[16];
	MSG_LOGO rlogo;
	long     rlen;

	/* Flush all old messages in the ring
	 *    **********************************/
	logo.type   = TypeStatus;
	logo.mod    = 0;
	logo.instid = InstId;
	while( tport_getmsg( &region, &logo, 1,
				&rlogo, &rlen, message, (long)16 ) != GET_NONE );

	/* Build status request message
	 *    ****************************/
	sprintf(message,"%d\n", MyModId );

	/* Set logo values of message
	 *    **************************/
	logo.type   = TypeReqStatus;
	logo.mod    = MyModId;
	logo.instid = InstId;

	/* Send status message to transport ring
	 *    **************************************/
	if ( tport_putmsg( &region, &logo, strlen(message), message ) != PUT_OK )
		fprintf(stderr, "status: Error sending message to transport region.\n" );

	return;
}
/*************************************************************
 *                      ReadRingName                         *
 * Read startstop's configfile as far as the first ring name *
 *************************************************************/
void ReadRingName( char *ringname, char *configFile )
{
	int        nfiles;
	char      *com, *str;
	char       configfile[20];

	strcpy( configfile, "" );

	nfiles = k_open( configFile );
	if ( nfiles == 0 )
	{
		printf( "status: Error opening file <%s>. Exiting.\n",
				configFile );
		exit( -1 );
	}

	/* Process Ring commands from startstop's command file
	 ***************************************************/
	while(nfiles > 0)          /* While there are command files open */
	{
		while(k_rd())         /* Read next line from active file  */
		{
			com = k_str();    /* Get the first token from line */

			/* Ignore blank lines & comments
			 *****************************/
			if( !com )           continue;
			if( com[0] == '#' )  continue;

			/* Process only 1st Ring command
			 *****************************/
			if( k_its( "Ring" ) )
			{
				str = k_str();
				strcpy( ringname, str );
				break;
			}

			/* See if there were any errors processing this command
			 ****************************************************/
			if( k_err() )
			{
				printf( "status: Bad <%s> command in <%s>. Exiting.\n",
						com, configfile );
				return;
			}
		}
		nfiles = k_close();
	}

	return;
}
