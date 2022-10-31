/* * * * * * * * * * * * * * * * *
 *                               *
 *        SHA-Check 0.40         *
 *                               *
 *        2022-10-22             *
 *                               *
 *        Zax                    *
 *                               *
 * * * * * * * * * * * * * * * * */

/*#################################
#	See below for Windows
#	compilation issues
##################################*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMLib.h"

#define PROG_VERSION "0.40"

#if __linux__
#define DB_LOCATION "/.scheck_db"
#endif

#if __windows__
#define DB_LOCATION "\\.scheck_db"
#endif

int main (int argc, char *argv [])

{

FILE *SMDB_IN_FP, *DB_LOCATION_FP, *FILE_TO_CHECK_FP;
FILE *SHA_PIPE;

int arg_no, switch_pos;		// args section
int database_ferr;		// database file error

char switch_chr;		// args section
char search_filename [FILEPATH_LENGTH] = "";
char database_filename [FILEPATH_LENGTH] = "";
char database_location [FILEPATH_LENGTH] = "";		// filepath to .scheck_db
char sha_command [FILEPATH_LENGTH] = "";		// holds sha256sum command for search file
char database_type;
char database_first_line = SW_ON;
char fileline [FILELINE_LENGTH];			// input line
char sha_line [FILELINE_LENGTH];			// sha input line
char match_found = FALSE;				// if match is found
char database_good = FALSE;				// has database passed sha_verify?

struct sha_database sha_db [1] = {0};			// fields for database
struct sha_database file_to_check [1] = {0};		// fields for search file

#if __linux__
strcpy (database_location, getenv ("HOME"));
strcat (database_location, DB_LOCATION);
#endif

#if __windows__
strcpy (database_location, getenv ("USERPROFILE"));
strcat (database_location, DB_LOCATION);
#endif

// Arguments section
for (arg_no = 1; arg_no < argc; arg_no++)		// loop through arguments
	{
	if ((int) argv [arg_no] [0] == '-')
		{
		for (switch_pos = 1; switch_pos < strlen (argv[arg_no]); switch_pos++)
			{
			switch_chr = (int) argv [arg_no] [switch_pos];
			switch (switch_chr)
				{
				case 'd':
					if (argv [arg_no + 1] != NULL)		// if database specified read next arg for name
						{
						strcpy (database_filename, argv [arg_no + 1]);
						}
					break;
				case 'V':
					printf ("SHA Check version %s\n", PROG_VERSION);
					exit (0);
				default:
					printf ("%s# SHA Check [V] <search file> [-d <database file>]\n%s", TEXT_YELLOW, TEXT_RESET);
					exit (0);
				}	// END switch
			}	// END for switch_pos
		}	// END if int argv
		else
		{
		if (strcmp (database_filename, argv [arg_no]))		// if arg isn't a switch and isn't the database name
			{
			strncpy (search_filename, argv [arg_no], FILEPATH_LENGTH);
			}
		}	// END else if int argv
	}	// END for arg_no
if (!search_filename [0])
	{
	exit_error ("No search file given.", "");
	}

// File open section
if (!database_filename [0])		// if no database given, fall back to stored location
	{
	DB_LOCATION_FP = fopen (database_location, "r");
	if (DB_LOCATION_FP != NULL)
		{
		fgets (database_filename, FILEPATH_LENGTH, DB_LOCATION_FP);	// read database location
		fclose (DB_LOCATION_FP);
		}
	}
SMDB_IN_FP = fopen (database_filename, "r");
if (SMDB_IN_FP == NULL)
	{
	exit_error ("Can't find Database: ", database_filename);
	}
FILE_TO_CHECK_FP = fopen (search_filename, "r");
if (FILE_TO_CHECK_FP == NULL)
	{
	exit_error ("Can't find file: ", search_filename);
	}

/*######################################################
#
#	FOR COMPILATION IN WINDOWS
#
#	The OS identifier used in the #if can vary
#	by compiler.
#
#	The output of CertUtil does also not always
#	return the same line terminators.
#
#	If compilation isn't working, see
#	SMLib.h and SMLib.c.
#
########################################################*/

// Get search file sha section
strcpy (file_to_check->filepath, search_filename);		// enter filepath into field
strcpy (file_to_check->dataset, "");				// dataset for search file always blank
strcpy (sha_command, SHA_CMD);					// enter OS specific command
strcat (sha_command, enquote (file_to_check->filepath));	// add filepath
#if __windows__
	strcat (sha_command, SHA_CMD_ARG);			// add argument to end of certutil
#endif
SHA_PIPE = popen (sha_command, "r");				// send SHA256SUM command and arguments
#if __windows__
	fgets(sha_line, FILELINE_LENGTH, SHA_PIPE);		// discard first line from certutil
#endif
fgets(sha_line, FILELINE_LENGTH, SHA_PIPE);			// receive reply
fclose (SHA_PIPE);
if (sha_verify (sha_line))					// verify SHA256SUM of search file
	{
	strncpy (file_to_check->sha, sha_line, SHA_LENGTH);	// enter SHA256SUM into field
	file_to_check->sha [SHA_LENGTH] = NULL_TERM;
	}
	else
	{
	exit_error ("Invalid SHA256SUM from file ", file_to_check->filepath);
	}

// Search section
do
	{
	database_ferr = (long)fgets (fileline, FILEPATH_LENGTH, SMDB_IN_FP);
	if (database_first_line)		// if database first line, sha_verify
		{
		database_type = sha_verify (fileline);
		if (database_type == UNKNOWN_TYPE)
			{
			fclose (SMDB_IN_FP);
			exit_error ("Unrecognised file type: ", database_filename);
			}
		database_good = TRUE;		// database can be trusted
		database_first_line = SW_OFF;
		}
	if (fileline != NULL && database_ferr)
		{
		if (database_type == SHA256_TYPE)				// load standard SHA256SUM output
			{
			strncpy (sha_db->sha, fileline, SHA_LENGTH);		// enter SHA256SUM into field
			sha_db->sha[SHA_LENGTH] = NULL_TERM;
			strcpy (sha_db->filepath, fileline + SHA_LENGTH + 2);	// enter filepath into field
			sha_db->filepath[strlen (sha_db->filepath) - 1] = NULL_TERM;
			strcpy (sha_db->dataset, database_filename);		// enter database filename as dataset
			}
			else							// load S2DB data
			{
			separate_fields (sha_db->sha, sha_db->filepath, sha_db->dataset, fileline);
			}
		if (!strcmp (file_to_check->sha, sha_db->sha))	// SHA256SUMs match
			{
			printf ("File: %s%s%s matches %s%s%s in %s%s%s\n", TEXT_BLUE, file_to_check->filepath,\
				 TEXT_RESET, TEXT_BLUE, sha_db->filepath, TEXT_RESET, TEXT_YELLOW, sha_db->dataset, TEXT_RESET);
			match_found = TRUE;
			}
		}

	} while (!feof (SMDB_IN_FP));
if (!match_found)	// SHA256SUMs match
	{
	printf ("No match.\n");
	}

// Clean up section
if (database_good)
	{
	DB_LOCATION_FP = fopen (database_location, "w");
        if (DB_LOCATION_FP != NULL)
		{
                fprintf (DB_LOCATION_FP, "%s", database_filename);		// sent database name to storage
                fclose (DB_LOCATION_FP);
		}
	}
fclose (SMDB_IN_FP);

}
