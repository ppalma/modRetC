
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


void ReadRingName( char*, char* );

static unsigned char InstId;
static unsigned char MyModId;
static unsigned char TypeReqStatus;
static unsigned char TypeStatus;
static int ringKey;
SHM_INFO   region;  


int main(int argc, char *argv[] )
{
	/* Change working directory to environment variable EW_PARAMS value
	 *    ****************************************************************/

	char *runPath;

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
	 *  ***********************************/
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
	char  ringname[MAX_RING_STR];
	char configFile[FILENAME_MAX] = DEF_CONFIG;
	ReadRingName( ringname, configFile );

if( (ringKey = GetKey(ringname)) == -1 )
   {
       printf( "status: Invalid ring name <%s>. Exiting.\n",
                ringname );
       return -1;
   }


	  printf("RING NAME %s Ring key %d\n",ringname,ringKey);
	printf("nChild %d\n", metaring.statmgr_location);
int i;
//	printf("nChild %d\n",nChild);
	 for ( i = 0; i < nChild; i++ )
   {
#ifdef _SOLARIS
        /* To prevent starting statmgr a second time, we'll just skip over it
 *          If it's not there, then the index to skip defaults to one past MAXCHILD, so
 *                   no other modules will be skipped. */
         if (i != metaring.statmgr_location)
         {
#endif /* _SOLARIS */
            ConstructIds( child[i].use_uname,
                          child[i].use_gname,
                          &child[i].use_uid,
                          &child[i].use_gid );
		printf("Child \n");

#ifdef _SOLARIS
         }
#endif /* _SOLARIS */
   }





	return 0;
}
/*************************************************************
 *    *                      ReadRingName                         *
 *       * Read startstop's configfile as far as the first ring name *
 *          *************************************************************/
void ReadRingName( char *ringname, char *configFile )
{
	int        nfiles;
	char      *com, *str;
	char       configfile[20];

	strcpy( configfile, "none" );

	nfiles = k_open( configFile );
	if ( nfiles == 0 )
	{
		printf( "status: Error opening file <%s>. Exiting.\n",
				configFile );
		exit( -1 );
	}

	/* Process Ring commands from startstop's command file
	 *    ***************************************************/
	while(nfiles > 0)          /* While there are command files open */
	{
		while(k_rd())         /* Read next line from active file  */
		{
			com = k_str();    /* Get the first token from line */

			/* Ignore blank lines & comments
			 *            *****************************/
			if( !com )           continue;
			if( com[0] == '#' )  continue;
			/* Process only 1st Ring command
			 *            *****************************/
			if( k_its( "Ring" ) )
			{
				str = k_str();
				strcpy( ringname, str );
				break;
			}

			/* See if there were any errors processing this command
			 *            ****************************************************/
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

