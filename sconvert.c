/* * * * * * * * * * * * * * * * *
 *                               *
 *     SHA-Convert 0.40          *
 *                               *
 *     2022-10-22                *
 *                               *
 *     Zax                       *
 *                               *
 * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMLib.h"

#define PROG_VERSION "0.40"

int main (int argc, char *argv [])

{

FILE *SSDB_IN_FP, *SSDB_OUT_FP;

int searchlist_ferr;
int line_count = 0;

char searchlist_filename [FILEPATH_LENGTH] = "";
char database_filename [FILEPATH_LENGTH] = "";
char fileline [FILELINE_LENGTH] = "";

struct sha_sort_database ssort_db [1] = {0};

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
SSDB_IN_FP = fopen (searchlist_filename,"r");
if (SSDB_IN_FP == NULL)
	{
	exit_error ("Can't find Search List: ", searchlist_filename);
	}

// Output open section
strncpy (ssort_db->dataset, searchlist_filename, strlen (searchlist_filename) - strlen (SHA256_EXTENSION));
strcat (database_filename, ssort_db->dataset);
strcat (database_filename, S2DB_EXTENSION);			// compose output filename
SSDB_OUT_FP = fopen (database_filename,"w");
if (SSDB_OUT_FP == NULL)
	{
	exit_error ("Can't open Database: ", database_filename);
	}

// Main section
printf ("SHA Convert version %s\nConverting %s%s%s to %s%s%s", PROG_VERSION, TEXT_YELLOW, searchlist_filename, TEXT_RESET, TEXT_BLUE, database_filename, TEXT_RESET);
do
	{
	searchlist_ferr = (long)fgets (fileline, FILEPATH_LENGTH, SSDB_IN_FP);
	if (fileline != NULL && searchlist_ferr)
		{
		if (sha_verify (fileline))		// check first 64 bytes for hex characters
			{
			strncpy (ssort_db->sha, fileline, SHA_LENGTH);				// load SHA256SUM into database struct
			ssort_db->sha [SHA_LENGTH] = NULL_TERM;					// add NULL terminator
			strcpy (ssort_db->filepath, fileline + SHA_OFFSET);			// load filepath into database struct
			ssort_db->filepath [strlen (ssort_db->filepath) - 1] = NULL_TERM;	// add NULL terminator
			fprintf (SSDB_OUT_FP, "%s\n", three_fields (ssort_db->sha, ssort_db->filepath, ssort_db->dataset));
			}		// write database struct contents to output file
			else
			{
			fclose (SSDB_IN_FP);
			fclose (SSDB_OUT_FP);
			remove (database_filename);
			exit_error ("   ...unrecognised file type: ", searchlist_filename);
			}
		line_count ++;
		}
	} while (!feof (SSDB_IN_FP));
printf ("   ...%d lines complete.\n", line_count);

// File close section
fclose (SSDB_IN_FP);
fclose (SSDB_OUT_FP);

}






