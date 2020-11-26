/* * * * * * * * * * * * * * * * *
 *                               *
 *     SHA-Find 0.46             *
 *                               *
 *     2020-11-25                *
 *                               *
 *     Zax                       *
 *                               *
 * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "SMLib.h"

#define PROG_VERSION "0.46"
#define FILTER_PROG_NAME "NFind"
#define SHA_CMD "sha256sum "
#define TWO_SPACES "  "
#define FILTER_FILE "sf_filter"
#define FILTER_OUT "sf_filter-o"
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
#define F_INCL 1
#define F_EXCL 2
#define FILTER_INITIAL_SIZE 64
#define FILTER_INCREMENT 64
#define FILTER_CEILING 1024
#define FIND_LIST_INITIAL_SIZE 1024
#define FIND_LIST_INCREMENT 1024
#define FIND_LIST_CEILING 262144
#define T_DIR 'd'
#define T_FIL 'f'
#define T_REJ 'r'
#define T_HDR 'h'

#if __linux__
#define DIR_TYPE 4
#define FILE_TYPE 8
#endif

#if _WIN32
#define DIR_TYPE 16
#define FILE_TYPE 0
#endif

struct find_list_entry
	{
	char object_type;
	char filtered;
	char filepath [FILEPATH_LENGTH];
	int filesize;
	};
struct filter_entry
	{
	char object_type;
	char filepath [FILEPATH_LENGTH];
	};

char filter_line_check (char *filter_line);

int main (int argc, char *argv [])

{

struct sfind_database *database_db, swap_db;
struct sfind_flags sfflags [1] = {0};
struct dirent *dir_ents;
struct stat file_stat;
struct find_list_entry *find_list;
struct filter_entry *filter_list;
sfflags->database_type = S2DB_TYPE;		// default file type S2DB
sfflags->sort = SORT_SHA;				// sort by SHA256SUM by default
sfflags->filtering = FALSE;

FILE *SHA_PIPE;
FILE *FILTER_FP;					// inclusion and exclusion filter list
FILE *FILOUT_FP;
FILE *DATABASE_FP;
DIR *DIR_PATH;

int line_index;
int filter_line_count = 0;
int filter_index = 0;
int find_list_write = 0;		// number of file items found in search
int find_list_read = 0;
int database_index = 0;
int file_type_count = 0;
int filter_ferr;
int arg_no, switch_pos;
int outer_loop, inner_loop;
int progress_count = 0;
int progress_interval;
int progress_percent = 0;
int filter_curr_size = 0;
int find_list_curr_size = 0;

long file_size_total = 0;
long file_size_accum = 0;

double file_size_mult = 0;
double file_progress = 0;

char database_dataset [DATASET_LENGTH] = "";		// holds dataset name
char database_filename [FILEPATH_LENGTH] = "";		// output file name with extension
char fileline [FILELINE_LENGTH] = "";				// holds line from filter file
char sha_line [FILELINE_LENGTH] = "";				// hold line from SHA256SUM output
char switch_chr;
char database_extension [8] = "";					// holds database extension based on flag
char sha_command [FILEPATH_LENGTH] = "";			// SHA256SUM command line with file argument
char path_sub [FILEPATH_LENGTH];					// holds SHA256SUM file argument
char C_W_D [256];									// base directory of search
char swap_made = TRUE;								// swap was made on last sort pass
char filter_match = FALSE;
char filter_check;
char output_current;

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
	sfflags->progress = SW_OFF;	// progress indicator will interfere with verbose output
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
	filter_list = (struct filter_entry *) malloc (sizeof (struct filter_entry) * FILTER_INITIAL_SIZE);
	filter_curr_size = FILTER_INITIAL_SIZE;
	if (sfflags->std_out == SW_OFF)
		{
		printf ("# Loading filter...");
		}
	do
		{
		filter_ferr = (long)fgets (fileline, FILEPATH_LENGTH, FILTER_FP);
		if (fileline != NULL && filter_ferr)
			{
			strcpy (filter_list [filter_index].filepath, fileline);
			filter_list [filter_index].filepath[strlen (filter_list [filter_index].filepath) - 1] = NULL_TERM;
			if (filter_list [filter_index].filepath[strlen (filter_list [filter_index].filepath) - 1] == '/')
				{
				filter_list [filter_index].filepath[strlen (filter_list [filter_index].filepath) - 1] = NULL_TERM;
				}
			filter_check = filter_line_check (filter_list [filter_index].filepath);
			if (filter_check)
				{
				if (stat (filter_list [filter_index].filepath, &file_stat) == 0)
					{
					if (file_stat.st_mode & S_IFREG)
						{
						filter_list [filter_index].object_type = T_FIL;
						}
					if (file_stat.st_mode & S_IFDIR)
						{
						filter_list [filter_index].object_type = T_DIR;
						}
					}
					else
					{
					filter_list [filter_index].object_type = T_REJ;
					}
				if (filter_check == 2)
					{
					filter_list [filter_index].object_type = T_HDR;
					}
				}
				else
				{
				filter_list [filter_index].object_type = T_REJ;
				}
			}
		if (filter_index + 1 == filter_curr_size)
			{
			filter_curr_size += FILTER_INCREMENT;
			filter_list = (struct filter_entry *) realloc (filter_list, sizeof (struct filter_entry) * filter_curr_size);
			}
		filter_index ++;
		} while (!feof (FILTER_FP));
	filter_line_count = filter_index - 1;
	if (sfflags->std_out == SW_OFF)
		{
		printf (" %d lines added.\n", filter_line_count);
		}
	}

// Initial search section
if (sfflags->std_out == SW_OFF)
	{
	printf ("# Building file list...");
	}
strcpy (database_filename, database_dataset);		// compose output filename
strcat (database_filename, database_extension);
find_list = (struct find_list_entry *) malloc (sizeof (struct find_list_entry) * FIND_LIST_INITIAL_SIZE);
find_list_curr_size = FIND_LIST_INITIAL_SIZE;
getcwd (C_W_D, 256);		// get present working directory
strcat (C_W_D, SLASH_TERM);
DIR_PATH = opendir (PATH_CURRENT);		// open directory
if (DIR_PATH != NULL)
	{
	while ((dir_ents = readdir (DIR_PATH)))		// get directory listing
		{
		switch (dir_ents->d_type)
			{
			case FILE_TYPE:					// set type to file
				find_list [find_list_write].object_type = T_FIL;
				break;
			case DIR_TYPE:					// set type to directory
				find_list [find_list_write].object_type = T_DIR;
				break;
			default:						// mark as unneeded type
				find_list [find_list_write].object_type = T_REJ;
				break;
			}								// filter out ".", ".." and temp file from search vvv
		if (!(strcmp (dir_ents->d_name, DIR_CURRENT) && strcmp (dir_ents->d_name, DIR_PREV) \
			&& strcmp (dir_ents->d_name, database_filename)))
			{
			find_list [find_list_write].object_type = T_REJ;
			}
		strcpy (find_list [find_list_write].filepath, dir_ents->d_name);
		if (sfflags->verbose)
			{
			printf ("%3d %c %2d -%s-\n", find_list_write, find_list [find_list_write].object_type, dir_ents->d_type, dir_ents->d_name);
			}
		if (find_list_write + 1 == find_list_curr_size)
			{
			find_list_curr_size += FIND_LIST_INCREMENT;
			find_list = (struct find_list_entry *) realloc (find_list, sizeof (struct find_list_entry) * find_list_curr_size);
			}
		find_list_write ++;
		}
	(void) closedir (DIR_PATH);
	}
	else
	{
	perror ("Couldn't open the directory");		// FIX
	}

// Feedback search section
while (find_list_read < find_list_write)
	{
	chdir (C_W_D);		// go back to the starting directory
	if (find_list [find_list_read].object_type == T_DIR)
		{
		strcpy (path_sub, C_W_D);
		strcat (path_sub, find_list [find_list_read].filepath);		// compose directory location for search
		chdir (path_sub);						// move to search directory
		DIR_PATH = opendir (path_sub);			// open directory
		if (DIR_PATH != NULL)
			{
			while ((dir_ents = readdir (DIR_PATH)))		// get directory listing
				{
				switch (dir_ents->d_type)
					{
					case FILE_TYPE:					// set type to file
						find_list [find_list_write].object_type = FILE_ENTRY;
						break;
					case DIR_TYPE:					// set type to directory
						find_list [find_list_write].object_type = DIR_ENTRY;
						break;
					default:						// mark as unneeded type
						find_list [find_list_write].object_type = UNKNOWN_ENTRY;
						break;
					}								// filter out "." and ".." from search vvv
				if (!(strcmp (dir_ents->d_name, DIR_CURRENT) && strcmp (dir_ents->d_name, DIR_PREV)))
					{
					find_list [find_list_write].object_type = T_REJ;
					}
				strcpy (find_list [find_list_write].filepath, find_list [find_list_read].filepath);
				strcat (find_list [find_list_write].filepath, "/");
				strcat (find_list [find_list_write].filepath, dir_ents->d_name);
				if (find_list_write + 1 == find_list_curr_size)
					{
					find_list_curr_size += FIND_LIST_INCREMENT;
					find_list = (struct find_list_entry *) realloc (find_list, sizeof (struct find_list_entry) * find_list_curr_size);
					}
				if (sfflags->verbose)								// append entry to temp file ^^^
					{
					printf ("%d %c >%s<\n", find_list_write, find_list [find_list_write].object_type, find_list [find_list_write].filepath);
					}
				find_list_write ++;
				}
			closedir (DIR_PATH);
			}
//			else
//			{
//			perror ("Couldn't open the directory");		// FIX
//			}


		}
	find_list_read ++;
	}
if (sfflags->std_out == SW_OFF)
	{
	printf ("%d files found.\n", find_list_write);
	}
if (sfflags->filtering > 0 && sfflags->std_out == SW_OFF)
	{
	printf ("# Applying filter...");
	}
for (find_list_read = 0; find_list_read < find_list_write; find_list_read ++)
	{
	output_current = TRUE;			// set so that if not filtering all results are output
	filter_match = FALSE;
	if (sfflags->filtering > 0)				// are we applying filtering?
		{
		output_current = FALSE;
		for (filter_index = 0; filter_index < filter_line_count; filter_index ++)		// cycle through filter list
			{
			if (!strcmp (filter_list [filter_index].filepath, find_list [find_list_read].filepath) && \
				(filter_list [filter_index].object_type == T_FIL || filter_list [filter_index].object_type == T_DIR))		// match found
				{
				filter_match = TRUE;
				}
			}
		if (sfflags->filtering == F_INCL && filter_match)
			{
			output_current = TRUE;
			}
		if (sfflags->filtering == F_EXCL && !filter_match)
			{
			output_current = TRUE;
			}
		}
	if (output_current)
		{
		if (find_list [find_list_read].object_type == T_FIL)
			{
			if (stat (find_list [find_list_read].filepath, &file_stat) == 0)
				{
				if (file_stat.st_mode & S_IFREG)
					{
					find_list [find_list_read].filesize = file_stat.st_size;
					}
				}
			file_type_count ++;
			file_size_total += find_list [find_list_read].filesize;
//printf ("FST=%d\t", file_size_total);
			}
		}
	}
if (sfflags->filtering > 0)
	{
	FILOUT_FP = fopen (FILTER_OUT, "w");
	if (FILOUT_FP == NULL)
		{
		exit_error ("Can't open filter for writing", "");
		}
	for (filter_index = 0; filter_index < filter_line_count; filter_index ++)
		{
		if (filter_index == 0)
			{
			fprintf (FILOUT_FP, "# Automatically generated by %s\n", FILTER_PROG_NAME);
			}
		if (filter_list [filter_index].object_type != T_REJ && filter_index != 0)
			{
			fprintf (FILOUT_FP, "%s\n", filter_list [filter_index].filepath);
			}
		}
	fclose (FILOUT_FP);
	free (filter_list);
	if (sfflags->std_out == SW_OFF)
		{
		printf ("%d files added.\n", file_type_count);
		}
	}

chdir (C_W_D);				// go back to the starting directory

// Database filepath load section
database_db = (struct sfind_database *) malloc (sizeof (struct sfind_database) * file_type_count);		// allocate memory for database
find_list_read = 0;
for (database_index = 0; database_index < file_type_count; database_index ++)
	{
	while (find_list [find_list_read].object_type != T_FIL && find_list_read < find_list_write)
		{
		find_list_read ++;
		}
	if (find_list [find_list_read].object_type == T_FIL)		// if object is file, do SHA256SUM
		{
		strcpy (database_db [database_index].filepath, find_list [find_list_read].filepath);
		strcpy (database_db [database_index].dataset, database_dataset);
		database_db [database_index].filesize = find_list [find_list_read].filesize;
//		printf ("%d\n", find_list [find_list_read].filesize);
		}
	find_list_read ++;
	}

// SHA256SUM generation section
if (sfflags->std_out == SW_OFF)
	{
	printf ("# Generating SHA256SUMs for %s%s%s...", TEXT_BLUE, database_filename, TEXT_RESET);
	}
if (sfflags->progress)
	{
	printf ("\n");
	}
file_size_mult = 100.0 / (float)file_size_total;
//fprintf (stderr, "%ld\t%f\n", file_size_total, file_size_mult);
for (line_index = 0; line_index < file_type_count; line_index ++)	//
	{
	if (sfflags->progress)
		{
		file_size_accum += database_db [line_index].filesize;
		file_progress = file_size_mult * file_size_accum;
		fprintf (stderr, "%c%.2f%%", CGE_RET, file_progress);
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
if (sfflags->progress && sfflags->std_out == SW_OFF)
	{
	fprintf (stderr, "%c", CGE_RET);
	}
if (sfflags->std_out == SW_OFF)
	{
	printf ("...SHA256SUMs done.\n", file_type_count);
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
	if (swap_made && outer_loop == 1 && sfflags->std_out == SW_OFF)
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
if (sfflags->std_out == SW_OFF)
	{
	printf ("%d lines written to %s%s%s\n", file_type_count, TEXT_BLUE, database_filename, TEXT_RESET);
	}
// Clean-up section
chdir (C_W_D);

}


char filter_line_check (char *filter_line)
{
char first_char;

if (strchr (filter_line, ':') != NULL)
	{
	return (0);
	}
first_char = filter_line [0];
if (first_char == 47)
	{
	return (0);
	}
if (first_char == 95)
	{
	return (0);
	}
if (first_char == 35)		// # char marks comment
	{
	return (2);
	}
if (first_char < 45 || first_char > 122)
	{
	return (0);
	}
if (first_char < 65 && first_char > 57)
	{
	return (0);
	}
if (first_char < 97 && first_char > 90)
	{
	return (0);
	}
return (1);
}
