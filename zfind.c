/* * * * * * * * * * * * * * * * *
 *                               *
 *     zfind 0.492               *
 *                               *
 *     2022-10-18                *
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

#define PROG_VERSION "0.492"
#define FILTER_PROG_NAME "SFind"
#define TWO_SPACES "  "
#define FILTER_FILE "sf_filter"
#define FILTER_BK ".sf_filter"
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
#define FILTER_INITIAL_SIZE 256
#define FILTER_INCREMENT 256
#define FILTER_CEILING 4096
#define T_DIR 'd'
#define T_FIL 'f'
#define T_REJ 'r'
#define T_COM 'c'
#define SHA_ZERO "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855\n"

#if __linux__
#define DIR_TYPE 4
#define FILE_TYPE 8
#define SHA_CMD "sha256sum "
#endif

#if _WIN32
#define DIR_TYPE 16
#define FILE_TYPE 0
#define SHA_CMD "certutil -hashfile "
#define SHA_CMD_ARG " sha256"
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
sfflags->sort = SORT_SHA;			// sort by SHA256SUM by default
sfflags->filtering = FALSE;
sfflags->progress = SW_ON;			// show progress by default

FILE *FILTER_FP;				// inclusion and exclusion filter list
FILE *FILOUT_FP;
FILE *DATABASE_FP;
DIR *DIR_PATH;

int line_index;
int filter_line_count = 0;
int filter_index = 0;
int find_list_write = 0;			// number of file items found in search
int find_list_read = 0;
int filter_ferr;
int arg_no, switch_pos;
int filter_curr_size = 0;
int find_list_curr_size = 0;

char database_dataset [DATASET_LENGTH] = "";		// holds dataset name
char database_filename [FILEPATH_LENGTH] = "";		// output file name with extension
char fileline [FILELINE_LENGTH] = "";			// holds line from filter file
char switch_chr;
char database_extension [8] = "";			// holds database extension based on flag
char path_sub [FILEPATH_LENGTH];			// holds SHA256SUM file argument
char dir_filter_test [FILEPATH_LENGTH] = "";		// holds composed directory for filter
char C_W_D [FILEPATH_LENGTH];				// base directory of search
char filter_check;

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
				case 'n':
					sfflags->progress = SW_OFF;
					break;
				case 'o':
					sfflags->database_type = SHA256_TYPE;
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
					exit_error ("# SHA find [-finosuvVx] <search file> <database file>","");
					break;
				}	// END switch
			}	// END for switch_pos
		}	// END for arg_no
		else
		{
		strncpy (database_dataset, argv [arg_no], DATASET_LENGTH);
		} 	// END else if int argv
	}

//printf ("Here 1\n");

if (argc < 2 || !strlen (database_dataset))		// no file argument, so stdout only
	{
	sfflags->std_out = TRUE;
	}
if (sfflags->verbose)
	{
	sfflags->progress = SW_OFF;			// progress indicator will interfere with verbose output
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
			strcpy (filter_list [filter_index].filepath, fileline);				// load filepath into filter
			filter_list [filter_index].filepath[strlen (filter_list [filter_index].filepath) - 1] = NULL_TERM;	// strip trailing return char
			if (filter_list [filter_index].filepath[strlen (filter_list [filter_index].filepath) - 1] == '/')
				{
				filter_list [filter_index].filepath[strlen (filter_list [filter_index].filepath) - 1] = NULL_TERM;	// strip trailing slash
				}
			filter_check = filter_line_check (filter_list [filter_index].filepath);		// check for invalid characteristics
			if (filter_check)
				{
				if (stat (filter_list [filter_index].filepath, &file_stat) == 0)
					{
					if (file_stat.st_mode & S_IFREG)
						{
						filter_list [filter_index].object_type = T_FIL;		// mark as file
						}
					if (file_stat.st_mode & S_IFDIR)
						{
						filter_list [filter_index].object_type = T_DIR;		// mark as directory
						}
					}
					else
					{
					filter_list [filter_index].object_type = T_REJ;		// mark as rejected
					}
				if (filter_check == 2)
					{
					filter_list [filter_index].object_type = T_COM;		// mark as comment
					}
				}
				else
				{
				filter_list [filter_index].object_type = T_REJ;		// mark as rejected
				}
			if (sfflags->verbose)
				{
				printf ("F-%d__%c__%s\n", filter_index, filter_list [filter_index].object_type, filter_list [filter_index].filepath);
				}
			}
		if (filter_index + 1 == filter_curr_size)		// allocated more memory if needed
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
find_list = (struct find_list_entry *) malloc (sizeof (struct find_list_entry) * DATABASE_INITIAL_SIZE);
find_list_curr_size = DATABASE_INITIAL_SIZE;
getcwd (C_W_D, FILEPATH_LENGTH);		// get present working directory
strcat (C_W_D, SLASH_TERM);
DIR_PATH = opendir (PATH_CURRENT);		// open directory
if (DIR_PATH != NULL)
	{
	while ((dir_ents = readdir (DIR_PATH)))		// get directory listing
		{
		switch (dir_ents->d_type)
			{
			case FILE_TYPE:						// set type to file
				find_list [find_list_write].object_type = T_FIL;
				break;
			case DIR_TYPE:						// set type to directory
				find_list [find_list_write].object_type = T_DIR;
				break;
			default:						// mark as unneeded type
				find_list [find_list_write].object_type = T_REJ;
				break;
			}								// VVV filter out ".", ".." and temp file from search
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
		if (find_list_write + 1 == find_list_curr_size)		// allocated more memory if needed
			{
			find_list_curr_size += DATABASE_INCREMENT;
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
	chdir (C_W_D);				// go back to the starting directory
	if (find_list [find_list_read].object_type == T_DIR)
		{
		strcpy (path_sub, C_W_D);
		strcat (path_sub, find_list [find_list_read].filepath);		// compose directory location for search
		chdir (path_sub);						// move to search directory
		DIR_PATH = opendir (path_sub);					// open directory
		if (DIR_PATH != NULL)
			{
			while ((dir_ents = readdir (DIR_PATH)))			// get directory listing
				{
				switch (dir_ents->d_type)
					{
					case FILE_TYPE:						// set type to file
						find_list [find_list_write].object_type = FILE_ENTRY;
						break;
					case DIR_TYPE:						// set type to directory
						find_list [find_list_write].object_type = DIR_ENTRY;
						break;
					default:						// mark as unneeded type
						find_list [find_list_write].object_type = UNKNOWN_ENTRY;
						break;
					}							// VVV filter out "." and ".." from search
				if (!(strcmp (dir_ents->d_name, DIR_CURRENT) && strcmp (dir_ents->d_name, DIR_PREV)))
					{
					find_list [find_list_write].object_type = T_REJ;
					}
				strcpy (find_list [find_list_write].filepath, find_list [find_list_read].filepath);
				strcat (find_list [find_list_write].filepath, "/");
				strcat (find_list [find_list_write].filepath, dir_ents->d_name);
				if (find_list_write + 1 == find_list_curr_size)		// allocated more memory if needed
					{
					find_list_curr_size += DATABASE_INCREMENT;
					find_list = (struct find_list_entry *) realloc (find_list, sizeof (struct find_list_entry) * find_list_curr_size);
					}
				if (sfflags->verbose)
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
	printf ("%d items found.\n", find_list_write);
	}
if (sfflags->filtering > 0 && sfflags->std_out == SW_OFF)
	{
	printf ("# Applying filter...");
	}

// Output
for (find_list_read = 0; find_list_read < find_list_write; find_list_read ++)
	{
//printf (".");
	find_list [find_list_read].filtered = TRUE;		// set so that if not filtering all results are output
	filter_match = FALSE;
	if (sfflags->filtering > 0)				// are we applying filtering?
		{
		find_list [find_list_read].filtered = FALSE;
		for (filter_index = 0; filter_index < filter_line_count; filter_index ++)		// cycle through filter list
			{
			if (strcmp (filter_list [filter_index].filepath, find_list [find_list_read].filepath) == 0 && \
				filter_list [filter_index].object_type == T_FIL)			// match found
				{
				filter_match = TRUE;
				}
			if (filter_list [filter_index].object_type == T_DIR)				// test for items in filter directory
				{
				strcpy (dir_filter_test, filter_list [filter_index].filepath);
				strcat (dir_filter_test, "/");
				if (!strncmp (dir_filter_test, find_list [find_list_read].filepath, strlen (dir_filter_test)))	// match found
					{
					filter_match = TRUE;
					}
				}
			}
		if (sfflags->filtering == F_INCL && filter_match)		// output if match with inclusive filter
			{
			find_list [find_list_read].filtered = TRUE;
			}
		if (sfflags->filtering == F_EXCL && !filter_match)		// output if no match with exclusive filter
			{
			find_list [find_list_read].filtered = TRUE;
			}
		}
	if (find_list [find_list_read].filtered)		// output matching line
		{
		if (find_list [find_list_read].object_type == T_FIL)		// output only files, no directories
			{
//			printf ("%s\t%c\n", find_list [find_list_read].filepath, find_list [find_list_read].object_type);
			printf ("%c\t%d\t%s\t%d\n", find_list [find_list_read].object_type, \
						find_list [find_list_read].filtered, \
						find_list [find_list_read].filepath, \
						find_list [find_list_read].filesize);
			}
		}
	}

// Clean-up section
chdir (C_W_D);

//free (database_db);	// free memory
//database_db = NULL;
//free (find_list);
//find_list = NULL;
//free (filter_list);
//filter_list = NULL;
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
