/* * * * * * * * * * * * * * * * *
 *                               *
 *        SHA-Dup 0.22           *
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

#define PROG_VERSION "0.22"
#define SHA_ZERO "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
#define ALL_DUPES 'A'
#define ONLY_FIRST 'F'
#define NOT_FIRST 'f'
#define ONLY_LAST 'L'
#define NOT_LAST 'l'
#define ALL_UNIQUE 'u'
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
int database_line = 0;
int last_line;
int dup_count = 0;
int database_alloc_size = DATABASE_INITIAL_SIZE;

char switch_chr;		// args section
char database_filename [FILEPATH_LENGTH] = "";
char database_type;
char database_first_line = SW_ON;
char fileline [FILELINE_LENGTH];			// input line
char sha_line [FILELINE_LENGTH];			// sha input line
char match_found = FALSE;				// if match is found
char dataset_out = TRUE;
char output_choice = ALL_DUPES;
char mark_first = WITH_COLOUR;
char zero_sha = FALSE;
char current_zero_sha = FALSE;
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
				case NOT_FIRST:
					output_choice = NOT_FIRST;
					mark_first = NO_MARK;
					break;
				case ONLY_FIRST:
					output_choice = ONLY_FIRST;
					mark_first = NO_MARK;
					break;
				case ONLY_LAST:
					output_choice = ONLY_LAST;
					mark_first = NO_MARK;
					break;
				case NOT_LAST:
					output_choice = NOT_LAST;
					mark_first = NO_MARK;
					break;
				case 'm':
					mark_first = WITH_HASH;
					break;
				case 'u':
					output_choice = ALL_UNIQUE;
					zero_sha = TRUE;
					mark_first = NO_MARK;
					break;
				case 'V':
					printf ("SHA Dup version %s\n", PROG_VERSION);
					exit (0);
				case 'z':
					zero_sha = TRUE;
					break;
				default:
					exit_error ("# SHA Dup [dfFlLmuVz] <database file>","");
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

// Database load section
sdup_db = (struct sdup_database *) malloc (sizeof (struct sdup_database) * database_alloc_size);
do
	{
	database_ferr = (long)fgets (fileline, FILEPATH_LENGTH, DATABASE_FP);
	if (database_line == 0)
		{
		database_type = sha_verify (fileline);
		if (database_type == UNKNOWN_TYPE)
			{
			fclose (DATABASE_FP);
			exit_error ("Unrecognised file type: ", database_filename);
			}
		}
	if (fileline != NULL && database_ferr)
		{
		if (database_type == SHA256_TYPE)		// load standard SHA256SUM output
			{
			strncpy (sdup_db [database_line].sha, fileline, SHA_LENGTH);
			sdup_db [database_line].sha [SHA_LENGTH] = NULL_TERM;
			strcpy (sdup_db [database_line].filepath, fileline + SHA_LENGTH + 2);
			sdup_db [database_line].filepath[strlen (sdup_db [database_line].filepath) - 1] = NULL_TERM;
			strcpy (sdup_db [database_line].dataset, database_filename);	// enter database filename as dataset
			}
			else		// load SHA256DB data
			{
			separate_fields (sdup_db [database_line].sha, sdup_db [database_line].filepath, sdup_db [database_line].dataset, fileline);
			}
		}
		if (database_line > 0 && !strcmp (sdup_db [database_line - 1].sha, sdup_db [database_line].sha))	// not first line and SHA256SUMs match
			{
			if (dup_count == 0)
				{
				sdup_db [database_line - 1].dup_num = 1;
				sdup_db [database_line].dup_num = 2;
				dup_count = 2;
				}
				else
				{
				sdup_db [database_line].dup_num = ++dup_count;
				}
			}
			else
			{
			dup_count = 0;
			}

	if (database_line + 1 == database_alloc_size)		// check memory usage, reallocate
		{
		database_alloc_size += DATABASE_INCREMENT;
		sdup_db = (struct sdup_database *) realloc (sdup_db, sizeof (struct sdup_database) * database_alloc_size);
		}
	database_line ++;
	} while (!feof (DATABASE_FP));
fclose (DATABASE_FP);

last_line = database_line;
for (database_line = 0; database_line <= last_line; database_line++)
	{
	current_zero_sha = strcmp (sdup_db [database_line].sha, SHA_ZERO);
	if (!(!zero_sha && !current_zero_sha))
		{	// zero test
		if (sdup_db [database_line].dup_num)	// Dup number not 0
			{
			if (sdup_db [database_line].dup_num == 1 && (output_choice == ALL_DUPES || output_choice == ONLY_FIRST || output_choice == NOT_LAST))	// first duplicate
				{
				switch (mark_first)
					{
					case WITH_COLOUR:
						printf ("%s%s%s", TEXT_YELLOW, sdup_db [database_line].filepath, TEXT_RESET);
						break;
					case WITH_HASH:
						printf ("#%s", sdup_db [database_line].filepath);
						break;
					default:
						printf ("%s", sdup_db [database_line].filepath);
					}	// end switch
				if (dataset_out)
					{
					printf ("\t%s", sdup_db [database_line].dataset);
					}
				printf ("\n");
				}	// if dup = 1
			if (sdup_db [database_line].dup_num > 1 && (output_choice == ALL_DUPES || output_choice == NOT_FIRST\
					 || output_choice == NOT_LAST || output_choice == ONLY_LAST))	// other duplicates
				{
				switch (output_choice)
					{
					case NOT_LAST:
						if (sdup_db [database_line + 1].dup_num)
							{
							printf ("%s", sdup_db [database_line].filepath);
							if (dataset_out)
								{
								printf ("\t%s", sdup_db [database_line].dataset);
								}
							printf ("\n");
							}
						break;
					case ONLY_LAST:
						if (!sdup_db [database_line + 1].dup_num)
							{
							printf ("%s", sdup_db [database_line].filepath);
							if (dataset_out)
								{
								printf ("\t%s", sdup_db [database_line].dataset);
								}
						printf ("\n");
							}
						break;
					default:
						printf ("%s", sdup_db [database_line].filepath);
						if (dataset_out)
							{
							printf ("\t%s", sdup_db [database_line].dataset);
							}
						printf ("\n");
					}	// end switch
//				printf ("%s\n", sdup_db [database_line].filepath);
				}
			if ((sdup_db [database_line].dup_num == 1 && output_choice == ALL_UNIQUE) || !current_zero_sha)	// output first duplicate if all unique
				{
				printf ("%s\t%s\t%s\n", sdup_db [database_line].sha, sdup_db [database_line].filepath, sdup_db [database_line].dataset);
				}
			}	// end dup found
			else
			{	// no dup found
			if (output_choice == ALL_UNIQUE)
				{
				printf ("%s\t%s\t%s\n", sdup_db [database_line].sha, sdup_db [database_line].filepath, sdup_db [database_line].dataset);
				}
			}	// end no dup
		}	// end zero test
	} // end for loop

}

