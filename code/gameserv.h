#define SYS_COMMAND	0
#define GNUM_PR		1
#define GNUM_CTTO	3

#define CMD_REQ_PLIST	4


#define NAME_LEN    25

#define CMD_NONE	0
#define CMD_MSG		1
#define CMD_ERR		2

#define CBUF_LEN 512
#define CMD_LEN 256


typedef unsigned short pmsg_len;

bool check_windows_socket();
void gameserv_socket(CHAR_DATA *ch);
void gameserv_close(CHAR_DATA *ch);
char *parse_command(char *instr, char *fmt, ...);
void gameserv_command(CHAR_DATA *ch, unsigned char gnum, unsigned char cmd, char *fmt, ...);

// Insert interp functions for modules below
void puerto_interp(CHAR_DATA *ch, char *cmd);
void coloretto_interp(CHAR_DATA *ch, char *cmd);
