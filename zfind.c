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
	};
struct file_sort_list_entry
	{
	char filepath [FILEPATH_LENGTH];
	int index;
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
struct file_sort_list_entry *fs_list;
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
int fs_list_curr_size = 0;
int outer_loop = 0, inner_loop;
int swap_index;

char database_dataset [DATASET_LENGTH] = "";		// holds dataset name
char database_filename [FILEPATH_LENGTH] = "";		// output file name with extension
char fileline [FILELINE_LENGTH] = "";			// holds line from filter file
char switch_chr;
char database_extension [8] = "";			// holds database extension based on flag
char path_sub [FILEPATH_LENGTH];			// holds SHA256SUM file argument
char dir_filter_test [FILEPATH_LENGTH] = "";		// holds composed directory for filter
char C_W_D [FILEPATH_LENGTH];				// base directory of search
char filter_check;
char filter_match = FALSE;
char swap_made = TRUE;

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

// Filter
fs_list = (struct file_sort_list_entry *) malloc (sizeof (struct file_sort_list_entry) * DATABASE_INITIAL_SIZE);
fs_list_curr_size = DATABASE_INITIAL_SIZE;
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
//LOAD FS_LIST with filepath and index
			printf ("%s\t%c\n", find_list [find_list_read].filepath, find_list [find_list_read].object_type);
			printf ("%c\t%d\t%s\t%d\n", find_list [find_list_read].object_type, \
						find_list [find_list_read].filtered, \
						find_list [find_list_read].filepath, \
						find_list [find_list_read].filesize);
			}
		}
	if (OUTPUTINDEX + 1 == fs_list_curr_size)		// allocated more memory if needed
		{
		fs_list_curr_size += DATABASE_INCREMENT;
		fs_list = (struct file_sort_list_entry *) realloc (find_list, sizeof (struct file_sort_list_entry) * fs_list_curr_size);
		}
	}

