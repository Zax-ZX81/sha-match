/* * * * * * * * * * * * * * * * *
 *                               *
 *     SHA-Find 0.44             *
 *                               *
 *     2020-11-15                *
 *                               *
 *     Zax                       *
 *                               *
 * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include "SMLib.h"

#define PROG_VERSION "0.44"
#define SHA_CMD "sha256sum "
#define TWO_SPACES "  "
#define TEMP_FILE ".sfind_temp1"
#define FILTER_FILE "sf_filter"
#define FILE_ENTRY 'f'
#define DIR_ENTRY 'd'
#define UNKNOWN_ENTRY 'x'
#define PATH_CURRENT "./"
#define SLASH_TERM "/"
#define DIR_CURRENT "."
#define DIR_PREV ".."
#define CGE_RET 13
#define SORT_NONE 0
#define SORT_SHA 1
#define SORT_FILE 2
#define NUM_OF_TEMP_FILES 2
#define F_INCL 1
#define F_EXCL 2

#if __linux__
#define DIR_TYPE 4
#define FILE_TYPE 8
#endif

#if _WIN32
#define DIR_TYPE 16
#define FILE_TYPE 0
#endif

struct file_list_entry
	{
	char filepath [FILEPATH_LENGTH];
	};

int main (int argc, char *argv [])

{

struct sha_database *database_db, swap_db;
struct shafind_flags sfflags [1] = {0};
struct dirent *dir_ents;
struct file_list_entry *filter_list;
sfflags->database_type = S2DB_TYPE;		// default file type S2DB
sfflags->sort = SORT_SHA;				// sort by SHA256SUM by default

FILE *SHA_PIPE;
FILE *FILTER_FP;					// inclusion and exclusion filter list
FILE *FIND_OUT_FP, *FIND_IN_FP;		// find temp file format: first char of line is type, rest is filepath
FILE *DATABASE_FP;
DIR *DIR_PATH;

int line_index;
int filter_line_count = 0;
int filter_index;
int database_index = 0;
int file_type_count = 0;
int object_count = 0;		// number of file items found in search
int find_in_ferr;
int filter_ferr;
int arg_no, switch_pos;
int outer_loop, inner_loop;
int progress_count = 0;
int progress_interval;
int progress_percent = 0;

char database_dataset [DATASET_LENGTH] = "";		// holds dataset name
char database_filename [FILEPATH_LENGTH] = "";		// output file name with extension
char find_line [FILELINE_LENGTH] = "";				// holds line from temp file
char fileline [FILELINE_LENGTH] = "";				// holds line from filter file
char sha_line [FILELINE_LENGTH] = "";				// hold line from SHA256SUM output
char switch_chr;
char database_extension [8] = "";					// holds database extension based on flag
char sha_command [FILEPATH_LENGTH] = "";			// SHA256SUM command line with file argument
char object_type;									// holds directory entry type: file, dir, other
char path_sub [FILEPATH_LENGTH];					// holds SHA256SUM file argument
char find_sub_target [FILEPATH_LENGTH];				// next subdirectory to search
char C_W_D [256];									// base directory of search
char swap_made = TRUE;								// swap was made on last sort pass
char filter_match = FALSE;

// Argument section
for (arg_no = 1; arg_no < argc; arg_no++)		// loop through arguments
	{
	if (argv [arg_no] [0] == '-')
		{
		for (switch_pos = 1; switch_pos < strlen (argv[arg_no]); switch_pos++)		// loop through switch characters
			{
			switch_chr = (int) argv [arg_no] [switch_pos];
			switch (switch_chr)
				{
				case 'f':
					sfflags->sort = SORT_FILE;
					break;
				case 'i':
					sfflags->filtering = F_INCL;
					break;
				case 'o':
					sfflags->database_type = SHA256_TYPE;
					break;
				case 'p':
					sfflags->progress = SW_ON;
					break;
				case 's':
					sfflags->std_out = SW_ON;				// if std_out only, supress file opening/writing
					break;
				case 'u':
					sfflags->sort = SORT_NONE;
					break;
				case 'v':
					sfflags->verbose = SW_ON;
					break;
				case 'V':
					printf ("SHA Find version %s\n", PROG_VERSION);
					exit (0);
				case 'x':
					sfflags->filtering = F_EXCL;
					break;
				default:
					exit_error ("# SHA find [-fnosuv] <search file> <database file>\n","");
					break;
				}	// END switch
			}	// END for switch_pos
		}	// END for arg_no
		else
		{
		strncpy (database_dataset, argv [arg_no], DATASET_LENGTH);
		} 	// END else if int argv
	}
if (argc < 2 || !strlen (database_dataset))		// no file argument, so stdout only
	{
	sfflags->std_out = TRUE;
	}
if (sfflags->verbose)
	{
	sfflags->progress = SW_OFF;	// progress indicator will interfer with verbose output
	}

// Output open section
if (sfflags->database_type == S2DB_TYPE)
	{
	strcpy (database_extension, S2DB_EXTENSION);
	}
	else
	{
	strcpy (database_extension, SHA256_EXTENSION);
	}
if (sfflags->filtering > 0)
	{
	FILTER_FP = fopen (FILTER_FILE, "r");
	if (FILTER_FP == NULL)
		{
		exit_error ("Can't find filter file: ", FILTER_FILE);
		}
	}
if (sfflags->filtering > 0)
	{
	do	// count lines in filter for memory allocation
		{
		filter_ferr = (long)fgets (fileline, FILEPATH_LENGTH, FILTER_FP);
		if (fileline != NULL && filter_ferr)
			{
			filter_line_count ++;
			}
		} while (!feof (FILTER_FP));
	rewind (FILTER_FP);
	printf ("# %sApplying filter with %d lines...%s\n", TEXT_YELLOW, filter_line_count,TEXT_RESET);
	filter_list = (struct file_list_entry *) malloc (sizeof (struct file_list_entry) * filter_line_count);
	for (filter_index = 0; filter_index < filter_line_count; filter_index ++)	// data entry for searchlist
		{
		fgets (fileline, FILEPATH_LENGTH, FILTER_FP);
		strcpy (filter_list [filter_index].filepath, fileline);
		filter_list [filter_index].filepath[strlen (filter_list [filter_index].filepath) - 1] = NULL_TERM;
		}
	}

FIND_OUT_FP = fopen (TEMP_FILE, "w");
if (FIND_OUT_FP == NULL)
	{
	exit_error ("Can't open temp file for writing", "");
	}

// Initial search section
printf ("# Building file list...");
strcpy (database_filename, database_dataset);		// compose output filename
strcat (database_filename, database_extension);
getcwd (C_W_D, 256);		// get present working directory
strcat (C_W_D, SLASH_TERM);
DIR_PATH = opendir (PATH_CURRENT);		// open directory
if (DIR_PATH != NULL)
	{
	while ((dir_ents = readdir (DIR_PATH)))		// get directory listing
		{
		sfflags->output_current = TRUE;			// set so that if not filtering all results are output
		filter_match = FALSE;
		if (sfflags->filtering > 0)				// are we applying filtering?
			{
			sfflags->output_current = FALSE;
			for (filter_index = 0; filter_index < filter_line_count; filter_index ++)		// cycle through filter list
				{
				if (!strcmp (filter_list [filter_index].filepath, dir_ents->d_name))		// match found
					{
					filter_match = TRUE;
					}
				}
			if (sfflags->filtering == F_INCL && filter_match)
				{
				sfflags->output_current = TRUE;
				}
			if (sfflags->filtering == F_EXCL && !filter_match)
				{
				sfflags->output_current = TRUE;
				}
			}
		if (sfflags->output_current)
			{
			object_count ++;
			switch (dir_ents->d_type)
				{
				case FILE_TYPE:					// set type to file
					object_type = FILE_ENTRY;
					file_type_count ++;			// increment counter for database memory allocation
					break;
				case DIR_TYPE:					// set type to directory
					object_type = DIR_ENTRY;
					break;
				default:						// mark as unneeded type
					object_type = UNKNOWN_ENTRY;
					break;
				}								// filter out ".", ".." and temp file from search vvv
			if (strcmp (dir_ents->d_name, DIR_CURRENT) && strcmp (dir_ents->d_name, DIR_PREV) \
				&& strcmp (dir_ents->d_name, database_filename) && strcmp (dir_ents->d_name, TEMP_FILE))
				{
				fprintf (FIND_OUT_FP, "%c./%s\n", object_type, dir_ents->d_name);		// append entry to temp file
				if (sfflags->verbose)
					{
					printf ("%d  %s/%s\n", object_count, find_sub_target, dir_ents->d_name);
					}
				}
			}
		}
	(void) closedir (DIR_PATH);
	}
	else
	{
	perror ("Couldn't open the directory");		// FIX
	}

// Feedback search section
fflush (FIND_OUT_FP);		// ensure temp file is up to date when read occurs
FIND_IN_FP = fopen (TEMP_FILE, "r");
if (FIND_IN_FP == NULL)
	{
	exit_error ("Can't open temp file for reading", "");
	}
while (!feof (FIND_IN_FP))
	{
	chdir (C_W_D);		// go back to the starting directory
	find_in_ferr = (long)fgets (find_line, FILEPATH_LENGTH, FIND_IN_FP);		// read temp file line
	if (find_line != NULL && find_in_ferr)
		{
		object_count ++;
		object_type = find_line [0];		// find entry type: 'd', 'f' or 'x'
		if (object_type == DIR_ENTRY)		// if object is directory, prepare to enter it
			{
			strcpy (find_sub_target, find_line + 1);		// copy remainder of line to search target
			find_sub_target [strlen (find_sub_target) - 1] = NULL_TERM;
			strcpy (path_sub, C_W_D);
			strcat (path_sub, find_sub_target);		// compose directory location for search
			chdir (path_sub);						// move to search directory
			DIR_PATH = opendir (path_sub);			// open directory
			if (DIR_PATH != NULL)
				{
				while ((dir_ents = readdir (DIR_PATH)))		// get directory listing
					{
					switch (dir_ents->d_type)
						{
						case FILE_TYPE:					// set type to file
							object_type = FILE_ENTRY;
							file_type_count ++;			// increment counter for database memory allocation
							break;
						case DIR_TYPE:					// set type to directory
							object_type = DIR_ENTRY;
							break;
						default:						// mark as unneeded type
							object_type = UNKNOWN_ENTRY;
							break;
						}								// filter out "." and ".." from search vvv
					if (strcmp (dir_ents->d_name, DIR_CURRENT) && strcmp (dir_ents->d_name, DIR_PREV))
						{
						fprintf (FIND_OUT_FP, "%c%s/%s\n", object_type, find_sub_target, dir_ents->d_name);
						if (sfflags->verbose)								// append entry to temp file ^^^
							{
							printf ("%d  %s/%s\n", object_count, find_sub_target, dir_ents->d_name);
							}
						}
					}
				}
			closedir (DIR_PATH);
			fflush (FIND_OUT_FP);		// ensure temp file is up to date when read occurs
			}
//			else
//			{
//			perror ("Couldn't open the directory");		// FIX
//			}
			
		
		}
	}
printf (" %d files found\n", file_type_count);
fclose (FIND_OUT_FP);		// finished collecting temp file entries
rewind (FIND_IN_FP);		// return to start of temp file for reprocessing
chdir (C_W_D);				// go back to the starting directory

// Database filepath load section
file_type_count = file_type_count - NUM_OF_TEMP_FILES;
database_db = (struct sha_database *) malloc (sizeof (struct sha_database) * file_type_count);		// allocate memory for database
while (!feof (FIND_IN_FP))
	{
	find_in_ferr = (long)fgets (find_line, FILEPATH_LENGTH, FIND_IN_FP);		// read temp file line
	if (find_line != NULL && find_in_ferr)
		{
		object_type = find_line [0];		// find entry type: 'd', 'f' or 'x'
		if (object_type == FILE_ENTRY)		// if object is file, do SHA256SUM
			{
			find_line [strlen (find_line) - 1] = NULL_TERM;
			strcpy (database_db [database_index].filepath, find_line + 1);
			database_index ++;
			}
		}
	}

// SHA256SUM generation section
if (sfflags->std_out == SW_OFF)
	{
	printf ("# Generating SHA256SUMS for %s%s%s\n", TEXT_BLUE, database_filename, TEXT_RESET);
	}
progress_interval = file_type_count / 101;
for (line_index = 0; line_index < file_type_count; line_index ++)	// 
	{
	if (sfflags->progress)
		{
		if (progress_count == progress_interval)
			{
			fprintf (stderr, "%c%d%%", CGE_RET, progress_percent ++);
			progress_count = 0;
			}
		progress_count ++;
		}
	strcpy (sha_command, SHA_CMD);
	strcat (sha_command, enquote(database_db [line_index].filepath));
	SHA_PIPE = popen (sha_command, "r");			// send SHA256SUM command and arguments
	fgets(sha_line, FILELINE_LENGTH, SHA_PIPE);		// receive reply
	if (sha_verify (sha_line))		// verify SHA256SUM
		{
		strncpy (database_db [line_index].sha, sha_line, SHA_LENGTH);		// enter SHA256SUM into database
		fclose (SHA_PIPE);
		database_db [line_index].sha [SHA_LENGTH] = NULL_TERM;
		strcpy (database_db [line_index].dataset, database_dataset);
		if (sfflags->verbose)
			{
			printf ("%s", sha_line);
			}
		}
		else
		{
		exit_error ("Invalid SHA256SUM from file ", database_db [line_index].filepath);
		}
	}
if (sfflags->progress)
	{
	fprintf (stderr, "%c", CGE_RET);
	}

// Sort section
outer_loop = 0;
while (outer_loop < file_type_count && swap_made == TRUE && sfflags->sort > 0)
	{
	swap_made = FALSE;
	for (inner_loop = 0; inner_loop < file_type_count - 1; inner_loop ++)
		{
		if (sfflags->sort == SORT_SHA)
			{
			if (strcmp (database_db [inner_loop].sha, database_db [inner_loop + 1].sha) > 0)
				{
				swap_db = database_db [inner_loop + 1];
				database_db [inner_loop + 1] = database_db [inner_loop];
				database_db [inner_loop] = swap_db;
				swap_made = TRUE;
				}
			}
		if (sfflags->sort == SORT_FILE)
			{
				if (strcmp (database_db [inner_loop].filepath, database_db [inner_loop + 1].filepath) > 0)
				{
				swap_db = database_db [inner_loop + 1];
				database_db [inner_loop + 1] = database_db [inner_loop];
				database_db [inner_loop] = swap_db;
				swap_made = TRUE;
				}
			}
		}
	if (swap_made && outer_loop == 1)
		{
		printf ("# %sSorting...%s\n", TEXT_YELLOW, TEXT_RESET);
		}
	outer_loop ++;
	}

// Output section
if (sfflags->std_out == SW_OFF)
	{
	DATABASE_FP = fopen (database_filename, "w");		// open output database
	if (DATABASE_FP == NULL)
		{
		exit_error ("Can't open database for output: ", database_filename);
		}
	}
for (line_index = 0; line_index < file_type_count; line_index ++)	// data entry for searchlist - first pass
	{
	if (sfflags->database_type == S2DB_TYPE)		// for S2DB output
		{
		if (sfflags->std_out)
			{
			printf("%s\t%s\t%s\n", database_db [line_index].sha, database_db [line_index].filepath, database_db [line_index].dataset);
			}
			else
			{
			fprintf(DATABASE_FP, "%s\t%s\t%s\n", database_db [line_index].sha, database_db [line_index].filepath, database_db [line_index].dataset);
			}
		}
		else		// for plain SHA256SUM output
		{
		if (sfflags->std_out)
			{
			printf("%s%s%s\n", database_db [line_index].sha, TWO_SPACES, database_db [line_index].filepath);
			}
			else
			{
			fprintf(DATABASE_FP, "%s%s%s\n", database_db [line_index].sha, TWO_SPACES, database_db [line_index].filepath);
			}
		}
	}
if (!sfflags->std_out)
	{
	printf ("%d lines written to %s%s%s\n", file_type_count, TEXT_BLUE, database_filename, TEXT_RESET);
	}
// Clean-up section
fclose (FIND_IN_FP);
chdir (C_W_D);
remove (TEMP_FILE);		// delete temp file
}