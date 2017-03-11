/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/
 
/***************************************************************************
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@efn.org)				   *
*	    Gabrielle Taylor						   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

// This is just a test...

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#if defined(unix)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"
#include "db.h"
#include "PyreInfo.h"
#include "EchoAffect.h"
#include "RoomPath.h"
#include "PhantasmInfo.h"
#include "Faction.h"
#include "NameMaps.h"
#include "ExperienceList.h"

extern RUMORS *rumor_free;
extern int g_num_rumors;
RUMORS *new_rumor()
{
    RUMORS *rumor;

    if (rumor_free == NULL)
    {
	rumor = (RUMORS*)alloc_perm(sizeof(*rumor));
	g_num_rumors++;
    }
    else
    { 
	rumor = rumor_free;
	rumor_free = rumor_free->next;
    }
    VALIDATE(rumor);
    rumor->sticky = FALSE;
    rumor->next = rumor_list;
    rumor_list = rumor;
    return rumor;
}

void free_rumor(RUMORS *rumor)
{
    RUMORS *prumor = NULL;
    if (!IS_VALID(rumor))
	return;

    if (rumor == rumor_list)
	rumor_list = rumor_list->next;
    else
    {
	for (prumor=rumor_list;prumor;prumor=prumor->next)
	    if (prumor->next == rumor)
		break;
	if (prumor == NULL)
	{
	    bug("free_rumor: previous rumor not found.", 0 );
	    return;
  	}
	prumor->next = rumor->next;
    }
    free_string( rumor->text);
    free_string( rumor->name);
    INVALIDATE(rumor);

    rumor->next = rumor_free;
    rumor_free = rumor;
}

void rumor_maint()
{
    // Non-sticky rumors get toasted after 1 week
    time_t threshold(current_time - (7 * NUM_SECONDS_DAY));
    RUMORS * prevRumor(NULL);
    RUMORS * nextRumor;
    for (RUMORS * rumor(rumor_list); rumor != NULL; rumor = nextRumor)
    {
        nextRumor = rumor->next;
        if (rumor->sticky || rumor->stamp > threshold)
        {
            prevRumor = rumor;
            continue;
        }
        
        // Rumor has expired, free it
        if (rumor == rumor_list) rumor_list = nextRumor;
        else if (prevRumor != NULL) prevRumor->next = nextRumor;

        free_string(rumor->text);
        free_string(rumor->name);
        INVALIDATE(rumor);
        rumor->next = rumor_free;
        rumor_free = rumor;
    }
    fwrite_rumors();
}

/* stuff for recyling notes */
extern NOTE_DATA *note_free;

NOTE_DATA *new_note()
{
    NOTE_DATA *note;

    if (note_free == NULL)
    {
	note = (NOTE_DATA*)alloc_perm(sizeof(*note));
	g_num_notes++;
    }
    else
    { 
	note = note_free;
	note_free = note_free->next;
    }
    VALIDATE(note);
    return note;
}

void free_note(NOTE_DATA *note)
{
    if (!IS_VALID(note))
	return;

    free_string( note->text    );
    free_string( note->subject );
    free_string( note->to_list );
    free_string( note->date    );
    free_string( note->sender  );
    free_string( note->fake_to );
    free_string( note->forger  );
    note->forged = 0;
    INVALIDATE(note);

    note->next = note_free;
    note_free   = note;
}

    
/* stuff for recycling ban structures */
BAN_DATA *ban_free;

BAN_DATA *new_ban(void)
{
    static BAN_DATA ban_zero = {0};
    BAN_DATA *ban;

    if (ban_free == NULL)
    {
	ban = (BAN_DATA*)alloc_perm(sizeof(*ban));
	g_num_bans++;
    }
    else
    {
	ban = ban_free;
	ban_free = ban_free->next;
    }

    *ban = ban_zero;
    VALIDATE(ban);
    ban->name = &str_empty[0];
    return ban;
}

void free_ban(BAN_DATA *ban)
{
    if (!IS_VALID(ban))
	return;

    free_string(ban->name);
    INVALIDATE(ban);

    ban->next = ban_free;
    ban_free = ban;
}

/* stuff for recycling header structures */
HEADER_DATA *header_free;