// Sort
while (swap_made == TRUE)
	{
	swap_made = FALSE;
	for (inner_loop = 0; inner_loop < find_list_write - 1; inner_loop ++)
		{
		if (strcmp (ssort_db [ssort_db [inner_loop].index].filepath, ssort_db [ssort>
			{
			swap_index = find_list [inner_loop + 1].index;
			find_list [inner_loop + 1].index = find_list [inner_loop].index;
			find_list [inner_loop].index = swap_index;
			swap_made = TRUE;
			}
		}
	}
for (line_index = 0; line_index < database_line; line_index ++) // print output
	{
	printf("%s\t%s\t%s\n", ssort_db [ssort_db [line_index].index].sha);
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


/*------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMLib.h"

#define PROG_VERSION "0.22"
#define SHA_ZERO "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
#define ALL_DUPES 'A'
#define ONLY_FIRST 'F'
#define NOT_FIRST 'f'
#define ONLY_LAST 'L'
#define NOT_LAST 'l'
#define ALL_UNIQUE 'u'
#define WITH_COLOUR 'C'
#define WITH_HASH 'H'
#define NO_MARK 'N'
#define NULL_STRING ""

struct sdup_database
	{
	char sha [SHA_LENGTH + 1];
	char filepath [FILEPATH_LENGTH];
	char dataset [DATASET_LENGTH];
	int dup_num;
	};

int main (int argc, char *argv [])

{

FILE *DATABASE_FP;

int arg_no, switch_pos;		// args section
int database_ferr;		// database file error
int database_line = 0;
int last_line;
int dup_count = 0;
int database_alloc_size = DATABASE_INITIAL_SIZE;

char switch_chr;		// args section
char database_filename [FILEPATH_LENGTH] = "";
char database_type;
char fileline [FILELINE_LENGTH];			// input line
char dataset_out = TRUE;
char output_choice = ALL_DUPES;
char mark_first = WITH_COLOUR;
char zero_sha = FALSE;
char current_zero_sha = FALSE;
struct sdup_database *sdup_db;				// sha database for duplicates

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
					dataset_out = FALSE;
					break;
				case NOT_FIRST:
					output_choice = NOT_FIRST;
					mark_first = NO_MARK;
					break;
				case ONLY_FIRST:
					output_choice = ONLY_FIRST;
					mark_first = NO_MARK;
					break;
				case ONLY_LAST:
					output_choice = ONLY_LAST;
					mark_first = NO_MARK;
					break;
				case NOT_LAST:
					output_choice = NOT_LAST;
					mark_first = NO_MARK;
					break;
				case 'm':
					mark_first = WITH_HASH;
					break;
				case 'u':
					output_choice = ALL_UNIQUE;
					zero_sha = TRUE;
					mark_first = NO_MARK;
					break;
				case 'V':
					printf ("SHA Dup version %s\n", PROG_VERSION);
					exit (0);
				case 'z':
					zero_sha = TRUE;
					break;
				default:
					exit_error ("# SHA Dup [dfFlLmuVz] <database file>","");
				}	// END switch
			}	// END for switch_pos
		}	// END if int argv
		else
		{
		strncpy (database_filename, argv [arg_no], FILEPATH_LENGTH);
		}	// END else if int argv
	}	// END for arg_no

// File open section
DATABASE_FP = fopen (database_filename, "r");
if (DATABASE_FP == NULL)
	{
	exit_error ("Can't find Database: ", database_filename);
	}

// Database load section
sdup_db = (struct sdup_database *) malloc (sizeof (struct sdup_database) * database_alloc_size);
do
	{
	database_ferr = (long)fgets (fileline, FILEPATH_LENGTH, DATABASE_FP);
	if (database_line == 0)
		{
		database_type = sha_verify (fileline);
		if (database_type == UNKNOWN_TYPE)
			{
			fclose (DATABASE_FP);
			exit_error ("Unrecognised file type: ", database_filename);
			}
		}
	if (fileline != NULL && database_ferr)
		{
		if (database_type == SHA256_TYPE)		// load standard SHA256SUM output
			{
			strncpy (sdup_db [database_line].sha, fileline, SHA_LENGTH);
			sdup_db [database_line].sha [SHA_LENGTH] = NULL_TERM;
			strcpy (sdup_db [database_line].filepath, fileline + SHA_LENGTH + 2);
			sdup_db [database_line].filepath[strlen (sdup_db [database_line].filepath) - 1] = NULL_TERM;
			strcpy (sdup_db [database_line].dataset, database_filename);	// enter database filename as dataset
			}
			else		// load SHA256DB data
			{
			separate_fields (sdup_db [database_line].sha, sdup_db [database_line].filepath, sdup_db [database_line].dataset, fileline);
			}
		}
		if (database_line > 0 && !strcmp (sdup_db [database_line - 1].sha, sdup_db [database_line].sha))	// not first line and SHA256SUMs match
			{
			if (dup_count == 0)
				{
				sdup_db [database_line - 1].dup_num = 1;
				sdup_db [database_line].dup_num = 2;
				dup_count = 2;
				}
				else
				{
				sdup_db [database_line].dup_num = ++dup_count;
				}
			}
			else
			{
			dup_count = 0;
			}

	if (database_line + 1 == database_alloc_size)		// check memory usage, reallocate
		{
		database_alloc_size += DATABASE_INCREMENT;
		sdup_db = (struct sdup_database *) realloc (sdup_db, sizeof (struct sdup_database) * database_alloc_size);
		}
	database_line ++;
	} while (!feof (DATABASE_FP));
fclose (DATABASE_FP);

last_line = database_line;
for (database_line = 0; database_line <= last_line; database_line++)
	{
	current_zero_sha = strcmp (sdup_db [database_line].sha, SHA_ZERO);
	if (!(!zero_sha && !current_zero_sha))
		{	// zero test
		if (sdup_db [database_line].dup_num)	// Dup number not 0
			{
			if (sdup_db [database_line].dup_num == 1 && (output_choice == ALL_DUPES || output_choice == ONLY_FIRST || output_choice == NOT_LAST))	// first duplicate
				{
				switch (mark_first)
					{
					case WITH_COLOUR:
						printf ("%s%s%s", TEXT_YELLOW, sdup_db [database_line].filepath, TEXT_RESET);
						break;
					case WITH_HASH:
						printf ("#%s", sdup_db [database_line].filepath);
						break;
					default:
						printf ("%s", sdup_db [database_line].filepath);
					}	// end switch
				if (dataset_out)
					{
					printf ("\t%s", sdup_db [database_line].dataset);
					}
				printf ("\n");
				}	// if dup = 1
			if (sdup_db [database_line].dup_num > 1 && (output_choice == ALL_DUPES || output_choice == NOT_FIRST\
					 || output_choice == NOT_LAST || output_choice == ONLY_LAST))	// other duplicates
				{
				switch (output_choice)
					{
					case NOT_LAST:
						if (sdup_db [database_line + 1].dup_num)
							{
							printf ("%s", sdup_db [database_line].filepath);
							if (dataset_out)
								{
								printf ("\t%s", sdup_db [database_line].dataset);
								}
							printf ("\n");
							}
						break;
					case ONLY_LAST:
						if (!sdup_db [database_line + 1].dup_num)
							{
							printf ("%s", sdup_db [database_line].filepath);
							if (dataset_out)
								{
								printf ("\t%s", sdup_db [database_line].dataset);
								}
						printf ("\n");
							}
						break;
					default:
						printf ("%s", sdup_db [database_line].filepath);
						if (dataset_out)
							{
							printf ("\t%s", sdup_db [database_line].dataset);
							}
						printf ("\n");
					}	// end switch
//				printf ("%s\n", sdup_db [database_line].filepath);
				}
			if ((sdup_db [database_line].dup_num == 1 && output_choice == ALL_UNIQUE) || !current_zero_sha)	// output first duplicate if all unique
				{
				printf ("%s\t%s\t%s\n", sdup_db [database_line].sha, sdup_db [database_line].filepath, sdup_db [database_line].dataset);
				}
			}	// end dup found
			else
			{	// no dup found
			if (output_choice == ALL_UNIQUE)
				{
				printf ("%s\t%s\t%s\n", sdup_db [database_line].sha, sdup_db [database_line].filepath, sdup_db [database_line].dataset);
				}
			}	// end no dup
		}	// end zero test
	} // end for loop

free (sdup_db);	// free memory
sdup_db = NULL;
}
*/
