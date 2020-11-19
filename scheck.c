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
#define DATABASE_LOCATOR_STRING "~/.scheck_db"
#endif

#if _WIN32
#define DATABASE_LOCATOR_STRING "\\.scheck_db"
#endif

int main (int argc, char *argv [])

{

FILE *DATABASE_FP, *DATABASE_LOCATOR_FP, *CHECK_FILE_FP;
FILE *SHA_PIPE;

int arg_no, switch_pos;		// args section
int line_index;
int database_ferr;			// database file error

char switch_chr;			// args section
char search_filename [FILEPATH_LENGTH] = "";
char database_filename [FILEPATH_LENGTH] = "";
char database_locator_path [FILEPATH_LENGTH] = "";
char sha_command [FILEPATH_LENGTH] = "";
char database_type;
char database_first_line = SW_ON;
char fileline [FILELINE_LENGTH];				// input line
char sha_line [FILELINE_LENGTH];				// sha input line
char match_found = FALSE;

struct sha_database database_db [1] = {0};
struct sha_database check_file [1] = {0};

#if __linux__
strcpy (database_locator_path, DATABASE_LOCATOR_STRING);
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
					if (argv [arg_no + 1] != NULL)
						{
						strcpy (database_filename, argv [arg_no + 1]);
						}
					break;
				case 'V':
					printf ("SHA Check version %s\n", PROG_VERSION);
					exit (0);
				default:
					exit_error ("# SHA Check [V] <search file> [-d <database file>]\n","");
				}	// END switch
			}	// END for switch_pos
		}	// END if int argv
		else
		{
		strncpy (search_filename, argv [arg_no], FILEPATH_LENGTH);
		}	// END else if int argv
	}	// END for arg_no

printf ("dbl=%s\tdb=%s\n", database_locator_path, database_filename);
if (database_filename [0] == 0)
	{
	DATABASE_LOCATOR_FP = fopen (database_locator_path, "r");
	if (DATABASE_LOCATOR_FP != NULL)
		{
		fgets (database_filename, FILEPATH_LENGTH, DATABASE_LOCATOR_FP);
		}
	}
printf ("db=%s\n", database_filename);
//exit (0);


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
strcpy (check_file->filepath, search_filename);
strcpy (check_file->dataset, "");
strcpy (sha_command, SHA_CMD);
strcat (sha_command, enquote (check_file->filepath));
SHA_PIPE = popen (sha_command, "r");			// send SHA256SUM command and arguments
fgets(sha_line, FILELINE_LENGTH, SHA_PIPE);		// receive reply
fclose (SHA_PIPE);
if (sha_verify (sha_line))		// verify SHA256SUM
	{
	strncpy (check_file->sha, sha_line, SHA_LENGTH);		// enter SHA256SUM into database
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
	if (database_first_line)
		{
		database_type = sha_verify (fileline);
		if (database_type == UNKNOWN_TYPE)
			{
			fclose (DATABASE_FP);
			exit_error ("Unrecognised file type: ", database_filename);
			}
		database_first_line = SW_OFF;
		}
	if (fileline != NULL && database_ferr)
		{
		if (database_type == SHA256_TYPE)					// load standard SHA256SUM output
			{
			strncpy (database_db->sha, fileline, SHA_LENGTH);
			database_db->sha[SHA_LENGTH] = NULL_TERM;
			strcpy (database_db->filepath, fileline + SHA_LENGTH + 2);
			database_db->filepath[strlen (database_db->filepath) - 1] = NULL_TERM;
			strcpy (database_db->dataset, database_filename);
			}
			else													// load S2DB data
			{
			separate_fields (database_db->sha, database_db->filepath, database_db->dataset, fileline);
			}
		if (!strcmp (check_file->sha, database_db->sha))	// SHA256SUMs match
			{
			match_found = TRUE;
			}
		}

	} while (!feof (DATABASE_FP) && match_found == FALSE);
if (match_found)	// SHA256SUMs match
	{
	printf ("File: %s matches %s in %s\n", check_file->filepath, database_db->filepath, database_db->dataset);
	}
else
	{
	printf ("No match.\n");
	}



fclose (DATABASE_FP);

}

