/* * * * * * * * * * * * * * * * *
 *                               *
 *         zfind 0.50            *
 *                               *
 *         2022-10-22            *
 *                               *
 *         Zax                   *
 *                               *
 * * * * * * * * * * * * * * * * */

/* * * * * * * * * * * * * * * * *
 *                               *
 *            TODO               *
 *                               *
 *     Update files changed      *
 *     since database built      *
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

#define PROG_VERSION "0.50"
#define FILTER_PROG_NAME "ZFind"

char filter_line_check (char *filter_line);

int main (int argc, char *argv [])

{
struct sfind_flags sfflags [1] = {0};
struct supdate_diff su_diff [1] = {0};
struct dirent *dir_ents;
struct stat file_stat;
struct find_list_entry *find_list;
struct fs_list_entry *fs_list;
struct filter_list_entry *filter_list;
struct sha_sort_database *ssort_db;

sfflags->filtering = FALSE;

FILE *SHA_PIPE;
FILE *FILT_IN_FP;				// inclusion and exclusion filter list
FILE *FILT_OUT_FP;
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
int fs_list_curr_size = 0;
int fs_list_index = 0;
int swap_index;
int database_alloc_size = DATABASE_INITIAL_SIZE;
int database_ferr;				// database file error
int database_index = 0;				// number of lines in search list
int file_index = 0;
int db_index = 0;
int chr_idx;

char database_in_filename [FILEPATH_LENGTH] = "";	// input file name with extension
char fileline [FILELINE_LENGTH] = "";			// holds line from filter file
char switch_chr;
char path_sub [FILEPATH_LENGTH];			// holds SHA256SUM file argument
char dir_filter_test [FILEPATH_LENGTH] = "";		// holds composed directory for filter
char C_W_D [FILEPATH_LENGTH];				// base directory of search
char filter_check;
char filter_match = FALSE;
char swap_made = TRUE;
char database_type;
char outype = 'X';
char dataset_name [FILEPATH_LENGTH] = "";
char sha_command [FILEPATH_LENGTH] = "";
char sha_line [FILELINE_LENGTH] = "";

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
					outype = 'f';
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
					outype = 's';
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
		strncpy (database_in_filename, argv [arg_no], DATASET_LENGTH);
		} 	// END else if int argv
	}

// Output open section
if (sfflags->filtering > 0)
	{
	FILT_IN_FP = fopen (FILTER_FILE, "r");
	if (FILT_IN_FP == NULL)
		{
		exit_error ("Can't find filter file: ", FILTER_FILE);
		}
	}

DATABASE_FP = fopen (database_in_filename, "r");
if (DATABASE_FP == NULL)
	{
	exit_error ("Can't find Database: ", database_in_filename);
	}

chr_idx = strlen (database_in_filename);
while (database_in_filename [chr_idx] != '.' && -- chr_idx);
if (chr_idx)
	{
	dataset_name [chr_idx--] = NULL_TERM;
	do
		{
		dataset_name [chr_idx] = database_in_filename [chr_idx];
		}
		while (chr_idx--);
	dataset_name [chr_idx] = database_in_filename [chr_idx];
	}

// Filter load section
if (sfflags->filtering)
	{
	filter_list = (struct filter_list_entry *) malloc (sizeof (struct filter_list_entry) * FILTER_INITIAL_SIZE);
	filter_curr_size = FILTER_INITIAL_SIZE;
	if (sfflags->std_out == SW_OFF)
		{
		printf ("# Loading filter...");
		}
	do
		{
		filter_ferr = (long)fgets (fileline, FILEPATH_LENGTH, FILT_IN_FP);
		if (fileline != NULL && filter_ferr)
			{
			strcpy (filter_list [filter_index].filepath, fileline);				// load filepath into filter
			filter_list [filter_index].filepath[strlen (filter_list [filter_index].filepath) - 1] = NULL_TERM;	// strip trailing return char
			if (filter_list [filter_index].filepath[strlen (filter_list [filter_index].filepath) - 1] == '/')
				{
				filter_list [filter_index].filepath[strlen (filter_list [filter_index].filepath) - 1] = NULL_TERM;	// strip trailing slash
				}
			filter_check = filter_line_check (filter_list [filter_index].filepath);		// check for invalid characteristics
			switch (filter_check)
				{
				case 1:
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
					break;
				case 2:
					filter_list [filter_index].object_type = T_COM;         // mark as comment
					break;
				default:
					filter_list [filter_index].object_type = T_REJ;         // mark as rejected
					break;
				}
			if (sfflags->verbose)
				{
				printf ("F\tFI=%d\tFO=%c\tFP=%s\n", filter_index, \
								filter_list [filter_index].object_type, \
								filter_list [filter_index].filepath);
				}
			}
		if (filter_index + 1 == filter_curr_size)		// allocated more memory if needed
			{
			filter_curr_size += FILTER_INCREMENT;
			filter_list = (struct filter_list_entry *) realloc (filter_list, sizeof (struct filter_list_entry) * filter_curr_size);
			}
		filter_index ++;
		} while (!feof (FILT_IN_FP));
	filter_line_count = filter_index - 1;
	if (sfflags->std_out == SW_OFF)
		{
		printf (" %d lines added.\n", filter_line_count);
		}
	}

// Initial search section
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
			}
		if (!(strcmp (dir_ents->d_name, DIR_CURRENT) && strcmp (dir_ents->d_name, DIR_PREV) \
			&& strcmp (dir_ents->d_name, database_in_filename)))
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
					}
				if (!(strcmp (dir_ents->d_name, DIR_CURRENT) && strcmp (dir_ents->d_name, DIR_PREV)))
					{
					find_list [find_list_write].object_type = T_REJ;	// filter out "." and ".." from search
					}
				strcpy (find_list [find_list_write].filepath, find_list [find_list_read].filepath);
				strcat (find_list [find_list_write].filepath, "/");
				strcat (find_list [find_list_write].filepath, dir_ents->d_name);
				if (find_list_write + 1 == find_list_curr_size)			// allocated more memory if needed
					{
					find_list_curr_size += DATABASE_INCREMENT;
					find_list = (struct find_list_entry *) realloc (find_list, sizeof (struct find_list_entry) * find_list_curr_size);
					}
				if (sfflags->verbose)
					{
					printf ("FBS\tFLW=%3d\tOT=%c\tFP=%s=\n", find_list_write, \
									find_list [find_list_write].object_type, \
									find_list [find_list_write].filepath);
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

// Find files in filelist with filter
fs_list = (struct fs_list_entry *) malloc (sizeof (struct fs_list_entry) * DATABASE_INITIAL_SIZE);
fs_list_curr_size = DATABASE_INITIAL_SIZE;
for (find_list_read = 0; find_list_read < find_list_write; find_list_read ++)
	{
//printf (".");
	find_list [find_list_read].filtered = TRUE;		// set so that if not filtering all results are output
	filter_match = TRUE;
	if (sfflags->filtering > 0)				// are we applying filtering?
		{
		find_list [find_list_read].filtered = FALSE;
		filter_match = FALSE;
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
				if (sfflags->verbose)
					{
					printf ("DL\tFLW=%d\tFLR=%d\tFFP=%s\tFLFP=%s\tFM=%d\tFILT=%d\n", find_list_write, \
										find_list_read, \
										filter_list [filter_index].filepath, \
										find_list [find_list_read].filepath, filter_match, \
										find_list [find_list_read].filtered);
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
			strcpy (fs_list [fs_list_index].filepath, find_list [find_list_read].filepath);
			fs_list [fs_list_index].index = fs_list_index;
			fs_list_index ++;
			}
		}
	if (fs_list_index + 1 == fs_list_curr_size)		// allocated more memory if needed
		{
		fs_list_curr_size += DATABASE_INCREMENT;
		fs_list = (struct fs_list_entry *) realloc (find_list, sizeof (struct fs_list_entry) * fs_list_curr_size);
		}
	}


// Output verified filter section
if (sfflags->filtering)
	{
	rename (FILTER_FILE, FILTER_BK);		//rename old filter
	FILT_OUT_FP = fopen (FILTER_FILE, "w");
	if (FILT_OUT_FP == NULL)
		{
		exit_error ("Can't open filter for writing", "");
		}
	for (filter_index = 0; filter_index < filter_line_count; filter_index ++)
		{
		if (sfflags->verbose)
			{
			printf ("OF\tFI=%d\tOT=%c\tFP=%s\n", filter_index, \
							filter_list [filter_index].object_type, \
							filter_list [filter_index].filepath);
			}
		if (filter_index == 0 && filter_list [filter_index].object_type != T_REJ)
			{
			fprintf (FILT_OUT_FP, "# Automatically generated by %s\n", FILTER_PROG_NAME);
			}
		if (filter_list [filter_index].object_type == T_FIL || filter_list [filter_index].object_type == T_DIR)
			{
			fprintf (FILT_OUT_FP, "%s\n", filter_list [filter_index].filepath);
			}
		}
	fclose (FILT_OUT_FP);
	free (filter_list);
	if (sfflags->std_out == SW_OFF)
		{
		printf ("%d files added.\n", filter_index);
		}
	}

chdir (C_W_D);		// go back to the starting directory


// Sort filelist
while (swap_made == TRUE)
	{
	swap_made = FALSE;
	for (line_index = 0; line_index < fs_list_index - 1; line_index ++)
		{
		if (strcmp (fs_list [fs_list [line_index].index].filepath, fs_list [fs_list [line_index + 1].index].filepath) > 0)
			{
			swap_index = fs_list [line_index + 1].index;
			fs_list [line_index + 1].index = fs_list [line_index].index;
			fs_list [line_index].index = swap_index;
			swap_made = TRUE;
			}
		}
	}
if (outype == 'f')
	{
	for (line_index = 0; line_index <= fs_list_index - 1; line_index ++) // print output
		{
		printf("%s\n", fs_list [fs_list [line_index].index].filepath);
		}
	}

// Database load section
ssort_db = (struct sha_sort_database *) malloc (sizeof (struct sha_sort_database) * database_alloc_size);
do
	{
	database_ferr = (long)fgets (fileline, FILEPATH_LENGTH, DATABASE_FP);
	if (database_index == 0)
		{
		database_type = sha_verify (fileline);
		if (database_type == UNKNOWN_TYPE)
			{
			exit_error ("Unrecognised file type: ", database_in_filename);
			}
		}
	if (fileline != NULL && database_ferr)
		{
		if (database_type == SHA256_TYPE)		// load standard SHA256SUM output
			{
			strncpy (ssort_db [database_index].sha, fileline, SHA_LENGTH);
			ssort_db [database_index].sha [SHA_LENGTH] = NULL_TERM;
			strcpy (ssort_db [database_index].filepath, fileline + SHA_LENGTH + 2);
			ssort_db [database_index].filepath[strlen (ssort_db [database_index].filepath) - 1] = NULL_TERM;
			strcpy (ssort_db [database_index].dataset, "");
			}
			else		// load SHA256DB data
			{
			separate_fields (ssort_db [database_index].sha, ssort_db [database_index].filepath, ssort_db [database_index].dataset, fileline);
			}
		ssort_db [database_index].index = database_index;	// load sort index in start position
		if (sfflags->verbose)
			{
			printf ("SL\tSs=%s\tSf=%s\tSd=%s\tSi=%d\n", ssort_db [database_index].sha, \
							ssort_db [database_index].filepath, \
							ssort_db [database_index].dataset, \
							ssort_db [database_index].index);
			}
		}
	if (database_index + 1 == database_alloc_size)		// check memory usage, reallocate
		{
		database_alloc_size += DATABASE_INCREMENT;
		ssort_db = (struct sha_sort_database *) realloc (ssort_db, sizeof (struct sha_sort_database) * database_alloc_size);
		}
	database_index ++;
	} while (!feof (DATABASE_FP));
fclose (DATABASE_FP);

// Database sort section
swap_made = TRUE;
while (swap_made == TRUE)
	{
	swap_made = FALSE;
	for (line_index = 0; line_index < database_index - 2; line_index ++)
		{
		if (strcmp (ssort_db [ssort_db [line_index].index].filepath, ssort_db [ssort_db [line_index + 1].index].filepath) > 0)
			{
			swap_index = ssort_db [line_index + 1].index;
			ssort_db [line_index + 1].index = ssort_db [line_index].index;
			ssort_db [line_index].index = swap_index;
			swap_made = TRUE;
			}
		}
	}

// Search section
//					Add progress indicator
file_index = 0;
db_index = 0;
while (file_index <= fs_list_index - 1 && db_index < database_index - 1)
	{
	if (strcmp (fs_list [fs_list [file_index].index].filepath, ssort_db [ssort_db [db_index].index].filepath) < 0)
		{		// Entry in file list is new
		su_diff->add ++;

		strcpy (sha_command, SHA_CMD);				// compose command
		strcat (sha_command, enquote (fs_list [fs_list [file_index].index].filepath));
		SHA_PIPE = popen (sha_command, "r");			// send SHA256SUM command and arguments
		fgets (sha_line, FILELINE_LENGTH, SHA_PIPE);		// receive reply

		if (sha_verify (sha_line))				// verify SHA256SUM
			{
			strncpy (ssort_db [database_index + su_diff->add - 2].sha, sha_line, SHA_LENGTH);	// enter SHA256SUM into database
			fclose (SHA_PIPE);
			ssort_db [database_index + su_diff->add - 2].sha [SHA_LENGTH] = NULL_TERM;
			strcpy (ssort_db [database_index + su_diff->add - 2].dataset, dataset_name);
			if (sfflags->verbose)
				{
				printf ("# %s\n", sha_line);
				}
			}
			else
			{
			printf ("Invalid SHA256SUM from file %s\n%s\n%s\n", ssort_db [database_index + su_diff->add - 2].filepath, sha_line, sha_command);
			exit (1);
			}

		strcpy (ssort_db [database_index + su_diff->add - 2].filepath, fs_list [fs_list [file_index].index].filepath);
		ssort_db [database_index + su_diff->add - 2].index = database_index + su_diff->add - 2;
		if (sfflags->verbose)
			{
			printf("FI=%d\t%s%s%s\tDI=%d\t%s\tSDA=%d\n", file_index, \
						TEXT_ORANGE, fs_list [fs_list [file_index].index].filepath, TEXT_RESET, \
						db_index, \
						ssort_db [ssort_db [db_index].index].filepath, su_diff->add);
			}
		file_index ++;
		if (database_index + su_diff->add + 1 == database_alloc_size)		// check memory usage, reallocate
			{
			database_alloc_size += DATABASE_INCREMENT;
			ssort_db = (struct sha_sort_database *) realloc (ssort_db, sizeof (struct sha_sort_database) * database_alloc_size);
			}
		}
		else if (strcmp (fs_list [fs_list [file_index].index].filepath, ssort_db [ssort_db [db_index].index].filepath) > 0)
		{		// Entry in database has been deleted
		ssort_db [ssort_db [db_index].index].sha [0] = 'x';
		su_diff->rem ++;
		if (sfflags->verbose)
			{
			printf("FI=%d\t%s\tSDR=%d\tDI=%d\t%s%s%s\n", file_index, \
						fs_list [fs_list [file_index].index].filepath, \
						su_diff->rem, db_index, \
						TEXT_ORANGE, ssort_db [ssort_db [db_index].index].filepath, TEXT_RESET);
			}
		db_index ++;
		}
	while (strcmp (fs_list [fs_list [file_index].index].filepath, ssort_db [ssort_db [db_index].index].filepath) == 0)
		{		// No change between file list and database
		su_diff->same ++;
		if (sfflags->verbose)
			{
			printf("FI=%d\t%s\tDI=%d\t%s\tSDS=%d\n", file_index, \
						fs_list [fs_list [file_index].index].filepath, \
						db_index, \
						ssort_db [ssort_db [db_index].index].filepath, su_diff->same);
			}
		file_index ++;
		db_index ++;
		}
	}

// Database resort section
for (line_index = 0; line_index < database_index + su_diff->add - 1; line_index ++)	// print output
	{
	ssort_db [line_index].index = line_index;
	}
swap_made = TRUE;
while (swap_made == TRUE)
	{
	swap_made = FALSE;
	for (line_index = 0; line_index < database_index + su_diff->add - 2; line_index ++)
		{
		if (strcmp (ssort_db [ssort_db [line_index].index].sha, ssort_db [ssort_db [line_index + 1].index].sha) > 0)
			{
			swap_index = ssort_db [line_index + 1].index;
			ssort_db [line_index + 1].index = ssort_db [line_index].index;
			ssort_db [line_index].index = swap_index;
			swap_made = TRUE;
			}
		}
	}

if (outype == 's')
	{
	for (line_index = 0; line_index < database_index + su_diff->add - 1 - su_diff->rem; line_index ++)	// print output
		{
		printf("%s\t%s\t%d\n", ssort_db [ssort_db [line_index].index].sha, \
					ssort_db [ssort_db [line_index].index].filepath, \
					ssort_db [ssort_db [line_index].index].index);
		}
	}

printf("# %s%s%s: %s\t%d entries added, %d entries removed, %d entries unchanged%s\n", TEXT_BLUE, dataset_name, TEXT_RESET, TEXT_YELLOW, su_diff->add, su_diff->rem, su_diff->same, TEXT_RESET);

// Clean-up section
chdir (C_W_D);

//free (database_db);	// free memory
//database_db = NULL;
free (find_list);
find_list = NULL;

}
