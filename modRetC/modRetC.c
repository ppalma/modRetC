
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: status.c,v 1.8 2007/03/28 18:22:29 paulf Exp $
 *
 *    Revision history:
 *     $Log: status.c,v $
 */



#include <startstop_unix_generic.h>
#include <earthworm_complex_funcs.h>

int main(int argc, char *argv[] )
{
	metaring.LogLocation = metaring.ParamLocation = metaring.BinLocation = NULL;
	strcpy ( metaring.ConfigFile, "modRetC.d" );
	logit_init( metaring.ConfigFile, (short) metaring.MyModId, MAXLINE*3, 1 );

	fprintf( stderr, "modRetC: Using config file %s\n", metaring.ConfigFile );
	char *runPath;
	char *logPath;
	runPath = getenv( "EW_PARAMS" );

	if ( runPath == NULL )
	{
		printf( "startstop: Environment variable EW_PARAMS not defined." );
		printf( " Exiting.\n" );
		exit (-1);
	}

	if ( *runPath == '\0' )
	{
		printf( "startstop: Environment variable EW_PARAMS " );
		printf( "defined, but has no value. Exiting.\n" );
		exit (-1);
	}

	if ( chdir( runPath ) == -1 )
	{
		printf( "startstop: Params directory not found: %s\n", runPath );
		printf( "startstop: Reset environment variable EW_PARAMS." );
		printf( " Exiting.\n" );
		exit (-1);
	}


	metaring.ParamLocation = (char *)malloc( strlen(runPath)+1 );
	if ( metaring.ParamLocation != NULL )
		strcpy( metaring.ParamLocation, runPath );

	logPath = getenv( "EW_LOG" );
	metaring.LogLocation = (char *)malloc( strlen(logPath)+1 );
	if ( metaring.LogLocation != NULL )
		strcpy ( metaring.LogLocation, logPath );
	int j;
	int rv = GetUtil_LoadTableCore(0);
	if ( rv == -1 )
		fprintf(stderr, "startstop: reconfigure rejected due to error\n" );
	else
	{

		for ( j = metaring.nRing; j < (newNRing); j++ ) {
			logit( "e" , "tport_create: creating ring number <%d>\n", j );
			tport_create( &(metaring.Region[j]),
					1024 * metaring.ringSize[j],
					metaring.ringKey[j] );
			metaring.nRing ++;
		}

	}
	//	sprintf (msg, "%d", child[metaring.statmgr_location].pid);
	printf ( "-----------%d\n", child[metaring.statmgr_location].pid);
	printf("adsfasdfasdf\n");


	return 0;
}

