#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#if defined(WIN32)
#include <Winsock2.h>
#else
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif
#include "merc.h"
#include "gameserv.h"

#define DEST_PORT 9886
#define DEST_IP "127.0.0.1"



#define CMD_GREETING	3
#define CMD_REQ_PLIST	4
#define CMD_DATA_PLIST	5

#define MSG_LOGIN_SUCCESS   0


char *parse_command(char *instr, char *fmt, ...)
{
    char *p, *c = instr;
    char *c_ptr;
    long *l_ptr;
    short *h_ptr;
    va_list argp;

    va_start(argp, fmt);

    for (p = fmt; *p != '\0'; p++)
    {
        if (*p != '%')
        {
            if (*p != *c)
                return NULL;
            else
                c++;

            continue;
        }

        switch (*++p)
        {
            case 'c':
                c_ptr = va_arg(argp, char *);
                *c_ptr = *c++;
                break;

            case 's':
                c_ptr = va_arg(argp, char *);
		memcpy(c_ptr, c, strlen(c) + 1);
//                strcpy(c_ptr, c);
                c += strlen(c_ptr) + 1;
                break;

	    case 'l':
		l_ptr = va_arg(argp, long *);
		*l_ptr = *(long *) c;
		c += sizeof(long);
		break;

	    case 'h':
		h_ptr = va_arg(argp, short *);
		*h_ptr = *(short *) c;
		c += sizeof(short);
		break;
		
            default:
                return NULL;
                break;
        }
    }

    va_end(argp);

    return c;
}

void gameserv_command(CHAR_DATA *ch, unsigned char gnum, unsigned char cmd, char *fmt, ...)
{
    char *p, *s;
    pmsg_len msg_len = sizeof(pmsg_len) + (sizeof(unsigned char) * 2);
    va_list argp;
    int i;
    short h;
    char buf[CBUF_LEN];
    unsigned char *buf_pnt = reinterpret_cast<unsigned char*>(buf + sizeof(pmsg_len));

    va_start(argp, fmt);

    *buf_pnt = gnum;
    buf_pnt += sizeof(unsigned char);

    *buf_pnt = cmd;
    buf_pnt += sizeof(unsigned char);

#if defined(PRDEBUG)
    bug("Command: %d", cmd);
#endif
    
    for (p = fmt; *p != '\0'; p++)
    {
	if (*p != '%')
	{
	    *buf_pnt++ = *p;
	    msg_len++;
	    continue;
	}

	switch(*++p)
	{
	    case 'c':
		i = va_arg(argp, int);
		*buf_pnt = (char) i;
		buf_pnt += sizeof(char);
		msg_len += sizeof(char);
		break;

	    case 's':
	        s = va_arg(argp, char *);
		memcpy(buf_pnt, s, strlen(s) + 1);
//		strncpy(buf_pnt, s, strlen(s));
		buf_pnt += strlen(s) + 1;
		msg_len += strlen(s) + 1;
		break;

	    case 'h':
// changing to int based on gcc warning
//		h = va_arg(argp, short);
		h = va_arg(argp, int);
		*(short *) buf_pnt = (short) h;
		buf_pnt += sizeof(short);
		msg_len += sizeof(short);
		break;
	}
    }

    va_end(argp);

    *(pmsg_len *)buf = msg_len;

    if (send(ch->pcdata->gamesock, buf, msg_len, 0) == SOCKET_ERROR)
    {
        bug ("gameserv_command: Error sending command, closing connection", 0);
	gameserv_close(ch);
    }
/*
    else
        bug("Sent command %d.", cmd);
*/

}

void gameserv_interp(CHAR_DATA *ch, char *cmd)
{
    char *p = cmd + sizeof(pmsg_len);
    char *c;
    char buf[CMD_LEN];
    char out_buf[MAX_STRING_LENGTH];

    switch (*p++)
    {
	case GNUM_PR:
	{
	    puerto_interp(ch, p);
	    return;
	}
	break;

	case GNUM_CTTO:
	{
	    coloretto_interp(ch, p);
	    return;
	}
	break;
    }

    switch (*p++)
    {
	case CMD_DATA_PLIST:
	{
	    long list_size;
	    unsigned short i;

	    c = parse_command(p, "%l", &list_size);

	    sprintf(out_buf, "{BPlayers currently connected ({C%ld{B):\n\r", list_size);
	    send_to_char(out_buf, ch);

	    for (i = 0; i < list_size; i++)
	    {
		c = parse_command(c, "%s", buf);
		sprintf(out_buf, "{c%s{x\n\r", buf);
		send_to_char(out_buf, ch);
	    }
	}
	break;
    }

    return;
}

