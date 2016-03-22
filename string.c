/***************************************************************************
 *  File: string.c                                                         *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/


#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <vector>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <time.h>
#include "merc.h"
#include "olc.h"
#include "StringUtil.h"

extern	void	save_helptext(HELP_DATA *pHelp);

/*****************************************************************************
 Name:		string_edit
 Purpose:	Clears string and puts player into editing mode.
 Called by:	none
 ****************************************************************************/
void string_edit( CHAR_DATA *ch, char **pString )
{
    if (ch->desc == NULL)
		return;

	send_to_char( "-========- Entering EDIT Mode -=========-\n\r", ch );
    send_to_char( "    Type .h on a new line for help\n\r", ch );
    send_to_char( " Terminate with a ~ or @ on a blank line.\n\r", ch );
    send_to_char( "-=======================================-\n\r", ch );

    if ( *pString == NULL )
    {
        *pString = str_dup( "" );
    }
    else
    {
        **pString = '\0';
    }

    ch->desc->pString = pString;

    return;
}

std::string join_from_lines(const std::vector<std::string> & lines)
{
	std::ostringstream result;
    for (size_t i(0); i < lines.size(); ++i)
		result << lines[i];
	return result.str();
}

std::string format_prog(const char * text, size_t indentCount, bool trimEnd)
{
    std::string indent(indentCount, ' ');
	size_t depth(0);
	std::vector<std::string> lines(split_into_lines(text));

	// Iterate over the lines, obtaining depths and applying indents
	bool lastLineWasIf(false);
	for (size_t i(0); i < lines.size(); ++i)
	{
		// Kill the current indent
		while (!lines[i].empty() && lines[i][0] == ' ')
			lines[i].erase(0, 1);

		// Blank lines are ignored
		if (lines[i].empty())
			continue;

		// Find depth changers
		bool ignoreLastDepth(false);
		if (!str_prefix("if", lines[i].c_str()))
		{
			++depth;
			ignoreLastDepth = true;
			lastLineWasIf = true;
		}
		else if (lastLineWasIf && (!str_prefix("or", lines[i].c_str()) || !str_prefix("and", lines[i].c_str())))
		{
			ignoreLastDepth = true;
		}
		else if (!str_prefix("else", lines[i].c_str()))
			ignoreLastDepth = true;
		else
		{
			if (!str_prefix("loop", lines[i].c_str()))
			{
				++depth;
				ignoreLastDepth = true;
			}
			else if (depth > 0 && (!str_prefix("endif", lines[i].c_str()) || !str_prefix("endloop", lines[i].c_str())))
			{
				--depth;
			}
			lastLineWasIf = false;
		}
		
		// Determine the number of indents to apply
		size_t numIndents(depth);
		if (ignoreLastDepth && numIndents > 0)
			--numIndents;

		// Build the indents
		std::string line;
		for (size_t j(0); j < numIndents; ++j)
			line += indent;

		// Apply the resultant string
		line += lines[i];

        // Handle trimming
        if (trimEnd)
        {
            size_t charsToTrim(0);
            for (size_t index(line.size()); index > 0; --index)
            {
                bool blank(false);
                switch (line[index - 1])
                {
                    case ' ':
                    case '\t':
                    case '\r':
                    case '\n': blank = true; break;
                }

                if (!blank)
                    break;

                ++charsToTrim;
            }

            line.resize(line.size() - charsToTrim);
            line += '\n';
        }

        // Assign the string back
		lines[i] = line;
	}

	// Rejoin the lines and return
	return join_from_lines(lines);
}

std::string add_line_numbers(const char * text)
{
	std::ostringstream result;
	result << std::setfill(' ');
	std::vector<std::string> lines(split_into_lines(text));
	
	// Calculate the max width
	size_t maxWidth(1);
	size_t count(lines.size());
	while (count >= 10)
	{
		++maxWidth;
		count /= 10;
	}
	
	// Now apply the numbers
	for (size_t i(0); i < lines.size(); ++i)
		result << std::setw(maxWidth) << i << "| " << lines[i];

	return result.str();
}

void replace_edited_string(CHAR_DATA * ch, const char * text)
{
	// Sanity checks
	assert(ch != NULL);
	assert(ch->desc != NULL);
	assert(ch->desc->pString != NULL);

	// Replace the old string with the new
	free_string(*ch->desc->pString);
	*ch->desc->pString = str_dup(text);
}

void replace_edited_string(CHAR_DATA * ch, const std::vector<std::string> & lines)
{
	replace_edited_string(ch, join_from_lines(lines).c_str());
}

