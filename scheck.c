/* * * * * * * * * * * * * * * * *
 *                               *
 *        SHA-Check 0.32         *
 *                               *
 *        2020-11-19             *
 *                               *
 *        Zax                    *
 *                               *
 * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMLib.h"

#define PROG_VERSION "0.32"
#define SHA_CMD "sha256sum "

#if __linux__
#define DATABASE_LOCATOR_STRING "/.scheck_db"
#endif

#if _WIN32
#define DATABASE_LOCATOR_STRING "\\.scheck_db"
#endif

int main (int argc, char *argv [])

{

FILE *DATABASE_FP, *DATABASE_LOCATOR_FP, *CHECK_FILE_FP;
FILE *SHA_PIPE;

int arg_no, switch_pos;		// args section
int database_ferr;		// database file error

char switch_chr;		// args section
char search_filename [FILEPATH_LENGTH] = "";
char database_filename [FILEPATH_LENGTH] = "";
char database_locator_path [FILEPATH_LENGTH] = "";	// filepath to .scheck_db
char sha_command [FILEPATH_LENGTH] = "";		// holds sha256sum command for search file
char database_type;
char database_first_line = SW_ON;
char fileline [FILELINE_LENGTH];			// input line
char sha_line [FILELINE_LENGTH];			// sha input line
char match_found = FALSE;				// if match is found
char database_good = FALSE;				// has database passed sha_verify?

struct sha_database database_db [1] = {0};		// fields for database
struct sha_database check_file [1] = {0};		// fields for search file

#if __linux__
strcpy (database_locator_path, getenv ("HOME"));
strcat (database_locator_path, DATABASE_LOCATOR_STRING);
#endif

#if _WIN32
strcpy (database_locator_path, getenv ("USERPROFILE"));
strcat (database_locator_path, DATABASE_LOCATOR_STRING);
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
					exit_error ("# SHA Check [V] <search file> [-d <database file>]","");
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
if (search_filename [0] == 0)
	{
	exit_error ("No search file given.", "");
	}

// File open section
if (database_filename [0] == 0)		// if no database given, fall back to stored location
	{
	DATABASE_LOCATOR_FP = fopen (database_locator_path, "r");
	if (DATABASE_LOCATOR_FP != NULL)
		{
		fgets (database_filename, FILEPATH_LENGTH, DATABASE_LOCATOR_FP);	// read database location
		fclose (DATABASE_LOCATOR_FP);
		}
	}
DATABASE_FP = fopen (database_filename, "r");
if (DATABASE_FP == NULL)
	{
	exit_error ("Can't find Database: ", database_filename);
	}
CHECK_FILE_FP = fopen (search_filename, "r");
if (CHECK_FILE_FP == NULL)
	{
	exit_error ("Can't find file: ", search_filename);
	}

// Get search file sha section
strcpy (check_file->filepath, search_filename);		// enter filepath into field
strcpy (check_file->dataset, "");			// dataset for search file always blank
strcpy (sha_command, SHA_CMD);
strcat (sha_command, enquote (check_file->filepath));
SHA_PIPE = popen (sha_command, "r");			// send SHA256SUM command and arguments
fgets(sha_line, FILELINE_LENGTH, SHA_PIPE);		// receive reply
fclose (SHA_PIPE);
if (sha_verify (sha_line))		// verify SHA256SUM of search file
	{
	strncpy (check_file->sha, sha_line, SHA_LENGTH);	// enter SHA256SUM into field
	check_file->sha [SHA_LENGTH] = NULL_TERM;
	}
	else
	{
	exit_error ("Invalid SHA256SUM from file ", check_file->filepath);
	}

// Search section
do
	{
	database_ferr = (long)fgets (fileline, FILEPATH_LENGTH, DATABASE_FP);
	if (database_first_line)	// if database first line, sha_verify
		{
		database_type = sha_verify (fileline);
		if (database_type == UNKNOWN_TYPE)
			{
			fclose (DATABASE_FP);
			exit_error ("Unrecognised file type: ", database_filename);
			}
		database_good = TRUE;		// database can be trusted
		database_first_line = SW_OFF;
		}
	if (fileline != NULL && database_ferr)
		{
		if (database_type == SHA256_TYPE)			// load standard SHA256SUM output
			{
			strncpy (database_db->sha, fileline, SHA_LENGTH);		// enter SHA256SUM into field
			database_db->sha[SHA_LENGTH] = NULL_TERM;
			strcpy (database_db->filepath, fileline + SHA_LENGTH + 2);	// enter filepath into field
			database_db->filepath[strlen (database_db->filepath) - 1] = NULL_TERM;
			strcpy (database_db->dataset, database_filename);		// enter database filename as dataset
			}
			else						// load S2DB data
			{
			separate_fields (database_db->sha, database_db->filepath, database_db->dataset, fileline);
			}
		if (!strcmp (check_file->sha, database_db->sha))	// SHA256SUMs match
			{
			printf ("File: %s%s%s matches %s%s%s in %s%s%s\n", TEXT_BLUE, check_file->filepath,\
				 TEXT_RESET, TEXT_BLUE, database_db->filepath, TEXT_RESET, TEXT_YELLOW, database_db->dataset, TEXT_RESET);
			match_found = TRUE;
			}
		}

	} while (!feof (DATABASE_FP));
if (!match_found)	// SHA256SUMs match
	{
	printf ("No match.\n");
	}

// Clean up section
if (database_good)
	{
	DATABASE_LOCATOR_FP = fopen (database_locator_path, "w");
        if (DATABASE_LOCATOR_FP != NULL)
		{
                fprintf (DATABASE_LOCATOR_FP, "%s", database_filename);		// sent database name to storage
                fclose (DATABASE_LOCATOR_FP);
		}
	}
fclose (DATABASE_FP);

}
