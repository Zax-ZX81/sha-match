/* * * * * * * * * * * * * * *
 *                           *
 *        sdup 0.31          *
 *                           *
 *        2022-10-22         *
 *                           *
 *        Zax                *
 *                           *
 * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMLib.h"
#include <sys/types.h>
#include <sys/stat.h>

#define PROG_VERSION "0.31"

int main (int argc, char *argv [])

{

FILE *SMDB_IN_FP;

int arg_no, switch_pos;		// args section
int database_ferr;		// database file error
int database_line = 0;
int last_line;
int dup_count = 0;
int database_alloc_size = DATABASE_INITIAL_SIZE;
int line_index;
int swap_index;
int ts_count;			// the number of timestamps found

char switch_chr;				// args section
char database_filename [FILEPATH_LENGTH] = "";
char database_type;
char fileline [FILELINE_LENGTH];		// input line
char sha_zero_chk [SHA_LENGTH + 1];

struct sdup_database *sdup_db;			// sha database for duplicates
struct stat file_stat;
struct sdup_flags sdflags [1] = {0};

sdflags->dataset_out = TRUE;			// output dataset by default
sdflags->o_choice = ALL_DUPES;			// choice for which dups to print
sdflags->c_scheme = NOT_SET;			// choice of time or alphbetical position
sdflags->mark_first = WITH_COLOUR;		// mark with colour by default
sdflags->zero_sha = FALSE;			// don't handle zero size files by default
sdflags->current_zero_sha = FALSE;
sdflags->swap_made = FALSE;
sdflags->verbose = FALSE;

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
					sdflags->dataset_out = FALSE;
					break;
				case KEEP_FIRST:			// default first choice if more than one specified
					sdflags->o_choice = KEEP_FIRST;
					sdflags->c_scheme = BY_ALPHA;
					sdflags->mark_first = NO_MARK;
					break;
				case KEEP_NEWEST:			// default second choice
					if (sdflags->o_choice != KEEP_FIRST)
						{
						sdflags->o_choice = KEEP_FIRST;
						sdflags->c_scheme = BY_TIME;
						sdflags->mark_first = NO_MARK;
						}
					break;
				case DROP_FIRST:
					if (sdflags->c_scheme == NOT_SET)
						{
						sdflags->o_choice = DROP_FIRST;
						sdflags->c_scheme = BY_ALPHA;
						sdflags->mark_first = NO_MARK;
						}
					break;
				case KEEP_LAST:
					if (sdflags->c_scheme == NOT_SET)
						{
						sdflags->o_choice = KEEP_LAST;
						sdflags->c_scheme = BY_ALPHA;
						sdflags->mark_first = NO_MARK;
						}
					break;
				case DROP_LAST:
					if (sdflags->c_scheme == NOT_SET)
						{
						sdflags->o_choice = DROP_LAST;
						sdflags->c_scheme = BY_ALPHA;
						sdflags->mark_first = NO_MARK;
						}
					break;
				case DROP_NEWEST:
					if (sdflags->c_scheme == NOT_SET)
						{
						sdflags->o_choice = DROP_FIRST;
						sdflags->c_scheme = BY_TIME;
						sdflags->mark_first = NO_MARK;
						}
					break;
				case KEEP_OLDEST:
					if (sdflags->c_scheme == NOT_SET)
						{
						sdflags->o_choice = KEEP_LAST;
						sdflags->c_scheme = BY_TIME;
						sdflags->mark_first = NO_MARK;
						}
					break;
				case DROP_OLDEST:
					if (sdflags->c_scheme == NOT_SET)
						{
						sdflags->o_choice = DROP_LAST;
						sdflags->c_scheme = BY_TIME;
						sdflags->mark_first = NO_MARK;
						}
					break;
				case 'm':
					sdflags->mark_first = WITH_HASH;
					break;
				case 'u':
					sdflags->o_choice = ALL_UNIQUE;
					sdflags->zero_sha = TRUE;
					sdflags->mark_first = NO_MARK;
					break;
				case 'V':
					printf ("SHA Dup version %s\n", PROG_VERSION);
					exit (0);
				case 'v':
					sdflags->verbose = TRUE;
					break;
				case 'z':
					sdflags->zero_sha = TRUE;
					break;
				default:
					printf ("%s# SHA Dup [dmuvVz] {keep choice} <database file>\n", TEXT_YELLOW);
					printf ("# [fFlLnNoO] use one only to mark which of a group to %sKEEP%s\n", TEXT_RED, TEXT_RESET);
					printf ("%s#\t\t\t\t\t\t\t Time-based choices require\n", TEXT_YELLOW);
					printf ("#\t\t\t\t\t\t\t files to be present\n");
					printf ("# -f keeps first file in group\t\t\t# -n keeps newest file in group\n", TEXT_YELLOW);
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

// Database load section
if (sdflags->c_scheme == BY_TIME)
	{
	fprintf (stderr, "# Gathering timestamps...\n");
	}
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
		strncpy (sha_zero_chk, fileline, SHA_LENGTH);
		sha_zero_chk [SHA_LENGTH] = '\n';
		sha_zero_chk [SHA_LENGTH + 1] = NULL_TERM;
		if ((!sdflags->zero_sha && !strcmp (sha_zero_chk, SHA_ZERO)))		// zero test
			{
			sdflags->current_zero_sha = TRUE;
			}
		if ((!sdflags->zero_sha && !sdflags->current_zero_sha))		// zero test
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
			sdup_db [database_line].index = database_line;
			if (sdflags->c_scheme == BY_TIME)
				{
				if (stat (sdup_db [database_line].filepath, &file_stat) == 0)
					{
					if (file_stat.st_mode & S_IFREG)
						{
						sdup_db [database_line].timestamp = file_stat.st_mtime;	// get file time stamp
						ts_count ++;
						fprintf (stderr, "%c%d", CGE_RET, ts_count);
						}	// end if file_stat
					}	// end if stat
				}	// end if sdflags
			}	// end zero test
		}	// end if fileline
	if (database_line + 1 == database_alloc_size)		// check memory usage, reallocate
		{
		database_alloc_size += DATABASE_INCREMENT;
		sdup_db = (struct sdup_database *) realloc (sdup_db, sizeof (struct sdup_database) * database_alloc_size);
		}
	if (sdflags->verbose)
		{
		printf ("S=%s\tF=%s=\tD=%s\tT=%d\tDL=%d\tCS=%c\n", sdup_db [database_line].sha, sdup_db [database_line].filepath, \
								sdup_db [database_line].dataset, sdup_db [database_line].timestamp, \
								database_line, sdflags->c_scheme);
		}
	if (!sdflags->current_zero_sha)
		{
		database_line ++;
		}
	sdflags->current_zero_sha = FALSE;
	} while (!feof (SMDB_IN_FP));
fclose (SMDB_IN_FP);
if (sdflags->c_scheme == BY_TIME)
	{
	fprintf (stderr, "\n");
	}

// Database sort section
sdflags->swap_made = TRUE;
while (sdflags->swap_made == TRUE)
	{
	sdflags->swap_made = FALSE;
	for (line_index = 0; line_index < database_line - 2; line_index ++)
		{
/*		if (sdflags->verbose)
			{
			printf ("CS=%c\tCL=%s=\tCT=%d\tNL=%s=\tNT=%d\n", sdflags->c_scheme, \
							sdup_db [sdup_db [line_index].index].filepath, sdup_db [sdup_db [line_index].index].timestamp, \
							sdup_db [sdup_db [line_index + 1].index].filepath, sdup_db [sdup_db [line_index + 1].index].timestamp);
			}*/
		if (strcmp (sdup_db [sdup_db [line_index].index].sha, sdup_db [sdup_db [line_index + 1].index].sha) == 0)
			{
			if ((sdflags->c_scheme == BY_ALPHA && strcmp (sdup_db [sdup_db [line_index].index].filepath, sdup_db [sdup_db [line_index + 1].index].filepath) > 0) \
			|| (sdflags->c_scheme == BY_TIME && sdup_db [sdup_db [line_index].index].timestamp > sdup_db [sdup_db [line_index + 1].index].timestamp))
				{
				swap_index = sdup_db [line_index + 1].index;
				sdup_db [line_index + 1].index = sdup_db [line_index].index;
				sdup_db [line_index].index = swap_index;
				sdflags->swap_made = TRUE;
				}
			}
		}
	}

