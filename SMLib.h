/* * * * * * * * * * * * * * * *
 *                             *
 *     SMLib.h       0.30      *
 *                             *
 *     2020-10-31     Zax      *
 *                             *
 * * * * * * * * * * * * * * * */

#define TRUE 1
#define FALSE 0
#define SHA_LENGTH 64
#define SHA_OFFSET SHA_LENGTH + 2
#define FILEPATH_LENGTH 1024
#define DATASET_LENGTH 32
#define FILELINE_LENGTH 2048
#define UNKNOWN_TYPE 0
#define SHA256_TYPE 1
#define S2DB_TYPE 2
#define FILE_ARG 2
#define SPACE_CHAR 32
#define TAB_CHAR 9
#define SW_ON 1
#define SW_OFF 0
#define NULL_TERM '\0'
#define SHA256_EXTENSION ".sha256"
#define S2DB_EXTENSION ".s2db"

#if __linux__
#define TEXT_RESET "\33[0m"
#define TEXT_YELLOW "\33[93m"
#define TEXT_BLUE "\33[94m"
#define TEXT_ORANGE "\33[33m"
#endif

#if _WIN32
#define TEXT_RESET ""
#define TEXT_YELLOW ""
#define TEXT_BLUE ""
#define TEXT_ORANGE ""
#endif

struct sha_database
	{
	char sha [SHA_LENGTH + 1];
	char filepath [FILEPATH_LENGTH];
	char dataset [DATASET_LENGTH];
	};

struct sfind_database
	{
	char sha [SHA_LENGTH + 1];
	char filepath [FILEPATH_LENGTH];
	char dataset [DATASET_LENGTH];
	int filesize;
	};

struct shamatch_flags
	{
	char d_out;			// print results from database
	char invert;			// print search file entries that don't match
	char multi;			// print all occurances of the same file that matches
//	char test;			// for testing
	char searchlist_type;		// search list filetype
	char database_type;		// database filetype
	char dataset_match;		// match found for current target
	char dataset_conflict;		// at least one data set conflict found
	char first_line;		// first line of database input
	char shamatch_found;
	};

struct sfind_flags
	{
	char sort;			// sort by either field or not
	char std_out;			// print output to stdout, supress file output
	char database_type;		// database filetype
	char verbose;			// mirror everything to stdout
	char filtering;			// read filter file
	char output_current;
	char progress;
	};

struct hex_lookup_line		// hex lookup table line
	{
	char idx;		// hexadecimal character
	int first;		// first line of hex division
	int last;		// last line of hex division
	};

void exit_error (char *message_a, char *message_b);
char *three_fields (char *field_a, char *field_b, char *field_c);
char sha_verify (char *file_line);
void separate_fields (char *field_one, char *field_two, char *field_three, char *fileline);
char hex_to_dec (char hex_char);
char *enquote (char *filepath);
