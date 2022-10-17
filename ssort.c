/* * * * * * * * * * * * * * * * *
 *                               *
 *        SHA-Sort 0.01          *
 *                               *
 *        2020-12-01             *
 *                               *
 *        Zax                    *
 *                               *
 * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMLib.h"

int main (int argc, char *argv [])

{
const char hex_string [16] = "0123456789abcdef";

FILE *DATABASE_FP;

int arg_no, switch_pos;		// args section
int search_index;			// search list line counter
int database_line = 0;	// number of lines in search list
int database_alloc_size = DATABASE_INITIAL_SIZE;
int line_index = 0;
int database_ferr;			// database file error
int outer_loop = 0, inner_loop;		// counters for sort loops

char switch_chr;			// args section
char database_filename [FILEPATH_LENGTH] = "";
char fileline [FILELINE_LENGTH];				// input line
char database_sha_prev [SHA_LENGTH + 1];		// SHA256SUM of previous database line
char output_line [FILEPATH_LENGTH] = "";		// output line
char swap_made = TRUE;
char database_type;
char first_line = SW_ON;

struct sha_database *database_db, swap_db;

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
				default:
					exit_error ("# SHA Sort <database file>","");
				}	// END switch
			}	// END for switch_pos
		}	// END if int argv
		else
		{
		if (strcmp (database_filename, "") == 0)
			{
			strncpy (database_filename, argv [arg_no], FILEPATH_LENGTH);
			}
		}	// END else if int argv
	}	// END for arg_no

DATABASE_FP = fopen (database_filename,"r");
if (DATABASE_FP == NULL)
	{
	exit_error ("Can't find Database: ", database_filename);
	}

// Database load section
database_db = (struct sha_database *) malloc (sizeof (struct sha_database) * database_alloc_size);
do
	{
	database_ferr = (long)fgets (fileline, FILEPATH_LENGTH, DATABASE_FP);
	if (database_line == 0)
		{
		database_type = sha_verify (fileline);
		if (database_type == UNKNOWN_TYPE)
			{
			exit_error ("Unrecognised file type: ", database_filename);
			}
		}
	if (fileline != NULL && database_ferr)
		{
		if (database_type == SHA256_TYPE)		// load standard SHA256SUM output
			{
			strncpy (database_db [database_line].sha, fileline, SHA_LENGTH);
			database_db [database_line].sha [SHA_LENGTH] = NULL_TERM;
			strcpy (database_db [database_line].filepath, fileline + SHA_LENGTH + 2);
			database_db [database_line].filepath[strlen (database_db [database_line].filepath) - 1] = NULL_TERM;
			strcpy (database_db [database_line].dataset, "");	// added just in case
			}
			else		// load SHA256DB data
			{
			separate_fields (database_db [database_line].sha, database_db [database_line].filepath, database_db [database_line].dataset, fileline);
			}
		}
	if (database_line + 1 == database_alloc_size)		// check memory usage, reallocate
		{
		database_alloc_size += DATABASE_INCREMENT;
		database_db = (struct sha_database *) realloc (database_db, sizeof (struct sha_database) * database_alloc_size);
		}
	database_line ++;
	} while (!feof (DATABASE_FP));
fclose (DATABASE_FP);

// Sort section
outer_loop = 1;
while (swap_made == TRUE)
	{
	swap_made = FALSE;
	for (inner_loop = 0; inner_loop < database_line - 2; inner_loop ++)
		{
		if (strcmp (database_db [inner_loop].sha, database_db [inner_loop + 1].sha) > 0)
			{
			swap_db = database_db [inner_loop + 1];
			database_db [inner_loop + 1] = database_db [inner_loop];
			database_db [inner_loop] = swap_db;
			swap_made = TRUE;
			}
		}
	if (outer_loop == 2 && swap_made)
		{
//		if (database_line > SORT_MAX_LINES)           // abandon sort if file too big
//			{
//			fclose (DATABASE_FP);
//			free (database_db);           // free memory
//			database_db = NULL;
//			exit_error ("Not sorting, file has too many lines: ", database_filename);
//			}
		printf ("# %sSorting...%s\n", TEXT_YELLOW, TEXT_RESET);
		}
	outer_loop ++;
if ((float) outer_loop / 10000) printf (".");
//printf ("%f\t",(float)outer_loop / 1000);
	}
printf ("");
for (line_index = 0; line_index < database_line; line_index ++)	// print output
	{
	printf("%s\t%s\t%s\n", database_db [line_index].sha, database_db [line_index].filepath, database_db [line_index].dataset);
	}

free (database_db);		// free memory
database_db = NULL;
}
