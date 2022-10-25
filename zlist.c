/* * * * * * * * * * * * * * * * *
 *                               *
 *       ZList 0.51              *
 *                               *
 *       2022-10-22              *
 *                               *
 *       Zax                     *
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

int main (int argc, char *argv [])

{
struct sfind_database *sfind_db;
struct sfind_flags sfflags [1] = {0};
struct dirent *dir_ents;
struct stat file_stat;
struct find_list_entry *find_list;
struct filter_list_entry *filter_list;
sfflags->database_type = S2DB_TYPE;		// default file type S2DB
sfflags->sort = SORT_SHA;			// sort by SHA256SUM by default
sfflags->filtering = FALSE;
sfflags->progress = SW_ON;			// show progress by default

FILE *SHA_PIPE;
FILE *SSDB_OUT_FP;				// Output file
DIR *DIR_PATH;

int line_index;
int filter_line_count = 0;
int filter_index = 0;
int find_list_write = 0;			// number of file items found in search
int find_list_read = 0;
int database_index = 0;
int database_alloc_size = DATABASE_INITIAL_SIZE;
int filter_ferr;
int arg_no, switch_pos;
int filter_curr_size = 0;
int find_list_curr_size = 0;
int swap_index;
int database_time;

unsigned long file_size_total = 0;
unsigned long file_size_accum = 0;

double file_size_mult = 0;
double file_progress = 0;

char database_name [DATASET_LENGTH] = "";		// holds dataset name
char database_filename [FILEPATH_LENGTH] = "";		// output file name with extension
char fileline [FILELINE_LENGTH] = "";			// holds line from filter file
char sha_line [FILELINE_LENGTH] = "";			// hold line from SHA256SUM output
char switch_chr;
char database_extension [8] = "";			// holds database extension based on flag
char sha_command [FILEPATH_LENGTH] = "";		// SHA256SUM command line with file argument
char path_sub [FILEPATH_LENGTH];			// holds SHA256SUM file argument
char dir_filter_test [FILEPATH_LENGTH] = "";		// holds composed directory for filter
char C_W_D [FILEPATH_LENGTH];				// base directory of search
char swap_made = TRUE;					// swap was made on last sort pass
char filter_match = FALSE;
char filter_check;
char sort_need_check = TRUE;

strcpy (database_name, argv [1]);
printf ("%s\n", database_name);
if (stat (database_name, &file_stat) == 0)
	{
	database_time = file_stat.st_mtime;		// get file size
	}
// Initial search section
find_list = (struct find_list_entry *) malloc (sizeof (struct find_list_entry) * DATABASE_INITIAL_SIZE);
find_list_curr_size = DATABASE_INITIAL_SIZE;
getcwd (C_W_D, FILEPATH_LENGTH);			// get present working directory
strcat (C_W_D, SLASH_TERM);
DIR_PATH = opendir (PATH_CURRENT);			// open directory
if (DIR_PATH != NULL)
	{
	while ((dir_ents = readdir (DIR_PATH)))		// get directory listing
		{
		switch (dir_ents->d_type)
			{
			case FILE_TYPE:							// set type to file
				find_list [find_list_write].object_type = T_FIL;
				break;
			case DIR_TYPE:							// set type to directory
				find_list [find_list_write].object_type = T_DIR;
				break;
			default:							// mark as unneeded type
				find_list [find_list_write].object_type = T_REJ;
				break;
			}
		if (!(strcmp (dir_ents->d_name, DIR_CURRENT) && strcmp (dir_ents->d_name, DIR_PREV) \
			&& strcmp (dir_ents->d_name, database_filename)))
			{						// Filter out ".", ".." and database file from search
			find_list [find_list_write].object_type = T_REJ;
			}
		strcpy (find_list [find_list_write].filepath, dir_ents->d_name);
		if (sfflags->verbose)
			{
			printf ("FS\tFLW=%3d\tOT=%c\tDT=%d\tDN=%s=\n", find_list_write, \
								find_list [find_list_write].object_type, \
								dir_ents->d_type, dir_ents->d_name);
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

// Database filepath load section
for (find_list_read = 0; find_list_read < find_list_write; find_list_read ++)
	{
	if (find_list [find_list_read].object_type == T_FIL)		// output only files, no directories
		{
		if (stat (find_list [find_list_read].filepath, &file_stat) == 0)
			{
			if (file_stat.st_mode & S_IFREG)
				{
				printf ("%s\t%u\t%d\t%d\n", find_list [find_list_read].filepath, file_stat.st_size, file_stat.st_mtime, file_stat.st_mtime - database_time);
				}
			}
		}
	}

chdir (C_W_D);		// go back to the starting directory

}