/*****************************************************************************
 Name:		string_append
 Purpose:	Puts player into append mode for given string.
 Called by:	(many)olc_act.c
 ****************************************************************************/
void string_append( CHAR_DATA *ch, char **pString )
{
	if (ch->desc == NULL)
		return;
	
    send_to_char( "-=======- Entering APPEND Mode -========-\n\r", ch );
    send_to_char( "    Type .h on a new line for help\n\r", ch );
    send_to_char( " Terminate with a ~ or @ on a blank line.\n\r", ch );
    send_to_char( "-=======================================-\n\r", ch );

    if ( *pString == NULL )
    {
        *pString = str_dup( "" );
    }
	
	if (IS_SET(ch->act, PLR_SHOWLINES))
	    send_to_char_bw(add_line_numbers(*pString).c_str(), ch );
	else
		send_to_char_bw(*pString, ch);
    
    if ( *(*pString + strlen( *pString ) - 1) != '\r' )
    send_to_char( "\n\r", ch );

    ch->desc->pString = pString;
}

/*****************************************************************************
 Name:		string_replace
 Purpose:	Substitutes one string for another.
 Called by:	string_add(string.c) (aedit_builder)olc_act.c.
 ****************************************************************************/
char * string_replace( char * orig, char * old, char * new_str)
{
    char xbuf[MAX_STRING_LENGTH];
    int i;

    xbuf[0] = '\0';
    strcpy( xbuf, orig );
    if ( strstr( orig, old ) != NULL )
    {
        i = strlen( orig ) - strlen( strstr( orig, old ) );
        xbuf[i] = '\0';
        strcat( xbuf, new_str);
        strcat( xbuf, &orig[i+strlen( old )] );
        free_string( orig );
    }

    return str_dup( xbuf );
}



/*****************************************************************************
 Name:		string_add
 Purpose:	Interpreter for string editing.
 Called by:	game_loop_xxxx(comm.c).
 ****************************************************************************/
