/* * * * * * * * * * * * * * * * *
 *                               *
 *     SHA-Convert 0.30          *
 *                               *
 *     2020-11-04                *
 *                               *
 *     Zax                       *
 *                               *
 * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMLib.h"

#define PROG_VERSION "0.30"


int main (int argc, char *argv [])

{

FILE *SEARCHLIST_FP, *DATABASE_FP;

int searchlist_ferr;
int line_count = 0;

char searchlist_filename [FILEPATH_LENGTH] = "";
char database_filename [FILEPATH_LENGTH] = "";
char fileline [FILELINE_LENGTH] = "";

struct sha_database database_db [1] = {0};

// Argument section
if (argc > FILE_ARG)
	{
	exit_error ("Too many arguments.  SHA convert <SHA_FILE.sha256>","");
	}
if (argc < FILE_ARG)
	{
	exit_error ("One argument needed.  SHA convert <SHA_FILE.sha256>","");
	}
strncpy (searchlist_filename, argv [FILE_ARG - 1], FILEPATH_LENGTH);
if (strlen (searchlist_filename) < strlen (SHA256_EXTENSION) || strchr (searchlist_filename, '.') == NULL)
	{
	exit_error ("Invalid file name: ", searchlist_filename);
	}
if (strcmp (strchr (searchlist_filename, '.'), SHA256_EXTENSION))
	{
	exit_error (searchlist_filename, " has wrong file extension.  <SHA_FILE.sha256>");
	}

// Input open section
SEARCHLIST_FP = fopen (searchlist_filename,"r");
if (SEARCHLIST_FP == NULL)
	{
	exit_error ("Can't find Search List: ", searchlist_filename);
	}

// Output open section
strncpy (database_db->dataset, searchlist_filename, strlen (searchlist_filename) - strlen (SHA256_EXTENSION));
strcat (database_filename, database_db->dataset);
strcat (database_filename, S2DB_EXTENSION);			// compose output filename
DATABASE_FP = fopen (database_filename,"w");
if (DATABASE_FP == NULL)
	{
	exit_error ("Can't open Database: ", database_filename);
	}

// Main section
printf ("SHA Convert version %s\nConverting %s%s%s to %s%s%s", PROG_VERSION, TEXT_YELLOW, searchlist_filename, TEXT_RESET, TEXT_BLUE, database_filename, TEXT_RESET);
do
	{
	searchlist_ferr = (long)fgets (fileline, FILEPATH_LENGTH, SEARCHLIST_FP);
	if (fileline != NULL && searchlist_ferr)
		{
		if (sha_verify (fileline))		// check first 64 bytes for hex characters
			{
			strncpy (database_db->sha, fileline, SHA_LENGTH);			// load SHA256SUM into database struct
			database_db->sha [SHA_LENGTH] = NULL_TERM;					// add NULL terminator
			strcpy (database_db->filepath, fileline + SHA_OFFSET);		// load filepath into database struct
			database_db->filepath [strlen (database_db->filepath) - 1] = NULL_TERM;		// add NULL terminator
			fprintf (DATABASE_FP, "%s\n", three_fields (database_db->sha, database_db->filepath, database_db->dataset));
			}		// write database struct contents to output file
			else
			{
			fclose (SEARCHLIST_FP);
			fclose (DATABASE_FP);
			remove (database_filename);
			exit_error ("   ...unrecognised file type: ", searchlist_filename);
			}
		line_count ++;
		}
	} while (!feof (SEARCHLIST_FP));
printf ("   ...%d lines complete.\n", line_count);

// File close section
fclose (SEARCHLIST_FP);
fclose (DATABASE_FP);

}






