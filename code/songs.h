#include "merc.h"

#define SONG_FUNC( fun )	bool fun( int sn, int level, CHAR_DATA *ch, void *vo, char *txt )

void expand_rystaias_light(ROOM_INDEX_DATA & startRoom, int level);
void do_theilalslastsailing(CHAR_DATA & ch);
void check_embraceofthedeeps_cauterize(CHAR_DATA & ch, int sn);
int gravebeat_total_levels(const CHAR_DATA & ch);
void check_gravebeat_room(CHAR_DATA & ch);
AFFECT_DATA * get_char_song(const CHAR_DATA & ch, int sn);

DECLARE_SONG_FUN( song_noteofshattering );
DECLARE_SONG_FUN( song_eleventhhour	);
DECLARE_SONG_FUN( song_marchofwar	);
DECLARE_SONG_FUN( song_sonicwave	);
DECLARE_SONG_FUN( song_soundbubble	);
DECLARE_SONG_FUN( song_wallsofjericho	);
DECLARE_SONG_FUN( song_cloakofshadows	);
DECLARE_SONG_FUN( song_auraoflight	);
DECLARE_SONG_FUN( song_psalmofhealing	);
DECLARE_SONG_FUN( song_serenadeoflife	);
DECLARE_SONG_FUN( song_disruption	);
DECLARE_SONG_FUN( song_invokesympathy	);
DECLARE_SONG_FUN( song_marchingtune	);
DECLARE_SONG_FUN( song_aegisofmusic	);
DECLARE_SONG_FUN( song_noteofstriking	);
DECLARE_SONG_FUN( song_echoesoffear	);
DECLARE_SONG_FUN( song_discord		);
DECLARE_SONG_FUN( song_songofsoothing	);
DECLARE_SONG_FUN( song_manasong		);
DECLARE_SONG_FUN( song_stuttering	);

DECLARE_SONG_FUN( song_hymntotourach	);

DECLARE_SONG_FUN(song_roaroftheexalted);
DECLARE_SONG_FUN(song_canticleofthelightbringer);
DECLARE_SONG_FUN(song_requiemofthemartyr);

DECLARE_SONG_FUN(song_gravebeat);
DECLARE_SONG_FUN(song_theembraceofthedeeps);
DECLARE_SONG_FUN(song_theilalslastsailing);

static const   struct  song_type   song_table  [] =
{
    { song_noteofshattering,    &gsn_noteofshattering,  INST_TYPE_PERC          },
	{ song_eleventhhour,    &gsn_eleventhhour,  INST_TYPE_PERC          },          { song_marchofwar,      &gsn_marchofwar,    INST_TYPE_PERC|INST_TYPE_BRASS  },
	{ song_sonicwave,       &gsn_sonicwave,     INST_TYPE_PERC          },
	{ song_wallsofjericho,  &gsn_wallsofjericho,    INST_TYPE_BRASS         },
	{ song_noteofstriking,  &gsn_noteofstriking,    INST_TYPE_PERC|INST_TYPE_BRASS  },
	{ song_soundbubble,     &gsn_soundbubble,   INST_TYPE_STRING        },
	{ song_serenadeoflife,  &gsn_serenadeoflife,    INST_TYPE_STRING        },
	{ song_disruption,      &gsn_baneofasjia,    INST_TYPE_STRING|INST_TYPE_BRASS},
	{ song_aegisofmusic,    &gsn_aegisofmusic,  INST_TYPE_STRING        },
	{ song_discord,     &gsn_discord,       INST_TYPE_STRING        },
	{ song_manasong,        &gsn_manasong,      INST_TYPE_STRING        },
	{ song_cloakofshadows,  &gsn_cloakofshadows,    INST_TYPE_WOODWIND      },
	{ song_auraoflight,     &gsn_auraoflight,   INST_TYPE_WOODWIND      },
	{ song_echoesoffear,    &gsn_echoesoffear,  INST_TYPE_WOODWIND|INST_TYPE_BRASS},
	{ song_songofsoothing,  &gsn_songofsoothing,    INST_TYPE_WOODWIND      },
	{ song_invokesympathy,  &gsn_invokesympathy,    INST_TYPE_VOICE         },
	{ song_stuttering,      &gsn_stuttering,    INST_TYPE_VOICE         },
	{ song_hymntotourach,   &gsn_hymntotourach, INST_TYPE_VOICE         },

	// Non-Specialized
	{ song_psalmofhealing,  &gsn_psalmofhealing,    INST_TYPE_VOICE    },
	{ song_marchingtune,    &gsn_marchingtune,  INST_TYPE_VOICE    },

    { song_roaroftheexalted,            &gsn_roaroftheexalted,          INST_TYPE_VOICE},
    { song_canticleofthelightbringer,   &gsn_canticleofthelightbringer, INST_TYPE_VOICE},
    { song_requiemofthemartyr,          &gsn_requiemofthemartyr,        INST_TYPE_STRING},
	
    { song_gravebeat,               &gsn_gravebeat,             INST_TYPE_PERC},
    { song_theembraceofthedeeps,    &gsn_theembraceofthedeeps,  INST_TYPE_VOICE},
    { song_theilalslastsailing,     &gsn_theilalslastsailing,   INST_TYPE_VOICE},
	
	{ NULL,         NULL,           INST_TYPE_VOICE    }
};
