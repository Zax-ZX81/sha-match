/* * * * * * * * * * * * * * * * *
 *                               *
 *        SHA-Sort 0.02          *
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
#include <time.h>

struct ssort_database
	{
	char sha [SHA_LENGTH + 1];
	char filepath [FILEPATH_LENGTH];
	char dataset [DATASET_LENGTH];
	int index;
	};

int main (int argc, char *argv [])

{

FILE *DATABASE_FP;

int arg_no, switch_pos;		// args section
int database_line = 0;	// number of lines in search list
int database_alloc_size = DATABASE_INITIAL_SIZE;
int line_index = 0;
int database_ferr;			// database file error
int outer_loop = 0, inner_loop;		// counters for sort loops
int swap_index;

char switch_chr;			// args section
char database_filename [FILEPATH_LENGTH] = "";
char fileline [FILELINE_LENGTH];				// input line
char swap_made = TRUE;
char database_type;
char print_result = FALSE;

time_t start_time;

struct ssort_database *ssort_db;

// Arguments section

//printf ("Here 1\n");
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
					print_result = TRUE;
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
ssort_db = (struct ssort_database *) malloc (sizeof (struct ssort_database) * database_alloc_size);
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
			strncpy (ssort_db [database_line].sha, fileline, SHA_LENGTH);
			ssort_db [database_line].sha [SHA_LENGTH] = NULL_TERM;
			strcpy (ssort_db [database_line].filepath, fileline + SHA_LENGTH + 2);
			ssort_db [database_line].filepath[strlen (ssort_db [database_line].filepath) - 1] = NULL_TERM;
			strcpy (ssort_db [database_line].dataset, "");
			}
			else		// load SHA256DB data
			{
			separate_fields (ssort_db [database_line].sha, ssort_db [database_line].filepath, ssort_db [database_line].dataset, fileline);
			}
		ssort_db [database_line].index = database_line;	// load sort index in start position
		}
	if (database_line + 1 == database_alloc_size)		// check memory usage, reallocate
		{
		database_alloc_size += DATABASE_INCREMENT;
		ssort_db = (struct ssort_database *) realloc (ssort_db, sizeof (struct ssort_database) * database_alloc_size);
		}
	database_line ++;
	} while (!feof (DATABASE_FP));
fclose (DATABASE_FP);

// Sort section
start_time = time (NULL);
outer_loop = 1;
while (swap_made == TRUE)
	{
	swap_made = FALSE;
	for (inner_loop = 0; inner_loop < database_line - 1; inner_loop ++)
		{
//		if (strcmp (ssort_db [ssort_db [inner_loop].index].sha, ssort_db [ssort_db [inner_loop + 1].index].sha) > 0)
		if (strcmp (ssort_db [ssort_db [inner_loop].index].filepath, ssort_db [ssort_db [inner_loop + 1].index].filepath) > 0)
			{
			swap_index = ssort_db [inner_loop + 1].index;
			ssort_db [inner_loop + 1].index = ssort_db [inner_loop].index;
			ssort_db [inner_loop].index = swap_index;
			swap_made = TRUE;
			}
		}
	if (outer_loop == 2 && swap_made)
		{
		printf ("# %sSorting...%s\n", TEXT_YELLOW, TEXT_RESET);
		}
	outer_loop ++;
	}
printf ("%d seconds.\n", time (NULL) - start_time);
if (print_result)
	{
	for (line_index = 0; line_index < database_line; line_index ++)	// print output
		{
		printf("%s\t%s\t%s\n", \
					ssort_db [ssort_db [line_index].index].sha, \
					ssort_db [ssort_db [line_index].index].filepath, \
					ssort_db [ssort_db [line_index].index].dataset);
		}
	}
free (ssort_db);		// free memory
ssort_db = NULL;
}
