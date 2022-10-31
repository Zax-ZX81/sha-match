/* * * * * * * * * * * * * * * *
 *                             *
 *     SMLib.h       0.40      *
 *                             *
 *     2022-10-22     Zax      *
 *                             *
 * * * * * * * * * * * * * * * */

/*#################################
#	See below for Windows
#	compilation issues
##################################*/

/*Binaries*/
#define TRUE 1
#define FALSE 0
#define SW_ON 1
#define SW_OFF 0

/*Characters and Strings*/
#define NULL_TERM '\0'
#define TAB_CHAR 9
#define RET_CHAR 10		// ### These are used for line endings,
#define CGE_RET 13		// ### including for CertUtil output
#define SPACE_CHAR 32
#define NULL_STRING ""
#define TWO_SPACES "  "

/*Directory Handling*/
#define DIR_CURRENT "."
#define DIR_PREV ".."
#define PATH_CURRENT "./"
#define SLASH_TERM "/"

/*File types*/
#define SHA256_TYPE 1
#define S2DB_TYPE 2
#define UNKNOWN_TYPE 0
#define CERTUTIL_SHA256 3
#define FILE_ARG 2

/*Filename Extensions*/
#define SHA256_EXTENSION ".sha256"
#define S2DB_EXTENSION ".s2db"

/*Database*/
#define DATABASE_CEILING 262144
#define DATABASE_INCREMENT 4096
#define DATABASE_INITIAL_SIZE 4096
#define DATASET_LENGTH 32
#define FILEPATH_LENGTH 1024
#define FILELINE_LENGTH 2048

/*Sort*/
#define SORT_NONE 0
#define SORT_SHA 1
#define SORT_FILE 2
#define SORT_MAX_LINES 80000
#define SWAP_TICK 500

/*Filters*/
#define FILTER_BK ".sf_filter"
#define FILTER_FILE "sf_filter"
#define FILTER_INITIAL_SIZE 256
#define FILTER_INCREMENT 256
#define FILTER_CEILING 4096
#define F_INCL 1
#define F_EXCL 2

/*Filter Codes*/
#define T_COM 'c'
#define T_DIR 'd'
#define T_FIL 'f'
#define T_REJ 'r'
#define DIR_ENTRY 'd'
#define FILE_ENTRY 'f'
#define UNKNOWN_ENTRY 'x'

/*Duplicate Codes*/
#define ALL_DUPES 'A'
#define ALL_UNIQUE 'u'
#define NOT_SET 'n'
#define BY_TIME 't'
#define BY_ALPHA 'a'
#define KEEP_FIRST 'f'
#define DROP_FIRST 'F'
#define KEEP_LAST 'l'
#define DROP_LAST 'L'
#define KEEP_NEWEST 'n'
#define DROP_NEWEST 'N'
#define KEEP_OLDEST 'o'
#define DROP_OLDEST 'O'

/*Marking*/
#define WITH_COLOUR 'C'
#define WITH_HASH 'H'
#define NO_MARK 'N'

/*Update*/
#define REMOVE_MAX 90
#define UPDATE_MIN 10

/*SHA256SUM*/
#define SHA_LENGTH 64
#define SHA_OFFSET SHA_LENGTH + 2
#define SHA_ZERO "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855\n"

#define DIR_TYPE 4
#define FILE_TYPE 8

/*######################################################
#
#	FOR COMPILATION IN WINDOWS
#
#	The OS identifier used in the #if can vary
#	by compiler.
#
#	If compilation isn't working, pay attention
#	to #if __x86_64__ and CR/LF and RET #defs
#	used for sha_verify () in SMLib.c.
#
########################################################*/

#if defined __x86_64__ && !defined __linux__
#define __windows__ 1
#endif

#if __linux__
#define SHA_CMD "sha256sum "
#endif

#if __windows__
#define SHA_CMD "certutil -hashfile "
#define SHA_CMD_ARG " sha256"
#endif

/*Text Colours*/
#if __linux__
#define TEXT_RESET "\33[0m"
#define TEXT_YELLOW "\33[93m"
#define TEXT_BLUE "\33[94m"
#define TEXT_ORANGE "\33[33m"
#define TEXT_RED "\33[1m\33[91m"
#endif