void string_add( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    /* Sort of a hack here I suppose */
    if (argument[0] == '\0' || (argument[0] == ' ' && argument[1] == '\0'))
    {
        strcpy( buf, *ch->desc->pString );
        strcat( buf, "\n\r" );
        free_string( *ch->desc->pString );
        *ch->desc->pString = str_dup( buf );
        return;
    }

    /*
     * Thanks to James Seng
     */
    smash_tilde( argument );

    if ( *argument == '.' )
    {
        char arg1 [MAX_INPUT_LENGTH];
        char arg2 [MAX_INPUT_LENGTH];
        char arg3 [MAX_INPUT_LENGTH];

        argument = one_argument( argument, arg1 );
        argument = first_arg( argument, arg2, FALSE );
		char * preArg3(argument);
        argument = first_arg( argument, arg3, FALSE );

        if ( !str_cmp( arg1, ".c" ) )
        {
            send_to_char( "String cleared.\n\r", ch );
	    free_string(*ch->desc->pString);
	    *ch->desc->pString = str_dup("");
//            **ch->desc->pString = '\0';
            return;
        }

        if ( !str_cmp( arg1, ".s" ) )
        {
            send_to_char( "String so far:\n\r", ch );

			if (IS_SET(ch->act, PLR_SHOWLINES))
            	send_to_char_bw(add_line_numbers(*ch->desc->pString).c_str(), ch);
			else
				send_to_char_bw(*ch->desc->pString, ch);
            return;
        }

		// Line deletion
		if (!str_cmp(arg1, ".d"))
		{
			// Make sure they specified a line
			size_t line(0);
			std::istringstream input(arg2);
			if (!(input >> line))
			{
				send_to_char("Usage:  .d <line_number>\n\r", ch);
				return;
			}

			// Split the string into lines and make sure the specified line is valid
			std::vector<std::string> lines(split_into_lines(*ch->desc->pString));
			if (line >= lines.size())
			{
				send_to_char("Invalid line number\n\r", ch);
				return;
			}

			// Remove the actual line, then reapply the string
			lines.erase(lines.begin() + line);
			replace_edited_string(ch, lines);
			std::ostringstream mess;
			mess << "Deleted line " << line << ".\n\r";
			send_to_char(mess.str().c_str(), ch);
			return;
		}

		// Line insertion
		if (!str_cmp(arg1, ".i"))
		{
			// Make sure they specified a line
			// No need to check arg3, because a blank line insert is acceptable
			size_t line(0);
			std::istringstream input(arg2);
			if (!(input >> line))
			{
				send_to_char("Usage:  .i <line_number> [text]\n\r", ch);
				return;
			}

			// Split the string into lines and make sure the specified line is valid
            std::vector<std::string> lines(split_into_lines(*ch->desc->pString));
			if (line >= lines.size())
			{
				send_to_char("Invalid line number\n\r", ch);
				return;
			}

			// Insert the line, then reapply the string
			lines.insert(lines.begin() + line, std::string(preArg3) + "\n\r");
			replace_edited_string(ch, lines);
			std::ostringstream mess;
			mess << "Inserted '" << preArg3 << "' before line " << line << ".\n\r";
			send_to_char(mess.str().c_str(), ch);
			return;
		}

		// Line modification
		if (!str_cmp(arg1, ".m"))
		{
			// Make sure they specified a line
			size_t line(0);
			std::istringstream input(arg2);
			if (!(input >> line))
			{
				send_to_char("Usage:  .m <line_number> [text]\n\r", ch);
				return;
			}

			// Split the string into lines and make sure the specified line is valid
			std::vector<std::string> lines(split_into_lines(*ch->desc->pString));
			if (line >= lines.size())
			{
				send_to_char("Invalid line number\n\r", ch);
				return;
			}

			// Modify the line, then reapply the string
			lines[line] = std::string(preArg3) + "\n\r";
			replace_edited_string(ch, lines);
			std::ostringstream mess;
			mess << "Modified line " << line << " to '" << preArg3 << "'.\n\r";
			send_to_char(mess.str().c_str(), ch);
			return;
		}

        if ( !str_cmp( arg1, ".r" ) )
        {
            if ( arg2[0] == '\0' )
            {
                send_to_char(
                    "usage:  .r \"old string\" \"new string\"\n\r", ch );
                return;
            }

	    smash_tilde( arg3 );   /* Just to be sure -- Hugin */
            *ch->desc->pString =
                string_replace( *ch->desc->pString, arg2, arg3 );
            sprintf( buf, "'%s' replaced with '%s'.\n\r", arg2, arg3 );
            send_to_char_bw( buf, ch );
            return;
        }

		if (IS_IMMORTAL(ch) && (!str_cmp(arg1, ".p") || !str_cmp(arg1, ".pt")))
		{
            size_t indentCount(2);
            if (arg2[0] != '\0')
            {
                std::istringstream input(arg2);
                if (!(input >> indentCount))
                {
                    send_to_char("Usage:  .p [indent_size]\n", ch);
                    send_to_char("Usage:  .pt [indent_size]\n", ch);
                    return;
                }
            }

			replace_edited_string(ch, format_prog(*ch->desc->pString, indentCount, arg1[2] == 't').c_str());
			send_to_char("String formatted as a prog.\n\r", ch);
			return;
		}

        if ( !str_cmp( arg1, ".f" ) )
        {
            *ch->desc->pString = format_string( *ch->desc->pString );
            send_to_char( "String formatted.\n\r", ch );
            return;
        }
        
        if ( !str_cmp( arg1, ".h" ) )
        {
            send_to_char( "Sedit help (commands on blank line):   \n\r", ch );
            send_to_char( ".r 'old' 'new'   - replace a substring \n\r", ch );
            send_to_char( "                   (requires '', \"\") \n\r", ch );
            send_to_char( ".h               - get help (this info)\n\r", ch );
            send_to_char( ".s               - show string so far  \n\r", ch );
            send_to_char( ".i 'line' 'text' - insert before a line\n\r", ch );
            send_to_char( ".m 'line' 'new'  - modify a line 	  \n\r", ch );
            send_to_char( ".d 'line'        - delete a line       \n\r", ch );
            send_to_char( ".f               - (word wrap) string  \n\r", ch );
			
			if (IS_IMMORTAL(ch))
            {
                send_to_char( ".p [indent]      - format as a prog    \n\r", ch );
                send_to_char( ".pt [indent]     - format as a prog and trim trailing spaces\n\r", ch );
            }

            send_to_char( ".c               - clear string so far \n\r", ch );
            send_to_char( "@                - end string          \n\r", ch );
            send_to_char( "~                - end string          \n\r", ch );
            return;
        }
            

        send_to_char( "SEdit:  Invalid dot command.\n\r", ch );
        return;
    }

    if ( *argument == '~' || *argument == '@' )
    {
	if ((ch->desc->editor == ED_HELP) && ch->desc->pEdit && ((HELP_DATA *) (ch->desc->pEdit))->text == *ch->desc->pString)
	    save_helptext((HELP_DATA *) ch->desc->pEdit);

// This is a hack, because I really can't figure out a clean way to do it.
	if (ch->desc->connected == CON_INIT_CHAR)
	    send_to_char("[ Press Enter to continue ]", ch);

        ch->desc->pString = NULL;
        return;
    }

    strcpy( buf, *ch->desc->pString );

    /*
     * Truncate strings to MAX_STRING_LENGTH.
     * --------------------------------------
     */
    if ( strlen( buf ) + strlen( argument ) >= ( MAX_STRING_LENGTH - 4 ) )
    {
        send_to_char( "String too long, last line skipped.\n\r", ch );

	/* Force character out of editing mode. */

	if ((ch->desc->editor == ED_HELP) && (ch->desc->pEdit) && ((HELP_DATA *) (ch->desc->pEdit))->text == *ch->desc->pString)
	    save_helptext((HELP_DATA *) ch->desc->pEdit);

        ch->desc->pString = NULL;
        return;
    }

    /*
     * Ensure no tilde's inside string.
     * --------------------------------
     */
    smash_tilde( argument );

    strcat( buf, argument );
    strcat( buf, "\n\r" );
    free_string( *ch->desc->pString );
    *ch->desc->pString = str_dup( buf );
    return;
}