HEADER_DATA *new_header(void)
{
    static HEADER_DATA header_zero = {0};
    HEADER_DATA *header;

    if (header_free == NULL)
    {
	header = (HEADER_DATA*)alloc_perm(sizeof(*header));
	g_num_headers++;
    }
    else
    {
	header = header_free;
	header_free = header_free->next;
    }

    *header = header_zero;
    return header;
}

void free_header(HEADER_DATA *header)
{
    free_string(header->name);
    header->next = header_free;
    header_free = header;
}

/* stuff for recycling descriptors */
DESCRIPTOR_DATA *descriptor_free;

DESCRIPTOR_DATA *new_descriptor(void)
{
    int i = 0;
	static DESCRIPTOR_DATA d_zero = {0};
    DESCRIPTOR_DATA *d;

    if (descriptor_free == NULL)
    {
	d = (DESCRIPTOR_DATA*)alloc_perm(sizeof(*d));
	g_num_descriptors++;
    }
    else
    {
	d = descriptor_free;
	descriptor_free = descriptor_free->next;
    }
	
	for (i = 0; i < MAX_SNOOP; ++i)
		d->snoop_by[i] = NULL;
    d->snooped = FALSE;
    *d = d_zero;
    VALIDATE(d);
    return d;
}

void free_descriptor(DESCRIPTOR_DATA *d)
{
    if (!IS_VALID(d))
	return;

    free_string( d->host );
    free(d->outbuf);
    g_outbuf_size -= d->outsize;
    INVALIDATE(d);
    d->next = descriptor_free;
    descriptor_free = d;
}

/* stuff for recycling gen_data */
GEN_DATA *gen_data_free;

GEN_DATA *new_gen_data(void)
{
    static GEN_DATA gen_zero = {0};
    GEN_DATA *gen;

    if (gen_data_free == NULL)
	gen = (GEN_DATA*)alloc_perm(sizeof(*gen));
    else
    {
	gen = gen_data_free;
	gen_data_free = gen_data_free->next;
    }
    *gen = gen_zero;
    VALIDATE(gen);
    return gen;
}

void free_gen_data(GEN_DATA *gen)
{
    if (!IS_VALID(gen))
	return;

    INVALIDATE(gen);

    gen->next = gen_data_free;
    gen_data_free = gen;
} 

/* stuff for recycling extended descs */
extern EXTRA_DESCR_DATA *extra_descr_free;

EXTRA_DESCR_DATA *new_extra_descr(void)
{
    EXTRA_DESCR_DATA *ed;

    if (extra_descr_free == NULL)
    {
	ed = (EXTRA_DESCR_DATA*)alloc_perm(sizeof(*ed));
	top_ed++;
    }
    else
    {
	ed = extra_descr_free;
	extra_descr_free = extra_descr_free->next;
    }

    ed->keyword = &str_empty[0];
    ed->description = &str_empty[0];
    ed->can_edit = TRUE;
    VALIDATE(ed);
    return ed;
}

void free_extra_descr(EXTRA_DESCR_DATA *ed)
{
    if (!IS_VALID(ed))
	return;

    free_string(ed->keyword);
    free_string(ed->description);
    ed->can_edit = TRUE;
    INVALIDATE(ed);
    
    ed->next = extra_descr_free;
    extra_descr_free = ed;
}


/* stuff for recycling affects */
AFFECT_DATA *affect_free;

AFFECT_DATA *new_affect(void)
{
    static AFFECT_DATA af_zero = {0};
af_zero.valid = TRUE;
    AFFECT_DATA *af;

    if (affect_free == NULL)
    {
	af = (AFFECT_DATA*)alloc_perm(sizeof(*af));
	top_affect++;
    }
    else
    {
	af = affect_free;
	affect_free = affect_free->next;
    }

    *af = af_zero;


    VALIDATE(af);
    return af;
}

void free_affect(AFFECT_DATA *af)
{
    if (!IS_VALID(af))
        return;

    if (af->point)
	{
		if (af->type == gsn_generic 	 || af->type == gsn_stringdata
		|| af->type == gsn_suggestion	 || af->type == gsn_finditems
		|| af->type == gsn_delayreaction || af->type == gsn_sacrifice)
		{
			char * temp(static_cast<char*>(af->point));
			free_string(temp);
		}
		else if (af->type == gsn_skill || af->type == gsn_borrow_knowledge) free(af->point);
        else if (af->type == gsn_bloodpyre) delete static_cast<PyreInfo*>(af->point);        
        else if (af->type == EchoAffect::GSN) delete static_cast<EchoAffect*>(af->point);
        else if (af->type == gsn_illusion) extract_obj(static_cast<OBJ_DATA*>(af->point));
		
        af->point = 0;
	}

    INVALIDATE(af);
    af->next = affect_free;
    affect_free = af;
}