#if __windows__
#define TEXT_RESET ""
#define TEXT_YELLOW ""
#define TEXT_BLUE ""
#define TEXT_ORANGE ""
#define TEXT_RED ""
#endif

struct sha_database
	{
	char sha [SHA_LENGTH + 1];
	char filepath [FILEPATH_LENGTH];
	char dataset [DATASET_LENGTH];
	};

struct sha_sort_database
	{
	char sha [SHA_LENGTH + 1];
	char filepath [FILEPATH_LENGTH];
	char dataset [DATASET_LENGTH];
	int index;
	};

struct sfind_database
	{
	char sha [SHA_LENGTH + 1];
	char filepath [FILEPATH_LENGTH];
	char dataset [DATASET_LENGTH];
	unsigned long filesize;
	int index;
	};

struct sdup_database
	{
	char sha [SHA_LENGTH + 1];
	char filepath [FILEPATH_LENGTH];
	char dataset [DATASET_LENGTH];
	int timestamp;
	int dup_num;
	int index;
	};

struct find_list_entry
	{
	char object_type;
	char filtered;
	char filepath [FILEPATH_LENGTH];
	unsigned long filesize;
	};

struct fs_list_entry
	{
	char filepath [FILEPATH_LENGTH];
	int index;
	};

struct filter_list_entry
	{
	char object_type;
	char filepath [FILEPATH_LENGTH];
	};

struct update_find_list_entry
	{
	char object_type;
	char filtered;
	char filepath [FILEPATH_LENGTH];
	unsigned long filesize;
	int timestamp;
	};

struct uf_sort_list_entry
	{
	char filepath [FILEPATH_LENGTH];
	unsigned long filesize;
	int timestamp;
	int index;
	};

struct hex_lookup_line		// hex lookup table line
	{
	char idx;		// hexadecimal character
	int first;		// first line of hex division
	int last;		// last line of hex division
	};

struct supdate_diff
	{
	int same;		// number of lines unchanged
	int upd;		// number of lines updated
	int rem;		// number of lines removed
	int add;		// number of lines added
	};

struct shamatch_flags
	{
	char d_out;			// print results from database
	char invert;			// print search file entries that don't match
	char multi;			// print all occurances of the same file that matches
	char multi_found;		// multiple found
//	char test;			// for testing
	char searchlist_type;		// search list filetype
	char zero;			// output matches with zero file size
	char database_type;		// database filetype
	char dataset_match;		// match found for current target
	char dataset_conflict;		// at least one data set conflict found
	char verbose;			// print working info to stdout
	char first_line;		// first line of database input
	char shamatch_found;		// sha match found on current pass
	};

struct sfind_flags
	{
	char sort;			// sort by either field or not
	char std_out;			// print output to stdout, supress file output
	char database_type;		// database filetype
	char verbose;			// mirror everything to stdout
	char filtering;			// read filter file
	char output_current;		// current line matches filter
	char progress;			// print progress for SHA256SUM generation
	};

struct supdate_flags
	{
	char show_del;			// read filter file
	char filtering;			// read filter file
	char force;			// force update
	char new_out;			// send to named output file
	char update;			// perform update
	char verbose;			// mirror everything to stdout
	};

struct sdup_flags
	{
	char o_choice;		// from switch for output choice
	char c_scheme;		// alpha for time based choice
	char mark_first;		// mark first in group with colour or hash
	char dataset_out;		// output dataset with report
	char swap_made;			// was swap made during this round
	char verbose;			// mirror everything to stdout
	char zero_sha;			// consider zero size files
	char current_zero_sha;		// current sha is for zero size file
	};


void exit_error (char *message_a, char *message_b);
char *three_fields (char *field_a, char *field_b, char *field_c);
char sha_verify (char *file_line);
void separate_fields (char *field_one, char *field_two, char *field_three, char *fileline);
char hex_to_dec (char hex_char);
char *enquote (char *filepath);
char filter_line_check (char *filter_line);