/*
 * Thanks to Kalgen for the new procedure (no more bug!)
 * Original wordwrap() written by Surreality.
 */
/*****************************************************************************
 Name:		format_string
 Purpose:	Special string formating and word-wrapping.
 Called by:	string_add(string.c) (many)olc_act.c
 ****************************************************************************/
char *format_string( char *oldstring /*, bool fSpace */)
{
    char xbuf[MAX_STRING_LENGTH];
    char xbuf2[MAX_STRING_LENGTH];
    char qbuf[MAX_STRING_LENGTH];
    char *rdesc, *ncheck, *q = qbuf;
    char schar;
    int i = 0;
    bool cap = TRUE;
  
    xbuf[0] = xbuf2[0] = qbuf[0] = 0;
  
    i=0;

    // We're going to begin by looking for quoted text (from notes) and not
    // format it...

    rdesc = oldstring;

    while (*rdesc == '>')
    {
        while (*rdesc != '\n' && *rdesc != '\r' && *rdesc != '\0')
        {
            *q = *rdesc;
            q++;
            rdesc++;
        }

        while (*rdesc == '\n' || *rdesc == '\r')
        {
            *q = *rdesc;
            q++;
            rdesc++;
        }

        *q = 0;
    }
  
    for ( ; *rdesc; rdesc++)
    {
        if (*rdesc=='\n')
        {
            for (ncheck = rdesc + 1; *ncheck == '\n' || *ncheck == '\r'; ncheck++)
            if (*ncheck == '\n')
            {
                xbuf[i] = '\n';
                xbuf[i+1] = '\r';
                xbuf[i+2] = '\n';
                xbuf[i+3] = '\r';
                rdesc += 3;
                i += 4;
                break;
            } 

            if (xbuf[i-1] != ' ' && xbuf[i-1] != '\r')
            {
            xbuf[i]=' ';
            i++;
            }
        }
        else if (*rdesc=='\r')
            ;
        else if (*rdesc==' ')
        {
            if (xbuf[i-1] != ' ')
            {
                xbuf[i]=' ';
            i++;
            }
        }
        else if (*rdesc==')')
        {
            if (xbuf[i-1]==' ' && xbuf[i-2]==' '
            && (xbuf[i-3]=='.' || xbuf[i-3]=='?' || xbuf[i-3]=='!'))
            {
            xbuf[i-2]=*rdesc;
            xbuf[i-1]=' ';
            xbuf[i]=' ';
            i++;
            }
            else
            {
            xbuf[i]=*rdesc;
            i++;
            }
        }
        else if (*rdesc=='.' || *rdesc=='?' || *rdesc=='!')
        {
            if (xbuf[i-1]==' ' && xbuf[i-2]==' '
            && (xbuf[i-3]=='.' || xbuf[i-3]=='?' || xbuf[i-3]=='!'))
            {
            xbuf[i-2]=*rdesc;
            if (*(rdesc+1) != '\"')
            {
                xbuf[i-1]=' ';
                xbuf[i]=' ';
                i++;
            }
            else	    
            {
                xbuf[i-1]='\"';
                xbuf[i]=' ';
                xbuf[i+1]=' ';
                i+=2;
                rdesc++;
            }
            }
            else
            {
            xbuf[i]=*rdesc;
            if (*(rdesc+1) != '\"')
            {
                xbuf[i+1]=' ';
                xbuf[i+2]=' ';
                i += 3;
            }
            else
            {
                xbuf[i+1]='\"';
                xbuf[i+2]=' ';
                xbuf[i+3]=' ';
                i += 4;
                rdesc++;
            }
            }

            cap = TRUE;
        }
        else
        {
            xbuf[i]=*rdesc;
            if ( cap )
            {
            cap = FALSE;
            xbuf[i] = UPPER( xbuf[i] );
            }
            i++;
        }
    }

    xbuf[i]=0;
    strcpy(xbuf2,xbuf);
  
    rdesc=xbuf2;
  
    xbuf[0]=0;

    strcat(xbuf, qbuf);
  
    for ( ; ; )
    {
	for (i=0; i<77; i++)
	{
	    if (!*(rdesc+i))
		break;

	    if (*(rdesc+i) == '\r')
	    {
		schar = *(rdesc+i+1);
		*(rdesc+i+1) = 0;
		strcat(xbuf, rdesc);
		rdesc += i+1;
		*rdesc = schar;
		while (*rdesc == ' ') rdesc++;
		i = 0;
	    }
	}

	if (i<77)
	    break;

	for (i=(xbuf[0]?76:73) ; i ; i--)
	{
	    if (*(rdesc+i)==' ') break;
	}

	if (i)
	{
	    *(rdesc+i)=0;
	    strcat(xbuf,rdesc);
	    strcat(xbuf,"\n\r");
	    rdesc += i+1;
	    while (*rdesc == ' ') rdesc++;
	}
	else
	{
	    bug ("No spaces", 0);
	    *(rdesc+75)=0;
	    strcat(xbuf,rdesc);
	    strcat(xbuf,"-\n\r");
	    rdesc += 76;
	}
    }

    while (*(rdesc+i)
     && (*(rdesc+i)==' '|| *(rdesc+i)=='\n'|| *(rdesc+i)=='\r'))
	i--;

    *(rdesc+i+1)=0;
    strcat(xbuf,rdesc);
    if (xbuf[strlen(xbuf)-2] != '\n')
	strcat(xbuf,"\n\r");

    free_string(oldstring);

    return(str_dup(xbuf));
}



