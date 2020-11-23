/* * * * * * * * * * * * * * * * *
 *                               *
 *     nfind 0.44             *
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
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "SMLib.h"

#define PROG_VERSION "0.44"
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
#define FILTER_INITIAL_SIZE 5
#define FILTER_INCREMENT 5
#define FILTER_CEILING 1024
#define FILELIST_INITIAL_SIZE 1024
#define FILELIST_INCREMENT 1024
#define FILELIST_CEILING 262144
#define T_DIR 'd'
#define T_FIL 'f'
#define T_OTH 'o'
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

struct file_list_entry
	{
	char object_type;
	char filtered;
	char filepath [FILEPATH_LENGTH];
	};
char filter_line_check (char *filter_line);

int main (int argc, char *argv [])

{

struct dirent *dir_ents;
struct stat file_stat;
struct shafind_flags sfflags [1] = {0};
struct file_list_entry *filter_list;

FILE *FILTER_FP;					// inclusion and exclusion filter list
DIR *DIR_PATH;

int line_index;
int filter_line_count = 0;
int filter_index = 0;
int file_type_count = 0;
int object_count = 0;		// number of file items found in search
int find_in_ferr;
int filter_ferr;
int arg_no, switch_pos;
int outer_loop, inner_loop;
int progress_count = 0;
int progress_interval;
int progress_percent = 0;
int filter_curr_size = 0;
int filelist_curr_size = 0;

char find_line [FILELINE_LENGTH] = "";				// holds line from temp file
char fileline [FILELINE_LENGTH] = "";				// holds line from filter file
char switch_chr;
char object_type;									// holds directory entry type: file, dir, other
char path_sub [FILEPATH_LENGTH];					// holds SHA256SUM file argument
char find_sub_target [FILEPATH_LENGTH];				// next subdirectory to search
char C_W_D [256];									// base directory of search
char swap_made = TRUE;								// swap was made on last sort pass
char filter_match = FALSE;
char filter_check;

// Argument section
/*for (arg_no = 1; arg_no < argc; arg_no++)		// loop through arguments
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
				case 'p':
					sfflags->progress = SW_ON;
					break;
				case 'u':
					sfflags->sort = SORT_NONE;
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
	}
*/
// Output open section
//if (sfflags->filtering > 0)
//	{
	FILTER_FP = fopen (FILTER_FILE, "r");
	if (FILTER_FP == NULL)
		{
		exit_error ("Can't find filter file: ", FILTER_FILE);
		}
//	}

//if (sfflags->filtering > 0)
//	{
	filter_list = (struct file_list_entry *) malloc (sizeof (struct file_list_entry) * FILTER_INITIAL_SIZE);
	filter_curr_size = FILTER_INITIAL_SIZE;
	do
		{
		filter_ferr = (long)fgets (fileline, FILEPATH_LENGTH, FILTER_FP);
		if (fileline != NULL && filter_ferr)
			{
			strcpy (filter_list [filter_index].filepath, fileline);
			filter_list [filter_index].filepath[strlen (filter_list [filter_index].filepath) - 1] = NULL_TERM;
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
//			if (filter_list [filter_index].object_type != T_REJ)
//				{
//				printf ("%s__%c\n", filter_list [filter_index].filepath, filter_list [filter_index].object_type);
//				}
			}
		if (filter_index + 1 == filter_curr_size)
			{
			filter_curr_size += FILTER_INCREMENT;
			filter_list = (struct file_list_entry *) realloc (filter_list, sizeof (struct file_list_entry) * filter_curr_size);
			}
//		printf ("FI=%d\tFCS=%d\n", filter_index, filter_curr_size);
		filter_index ++;
		} while (!feof (FILTER_FP));
//	}
filter_line_count = filter_index;
for (filter_index = 0; filter_index < filter_line_count; filter_index ++)
	{
	printf ("%s__%c\n", filter_list [filter_index].filepath, filter_list [filter_index].object_type);
	}
/*
// Initial search section
printf ("# Building file list...");
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
			}
//			else
//			{
//			perror ("Couldn't open the directory");		// FIX
//			}
		}
	}
printf (" %d files found\n", file_type_count);
chdir (C_W_D);				// go back to the starting directory

*/
free (filter_list);
}

char filter_line_check (char *filter_line)
{
int char_index;
char first_char;

first_char = filter_line [0];
if (first_char == 47)
	{
	return (0);
	}
if (first_char == 35)
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