void gameserv_process(CHAR_DATA *ch)
{
    int nbytes;
    char buf[CBUF_LEN];
    char *t, *newbuf;
    char *cmd;
    pmsg_len cmd_size;

    if ((nbytes = recv(ch->pcdata->gamesock, buf, CBUF_LEN - ch->pcdata->cbuf_size, 0)) <= 0)
    {
    }
    else
    {
	if (ch->pcdata->cmd_buf)
	{
	    newbuf = (char*)malloc(ch->pcdata->cbuf_size + nbytes);
	    memcpy(newbuf, ch->pcdata->cmd_buf, ch->pcdata->cbuf_size);
	    t = newbuf + ch->pcdata->cbuf_size;
	    memcpy(t, buf, nbytes);
	    free(ch->pcdata->cmd_buf);
	    ch->pcdata->cmd_buf = newbuf;
	}
	else
	{
	    ch->pcdata->cmd_buf = (char*)malloc(nbytes);
	    memcpy(ch->pcdata->cmd_buf, buf, nbytes);
	}

	ch->pcdata->cbuf_size += nbytes;
	cmd = ch->pcdata->cmd_buf;
	   
        while (ch->pcdata->cbuf_size > 0 && ch->pcdata->cbuf_size >= (cmd_size = *(pmsg_len *) cmd))
        {
            gameserv_interp(ch, cmd);
	    cmd += cmd_size;
	    ch->pcdata->cbuf_size -= cmd_size;
        }

	if (ch->pcdata->cbuf_size > 0)
	{
	    newbuf = (char*)malloc(ch->pcdata->cbuf_size);
	    memcpy(newbuf, cmd, ch->pcdata->cbuf_size);
	    free(ch->pcdata->cmd_buf);
	    ch->pcdata->cmd_buf = newbuf;
	}
	else
	{
	    free(ch->pcdata->cmd_buf);
	    ch->pcdata->cmd_buf = NULL;
	}
    }

    return;
}


void gameserv_socket(CHAR_DATA *ch)
{
    int prsock;
    const char x = 1;
    struct sockaddr_in dest_addr;
    char pwd[25];

    if ((prsock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        bug("gameserv_socket: Error initializing socket.", 0);
	return;
    }

    if (setsockopt(prsock, SOL_SOCKET, SO_REUSEADDR, &x, sizeof(int)) == SOCKET_ERROR)
    {
        bug("gameserv_socket: Error initializing socket: reuseaddr.", 0);
        return;
    }

    if (setsockopt(prsock, SOL_SOCKET, SO_KEEPALIVE, &x, sizeof(int)) == SOCKET_ERROR)
    {
        bug("gameserv_socket: Error initializing socket: keepalive.", 0);
        return;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DEST_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(DEST_IP);
    memset(&(dest_addr.sin_zero), '\0', 8);

    connect(prsock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));

    ch->pcdata->gamesock = prsock;

    sprintf(pwd, "%ld", ch->id);
    gameserv_command(ch, SYS_COMMAND, CMD_GREETING, "GS%c%c%s%s", 0, 1, ch->name, pwd);

    return;
}

void gameserv_close(CHAR_DATA *ch)
{
#if defined(WIN32)
    closesocket(ch->pcdata->gamesock);
#else
    close(ch->pcdata->gamesock);
#endif
    ch->pcdata->gamesock = INVALID_SOCKET;
}

#if defined(WIN32)
bool check_windows_socket()
{
    static bool wsaStarted = FALSE;
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD( 2, 0 );
    int err;

    if (!wsaStarted)
    {
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 )
	    return FALSE;

	wsaStarted = TRUE;
    }

    return TRUE;
}
#endif

