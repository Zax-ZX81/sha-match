/* * * * * * * * * * * * * * * *
 *                             *
 *     SMLib.c       0.31      *
 *                             *
 *     2020-10-31     Zax      *
 *                             *
 * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMLib.h"


void exit_error (char *message_a, char *message_b)	// Print two strings as error messages in orange
{
printf ("%s%s%s%s\n", TEXT_ORANGE, message_a, message_b, TEXT_RESET);
exit (1);
}


char sha_verify (char *fileline)
{
int char_index;
char *char_pos;
const char hex_string [16] = "0123456789abcdef";

for (char_index = 0; char_index < SHA_LENGTH; char_index ++)
	{
	char_pos = strchr (hex_string, fileline [char_index]);
	if (char_pos == NULL)
		{
		return (UNKNOWN_TYPE);
		}
	}
if (fileline [char_index] == SPACE_CHAR)
	{
	return (SHA256_TYPE);
	}
if (fileline [char_index] == TAB_CHAR)
	{
	return (S2DB_TYPE);
	}
return (UNKNOWN_TYPE);
}


char *three_fields (char *field_a, char *field_b, char *field_c)	// Join three fields with TAB delimiters
{
char *out_string = malloc (FILELINE_LENGTH);
char str_term [2] = {'\t','\0'};

strcpy (out_string, field_a);
strcat (out_string, str_term);
strcat (out_string, field_b);
strcat (out_string, str_term);
strcat (out_string, field_c);

return out_string;
}


void separate_fields (char *field_one, char *field_two, char *field_three, char *fileline)
{					// read three column, tab delimited line
int fileline_len;
int tab_offset;
int char_index;
int out_idx = 0;

fileline_len = strlen (fileline) - 1;
tab_offset = -(fileline - strrchr (fileline, TAB_CHAR));
strncpy (field_one, fileline, SHA_LENGTH);
for (char_index = SHA_LENGTH + 1; char_index < tab_offset; char_index ++)
	{
	field_two [out_idx++] = fileline[char_index];
	}
field_two [out_idx] = NULL_TERM;
out_idx = 0;
for (char_index = tab_offset + 1; char_index < fileline_len; char_index ++)
	{
	field_three [out_idx++] = fileline[char_index];
	}
field_three [out_idx] = NULL_TERM;
}


char hex_to_dec (char hex_char)
{
if (hex_char > 57)
	{
	return (hex_char - 87);
	}
	else
	{
	return (hex_char - 48);
	}
}


char *enquote (char *filepath)	// Encapsulate a string in double quotes
{
char *out_string = malloc (FILELINE_LENGTH);
char quote_marks [] = {'"', '\0'};

strcpy (out_string, quote_marks);
strcat (out_string, filepath);
strcat (out_string, quote_marks);

return out_string;
}