/* stuff for recycling events */
EVENT_DATA *event_free = NULL;
int top_event = 0, event_count = 0;

EVENT_DATA *new_event(void)
{
    static EVENT_DATA ev_zero = {0};
    EVENT_DATA *ev;

    if (event_free == NULL)
    {
	ev = (EVENT_DATA*)alloc_perm(sizeof(*ev));
	top_event++;
    }
    else
    {
	ev = event_free;
	event_free = event_free->next;
    }

    event_count++;
    *ev = ev_zero;


    VALIDATE(ev);
    return ev;
}

void free_event(EVENT_DATA *ev)
{
    if (!IS_VALID(ev))
	return;

    INVALIDATE(ev);
    event_count--;
    ev->next = event_free;
    event_free = ev;
}

/* stuff for recycling events */
CHAIN_DATA *chain_free = NULL;
int top_chain = 0, chain_count = 0;

CHAIN_DATA *new_chain(void)
{
    static CHAIN_DATA chain_zero = {0};
    CHAIN_DATA *chain;

    if (chain_free == NULL)
    {
	chain = (CHAIN_DATA*)alloc_perm(sizeof(*chain));
	top_chain++;
    }
    else
    {
	chain = chain_free;
	chain_free = chain_free->next;
    }

    chain_count++;
    *chain = chain_zero;


    VALIDATE(chain);
    return chain;
}

void free_chain(CHAIN_DATA *chain)
{
    if (!IS_VALID(chain))
	return;

    INVALIDATE(chain);
    chain_count--;
    chain->next = chain_free;
    chain_free = chain;
}



/* stuff for recycling objects */
OBJ_DATA *obj_free;

OBJ_DATA *new_obj(void)
{
    static OBJ_DATA obj_zero = {0};
    OBJ_DATA *obj;
	int i;

    if (obj_free == NULL)
    {
	obj = (OBJ_DATA*)alloc_perm(sizeof(*obj));
	object_count++;
    }
    else
    {
	obj = obj_free;
	obj_free = obj_free->next;
    }
    *obj = obj_zero;
    obj->next = NULL;
    obj->next_content = NULL;
    obj->contains = NULL;
    obj->in_obj = NULL;
    obj->on = NULL;
    obj->next_rand = NULL;
    obj->carried_by = NULL;
    obj->extra_descr = NULL;
    obj->affected = NULL;
    obj->pIndexData = NULL;
    obj->in_room = NULL;
    obj->opact = NULL;
    obj->prog_data = NULL;
    memset(obj->objvalue, 0, sizeof(obj->objvalue));
    memset(obj->stringvalue, 0, sizeof(obj->stringvalue)); 
	for (i = 0; i < 3; i++)
		obj->extra_flags[i] = 0;
    VALIDATE(obj);

    return obj;
}

void free_obj(OBJ_DATA *obj)
{
    AFFECT_DATA *paf, *paf_next;
    EXTRA_DESCR_DATA *ed, *ed_next;
    int i;

    if (!IS_VALID(obj))
	return;

    for (paf = obj->affected; paf != NULL; paf = paf_next)
    {
        paf_next = paf->next;
        free_affect(paf);
    }
    obj->affected = NULL;

    for (ed = obj->extra_descr; ed != NULL; ed = ed_next )
    {
        ed_next = ed->next;
        free_extra_descr(ed);
    }
    obj->extra_descr = NULL;

    NameMaps::Remove(*obj);
    free_string( obj->name        ); obj->name = NULL;
    free_string( obj->description ); obj->description = NULL;
    free_string( obj->short_descr ); obj->short_descr = NULL;
    free_string( obj->owner       ); obj->owner = NULL;
    free_string( obj->killed_by	  ); obj->killed_by = NULL;

    for (i = 0; i < MAX_LASTOWNER; i++)
    {
		free_string(obj->lastowner[i]);
        obj->lastowner[i] = NULL;
    }

    for (int i(0); i < MAX_MOBVALUE; ++i)
        free_string(obj->stringvalue[0]);


    INVALIDATE(obj);

    obj->next   = obj_free;
    obj_free    = obj; 
}


