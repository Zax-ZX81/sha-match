/* * * * * * * * * * * * * * * * *
 *                               *
 *        SHA-Match 0.33         *
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

#define PROG_VERSION "0.33"
#define SHA_ZERO "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

int main (int argc, char *argv [])

{
const char hex_string [16] = "0123456789abcdef";

FILE *SEARCHLIST_FP, *DATABASE_FP;

int arg_no, switch_pos;		// args section
int searchlist_ferr;		// search list file error
int search_index;			// search list line counter
int searchlist_lines = 0;	// number of lines in search list
int searchlist_alloc_size = DATABASE_INITIAL_SIZE;
int line_index = 0;
int database_ferr;			// database file error
int outer_loop = 0, inner_loop;		// counters for sort loops

char switch_chr;			// args section
char searchlist_filename [FILEPATH_LENGTH] = "";
char database_filename [FILEPATH_LENGTH] = "";
char fileline [FILELINE_LENGTH];				// input line
char database_sha_prev [SHA_LENGTH + 1];		// SHA256SUM of previous database line
char output_line [FILEPATH_LENGTH] = "";		// output line
char hex_idx;
char hex_char;
char hex_lookup_offset;
char swap_made = TRUE;

struct sha_database *searchlist_db, swap_db;
struct sha_database database_db [1] = {0};
struct shamatch_flags smflags [1] = {0};
struct hex_lookup_line *hex_lookup;

smflags->first_line = SW_ON;
hex_lookup = (struct hex_lookup_line *) malloc (sizeof (struct hex_lookup_line) * 16);

for (hex_idx = 0; hex_idx < 16; hex_idx ++)		// initialise hex lookup table
	{
	hex_lookup [(int) hex_idx].idx = hex_string [(int) hex_idx];
	hex_lookup [(int) hex_idx].first = 0;
	hex_lookup [(int) hex_idx].last = 0;
	}

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
					smflags->d_out = SW_ON;
					break;
				case 'm':
					smflags->multi = SW_ON;
					break;
				case 'i':
					smflags->invert = SW_ON;
					break;
				case 'V':
					printf ("SHA Match version %s\n", PROG_VERSION);
					exit (0);
				case 'z':
					smflags->zero = SW_ON;
					break;
				default:
					exit_error ("# SHA Match [dimV] <search file> <database file>","");
				}	// END switch
			}	// END for switch_pos
		}	// END if int argv
		else
		{
		if (strcmp (searchlist_filename, "") == 0)
			{
			strncpy (searchlist_filename, argv [arg_no], FILEPATH_LENGTH);
			}
			else
			{
			if (strcmp (database_filename, "") == 0)
				{
				strncpy (database_filename, argv [arg_no], FILEPATH_LENGTH);
				}
			}	// END if strcmp searchlist
		}	// END else if int argv
	}	// END for arg_no
if (smflags->invert)	// clear switches that conflict with inverse output
	{
	smflags->d_out = SW_OFF;
	smflags->multi = SW_OFF;
	}

SEARCHLIST_FP = fopen (searchlist_filename,"r");
DATABASE_FP = fopen (database_filename,"r");

if (SEARCHLIST_FP == NULL)
	{
	exit_error ("Can't find Search List: ", searchlist_filename);
	}
if (DATABASE_FP == NULL)
	{
	exit_error ("Can't find Database: ", database_filename);
	}

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

// Sort section
outer_loop = 1;
while (swap_made == TRUE)
	{
	swap_made = FALSE;
	for (inner_loop = 0; inner_loop < searchlist_lines - 2; inner_loop ++)
		{
		if (strcmp (searchlist_db [inner_loop].sha, searchlist_db [inner_loop + 1].sha) > 0)
			{
			swap_db = searchlist_db [inner_loop + 1];
			searchlist_db [inner_loop + 1] = searchlist_db [inner_loop];
			searchlist_db [inner_loop] = swap_db;
			swap_made = TRUE;
			}
		}
	if (outer_loop == 2 && swap_made)
		{
		if (searchlist_lines > SORT_MAX_LINES)           // abandon sort if file too big
			{
			fclose (DATABASE_FP);
			free (searchlist_db);           // free memory
			searchlist_db = NULL;
			exit_error ("Not sorting, file has too many lines: ", searchlist_filename);
			}
		printf ("# %sSorting...%s\n", TEXT_YELLOW, TEXT_RESET);
		}
	outer_loop ++;
	}

// Search list index section
hex_idx = 0;
for (line_index = 0; line_index < searchlist_lines; line_index ++)	// build search list index - first pass
	{
	hex_char = searchlist_db [line_index].sha [0];	// get first hex character of SHA256SUM
	while (hex_lookup [(int) hex_idx].idx < hex_char)	// skip missing hex chars
		{
		hex_idx ++;
		}
	if (hex_char == hex_lookup [(int) hex_idx].idx && hex_lookup [(int) hex_idx].first == 0)
		{
		hex_lookup [(int) hex_idx].first = line_index + 1;	// set first line in set
		hex_idx ++;
		}
	}
for (hex_idx = 0; hex_idx < 16; hex_idx ++)		// build search list index - second pass
	{
	if (hex_lookup [(int) hex_idx].first != 0)
		{
		hex_lookup_offset = 1;
		while (hex_lookup [hex_idx - hex_lookup_offset].first == 0)	// skip missing hex chars
			{
			hex_lookup_offset ++;
			}
		hex_lookup [hex_idx - hex_lookup_offset].last = hex_lookup [(int) hex_idx].first - 1; // set last line in set
		}
	}
hex_lookup [hex_idx - 1].last = line_index;	// set last line

// Search section
do
	{
	database_ferr = (long)fgets (fileline, FILEPATH_LENGTH, DATABASE_FP);
	if (smflags->first_line)
		{
		smflags->database_type = sha_verify (fileline);
		if (smflags->database_type == UNKNOWN_TYPE)
			{
			fclose (DATABASE_FP);
			exit_error ("Unrecognised file type: ", database_filename);
			}
		smflags->first_line = SW_OFF;
		}
	if (fileline != NULL && database_ferr)
		{
		if (smflags->database_type == SHA256_TYPE)					// load standard SHA256SUM output
			{
			strncpy (database_db->sha, fileline, SHA_LENGTH);
			database_db->sha[SHA_LENGTH] = NULL_TERM;
			strcpy (database_db->filepath, fileline + SHA_LENGTH + 2);
			database_db->filepath[strlen (database_db->filepath) - 1] = NULL_TERM;
			strcpy (database_db->dataset, "");						// added just in case
			}
			else													// load SHA256DB data
			{
			separate_fields (database_db->sha, database_db->filepath, database_db->dataset, fileline);
			}
		hex_char = hex_to_dec (database_db->sha [0]);				// get hex character from database target
		for (search_index = hex_lookup [(int) hex_char].first - 1; search_index <= hex_lookup [(int) hex_char].last; search_index ++)
			{														// loop through only one hex bracket
			if (!strcmp (searchlist_db [search_index].sha, database_db->sha))	// SHA256SUMs match
				{
				smflags->shamatch_found = TRUE;
				if (!strcmp (searchlist_db [search_index].dataset, database_db->dataset) && smflags->searchlist_type == S2DB_TYPE)
					{
					smflags->dataset_match = 1;						// handle data set conflicts
					smflags->dataset_conflict = 1;
					}
				if (!strcmp (database_db->sha, database_sha_prev))	// test for multi
					{
					if (smflags->multi)								// check multi switch
						{
						if (smflags->d_out)							// check database switch
							{
							strcpy (output_line, database_db->filepath);	// multi database output
							}
							else
							{
							strcpy (output_line, searchlist_db [search_index].filepath);	// multi search list output
							}
						}	// END multi switch
						else
						{
						smflags->shamatch_found = FALSE;
						}
					}	// END multi test
					else
					{
					if (smflags->d_out)	// check database output switch
						{
						strcpy (output_line, database_db->filepath);	// multi database output
						}
						else
						{
						strcpy (output_line, searchlist_db [search_index].filepath);	// multi search list output
						}
					}	// END multi test else
				}	// END found match
			}	// END for search index
		if (!smflags->shamatch_found && smflags->invert)
			{
			strcpy (output_line, three_fields (database_db->sha, database_db->filepath, database_db->dataset));
			}
		}
	if (!strcmp (database_db->sha, SHA_ZERO) && !smflags->zero)		// supress output for zero size files if switch not set
		{
		smflags->shamatch_found = FALSE;
		}
	if ((smflags->shamatch_found && !smflags->dataset_match && !smflags->invert) || (!smflags->shamatch_found && smflags->invert))
		{
		printf ("%s\n", output_line);	// print output line
		}
	smflags->shamatch_found = FALSE;
	output_line [0] = 0;
	strcpy (database_sha_prev, database_db->sha);
	} while (!feof (DATABASE_FP));

if (smflags->dataset_conflict)
	{
	printf ("%s# Some results excluded because of dataset conflict%s\n", TEXT_ORANGE, TEXT_RESET);
	}

fclose (DATABASE_FP);

free (searchlist_db);		// free memory
searchlist_db = NULL;
}
