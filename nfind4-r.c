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
#define FILTER_PROG_NAME "NFind"
#define TWO_SPACES "  "
#define FILTER_FILE "nf_filter"
#define FILTER_OUT "nf_filter-o"
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
struct file_list_entry *filter_list, *find_list;

FILE *FILTER_FP;					// inclusion and exclusion filter list
FILE *FILOUT_FP;
DIR *DIR_PATH;

int filter_line_count = 0;
int filter_index = 0;
int find_list_write = 0;		// number of file items found in search
int find_list_read = 0;
int filter_ferr;
int arg_no, switch_pos;
int filter_curr_size = 0;
int find_list_curr_size = 0;

char fileline [FILELINE_LENGTH] = "";				// holds line from filter file
char switch_chr;
char path_sub [FILEPATH_LENGTH];					// holds SHA256SUM file argument
char C_W_D [256];									// base directory of search
char filter_match = FALSE;
char filter_check;
char output_current;
char filtering = FALSE;

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
				case 'i':
					filtering = F_INCL;
					break;
				case 'V':
					printf ("SHA Find version %s\n", PROG_VERSION);
					exit (0);
				case 'x':
					filtering = F_EXCL;
					break;
				default:
					exit_error ("# SHA find [-fnosuv] <search file> <database file>\n","");
					break;
				}	// END switch
			}	// END for switch_pos
		}	// END for arg_no
	}

// Output open section
if (filtering > 0)
	{
	FILTER_FP = fopen (FILTER_FILE, "r");
	if (FILTER_FP == NULL)
		{
		exit_error ("Can't find filter file: ", FILTER_FILE);
		}
	}

if (filtering > 0)
	{
	filter_list = (struct file_list_entry *) malloc (sizeof (struct file_list_entry) * FILTER_INITIAL_SIZE);
	filter_curr_size = FILTER_INITIAL_SIZE;
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
			filter_list = (struct file_list_entry *) realloc (filter_list, sizeof (struct file_list_entry) * filter_curr_size);
			}
		filter_index ++;
		} while (!feof (FILTER_FP));
	}
filter_line_count = filter_index - 1;
// Initial search section
printf ("\n# Building file list...\n");
//printf ("Filtering %c\n", filtering);
find_list = (struct file_list_entry *) malloc (sizeof (struct file_list_entry) * FIND_LIST_INITIAL_SIZE);
find_list_curr_size = FIND_LIST_INITIAL_SIZE;
getcwd (C_W_D, 256);		// get present working directory
strcat (C_W_D, SLASH_TERM);
DIR_PATH = opendir (PATH_CURRENT);		// open directory
if (DIR_PATH != NULL)
	{
	while (dir_ents = readdir (DIR_PATH))		// get directory listing
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
		if (!(strcmp (dir_ents->d_name, DIR_CURRENT) && strcmp (dir_ents->d_name, DIR_PREV)))
			{
			find_list [find_list_write].object_type = T_REJ;
			}
		strcpy (find_list [find_list_write].filepath, dir_ents->d_name);
		if (find_list_write + 1 == find_list_curr_size)
			{
			find_list_curr_size += FIND_LIST_INCREMENT;
			find_list = (struct file_list_entry *) realloc (find_list, sizeof (struct file_list_entry) * find_list_curr_size);
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
	if (find_list [find_list_read].object_type == T_DIR)		// if object is directory, prepare to enter it
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
						find_list [find_list_write].object_type = T_FIL;
						break;
					case DIR_TYPE:					// set type to directory
						find_list [find_list_write].object_type = T_DIR;
						break;
					default:						// mark as unneeded type
						find_list [find_list_write].object_type = T_REJ;
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
					find_list = (struct file_list_entry *) realloc (find_list, sizeof (struct file_list_entry) * find_list_curr_size);
					}
				find_list_write ++;
				}
			}
		closedir (DIR_PATH);
		}
//			else
//			{
//			perror ("Couldn't open the directory");		// FIX
//			}
	find_list_read ++;
	}
for (find_list_read = 0; find_list_read < find_list_write; find_list_read ++)
	{
	output_current = TRUE;			// set so that if not filtering all results are output
	filter_match = FALSE;
	if (filtering > 0)				// are we applying filtering?
		{
		output_current = FALSE;
		for (filter_index = 0; filter_index < filter_line_count; filter_index ++)		// cycle through filter list
			{
			if (!strcmp (filter_list [filter_index].filepath, find_list [find_list_read].filepath) && \
				(filter_list [filter_index].object_type == T_FIL || filter_list [filter_index].object_type == T_DIR))		// match found
				{
				filter_match = TRUE;
				filter_list [filter_index].filtered = TRUE;
				}
			}
		if (filtering == F_INCL && filter_match)
			{
			output_current = TRUE;
			}
		if (filtering == F_EXCL && !filter_match)
			{
			output_current = TRUE;
			}
		}
	if (output_current)
		{
		if (find_list [find_list_read].object_type == T_FIL)
			{
			printf ("%s\n", find_list [find_list_read].filepath);
			}
		}
	}
if (filtering > 0)
	{
	FILOUT_FP = fopen (FILTER_OUT, "w");
	if (FILOUT_FP == NULL)
		{
		exit_error ("Can't open filter for writing", "");
		}
	for (filter_index = 0; filter_index < filter_line_count; filter_index ++)
		{
		if (filter_index == 0 && filter_list [filter_index].object_type == T_HDR)
			{
			fprintf (FILOUT_FP, "# Automatically generated by %s\n", FILTER_PROG_NAME);
			}
		if (filter_list [filter_index].object_type != T_REJ && filter_index != 0)
			{
			fprintf (FILOUT_FP, "%s\n", filter_list [filter_index].filepath);
			printf ("_%c_%s\n", filter_list [filter_index].object_type, filter_list [filter_index].filepath);
			}
		}
	fclose (FILOUT_FP);
	free (filter_list);
	}

chdir (C_W_D);				// go back to the starting directory

free (find_list);
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