/* stuff for recyling characters */
CHAR_DATA *char_free;

CHAR_DATA *new_char (void)
{
    static CHAR_DATA ch_zero = {0};
    CHAR_DATA *ch;
    int i;

    if (char_free == NULL)
    {
	ch = (CHAR_DATA*)alloc_perm(sizeof(*ch));
	g_num_char_data++;
    }
    else
    {
	ch = char_free;
	char_free = char_free->next;
    }

    *ch				= ch_zero;
    VALIDATE(ch);
	ch->affected		= NULL;
	ch->song			= NULL;
	ch->harmony			= NULL;
    setName(*ch, &str_empty[0]);
    ch->pose = NULL;
    ch->short_descr             = &str_empty[0];
    ch->orig_short		= &str_empty[0];
    ch->long_descr              = &str_empty[0];
    ch->orig_long		= &str_empty[0];
    ch->description             = &str_empty[0];
    ch->orig_description	= &str_empty[0];
    setFakeName(*ch, &str_empty[0]);
    setUniqueName(*ch, &str_empty[0]);
    ch->prompt                  = &str_empty[0];
    ch->prefix			= &str_empty[0];
    ch->battlecry		= &str_empty[0];
    ch->logon                   = current_time;
    ch->laston			= current_time;
    ch->lines                   = PAGELEN;
    for (i = 0; i < 4; i++)
        ch->armor[i]            = 100;
    ch->position                = POS_STANDING;
    ch->hit                     = 100;
    ch->max_hit                 = 100;
    ch->mana                    = 100;
    ch->max_mana                = 100;
    ch->move                    = 100;
    ch->max_move                = 100;
    ch->fake_hit		= 0;
    ch->extra_flags		= 0;
    ch->familiar		= NULL;
    ch->recall_to		= NULL;
    ch->mount			= NULL;
    ch->mount_data		= NULL;
    ch->lastnote		= NULL;
    memset(ch->mobvalue, 0, sizeof(ch->mobvalue));
    memset(ch->stringvalue, 0, sizeof(ch->stringvalue));
    ch->speaking		= 0;
    ch->plength			= 0;
    ch->religion		= -1;
    ch->symbols_known		= 7;
    for (i = 0; i < MAX_COIN; i++)
	ch->bank[i] = 0;
    for (i = 0; i < ARROW_TYPES; i++)
        ch->arrows[i]           = 0;
    for (i = 0; i < MAX_STATS; i++)
    {
        ch->perm_stat[i] = 13;
        ch->mod_stat[i] = 0;
        ch->max_stat[i] = 0;
	ch->attr_prime[i] = 0;
    }
    for (i = 0; i < MAX_RESIST; i++)
	ch->resist[i] = 0;
    ch->lastword = NULL;
    ch->ticks_slept = 0;
    ch->experience_list = new ExperienceList;
    return ch;
}