/*
 * Used above in string_add.  Because this function does not
 * modify case if fCase is FALSE and because it understands
 * parenthesis, it would probably make a nice replacement
 * for one_argument.
 */
/*****************************************************************************
 Name:		first_arg
 Purpose:	Pick off one argument from a string and return the rest.
 		Understands quates, parenthesis (barring ) ('s) and
 		percentages.
 Called by:	string_add(string.c)
 ****************************************************************************/
char *first_arg( char *argument, char *arg_first, bool fCase )
{
    char cEnd;

    while ( *argument == ' ' )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"'
      || *argument == '%'  || *argument == '(' )
    {
        if ( *argument == '(' )
        {
            cEnd = ')';
            argument++;
        }
        else cEnd = *argument++;
    }

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
    if ( fCase ) *arg_first = LOWER(*argument);
            else *arg_first = *argument;
	arg_first++;
	argument++;
    }
    *arg_first = '\0';

    while ( *argument == ' ' )
	argument++;

    return argument;
}




/*
 * Used in olc_act.c for aedit_builders.
 */
char * string_unpad( char * argument )
{
    char buf[MAX_STRING_LENGTH];
    char *s;

    s = argument;

    while ( *s == ' ' )
        s++;

    strcpy( buf, s );
    s = buf;

    if ( *s != '\0' )
    {
        while ( *s != '\0' )
            s++;
        s--;

        while( *s == ' ' )
            s--;
        s++;
        *s = '\0';
    }

    free_string( argument );
    return str_dup( buf );
}



/*
 * Same as capitalize but changes the pointer's data.
 * Used in olc_act.c in aedit_builder.
 */
char * string_proper( char * argument )
{
    char *s;

    s = argument;

    while ( *s != '\0' )
    {
        if ( *s != ' ' )
        {
            *s = UPPER(*s);
            while ( *s != ' ' && *s != '\0' )
                s++;
        }
        else
        {
            s++;
        }
    }

    return argument;
}

char *replace_str(char *str, char*orig, char *rep)
{
    static char buffer[4096];
    char *p;

    if (!(p = strstr(str,orig)))
	return str;

    strncpy(buffer,str,p-str);
    buffer[p-str] = '\0';
   
    sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

    return buffer;
} 
