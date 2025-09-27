/* * * * * * * * * * * * * * * *
 *                             *
 *        smatch 0.41          *
 *                             *
 *        2022-10-22           *
 *                             *
 *        Zax                  *
 *                             *
 * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMLib.h"

#define PROG_VERSION "0.41"

int main (int argc, char *argv [])

{
const char hex_string [16] = "0123456789abcdef";

FILE *SSDB_IN_FP, *SMDB_IN_FP;

int arg_no, switch_pos;			// args section
int searchlist_ferr;			// search list file error
int search_index;			// search list line counter
int searchlist_lines = 0;		// number of lines in search list
int searchlist_alloc_size = DATABASE_INITIAL_SIZE;
int line_index;
int database_ferr;			// database file error
int swap_index;
int hex_idx;
int hex_char;
int hex_lookup_offset;
int chr_idx;

char switch_chr;					// args section
char searchlist_filename [FILEPATH_LENGTH] = "";
char dataset_name [FILEPATH_LENGTH] = "";
char database_filename [FILEPATH_LENGTH] = "";
char fileline [FILELINE_LENGTH];			// input line
char database_sha_prev [SHA_LENGTH + 1];		// SHA256SUM of previous database line
char output_line [FILEPATH_LENGTH] = "";		// output line
char swap_made = TRUE;
char sort_need_check = TRUE;

struct sha_sort_database *ssort_db;
struct sha_database sha_db [1] = {0};
struct shamatch_flags smflags [1] = {0};
struct hex_lookup_line *hex_lookup;

smflags->first_line = SW_ON;
hex_lookup = (struct hex_lookup_line *) malloc (sizeof (struct hex_lookup_line) * 16);

for (hex_idx = 0; hex_idx < 16; hex_idx ++)		// initialise hex lookup table
	{
	hex_lookup [hex_idx].idx = hex_string [hex_idx];
	hex_lookup [hex_idx].first = 0;
	hex_lookup [hex_idx].last = 0;
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
				case 'v':
					smflags->verbose = SW_ON;
					break;
				case 'V':
					printf ("SHA Match version %s\n", PROG_VERSION);
					exit (0);
				case 'z':
					smflags->zero = SW_ON;
					break;
				default:
					printf ("%s# SHA Match [dimvVz] <search file> <database file>\n", TEXT_YELLOW);
					printf ("# -i invert output\n%s", TEXT_RESET);
					exit (0);
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

// File open section
SSDB_IN_FP = fopen (searchlist_filename,"r");
SMDB_IN_FP = fopen (database_filename,"r");

if (SSDB_IN_FP == NULL)
	{
	exit_error ("Can't find Search List: ", searchlist_filename);
	}
if (SMDB_IN_FP == NULL)
	{
	exit_error ("Can't find Database: ", database_filename);
	}

chr_idx = strlen (searchlist_filename);				// derive dataset name from database filename
while (searchlist_filename [chr_idx] != '.' && -- chr_idx);
if (chr_idx)
	{
	dataset_name [chr_idx--] = NULL_TERM;
	do
		{
		dataset_name [chr_idx] = searchlist_filename [chr_idx];
		}
		while (chr_idx--);
	dataset_name [chr_idx] = searchlist_filename [chr_idx];
	}

// Search list load section
ssort_db = (struct sha_sort_database *) malloc (sizeof (struct sha_sort_database) * searchlist_alloc_size);
do
	{
	searchlist_ferr = (long)fgets (fileline, FILEPATH_LENGTH, SSDB_IN_FP);
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
			strncpy (ssort_db [searchlist_lines].sha, fileline, SHA_LENGTH);
			ssort_db [searchlist_lines].sha [SHA_LENGTH] = NULL_TERM;
			strcpy (ssort_db [searchlist_lines].filepath, fileline + SHA_LENGTH + 2);
			ssort_db [searchlist_lines].filepath[strlen (ssort_db [searchlist_lines].filepath) - 1] = NULL_TERM;
			strcpy (ssort_db [searchlist_lines].dataset, dataset_name);
			}
			else		// load SHA256DB data
			{
			separate_fields (ssort_db [searchlist_lines].sha, ssort_db [searchlist_lines].filepath, ssort_db [searchlist_lines].dataset, fileline);
			}
		ssort_db [searchlist_lines].index = searchlist_lines;
		if (smflags->verbose)
			{
			fprintf (stderr, "SL\tSs=%s\tSf=%s\tSd=%s\tSi=%d\n", ssort_db [searchlist_lines].sha, \
							ssort_db [searchlist_lines].filepath, \
							ssort_db [searchlist_lines].dataset, \
							ssort_db [searchlist_lines].index);
			}
		}
	if (searchlist_lines + 1 == searchlist_alloc_size)		// check memory usage, reallocate
		{
		searchlist_alloc_size += DATABASE_INCREMENT;
		ssort_db = (struct sha_sort_database *) realloc (ssort_db, sizeof (struct sha_sort_database) * searchlist_alloc_size);
		}
	searchlist_lines ++;
	} while (!feof (SSDB_IN_FP));
fclose (SSDB_IN_FP);
searchlist_lines --;

// Sort section
while (swap_made == TRUE)
	{
	swap_made = FALSE;
	for (line_index = 0; line_index < searchlist_lines - 1; line_index ++)
		{
		if (strcmp (ssort_db [ssort_db [line_index].index].sha, ssort_db [ssort_db [line_index + 1].index].sha) > 0)
			{
			swap_index = ssort_db [line_index + 1].index;
			ssort_db [line_index + 1].index = ssort_db [line_index].index;
			ssort_db [line_index].index = swap_index;
			swap_made = TRUE;
			}
		}
	if (sort_need_check && swap_made)
		{
		if (searchlist_lines > SORT_MAX_LINES)           // abandon sort if file too big
			{
			fclose (SMDB_IN_FP);
			free (ssort_db);           // free memory
			ssort_db = NULL;
			exit_error ("Not sorting, file has too many lines: ", searchlist_filename);
			}
		printf ("# %sSorting...%s\n", TEXT_YELLOW, TEXT_RESET);
		}
	sort_need_check = FALSE;
	}

// Search list index section
hex_idx = 0;
for (line_index = 0; line_index < searchlist_lines; line_index ++)	// build search list index - first pass
	{
	hex_char = ssort_db [ssort_db [line_index].index].sha [0];	// get first hex character of SHA256SUM
	while (hex_lookup [hex_idx].idx < hex_char)		// skip missing hex chars
		{
		hex_idx ++;
		}
	if (hex_char == hex_lookup [hex_idx].idx && hex_lookup [hex_idx].first == 0)
		{
		hex_lookup [hex_idx].first = line_index + 1;	// set first line in set
		hex_idx ++;
		}
	}
for (hex_idx = 0; hex_idx < 16; hex_idx ++)		// build search list index - second pass
	{
	if (hex_lookup [hex_idx].first != 0)
		{
		hex_lookup_offset = 1;
		while (hex_lookup [hex_idx - hex_lookup_offset].first == 0)	// skip missing hex chars
			{
			hex_lookup_offset ++;
			}
		hex_lookup [hex_idx - hex_lookup_offset].last = hex_lookup [hex_idx].first - 1; // set last line in set
		}
	}
hex_lookup [hex_idx - 1].last = line_index;		// set last line


// Search section
do
	{
	database_ferr = (long)fgets (fileline, FILEPATH_LENGTH, SMDB_IN_FP);
	if (smflags->first_line)
		{
		smflags->database_type = sha_verify (fileline);
		if (smflags->database_type == UNKNOWN_TYPE)
			{
			fclose (SMDB_IN_FP);
			exit_error ("Unrecognised file type: ", database_filename);
			}
		smflags->first_line = SW_OFF;
		}
	if (fileline != NULL && database_ferr)
		{
		if (smflags->database_type == SHA256_TYPE)			// load standard SHA256SUM output
			{
			strncpy (sha_db->sha, fileline, SHA_LENGTH);
			sha_db->sha[SHA_LENGTH] = NULL_TERM;
			strcpy (sha_db->filepath, fileline + SHA_LENGTH + 2);
			sha_db->filepath[strlen (sha_db->filepath) - 1] = NULL_TERM;
			strcpy (sha_db->dataset, "");				// added just in case
			}
			else													// load SHA256DB data
			{
			separate_fields (sha_db->sha, sha_db->filepath, sha_db->dataset, fileline);
			}
		hex_char = hex_to_dec (sha_db->sha [0]);			// get hex character from database target
		for (search_index = hex_lookup [hex_char].first - 1; search_index <= hex_lookup [hex_char].last; search_index ++)
			{										// loop through only one hex bracket
			if (!strcmp (ssort_db [ssort_db [search_index].index].sha, sha_db->sha))	// SHA256SUMs match
				{
				smflags->shamatch_found = TRUE;
				if (!strcmp (ssort_db [ssort_db [search_index].index].dataset, sha_db->dataset))
					{
					smflags->dataset_match = TRUE;						// handle data set conflicts
					smflags->dataset_conflict = TRUE;
					}
				if (!strcmp (sha_db->sha, database_sha_prev))	// test for multi
					{
					smflags->multi_found = TRUE;
					if (smflags->multi)								// check multi switch
						{
						if (smflags->d_out)							// check database switch
							{
							strcpy (output_line, sha_db->filepath);	// multi database output
							}
							else
							{
							strcpy (output_line, ssort_db [ssort_db [search_index].index].filepath);	// multi search list output
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
						strcpy (output_line, sha_db->filepath);	// multi database output
						}
						else
						{
						strcpy (output_line, ssort_db [ssort_db [search_index].index].filepath);	// multi search list output
						}
					}	// END multi test else
				}	// END found match
			if (smflags->verbose)
				{
				fprintf (stderr, "SS\tSs=%s\tSf=%s\tSd=%s\tSi=%d\tDs=%s\tDf=%s\tDd=%s\tSM=%d\tDC=%d\n", ssort_db [ssort_db [search_index].index].sha, \
								ssort_db [ssort_db [search_index].index].filepath, \
								ssort_db [ssort_db [search_index].index].dataset, \
								ssort_db [search_index].index, \
								sha_db->sha, \
								sha_db->filepath, \
								sha_db->dataset, \
								smflags->shamatch_found, \
								smflags->dataset_match);
				}
			}	// END for search index
		if (!smflags->shamatch_found && smflags->invert)
			{
			strcpy (output_line, three_fields (sha_db->sha, sha_db->filepath, sha_db->dataset));
			}
		}
	if (!strcmp (sha_db->sha, SHA_ZERO) && !smflags->zero)		// supress output for zero size files if switch not set
		{
		smflags->shamatch_found = FALSE;
		}
	if (((smflags->shamatch_found && !smflags->dataset_match && !smflags->invert) || \
			(!smflags->shamatch_found && smflags->invert && !smflags->multi_found)) && strcmp (output_line, NULL_STRING))
		{
		printf ("%s\n", output_line);	// print output line
		}
	smflags->shamatch_found = FALSE;
	smflags->dataset_match = FALSE;
	smflags->multi_found = FALSE;
	output_line [0] = 0;
	strcpy (database_sha_prev, sha_db->sha);
	} while (!feof (SMDB_IN_FP));

if (smflags->dataset_conflict)
	{
	printf ("%s# Some results excluded because of dataset conflict%s\n", TEXT_ORANGE, TEXT_RESET);
	}

fclose (SMDB_IN_FP);

free (ssort_db);		// free memory
ssort_db = NULL;
}