void free_char(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *pRoomIndex;
    int iHash, i;
    MEMORY_DATA *md, *md_next, *md_last;
    TRACK_DATA *pTrack, *next_track, *prev_track;
    CHAR_DATA *vch, *vch_next;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    if (!IS_VALID(ch))
        return;

    if (IS_NPC(ch))
        mobile_count--;

    if (!IS_NPC(ch))
      loading_char = TRUE;
    for (obj = ch->carrying; obj != NULL; obj = obj_next)
    {
	obj_next = obj->next_content;
	extract_obj(obj);
    }
    if (!IS_NPC(ch))
      loading_char = FALSE;

    ch->carrying = NULL;

    for (paf = ch->affected; paf != NULL; paf = paf_next)
    {
	paf_next = paf->next;
	affect_remove(ch,paf);
    }

    ch->affected = NULL;

   /* The problem: tracks have characters pointers, as does the demontrack field, and
	now mob memory. This is broken. Since this shit is never freed, someone
	logs off, and the next person to log on inheirits their tracks, as well
	as grossly nasty demons that want to cut them up, as well as who knows
	what fucked up prog shit. Free _ALL_ this crap. */

    for ( vch = char_list ; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next;
        if (vch->demontrack == ch)
            vch->demontrack = NULL;
        if (vch->tracking == ch)
            vch->tracking = NULL;
        md_last = NULL;
        for (md = vch->memgroup ; md != NULL ; md = md_next )
        {
            md_next = md->next;
            if (md->ch == ch)
            {
            if (md_last)
                md_last->next = md->next;
            else
                vch->memgroup = md->next;
            free(md);
            g_num_mobmemory--;
            break;
            }
            md_last = md;
        }

        for (i = 0; i < MAX_MOBFOCUS; i++)
            if (vch->memfocus[i] == ch)
            {
            if (IS_NPC(vch) && (vch->pIndexData->vnum == MOB_VNUM_DEMON_GUARDIAN))
            {
                act("$n fades away.", vch, NULL, NULL, TO_ROOM);
                extract_char(vch, TRUE);
                continue;
            }
            vch->memfocus[i] = NULL;
            }

        if (vch->guarding == ch)
            vch->guarding = NULL;
    }


    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next)
        {
	    for (i = 0; i < MAX_ROOMFOCUS; i++)
		if (pRoomIndex->roomfocus[i] == ch)
		    pRoomIndex->roomfocus[i] = NULL;

	    if (!pRoomIndex->tracks || pRoomIndex->tracks->ch == NULL)
		continue;

	    if (pRoomIndex->tracks->next->ch == NULL && pRoomIndex->tracks->ch == ch)
	    {
		pTrack = pRoomIndex->tracks;
		pRoomIndex->tracks = pRoomIndex->tracks->next;
	    }

	    prev_track = pRoomIndex->tracks;
	    for (pTrack = pRoomIndex->tracks; pTrack != NULL; prev_track = pTrack, pTrack = next_track)
            {
		next_track = pTrack->next;
		if ( pTrack->ch == ch)
		{
		    if (prev_track != NULL)
			prev_track->next = next_track;
		    else
			pRoomIndex->tracks = next_track;
		}
	    }
        }
    }

    NameMaps::Remove(*ch);
	free_string(ch->name);
	free_string(ch->description);
	free_string(ch->prompt);
	free_string(ch->prefix);
	free_string(ch->short_descr);
	free_string(ch->long_descr);
	free_string(ch->fake_name);
    for (int i(0); i < MAX_MOBVALUE; ++i)
        free_string(ch->stringvalue[0]);

    if (ch->pcdata != NULL)
    	free_pcdata(ch->pcdata);

    ClearPath(ch);
    delete ch->experience_list;

    ch->next = char_free;
    char_free  = ch;

    INVALIDATE(ch);
}

PC_DATA *pcdata_free;

PC_DATA *new_pcdata(void)
{
    int alias;

    static PC_DATA pcdata_zero = {0};
    PC_DATA *pcdata;

    if (pcdata_free == NULL)
    {
        pcdata = (PC_DATA*)alloc_perm(sizeof(*pcdata));
        g_num_pcdata++;
    }
    else
    {
        pcdata = pcdata_free;
        pcdata_free = pcdata_free->next;
    }

    *pcdata = pcdata_zero;

    for (alias = 0; alias < MAX_ALIAS; alias++)
    {
        pcdata->alias[alias] = NULL;
        pcdata->alias_sub[alias] = NULL;
    }

    for (alias = 0; alias < MAX_GODS; alias++)
    {
        pcdata->sacrifices[alias].level=0;
        pcdata->sacrifices[alias].number=0;
    }

    pcdata->buffer = new_buf();
    pcdata->extra_descr = NULL;
    pcdata->faction_standing = new FactionStanding;
    pcdata->familiarname    = NULL;
    pcdata->familiargender  = 0;
 
    VALIDATE(pcdata);
    return pcdata;
}
	

void free_pcdata(PC_DATA *pcdata)
{
    int alias;
    EXTRA_DESCR_DATA *ed, *ed_next;

    if (!IS_VALID(pcdata))
	return;

	free_string(pcdata->last_death_location);
    free_string(pcdata->pwd);
    free_string(pcdata->bamfin);
    free_string(pcdata->bamfout);
    free_string(pcdata->title);
    free_string(pcdata->extitle);
    free_string(pcdata->familiarname);
    free_buf(pcdata->buffer);
    free(pcdata->bitptr);
    g_num_bitptr--;
    free(pcdata->travelptr);
    g_num_travelptr--;
    delete pcdata->faction_standing;
    
    for (alias = 0; alias < MAX_ALIAS; alias++)
	if (pcdata->alias[alias])
	{
	    free_string(pcdata->alias[alias]);
	    free_string(pcdata->alias_sub[alias]);
	}

    for (ed = pcdata->extra_descr; ed; ed = ed_next)
    {
        ed_next = ed->next;
        free_extra_descr(ed);
    }

    delete pcdata->somatic_arts_info;
    delete pcdata->phantasmInfo;

    INVALIDATE(pcdata);
    pcdata->next = pcdata_free;
    pcdata_free = pcdata;
}

