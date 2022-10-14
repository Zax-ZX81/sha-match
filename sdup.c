/* * * * * * * * * * * * * * * * *
 *                               *
 *        SHA-Dup 0.1            *
 *                               *
 *        2022-10-14             *
 *                               *
 *        Zax                    *
 *                               *
 * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMLib.h"

#define PROG_VERSION "0.1"
#define SHA_ZERO "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
#define ALL_OUT 'A'
#define ONLY_FIRST 'F'
#define ALL_BUT_FIRST 'f'
#define ONLY_LAST 'L'
#define ALL_BUT_LAST 'l'
#define WITH_COLOUR 'C'
#define WITH_HASH 'H'
#define NO_MARK 'N'
#define NULL_STRING ""

struct sdup_database
	{
	char sha [SHA_LENGTH + 1];
	char filepath [FILEPATH_LENGTH];
	char dataset [DATASET_LENGTH];
	int dup_num;
	};

int main (int argc, char *argv [])

{

FILE *DATABASE_FP;

int arg_no, switch_pos;		// args section
int database_ferr;		// database file error

char switch_chr;		// args section
char database_filename [FILEPATH_LENGTH] = "";
char database_type;
char database_first_line = SW_ON;
char fileline [FILELINE_LENGTH];			// input line
char sha_line [FILELINE_LENGTH];			// sha input line
char match_found = FALSE;				// if match is found
char dataset_out = TRUE;
char output_choice = ALL_OUT;
char mark_first = WITH_COLOUR;
char zero_sha = FALSE;

struct sha_database database_db [1] = {0};		// fields for database
struct sha_database previous_line [1] = {0};		// fields for search file
struct sdup_database *sdup_db;				// sha database for duplicates

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
					dataset_out = FALSE;
					break;
				case 'f':
					output_choice = ONLY_FIRST;
					mark_first = NO_MARK;
					break;
				case 'F':
					output_choice = ALL_BUT_FIRST;
					mark_first = NO_MARK;
					break;
				case 'l':
					output_choice = ONLY_LAST;
					mark_first = NO_MARK;
					break;
				case 'L':
					output_choice = ALL_BUT_LAST;
					mark_first = NO_MARK;
					break;
				case 'm':
					mark_first = WITH_HASH;
					break;
				case 'z':
					zero_sha = TRUE;
					break;
				case 'V':
					printf ("SHA Dup version %s\n", PROG_VERSION);
					exit (0);
				default:
					exit_error ("# SHA Dup [dfFlLmVz] <database file>","");
				}	// END switch
			}	// END for switch_pos
		}	// END if int argv
		else
		{
		strncpy (database_filename, argv [arg_no], FILEPATH_LENGTH);
		}	// END else if int argv
	}	// END for arg_no

// File open section
DATABASE_FP = fopen (database_filename, "r");
if (DATABASE_FP == NULL)
	{
	exit_error ("Can't find Database: ", database_filename);
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
		if (!strcmp (previous_line->sha, database_db->sha))	// SHA256SUMs match
			{
			if (!match_found)	// first duplicate
				{
				if (mark_first == WITH_COLOUR)
					{
					printf ("%s%s%s", TEXT_YELLOW, previous_line->filepath, TEXT_RESET);
					}
				if (mark_first == WITH_HASH)
					{
					printf ("#%s", previous_line->filepath);
					}
				if (dataset_out)
					{
					printf ("\t%s\n%s\t%s\n", previous_line->dataset, database_db->filepath, database_db->dataset);
					}
					else
					{
					printf ("\n%s\n", database_db->filepath);
					}
				}
				else		// subsequent duplicates
				{
				if (dataset_out)
					{
					printf ("%s\t%s\n", database_db->filepath, database_db->dataset);
					}
					else
					{
					printf ("%s\n", database_db->filepath);
					}
				}
			match_found = TRUE;
			}
			else		// no match, so update previous
			{
			strcpy (previous_line->sha, database_db->sha);
			strcpy (previous_line->filepath, database_db->filepath);
			strcpy (previous_line->dataset, database_db->dataset);
			match_found = FALSE;
			}
		}

	} while (!feof (DATABASE_FP));

fclose (DATABASE_FP);

}
/* * * * * * * * * * * * * * * * *

// Search list load section
searchlist_db = (struct sha_database *) malloc (sizeof (struct sha_database) * searchlist_alloc_size);
do
	{
	searchlist_ferr = (long)fgets (fileline, FILEPATH_LENGTH, SEARCHLIST_FP);
	if (searchlist_lines == 0)
		{
		smflags->searchlist_type = sha_verify (fileline);
		if (smflags->searchlist_type == UNKNOWN_TYPE)
			{
			exit_error ("Unrecognised file type: ", searchlist_filename);
			}
		}
	if (fileline != NULL && searchlist_ferr)
		{
		if (smflags->searchlist_type == SHA256_TYPE)		// load standard SHA256SUM output
			{
			strncpy (searchlist_db [searchlist_lines].sha, fileline, SHA_LENGTH);
			searchlist_db [searchlist_lines].sha [SHA_LENGTH] = NULL_TERM;
			strcpy (searchlist_db [searchlist_lines].filepath, fileline + SHA_LENGTH + 2);
			searchlist_db [searchlist_lines].filepath[strlen (searchlist_db [searchlist_lines].filepath) - 1] = NULL_TERM;
			strcpy (searchlist_db [searchlist_lines].dataset, "");	// added just in case
			}
			else		// load SHA256DB data
			{
			separate_fields (searchlist_db [searchlist_lines].sha, searchlist_db [searchlist_lines].filepath, searchlist_db [searchlist_lines].dataset, fileline);
			}
		}
	if (searchlist_lines + 1 == searchlist_alloc_size)		// check memory usage, reallocate
		{
		searchlist_alloc_size += DATABASE_INCREMENT;
		searchlist_db = (struct sha_database *) realloc (searchlist_db, sizeof (struct sha_database) * searchlist_alloc_size);
		}
	searchlist_lines ++;
	} while (!feof (SEARCHLIST_FP));
fclose (SEARCHLIST_FP);
 * * * * * * * * * * * * * * * * */