// Mark duplicates section
for (line_index = 1; line_index < database_line - 2; line_index ++)
	{
	if (sdflags->verbose)
		{
		printf ("S-1=%s\tS=%s\n", sdup_db [sdup_db [line_index - 1].index].sha, sdup_db [sdup_db [line_index].index].sha);
		}
	if (!strcmp (sdup_db [sdup_db [line_index - 1].index].sha, sdup_db [sdup_db [line_index].index].sha))	// SHA256SUMs match
		{
		if (dup_count == 0)
			{
			sdup_db [sdup_db [line_index - 1].index].dup_num = 1;
			sdup_db [sdup_db [line_index].index].dup_num = 2;
			dup_count = 2;
			}
			else
			{
			sdup_db [sdup_db [line_index].index].dup_num = ++dup_count;
			}
		if (sdflags->verbose)
			{
			printf ("%d\n", dup_count);
			}
		}
		else
		{
		if (sdflags->verbose)
			{
			printf ("\n");
			}
		dup_count = 0;
		}
	}

// Search section
last_line = database_line;
for (database_line = 0; database_line <= last_line; database_line++)
	{
	sdflags->current_zero_sha = strcmp (sdup_db [sdup_db [database_line].index].sha, SHA_ZERO);
	if (sdup_db [sdup_db [database_line].index].dup_num)	// Dup number not 0
		{
		if (sdup_db [sdup_db [database_line].index].dup_num == 1 && (sdflags->o_choice == ALL_DUPES || \
									sdflags->o_choice == DROP_FIRST || \
									sdflags->o_choice == DROP_LAST))	// first duplicate
			{
			switch (sdflags->mark_first)
				{
				case WITH_COLOUR:
					printf ("%s%s%s", TEXT_YELLOW, sdup_db [sdup_db [database_line].index].filepath, TEXT_RESET);
					break;
				case WITH_HASH:
					printf ("#%s", sdup_db [sdup_db [database_line].index].filepath);
					break;
				default:
					printf ("%s", sdup_db [sdup_db [database_line].index].filepath);
				}	// end switch
			if (sdflags->dataset_out)
				{
				printf ("\t%s", sdup_db [sdup_db [database_line].index].dataset);
				}
			printf ("\n");
			}	// end if dup = 1
		if (sdup_db [sdup_db [database_line].index].dup_num > 1 && (sdflags->o_choice == ALL_DUPES || sdflags->o_choice == KEEP_FIRST\
				 || sdflags->o_choice == DROP_LAST || sdflags->o_choice == KEEP_LAST))	// other duplicates
			{
			switch (sdflags->o_choice)
				{
				case DROP_LAST:
					if (sdup_db [sdup_db [database_line + 1].index].dup_num)
						{
						printf ("%s", sdup_db [sdup_db [database_line].index].filepath);
						if (sdflags->dataset_out)
							{
							printf ("\t%s", sdup_db [sdup_db [database_line].index].dataset);
							}
						printf ("\n");
						}
					break;
				case KEEP_LAST:
					if (!sdup_db [sdup_db [database_line + 1].index].dup_num)
						{
						printf ("%s", sdup_db [sdup_db [database_line].index].filepath);
						if (sdflags->dataset_out)
							{
							printf ("\t%s", sdup_db [sdup_db [database_line].index].dataset);
							}
					printf ("\n");
						}
					break;
				default:
					printf ("%s", sdup_db [sdup_db [database_line].index].filepath);
					if (sdflags->dataset_out)
						{
						printf ("\t%s", sdup_db [sdup_db [database_line].index].dataset);
						}
					printf ("\n");
				}	// end switch
			}
		if ((sdup_db [sdup_db [database_line].index].dup_num == 1 && sdflags->o_choice == ALL_UNIQUE) || !sdflags->current_zero_sha)	// output first duplicate if all unique
			{
			printf ("%s\t%s\t%s\n", sdup_db [sdup_db [database_line].index].sha, sdup_db [sdup_db [database_line].index].filepath, \
						sdup_db [sdup_db [database_line].index].dataset);
			}
		}	// end dup found
		else
		{	// no dup found
		if (sdflags->o_choice == ALL_UNIQUE)
			{
			printf ("%s\t%s\t%s\n", sdup_db [sdup_db [database_line].index].sha, sdup_db [sdup_db [database_line].index].filepath, \
						sdup_db [sdup_db [database_line].index].dataset);
			}
		}	// end else no dup
	}	// end for loop
if (sdflags->c_scheme == BY_TIME)
	{
	fprintf (stderr, "# %d of %d timestamps found\n", ts_count, database_line);
	}
free (sdup_db);	// free memory
sdup_db = NULL;
}