/* stuff for setting ids */
long	last_pc_id;
extern long	last_mob_id;


long get_pc_id(void)
{
    int val;

    val = (current_time <= last_pc_id) ? last_pc_id + 1 : current_time;
    last_pc_id = val;
    return val;
}


long get_mob_id(void)
{
    last_mob_id++;
    return last_mob_id;
}

/* procedures and constants needed for buffering */

BUFFER *buf_free;

/* buffer sizes -- be sure to check rgSizeList when changing */
const int buf_size[MAX_BUF_LIST] =
{
    16,32,64,128,256,1024,2048,4096,8192,16384,32768-128
};

/* local procedure for finding the next acceptable size */
/* -1 indicates out-of-boundary error */
int get_size (int val)
{
    int i;

    for (i = 0; i < MAX_BUF_LIST; i++)
	if (buf_size[i] >= val)
	{
	    return buf_size[i];
	}
    
    return -1;
}

BUFFER *new_buf()
{
    BUFFER *buffer;

    if (buf_free == NULL) 
    {
	buffer = (BUFFER*)alloc_perm(sizeof(*buffer));
	g_num_buffer++;
    }
    else
    {
	buffer = buf_free;
	buf_free = buf_free->next;
    }

    buffer->next	= NULL;
    buffer->state	= BUFFER_SAFE;
    buffer->size	= get_size(BASE_BUF);

    buffer->string	= (char*)malloc(buffer->size);
    g_size_bufstrings += buffer->size;
    buffer->string[0]	= '\0';
    VALIDATE(buffer);

    return buffer;
}

BUFFER *new_buf_size(int size)
{
    BUFFER *buffer;
 
    if (buf_free == NULL)
    {
        buffer = (BUFFER*)alloc_perm(sizeof(*buffer));
	g_num_buffer++;
    }
    else
    {
        buffer = buf_free;
        buf_free = buf_free->next;
    }
 
    buffer->next        = NULL;
    buffer->state       = BUFFER_SAFE;
    buffer->size        = get_size(size);
    if (buffer->size == -1)
    {
        bug("new_buf: buffer size %d too large.",size);
        exit(1);
    }
    buffer->string      = (char*)malloc(buffer->size);
    g_size_bufstrings += buffer->size;
    buffer->string[0]   = '\0';
    VALIDATE(buffer);
 
    return buffer;
}


void free_buf(BUFFER *buffer)
{
    if (!IS_VALID(buffer))
	return;

    free(buffer->string);
    g_size_bufstrings -= buffer->size;
    buffer->string = NULL;
    buffer->size   = 0;
    buffer->state  = BUFFER_FREED;
    INVALIDATE(buffer);

    buffer->next  = buf_free;
    buf_free      = buffer;
}


bool add_buf(BUFFER *buffer, const char *string)
{
    int len;
    char *oldstr;
    int oldsize;

    oldstr = buffer->string;
    oldsize = buffer->size;

    if (buffer->state == BUFFER_OVERFLOW) /* don't waste time on bad strings! */
	return FALSE;

    len = strlen(buffer->string) + strlen(string) + 1;

    while (len >= buffer->size) /* increase the buffer size */
    {
	buffer->size 	= get_size(buffer->size + 1);
	{
	    if (buffer->size == -1) /* overflow */
	    {
		buffer->size = oldsize;
		buffer->state = BUFFER_OVERFLOW;
		bug("buffer overflow past size %d",buffer->size);
		return FALSE;
	    }
  	}
    }

    if (buffer->size != oldsize)
    {
	buffer->string	= (char*)malloc(buffer->size);
	g_size_bufstrings += buffer->size;

	strcpy(buffer->string,oldstr);
	free(oldstr);
	g_size_bufstrings -= oldsize;
    }

    strcat(buffer->string,string);
    return TRUE;
}


void clear_buf(BUFFER *buffer)
{
    buffer->string[0] = '\0';
    buffer->state     = BUFFER_SAFE;
}

char *buf_string(BUFFER *buffer)
{
    return buffer->string;
}

