#define OBJ_VNUM_CARDS_GENERIC		20110

#define MAX_POKERPLAYER		4
#define MAX_CARDPLAYERS		4

#define CARDS_GAME_HEARTS	1

#define CARDS_HEARTS_TURN_1F	1
#define CARDS_HEARTS_TURN_2F	2
#define CARDS_HEARTS_TURN_3F	3
#define CARDS_HEARTS_TURN_4F	4
#define CARDS_HEARTS_TURN_1A	5
#define CARDS_HEARTS_TURN_2A	6
#define CARDS_HEARTS_TURN_3A	7
#define CARDS_HEARTS_TURN_4A	8
#define CARDS_HEARTS_TURN_1B	9
#define CARDS_HEARTS_TURN_2B	10
#define CARDS_HEARTS_TURN_3B	11
#define CARDS_HEARTS_TURN_4B	12
#define CARDS_HEARTS_PASS	13

#define CARD_QS			43

#define IS_HEART(x)		((x % 4) == 2)

#define CPL			9

typedef struct	poker_data	POKER_DATA;
typedef struct	cards_data	CARDS_DATA;


struct poker_data
{
    long	player_id[MAX_POKERPLAYER];
};

struct cards_data
{
    int		id;
    long	player_id[MAX_CARDPLAYERS];
};
