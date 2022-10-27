/* * * * * * * * * * * * * * * * *
 *                               *
 *        SHA-Dup 0.30           *
 *                               *
 *        2022-10-22             *
 *                               *
 *        Zax                    *
 *                               *
 * * * * * * * * * * * * * * * * */

/* * * * * * * * * * * * * * * * *
 *            TODO               *
 *                               *
 * Add datestamps as dup choice  *
 * Need secondary search         *
 *                               *
 * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMLib.h"

#define PROG_VERSION "0.30"

int main (int argc, char *argv [])

{

FILE *SMDB_IN_FP;

int arg_no, switch_pos;		// args section
int database_ferr;		// database file error
int database_line = 0;
int last_line;
int dup_count = 0;
int database_alloc_size = DATABASE_INITIAL_SIZE;

char switch_chr;				// args section
char database_filename [FILEPATH_LENGTH] = "";
char database_type;
char fileline [FILELINE_LENGTH];		// input line
char dataset_out = TRUE;
char output_choice = ALL_DUPES;			// choice for which dups to print
char choice_scheme = NOT_SET;			// choice of time or alphbetical position
char mark_first = WITH_COLOUR;
char zero_sha = FALSE;				// don't handle zero size files by default
char current_zero_sha = FALSE;
struct sdup_database *sdup_db;			// sha database for duplicates

// Arguments section
for (arg_no = 1; arg_no < argc; arg_no++)	// loop through arguments
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
				case KEEP_NEWEST:			// default first choice if more than one specified
					output_choice = KEEP_NEWEST;
					choice_scheme = BY_TIME;
					mark_first = NO_MARK;
					break;
				case KEEP_FIRST:			// default second choice
					if (output_choice != KEEP_NEWEST)
						{
						output_choice = KEEP_FIRST;
						choice_scheme = BY_ALPHA;
						mark_first = NO_MARK;
						}
					break;
				case DROP_FIRST:
					if (choice_scheme == NOT_SET)
						{
						output_choice = DROP_FIRST;
						choice_scheme = BY_ALPHA;
						mark_first = NO_MARK;
						}
					break;
				case KEEP_LAST:
					if (choice_scheme == NOT_SET)
						{
						output_choice = KEEP_LAST;
						choice_scheme = BY_ALPHA;
						mark_first = NO_MARK;
						}
					break;
				case DROP_LAST:
					if (choice_scheme == NOT_SET)
						{
						output_choice = DROP_LAST;
						choice_scheme = BY_ALPHA;
						mark_first = NO_MARK;
						}
					break;
				case DROP_NEWEST:
					if (choice_scheme == NOT_SET)
						{
						output_choice = DROP_NEWEST;
						choice_scheme = BY_TIME;
						mark_first = NO_MARK;
						}
					break;
				case KEEP_OLDEST:
					if (choice_scheme == NOT_SET)
						{
						output_choice = KEEP_OLDEST;
						choice_scheme = BY_TIME;
						mark_first = NO_MARK;
						}
					break;
				case DROP_OLDEST:
					if (choice_scheme == NOT_SET)
						{
						output_choice = DROP_OLDEST;
						choice_scheme = BY_TIME;
						mark_first = NO_MARK;
						}
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
					printf ("%s# SHA Dup [dmuVz] {keep choice} <database file>\n", TEXT_YELLOW);
					printf ("# [fFlLnNoO] use one only to mark which of a group to %sKEEP%s\n", TEXT_RED, TEXT_RESET);
					printf ("%s# -f keeps first file in group\t\t\t# -n keeps newest file in group\n", TEXT_YELLOW);
					printf ("# -F keeps ALL BUT first file in group\t\t# -N keeps ALL BUT newest file in group\n");
					printf ("# -l keeps last file in group\t\t\t# -o keeps oldest file in group\n");
					printf ("# -L keeps ALL BUT last file in group\t\t# -O keeps ALL BUT oldest file in group%s\n", TEXT_RESET);
					exit (0);
				}	// END switch
			}	// END for switch_pos
		}	// END if int argv
		else
		{
		strncpy (database_filename, argv [arg_no], FILEPATH_LENGTH);
		}	// END else if int argv
	}	// END for arg_no

// File open section
SMDB_IN_FP = fopen (database_filename, "r");
if (SMDB_IN_FP == NULL)
	{
	exit_error ("Can't find Database: ", database_filename);
	}

// sdup_db [database_line].timestamp
// Database load section
// Load section should load data only, not mark duplicates
// If time choice get timestamps
sdup_db = (struct sdup_database *) malloc (sizeof (struct sdup_database) * database_alloc_size);
do
	{
	database_ferr = (long)fgets (fileline, FILEPATH_LENGTH, SMDB_IN_FP);
	if (database_line == 0)
		{
		database_type = sha_verify (fileline);
		if (database_type == UNKNOWN_TYPE)
			{
			fclose (SMDB_IN_FP);
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
	} while (!feof (SMDB_IN_FP));
fclose (SMDB_IN_FP);

// Sort section
// Sort with index by sha then alpha for fFlL
// Sort with index by sha then time for nNoO
// Mark duplicates during/after search
// Use re-referenced index in search

// Search section
// Divide search section into time and alpha
last_line = database_line;
for (database_line = 0; database_line <= last_line; database_line++)
	{
	current_zero_sha = strcmp (sdup_db [database_line].sha, SHA_ZERO);
	if (!(!zero_sha && !current_zero_sha))
		{					// zero test
		if (sdup_db [database_line].dup_num)	// Dup number not 0
			{
			if (sdup_db [database_line].dup_num == 1 && (output_choice == ALL_DUPES || output_choice == DROP_FIRST || output_choice == DROP_LAST))	// first duplicate
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
				}	// end if dup = 1
			if (sdup_db [database_line].dup_num > 1 && (output_choice == ALL_DUPES || output_choice == KEEP_FIRST\
					 || output_choice == DROP_LAST || output_choice == KEEP_LAST))	// other duplicates
				{
				switch (output_choice)
					{
					case DROP_LAST:
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
					case KEEP_LAST:
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
			}	// end else no dup
		}	// end zero test
	}	// end for loop

free (sdup_db);	// free memory
sdup_db = NULL;
}
