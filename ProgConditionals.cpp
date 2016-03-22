#include "ProgConditionals.h"
#include "tables.h"
#include "Luck.h"
#include "Faction.h"
#include "olc.h"
#include "languages.h"
#include "mob_prog.h"

#if defined(WIN32)
#include <cregex.h>
#else
#include <regex.h>
#endif

#define PC_FUN(fun_name) static int fun_name(ProgConditionals::Context & context)

PC_FUN(CheckRand)
{
    if (!is_number(context.arg))
        return -1;

    return (number_percent() <= atoi(context.arg));
}

PC_FUN(CheckCompstr)
{
    if (!is_number(context.arg))
	{
	    bug("Mob: %d, bad argument to 'compstr'.", context.prog->mob->pIndexData->vnum);
	    return -1;
    }

	int i(atoi(context.arg));
	context.proctxt = one_argument(context.proctxt, context.checktxt);
	for (int j = 1; j < i; j++)
	    context.proctxt = one_argument(context.proctxt, context.checktxt);

	return mprog_seval(context.checktxt, context.opr, context.val);
}

PC_FUN(CheckCompx) // compx
{
	char * comparg = context.prog->txt;
	while (comparg[0] != '\0')
	{
	    if (!str_prefix(context.val, comparg))
            return 1;

	    comparg = one_argument(comparg, NULL);
	}

	return 0;
}

PC_FUN(CheckRegex) // regex
{
	regex_t expr;
	int success;

	if (regcomp(&expr, context.val, REG_EXTENDED))
	    return -1;

    success = regexec(&expr, context.prog->txt, 0, NULL, 0);
    regfree(&expr);

    if (!success)
        return 1;
    
    if (success == REG_NOMATCH)
        return 0;
    
    return -1;

}

PC_FUN(CheckExists) // isexists
{
    if (context.arg[1] == 'i')
        return 0;
    
    return 1;
}

PC_FUN(CheckAccountPoints) // acctpoints
{
    if (context.target == NULL)
        return -1;

    int rhsvl = atoi(context.val);
    if (context.target->desc && context.target->desc->acct)
        return mprog_veval(context.target->desc->acct->award_points,context.opr,rhsvl);
    
    return 0;
}

PC_FUN(CheckStrength) // strength
{
    if (context.target == NULL)
        return -1;
    
    int rhsvl = atoi(context.val);
    return mprog_veval(get_curr_stat(context.target, STAT_STR), context.opr, rhsvl);
}

PC_FUN(CheckDexterity) // dexterity
{
    if (context.target == NULL)
        return -1;
    
    int rhsvl = atoi(context.val);
    return mprog_veval(get_curr_stat(context.target, STAT_DEX), context.opr, rhsvl);
}

PC_FUN(CheckConstitution) // constitution
{
    if (context.target == NULL)
        return -1;
    
    int rhsvl = atoi(context.val);
    return mprog_veval(get_curr_stat(context.target, STAT_CON), context.opr, rhsvl);
}

PC_FUN(CheckCharisma) // charisma
{
	if (context.target == NULL)
        return -1;
	
    int rhsvl = atoi(context.val);
	return mprog_veval(get_curr_stat(context.target, STAT_CHR), context.opr, rhsvl);
}

PC_FUN(CheckIntelligence) // intelligence
{
	if (context.target == NULL)
        return -1;
    
    int rhsvl = atoi(context.val);
	return mprog_veval(get_curr_stat(context.target, STAT_INT), context.opr, rhsvl);
}

PC_FUN(CheckWisdom) // wisdom
{
	if (context.target == NULL)
        return -1;
    
    int rhsvl = atoi(context.val);
    return mprog_veval(get_curr_stat(context.target, STAT_WIS), context.opr, rhsvl);
}

PC_FUN(CheckHP) // hp
{
    if (context.target) return mprog_veval(context.target->hit, context.opr, atoi(context.val));
    if (context.prog->mob) return mprog_veval(context.prog->mob->hit, context.opr, atoi(context.val));
    return -1;
}

PC_FUN(CheckMana) // mana
{
	if (context.target) return mprog_veval(context.target->mana, context.opr, atoi(context.val));
	if (context.prog->mob) return mprog_veval(context.prog->mob->mana, context.opr, atoi(context.val));   
	return -1;
}

PC_FUN(CheckMove) // move
{
	if (context.target) return mprog_veval(context.target->move, context.opr, atoi(context.val));
	if (context.prog->mob) return mprog_veval(context.prog->mob->move, context.opr, atoi(context.val));   
	return -1;
}

PC_FUN(CheckSize) // size
{
	if (context.target == NULL)
        return -1;

    int rhsvl;
    if (is_number(context.val)) rhsvl = atoi(context.val);
    else rhsvl = size_lookup(context.val);
	return mprog_veval(context.target->size, context.opr, rhsvl);
}

PC_FUN(CheckRange) // isrange
{
    if (context.target == NULL)
        return -1;
    
    CHAR_DATA *vch = get_char_world(context.target, context.val);
    if (vch) 
        return (IS_PK(vch, context.target));
    
    return -1;
}

PC_FUN(CheckAnyPCArea) // isanypcarea
{
    if (context.prog->mob)
    {
        if (!context.prog->mob->in_room || !context.prog->mob->in_room->area)
        return -1;
        else
        return (!context.prog->mob->in_room->area->empty);
    }
    else if (context.prog->obj && context.prog->obj->in_room && context.prog->obj->in_room->area)
        return (!context.prog->obj->in_room->area->empty);
    else
        return -1;
}

PC_FUN(CheckAnyObjHere) // isanyobjhere
{
	if (context.target && context.target->in_room && context.target->in_room->contents)
	    return 1;
        else if (context.prog->mob && context.prog->mob->in_room && context.prog->mob->in_room->contents)
	    return 1;
	else if (context.prog->obj && context.prog->obj->in_room && context.prog->obj->in_room->contents)
	    return 1;

	return 0;
}

PC_FUN(CheckAnyPCHere) // isanypchere
{
        CHAR_DATA *vch;
	if (context.target && context.target->in_room)
	{
	    for (vch = context.target->in_room->people; vch; vch = vch->next_in_room)
		if (!IS_NPC(vch))
		    return 1;
	}
	else if (context.prog->obj && context.prog->obj->in_room)
	{
	    for (vch = context.prog->obj->in_room->people; vch; vch = vch->next_in_room)
		if (!IS_NPC(vch))
		    return 1;
	}
	else if (context.prog->mob && context.prog->mob->in_room)
	{
	    for (vch = context.prog->mob->in_room->people; vch; vch = vch->next_in_room)
		if (!IS_NPC(vch))
		    return 1;
	}
	else if (context.prog->room)
	{
	    for (vch = context.prog->room->people; vch; vch = vch->next_in_room)
		if (!IS_NPC(vch))
		    return 1;
	}

	return 0;
}

PC_FUN(CheckAnyNPCHere) // isanynpchere
{
        CHAR_DATA *vch;
	if (context.target && context.target->in_room)
	{
	    for (vch = context.target->in_room->people; vch; vch = vch->next_in_room)
		if (IS_NPC(vch))
		    return 1;
	}
	else if (context.prog->obj && context.prog->obj->in_room)
	{
	    for (vch = context.prog->obj->in_room->people; vch; vch = vch->next_in_room)
		if (IS_NPC(vch))
		    return 1;
	}
	else if (context.prog->mob && context.prog->mob->in_room)
	{
	    for (vch = context.prog->mob->in_room->people; vch; vch = vch->next_in_room)
		if (IS_NPC(vch))
		    return 1;
	}
	else if (context.prog->room)
	{
	    for (vch = context.prog->room->people; vch; vch = vch->next_in_room)
		if (IS_NPC(vch))
		    return 1;
	}

	return 0;
}

PC_FUN(CheckCanSee) // cansee
{
	if (context.target && context.rhstarg)
	    return can_see(context.target, context.rhstarg);
	else
	    return -1;
}

PC_FUN(CheckTanking) // tanking
{
	if (context.target)
	{
	    CHAR_DATA *vch;

	    if (context.target->fighting == NULL)
            return 0;

	    for (vch = context.target->in_room->people; vch; vch = vch->next_in_room)
        {
            if (vch->fighting == context.target)
                return 1;
        }

	    return 0;
	}
    return -1;
}

PC_FUN(CheckBitSet) // isbitset
{
	if (context.target)
	{
        int rhsvl = atoi(context.val);
	    return (!IS_NPC(context.target) && BIT_GET(context.target->pcdata->bitptr, rhsvl));
	}
	else return -1;
}

PC_FUN(CheckCriminal) // iscriminal
{
	if (context.target)
	    return (!IS_NPC(context.target) && IS_SET(context.target->act, PLR_REWARD));
	else
	    return -1;
}

PC_FUN(CheckPC) // ispc
{
	if (context.target)
	    return (!IS_NPC(context.target));
	else
	    return -1;
}

PC_FUN(CheckGroup) // isgroup
{
	if (context.target && context.rhstarg)
	    return (is_same_group(context.target, context.rhstarg));
	else
	    return -1;
}

PC_FUN(CheckFollowing) // isfollowing
{
	if (context.target && context.rhstarg)
	    return (context.target->leader == context.rhstarg);
	else
	    return -1;
}

PC_FUN(CheckNPC) // isnpc
{
	if (context.target)
	    return (IS_NPC(context.target));
	else
	    return -1;
}

PC_FUN(CheckPlaying) // isplaying
{
	if (is_number(context.val))
	{
	    int rhsvl = atoi(context.val);
	    if (rhsvl == 0)
		if (context.target->song)
		    return TRUE;
		else
		    return FALSE;
	    else 
		if (context.target->song && context.target->song->type == rhsvl)
		    return TRUE;
		else
		    return FALSE;
	    
	}
	else
	    return -1;    
}

PC_FUN(CheckHarmonizing) // isharmonizing
{
	if (is_number(context.val))
	{
	    int rhsvl = atoi(context.val);
	    if (rhsvl == 0)
		if (context.target->harmony)
		    return TRUE;
		else
		    return FALSE;
	    else 
		if (context.target->harmony && context.target->harmony->type == rhsvl)
		    return TRUE;
		else
		    return FALSE;
	    
	}
	else
	    return -1;    
}

PC_FUN(CheckOpen) // isopen
{
    int rhsvl;
    if (is_number(context.val))
        rhsvl = atoi(context.val);
    else
    {
        if      (!str_prefix(context.val, "north")) rhsvl = 0; 
        else if (!str_prefix(context.val, "east" )) rhsvl = 1;
        else if (!str_prefix(context.val, "south")) rhsvl = 2;
        else if (!str_prefix(context.val, "west" )) rhsvl = 3;
        else if (!str_prefix(context.val, "up"   )) rhsvl = 4;
        else if (!str_prefix(context.val, "down" )) rhsvl = 5;
        else rhsvl = -1;
    }

    if ((rhsvl < 0) && (rhsvl > 5))
    {
    if (context.prog->mob)
        bug("Mob: %d bad rhsvl to isopen", context.prog->mob->pIndexData->vnum);
    else if (context.prog->obj)
    bug("Obj: %d bad rhsvl to isopen", context.prog->obj->pIndexData->vnum);
    else if (context.prog->room)
    bug("Room: %d bad rhsvl to isopen", context.prog->room->vnum);
        return -1;
    }

    if (context.target && context.target->in_room)
    {
        if (context.target->in_room->exit[rhsvl])
            if (IS_SET(context.target->in_room->exit[rhsvl]->exit_info, EX_CLOSED))
                return mprog_veval(0, context.opr, 1);
            else
                return mprog_veval(1, context.opr, 1);
        else
            return 0; // Not open because doesn't exist
    }

    if (context.prog->obj && context.prog->obj->in_room)
    {
            if (context.prog->obj->in_room->exit[rhsvl])
        if (IS_SET(context.prog->obj->in_room->exit[rhsvl]->exit_info, EX_CLOSED))
            return mprog_veval(0, context.opr, 1);
        else
            return mprog_veval(1, context.opr, 1);
        else
        return -1;
    }

    if (context.prog->room)
    {
        if (context.prog->room->exit[rhsvl])
            if (IS_SET(context.prog->room->exit[rhsvl]->exit_info, EX_CLOSED))
            return mprog_veval(0, context.opr, 1);
        else
            return mprog_veval(1, context.opr, 1);
        else
        return -1;
    }

    return -1;
}

PC_FUN(CheckMale) // ismale
{
	if (context.target)
	    return IS_MALE(context.target);
	else
	    return -1;
}

PC_FUN(CheckFemale) // isfemale
{
	if (context.target)
	    return IS_FEMALE(context.target);
	else
	    return -1;
}

PC_FUN(CheckNeuter) // isneuter
{
	if (context.target)
	    return IS_NEUTER(context.target);
	else
	    return -1;
}

PC_FUN(CheckNeutral) // isneutral
{
	if (context.target)
	    return IS_NEUTRAL(context.target);
	else
	    return -1;
}

PC_FUN(CheckWizi) // iswizi
{
	if (context.target)
	{
	    if ((IS_NPC(context.target) && IS_AFFECTED(context.target, AFF_WIZI))
	     || (!IS_NPC(context.target) && (context.target->invis_level >= LEVEL_IMMORTAL)))
		return TRUE;
	    else
		return FALSE;
	}
	else return -1;
}

PC_FUN(CheckRace) // israce
{
	if (context.target)
	    return mprog_seval(race_table[context.target->race].name, context.opr, context.val);
	else
	    return -1;
}

PC_FUN(CheckAuraGrade) // auragrade
{
	if (context.target && !IS_NPC(context.target))
	    return mprog_veval(aura_grade(context.target), context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckPort) // port
{
    return mprog_veval(port, context.opr, atoi(context.val));
}

PC_FUN(CheckMajorSphere) // majorsphere
{
    if (context.target == NULL || IS_NPC(context.target))
        return -1;

    int sphere(sphere_lookup(context.val));
    if (sphere < 0)
        sphere = atoi(context.val);

    return mprog_veval(context.target->pcdata->major_sphere, context.opr, sphere);
}

PC_FUN(CheckMinorSphere) // minorsphere
{
    if (context.target == NULL || IS_NPC(context.target))
        return -1;

    int sphere(sphere_lookup(context.val));
    if (sphere < 0)
        sphere = atoi(context.val);

    return mprog_veval(context.target->pcdata->minor_sphere, context.opr, sphere);
}

PC_FUN(CheckChosenPath) // chosenpath
{
    if (context.target == NULL || IS_NPC(context.target))
        return -1;

    int chosenPath(path_lookup(context.val));
    if (chosenPath < 0)
        chosenPath = atoi(context.val);

    return mprog_veval(context.target->pcdata->chosen_path, context.opr, chosenPath);
}

PC_FUN(CheckHouse) // inhouse
{
	if (context.target)
	    return mprog_seval(clan_table[context.target->clan].who_name, context.opr, context.val);
	else
	    return -1;
}

PC_FUN(CheckFountainHere) // fountainhere    
{
	OBJ_DATA *fountain;
    	for (fountain = context.target->in_room->contents; fountain != NULL; fountain = fountain->next_content)
	{
		if (fountain->item_type == ITEM_FOUNTAIN)
	        	return 1;
	}
	return 0;
}

PC_FUN(CheckInHouse) // isinhouse    
{
	if (context.target && context.target->in_room)
	    return (context.target->in_room->clan == context.target->clan);
	else
	    return -1;
}

PC_FUN(CheckAstral) // isastral
{
	if (context.target)
	    return (is_affected(context.target, gsn_astralprojection));
	else if (context.prog->obj)
	    return (obj_is_affected(context.prog->obj, gsn_astralprojection));
	else
	    return -1;
}

PC_FUN(CheckGhost) // isghost
{
    if (context.target)
    {
        if (IS_OAFFECTED(context.target, AFF_GHOST)) return true;
        if (IS_NPC(context.target) && IS_SET(context.target->nact, ACT_SHADE)) return true;
        return false;
    }
    else
        return -1;
}

PC_FUN(CheckEvil) // isevil
{
	if (context.target)
	    return IS_EVIL(context.target);
	else
	    return -1;
}    

PC_FUN(CheckChaotic) // ischaotic
{
	if (context.target)
	    return IS_CHAOTIC(context.target);
	else
	    return -1;
}

PC_FUN(CheckBalanced) // isbalanced	
{
	if (context.target)
	    return IS_BALANCED(context.target);
	else
	    return -1;
}

PC_FUN(CheckLawful) // islawful
{
	if (context.target)
	    return IS_LAWFUL(context.target);
	else
	    return -1;
}

PC_FUN(CheckFlying) // isflying
{
	if (context.target)
	    return is_flying(context.target);
	else
	    return -1;
}

PC_FUN(CheckOverlimit) // isoverlimit
{
    OBJ_INDEX_DATA * iData;
    if ((iData = get_obj_index(atoi(context.arg))) == NULL)
	{
	    sprintf(context.buf, "%s %d, bad argument to isoverlimit (%s)",
		context.prog->mob ? "Mob" : context.prog->obj ? "Obj" : context.prog->room ? "prog->room" : "???",
		context.prog->mob ? context.prog->mob->pIndexData->vnum :
		context.prog->obj ? context.prog->obj->pIndexData->vnum :
		context.prog->room ? context.prog->room->vnum : -1, context.arg);
	    bug(context.buf, 0);
	    return 1;
	}

	if (iData->limit == 0 || iData->current <= iData->limit)
	    return 0;
	else
	    return 1;
}

PC_FUN(CheckUnderlimit) // isunderlimit
{
    OBJ_INDEX_DATA * iData;
    if ((iData = get_obj_index(atoi(context.arg))) == NULL)
	{
	    sprintf(context.buf, "%s %d, bad argument to isunderlimit (%s)",
		context.prog->mob ? "Mob" : context.prog->obj ? "Obj" : context.prog->room ? "prog->room" : "???",
		context.prog->mob ? context.prog->mob->pIndexData->vnum :
		context.prog->obj ? context.prog->obj->pIndexData->vnum :
		context.prog->room ? context.prog->room->vnum : -1, context.arg);
	    bug(context.buf, 0);
	    return 1;
	}

	if (iData->limit == 0 || iData->current < iData->limit)
	    return 1;
	else
	    return 0;
}

PC_FUN(CheckLimited) // islimited
{
	if (context.prog->obj && context.prog->obj->pIndexData)
	{
	    if (context.prog->obj->pIndexData->limit_factor == 0)
		return 0;
	    else
		return 1;
	}
	else
	    return -1;
}

PC_FUN(CheckMobVCheck) // mobvcheck
{
	if (context.prog->mob && context.target)
	    return mprog_veval(get_mval(context.prog->mob, context.target->name), context.opr, atoi(context.val));
	else if (context.prog->mob && (context.arg[1] == 'x'))
	    return mprog_veval(get_mval(context.prog->mob, context.txt), context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckRemembers) // remembers
{
	if (context.prog->mob && context.target)
	    return mob_remembers(context.prog->mob, context.target->name);
	else if (context.prog->mob && (context.arg[1] == 'x'))
	    return mob_remembers(context.prog->mob, context.txt);
	else
	    return -1;
}

PC_FUN(CheckFocused) // isfocused
{
    int rhsvl;
	if (is_number(context.val))
	    rhsvl = atoi(context.val);
	else
	    rhsvl = 0;

	if (context.target)
	    return (context.target->memfocus[rhsvl] != NULL);
	else if (context.prog->mob)
	    return (context.prog->mob->memfocus[rhsvl] != NULL);
	else if (context.prog->obj)
	    return (context.prog->obj->objfocus[rhsvl] != NULL);
	else if (context.prog->room)
	    return (context.prog->room->roomfocus[rhsvl] != NULL);
	else
	    return -1;
}

PC_FUN(CheckValue) // context.value
{
	if (is_number(context.arg))
	{
	    if (context.prog->mob)
		return mprog_veval( context.prog->mob->mobvalue[atoi(context.arg)], context.opr, atoi(context.val) );
	    else if (context.prog->obj)
    		return mprog_veval( context.prog->obj->objvalue[atoi(context.arg)], context.opr, atoi(context.val) );
	    else if (context.prog->room)
    		return mprog_veval( context.prog->room->roomvalue[atoi(context.arg)], context.opr, atoi(context.val) );
	    else
		return -1;
	}
	else
	{
	    PROG_VARDATA *c_var = mprog_find_pvar(context.arg, context.prog, FALSE);

	    if (c_var)
	    {
		if (is_number(c_var->value))
		    return mprog_veval(atoi(c_var->value), context.opr, atoi(context.val));
		else
		    return mprog_seval(c_var->value, context.opr, context.val);
	    }
	    else
		return -1;
	}
}

PC_FUN(CheckMobValue) // mobvalue
{
	if (context.prog->mob)
	{
	    if (is_number(context.arg))
		return mprog_veval( context.prog->mob->mobvalue[atoi(context.arg)], context.opr, atoi(context.val) );
	    else
	    {
		PROG_VARDATA *c_var = mprog_find_pvar(context.arg, context.prog, FALSE);

		if (c_var)
		{
		    if (is_number(c_var->value))
			return mprog_veval(atoi(c_var->value), context.opr, atoi(context.val));
		    else
			return mprog_seval(c_var->value, context.opr, context.val);
		}
		else
		    return -1;
	    }
	}
	else
	    return -1;
}

PC_FUN(CheckObjValue) // objvalue
{
	if (context.prog->obj)
	{
	    if (is_number(context.arg))
		return mprog_veval( context.prog->obj->objvalue[atoi(context.arg)], context.opr, atoi(context.val) );
	    else
	    {
		PROG_VARDATA *c_var = mprog_find_pvar(context.arg, context.prog, FALSE);

		if (c_var)
		{
		    if (is_number(c_var->value))
			return mprog_veval(atoi(c_var->value), context.opr, atoi(context.val));
		    else
			return mprog_seval(c_var->value, context.opr, context.val);
		}
		else
		    return -1;
	    }
	}
	else
	    return -1;
}

PC_FUN(CheckRoomValue) // roomvalue
{
	if (context.prog->room)
	{
	    if (is_number(context.arg))
		return mprog_veval( context.prog->room->roomvalue[atoi(context.arg)], context.opr, atoi(context.val) );
	    else
	    {
		PROG_VARDATA *c_var = mprog_find_pvar(context.arg, context.prog, FALSE);

		if (c_var)
		{
		    if (is_number(c_var->value))
			return mprog_veval(atoi(c_var->value), context.opr, atoi(context.val));
		    else
			return mprog_seval(c_var->value, context.opr, context.val);
		}
		else
		    return -1;
	    }
	}
	else
	    return -1;
}

PC_FUN(CheckCost) // cost
{
	if (context.prog->obj)
	    return mprog_veval(context.prog->obj->cost, context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckDemonstate) // demonstate
{
	if (context.prog->mob == NULL)
        return -1;
    
    int lhsvl = context.prog->mob->demonstate;
    int rhsvl = atoi(context.val);
    return mprog_veval(lhsvl, context.opr, rhsvl);
}

PC_FUN(CheckIsDay) // isday
{
    int lhsvl = time_info.day;
    int rhsvl = atoi(context.val);
    return mprog_veval( lhsvl, context.opr, rhsvl );
}

PC_FUN(CheckDay) // day
{
	int cday;

        if (context.arg[0] != '\0')
	{
	    if (is_number(context.arg))
		cday = atoi(context.arg);
	    else
	    {
		sprintf(context.buf, "%s %d, bad argument to day ifcheck (%s)",
		    context.prog->mob ? "Mob" : context.prog->obj ? "Obj" : context.prog->room ? "prog->room" : "???",
		    context.prog->mob ? context.prog->mob->pIndexData->vnum :
		    context.prog->obj ? context.prog->obj->pIndexData->vnum :
		    context.prog->room ? context.prog->room->vnum : -1, context.arg);
		bug(context.buf, 0);
		return 0;
	    }
	}
	else
	    cday = TtoDY(current_time);

    int rhsvl;
	if (context.val[0] == '\0')
	{
	    rhsvl = TtoDY(current_time);
	}
	else if (!is_number(context.val))
	{
	    sprintf(context.buf, "%s %d, bad context.value to day ifcheck (%s)",
		context.prog->mob ? "Mob" : context.prog->obj ? "Obj" : context.prog->room ? "prog->room" : "???",
		context.prog->mob ? context.prog->mob->pIndexData->vnum :
		context.prog->obj ? context.prog->obj->pIndexData->vnum :
		context.prog->room ? context.prog->room->vnum : -1, context.val);
	    bug(context.buf, 0);
	    return 0;
	}
	else
	    rhsvl = atoi(context.val);

	return mprog_veval( cday, context.opr, rhsvl );
}

PC_FUN(CheckMonth) // month
{
    int rhsvl;
	if (is_number(context.val))
	    rhsvl = atoi(context.val);
	else
	{
	    sprintf(context.buf, "%s %d, bad context.value to month ifcheck (%s)",
		context.prog->mob ? "Mob" : context.prog->obj ? "Obj" : context.prog->room ? "prog->room" : "???",
		context.prog->mob ? context.prog->mob->pIndexData->vnum :
		context.prog->obj ? context.prog->obj->pIndexData->vnum :
		context.prog->room ? context.prog->room->vnum : -1, context.val);
	    bug(context.buf, 0);
	    return 0;
	}

	return mprog_veval(get_current_month(), context.opr, rhsvl);
}

PC_FUN(CheckSeason) // isseason
{
	return mprog_veval( time_info.season, context.opr, season_lookup(context.val));
}

PC_FUN(CheckPhase) // isphase
{
    int lhsvl = time_info.phase_lunus;
    int rhsvl = atoi(context.val);
    return mprog_veval( lhsvl, context.opr, rhsvl );  
}

PC_FUN(CheckRhosPhase) // rhosphase
{
	int lhsvl = time_info.phase_rhos;
	int rhsvl = atoi(context.val);
	return mprog_veval( lhsvl, context.opr, rhsvl );
}

PC_FUN(CheckRhosSize) // rhossize
{
	int lhsvl = time_info.size_rhos;
	int rhsvl = atoi(context.val);
	return mprog_veval (lhsvl, context.opr, rhsvl );
}

PC_FUN(CheckCloudCover) // cloudcover
{
	int lhsvl = -1;
        if (context.prog->mob && context.prog->mob->in_room)
        {
	    lhsvl = context.prog->mob->in_room->area->w_cur.cloud_cover;
	    if (context.prog->mob->in_room->sector_type == SECT_UNDERGROUND
	    || IS_SET(context.prog->mob->in_room->room_flags,ROOM_NOWEATHER))
		lhsvl = -1;
        }
        else if (context.prog->obj && context.prog->obj->in_room)
	{
	    lhsvl = context.prog->obj->in_room->area->w_cur.cloud_cover;
	    if (context.prog->obj->in_room->sector_type == SECT_UNDERGROUND
	    || IS_SET(context.prog->obj->in_room->room_flags,ROOM_NOWEATHER))
		lhsvl = -1;
	}
	else if (context.prog->room)
	{
	    lhsvl = context.prog->room->area->w_cur.cloud_cover;
	    if (context.prog->room->sector_type == SECT_UNDERGROUND
	    || IS_SET(context.prog->room->room_flags,ROOM_NOWEATHER))
		lhsvl = -1;
	}
    int rhsvl = atoi(context.val);
    return mprog_veval( lhsvl, context.opr, rhsvl );
}

PC_FUN(CheckPrecip) // precip
{
	int lhsvl = -1;
        if (context.prog->mob && context.prog->mob->in_room)
        {
	    lhsvl = context.prog->mob->in_room->area->w_cur.precip_type;
	    if (area_is_affected(context.prog->mob->in_room->area, gsn_icestorm))
		lhsvl = 3;
	    else
		if (context.prog->mob->in_room->area->w_cur.storm_str == 0)
		    lhsvl = -1;
	    if (context.prog->mob->in_room->sector_type == SECT_UNDERGROUND
	    || IS_SET(context.prog->mob->in_room->room_flags,ROOM_NOWEATHER))
		lhsvl = -1;
        }
        else if (context.prog->obj && context.prog->obj->in_room)
	{
	    lhsvl = context.prog->obj->in_room->area->w_cur.precip_type;
	    if (area_is_affected(context.prog->mob->in_room->area, gsn_icestorm))
		lhsvl = 3;
	    else
	    	if (context.prog->obj->in_room->area->w_cur.storm_str == 0)
		    lhsvl = -1;
	    if (context.prog->mob->in_room->sector_type == SECT_UNDERGROUND
	    || IS_SET(context.prog->mob->in_room->room_flags,ROOM_NOWEATHER))
		lhsvl = -1;
	}
	else if (context.prog->room)
	{
	    lhsvl = context.prog->room->area->w_cur.precip_type;
	    if (area_is_affected(context.prog->mob->in_room->area, gsn_icestorm))
		lhsvl = 3;
	    else
		if (context.prog->room->area->w_cur.storm_str == 0)
		    lhsvl = -1;
	    if (context.prog->mob->in_room->sector_type == SECT_UNDERGROUND
	    || IS_SET(context.prog->mob->in_room->room_flags,ROOM_NOWEATHER))
		lhsvl = -1;
	}
    int rhsvl = atoi(context.val);
    return mprog_veval( lhsvl, context.opr, rhsvl );
}

PC_FUN(CheckWeather) // isweather
{
    return 0;
}

PC_FUN(CheckLight) // islight
{
	if ((time_info.hour >= season_table[time_info.season].sun_up)
	 && (time_info.hour < season_table[time_info.season].sun_down))
	    return TRUE;
	else
	    return FALSE;
}

PC_FUN(CheckDayOfWeek) // isdayofweek
{
    int lhsvl = time_info.week;
    int rhsvl = atoi(context.val);
    return mprog_veval(lhsvl, context.opr, rhsvl);
}

PC_FUN(CheckTime) // istime
{
    int lhsvl = time_info.hour;
    int rhsvl = atoi(context.val);
    return mprog_veval( lhsvl, context.opr, rhsvl );
}

PC_FUN(CheckWielding) // iswielding
{
	if (context.target)
	{
	    int rhsvl = atoi(context.val);
	    for (OBJ_DATA * pObj = context.target->carrying; pObj; pObj = pObj->next_content)
	    {
            if ((pObj->pIndexData->vnum == rhsvl) && pObj->worn_on)
                return 1;
	    }
	    return 0;
	}
	else return -1;
}

PC_FUN(CheckAllCarryCount)
{
    if (context.target == NULL)
        return 0;

    int count(0);
    for (OBJ_DATA * obj = context.target->carrying; obj != NULL; obj = obj->next_content)
        ++count;

    return mprog_veval(count, context.opr, atoi(context.val));
}

PC_FUN(CheckCarryCount)
{
    if (context.target == NULL)
        return 0;

    int count(0);
    for (OBJ_DATA * obj = context.target->carrying; obj != NULL; obj = obj->next_content)
    {
        if (can_see_obj(context.target, obj))
            ++count;
    }

    return mprog_veval(count, context.opr, atoi(context.val));
}

PC_FUN(CheckCarrying) // iscarrying
{
    if (context.target == NULL)
        return 0;
   
    if (is_number(context.val))
    {
        // Match by vnum
        int rhsvl = atoi(context.val);
        for (OBJ_DATA * pObj = context.target->carrying; pObj; pObj = pObj->next_content)
        {
            if (pObj->pIndexData->vnum == rhsvl)
                return 1;
        }
        return 0;
    }

    // Match by name
    for (OBJ_DATA * pObj = context.target->carrying; pObj != NULL; pObj = pObj->next_content)
    {
        if (!strcmp(context.val, pObj->name))
            return 1;
    }
    return 0;
}

PC_FUN(CheckGood) // isgood       
{
	if (context.target)
	    return IS_GOOD(context.target);
	else
	    return -1;
}

PC_FUN(CheckFighting) // isfight, isfighting
{
	if (context.target)
	{
	    if (context.rhstarg)
	    {
		if (!str_cmp(context.opr, "!=")) 
		    return (context.target->fighting != context.rhstarg);
		else
		    return (context.target->fighting == context.rhstarg);
	    }
	    else
	        return (context.target->fighting != NULL);
	}
	else
	    return -1;
}

PC_FUN(CheckImmort) // isimmort
{
	if (context.target)
	    return (IS_IMM_TRUST(context.target));
	else
	    return -1;
}

PC_FUN(CheckCharmed) // ischarmed
{
	if (context.target)
	{
	    if (IS_AFFECTED(context.target, AFF_CHARM))
		return 1;
	    else
		return 0;
	}
	else
	    return -1;
}

PC_FUN(CheckTracking) // istracking
{
	if (context.prog->mob && context.target)
	    return (context.prog->mob->tracking == context.target);
	else
	    return -1;
}

PC_FUN(CheckTrack) // istrack 
{
	if (context.target)
	    return (context.target->tracking != NULL);
	else
	    return -1;
}

PC_FUN(CheckFollow) // isfollow
{
	if (context.target)
	    return ( context.target->master && 
		     (context.target->master->in_room == context.target->in_room));
	else
	    return -1;
}

PC_FUN(CheckWearSlotFull) // iswearslotfull
{
	if (context.target)
	{
        int rhsvl = atoi(context.val);
	    return (get_eq_char(context.target, rhsvl) != NULL);
	}
	else return -1;
}		

PC_FUN(CheckIndoors) // isindoors
{
	if (context.target && context.target->in_room)
	    if (IS_SET(context.target->in_room->room_flags, ROOM_INDOORS))
		return 1;
	    else
		return 0;
	else
	    return -1;
}

PC_FUN(CheckMaster) // ismaster
{
	if (context.target && context.prog->mob)
	    return (context.prog->mob->master == context.target);
	else
	    return -1;
}

PC_FUN(CheckRoomAffected) // roomaffected
{
	int rhsvl = atoi(context.val);

	if (context.target)
	    return ( room_is_affected(context.target->in_room, rhsvl) );
	else if (context.prog->obj)
	    return ( room_is_affected(context.prog->obj->carried_by ? context.prog->obj->carried_by->in_room : context.prog->obj->in_room, rhsvl) );
	else
	    return -1;
}

PC_FUN(CheckSNAffected) // snaffected
{
    if (context.target || context.prog->obj)
    {
        int sn;
        if ((sn = atoi(context.val)) < 1)
            return -1;

        bool present(false);
    
        if (context.target)
          present = is_affected(context.target, sn);
        else if (context.prog->obj)
          present = obj_is_affected(context.prog->obj, sn);

        if (strcmp(context.opr, "!=") == 0)
            return !present;

        return present;
    }
    else 
        return -1;
}

PC_FUN(CheckAffected) // isaffected
{
	if (context.target)
	    return (context.target->affected_by & atoi( context.val ) );
	else
	    return -1;
}


PC_FUN(CheckOAffected) // isoaffected
{
	if (context.target)
	    return (context.target->oaffected_by & atoi( context.val ) );
	else
	    return -1;
}

PC_FUN(CheckNAffected) // isnaffected
{
	if (context.target)
	    return (context.target->naffected_by & atoi( context.val ) );
	else
	    return -1;
}

PC_FUN(CheckPAffected) // ispaffected
{
	if (context.target)
	    return (context.target->paffected_by & atoi( context.val ) );
	else
	    return -1;
}

PC_FUN(CheckHitPercent) // hitprcnt
{
	if (context.target)
	{
	    int lhsvl = context.target->hit*100 / context.target->max_hit;
	    int rhsvl = atoi( context.val );
	    return mprog_veval(lhsvl, context.opr, rhsvl);
	}
	else return -1;
}

PC_FUN(CheckObjHere) // objhere
{
	int lhsvl = atoi(context.arg);
	if (context.prog->mob && context.prog->mob->in_room)
	{
	    for (OBJ_DATA * pObj = context.prog->mob->in_room->contents; pObj; pObj = pObj->next_content)
		if (pObj->pIndexData->vnum == lhsvl)
		    return 1;
	}
	else if (context.prog->obj && context.prog->obj->in_room)
	{
	    for (OBJ_DATA * pObj = context.prog->obj->in_room->contents; pObj; pObj = pObj->next_content)
		if (pObj->pIndexData->vnum == lhsvl)
		    return 1;
	}
	else if (context.prog->obj && context.prog->obj->carried_by && context.prog->obj->carried_by->in_room)
	{
	    for (OBJ_DATA * pObj = context.prog->obj->carried_by->in_room->contents; pObj; pObj = pObj->next_content)
		if (pObj->pIndexData->vnum == lhsvl)
		    return 1;
	}
	else if (context.prog->room)
	{
	    for (OBJ_DATA * pObj = context.prog->room->contents; pObj; pObj = pObj->next_content)
		if (pObj->pIndexData->vnum == lhsvl)
		    return 1;
	}

	return 0;
}

PC_FUN(CheckMobHere) // mobhere
{
	CHAR_DATA *vch;
	int lhsvl = atoi(context.arg);
	if (context.prog->mob && context.prog->mob->in_room)
	{
	    for (vch = context.prog->mob->in_room->people; vch; vch = vch->next_in_room)
		if (IS_NPC(vch) && (vch->pIndexData->vnum == lhsvl))
		    return 1;
	}
	else if (context.prog->obj && context.prog->obj->in_room)
	{
	    for (vch = context.prog->obj->in_room->people; vch; vch = vch->next_in_room)
		if (IS_NPC(vch) && (vch->pIndexData->vnum == lhsvl))
		    return 1;
	}
	else if (context.prog->obj && context.prog->obj->carried_by && context.prog->obj->carried_by->in_room)
	{
	    for (vch = context.prog->obj->carried_by->in_room->people; vch; vch = vch->next_in_room)
		if (IS_NPC(vch) && (vch->pIndexData->vnum == lhsvl))
		    return 1;
	}
	else if (context.prog->room)
	{
	    for (vch = context.prog->room->people; vch; vch = vch->next_in_room)
		if (IS_NPC(vch) && (vch->pIndexData->vnum == lhsvl))
		    return 1;
	}

	return 0;
}

PC_FUN(CheckAdrenaline) // adrenaline
{
	if (context.target)
	{
	    if (IS_NPC(context.target))
            return 0;
        int rhsvl = atoi(context.val);
	    return mprog_veval(context.target->pcdata->adrenaline, context.opr, rhsvl);
	}
	else return -1;
}

PC_FUN(CheckHere) // ishere
{
	if (context.prog->mob && context.target)
	    return (context.prog->mob->in_room == context.target->in_room);
	else if (context.prog->obj && context.prog->obj->carried_by && context.target)
	    return (context.prog->obj->carried_by->in_room == context.target->in_room);
	else if (context.prog->obj && context.prog->obj->in_room && context.target)
	    return (context.prog->obj->in_room == context.target->in_room);
	else if (context.prog->room && context.target)
	    return (context.prog->room == context.target->in_room);
	else
	    return -1;
}

PC_FUN(CheckInRoom) // inroom
{
	if (context.target && context.target->in_room)
	    return mprog_veval(context.target->in_room->vnum, context.opr, atoi(context.val));
	else if (context.prog->obj && context.prog->obj->in_room)
	    return mprog_veval(context.prog->obj->in_room->vnum, context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckInArea) // inarea
{
    OBJ_DATA * pObj(NULL);
	if (context.target && context.target->in_room && context.target->in_room->area)
	    if (context.prog->mob && context.prog->mob->in_room && context.prog->mob->in_room->area)
		return (context.prog->mob->in_room->area == context.target->in_room->area);
	    else if (!context.prog->mob && context.prog->obj)
	    {
		if (context.prog->obj->in_obj)
		    for (pObj = context.prog->obj->in_obj; pObj->in_obj; pObj = pObj->in_obj)
			;
		else
		    pObj = context.prog->obj;

		if (pObj->carried_by)
		    if (pObj->carried_by->in_room && pObj->carried_by->in_room->area)
		    	return (pObj->carried_by->in_room->area == context.target->in_room->area);
		    else
			return 0;
		else if (pObj->in_room)
		    if (pObj->in_room->area)
			return (pObj->in_room->area == context.target->in_room->area);
		    else
			return 0;
		else
		    return -1;
	    }
	    else
		return -1;
	else
	    return -1;
}

PC_FUN(CheckRoomFlag) // roomflag
{
	if (context.target && context.target->in_room)
	    return mprog_veval(IS_SET(context.target->in_room->room_flags, flag_value(room_flags, context.val)) ? 1 : 0, context.opr, TRUE);
	else if (context.prog->obj && context.prog->obj->in_room)
	    return mprog_veval(IS_SET(context.prog->obj->in_room->room_flags, flag_value(room_flags, context.val)) ? 1 : 0, context.opr, TRUE);
	else if (context.prog->mob && context.prog->mob->in_room)
	    return mprog_veval(IS_SET(context.prog->mob->in_room->room_flags, flag_value(room_flags, context.val)) ? 1 : 0, context.opr, TRUE);
	else
	    return -1;
}

PC_FUN(CheckHasBoat) // hasboat
{
	if (context.target)
	    return (has_boat(context.target));
	else
	    return -1;
}

PC_FUN(CheckSector) // sector
{
	if (context.target && context.target->in_room) 
	    return mprog_veval(context.target->in_room->sector_type, context.opr, flag_value(sector_flags, context.val));
	else if (context.prog->obj && context.prog->obj->in_room)
	    return mprog_veval(context.prog->obj->in_room->sector_type, context.opr, flag_value(sector_flags, context.val));
	else
	    return -1;
}

PC_FUN(CheckSex) // sex
{
	if (context.target)
	    return mprog_veval(context.target->sex, context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckPosition) // position
{
	if (context.target)
	    return mprog_veval(context.target->position, context.opr, atoi(context.val));
	else
	    return -1; 
}

PC_FUN(CheckLevel) // level
{
	if (context.target)
	    return mprog_veval(get_trust(context.target), context.opr, atoi(context.val));
	else if (context.prog->obj)
	    return mprog_veval(context.prog->obj->level, context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckClass) // class
{
	if (context.target)
	    return mprog_veval(context.target->class_num, context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckNewbie) // newbie    
{
	if (context.target && !IS_NPC(context.target))
	    return IS_SET(context.target->nact,PLR_NEWBIE) ? 1 : 0;
	else
	    return -1;
}

PC_FUN(CheckGoldAmount) // goldamt
{
	if (context.target)
	    return mprog_veval((int) coins_to_value(context.target->coins), context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckAnimal) // isanimal
{
	if (context.target && context.target->pIndexData)
	{
	    if (IS_SET(context.target->pIndexData->act, ACT_ANIMAL) || IS_SET(context.target->pIndexData->form, FORM_ANIMAL))
		return 1;
	    else
		return 0;
	}
	else if (context.target == NULL && context.prog->mob && context.prog->mob->pIndexData)
	{
	    if (IS_SET(context.prog->mob->pIndexData->act, ACT_ANIMAL) || IS_SET(context.prog->mob->pIndexData->form, FORM_ANIMAL))
		return 1;
	    else
		return 0;
	}
	else
	    return -1;
}

PC_FUN(CheckMaterial) // material
{
	if (context.prog->obj)
	    return mprog_seval(material_table[context.prog->obj->material].name, context.opr, context.val);
	else
	    return -1;
}

PC_FUN(CheckObjType) // objtype
{
	if (context.prog->obj)
        {
	    if (is_number(context.val))
	        return mprog_veval(context.prog->obj->item_type, context.opr, atoi(context.val));
	    else
		return mprog_veval(context.prog->obj->item_type, context.opr, flag_value(type_flags, context.val));
	}
	else
	    return -1;
}

PC_FUN(CheckObjVal0) // objval0
{
	if (context.prog->obj)
	    return mprog_veval(context.prog->obj->value[0], context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckObjVal1) // objval1
{
	if (context.prog->obj)
	    return mprog_veval(context.prog->obj->value[1], context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckObjVal2) // objval2
{
	if (context.prog->obj)
	    return mprog_veval(context.prog->obj->value[2], context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckObjVal3) // objval3
{
	if (context.prog->obj)
	    return mprog_veval(context.prog->obj->value[3], context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckNumber) // number
{
	if (context.target)
	{
	    if (!IS_NPC(context.target))
		return 0;
	    return mprog_veval(context.target->pIndexData->vnum, context.opr, atoi(context.val));
	}
	else if (context.prog->obj)
	    return mprog_veval(context.prog->obj->pIndexData->vnum, context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckWeight) // weight
{
	if (context.target)
	    return mprog_veval(get_carry_weight(*context.target), context.opr, atoi(context.val));
	else if (context.prog->obj)
	    return mprog_veval(context.prog->obj->weight, context.opr, atoi(context.val));
	else
	    return -1;
}

PC_FUN(CheckValName) // context.valname
{
	if (context.target && context.rhstarg)
	    return mprog_seval(context.target->name, context.opr, context.rhstarg->name);
	
    if (context.target && (context.val[1] == 'x'))
	    return mprog_seval(context.target->name, context.opr, context.txt);
	
	return -1;
}

PC_FUN(CheckName) // name
{
	if (context.target)
    {
        if (context.rhstarg)
            return mprog_seval(context.target->name, context.opr, context.rhstarg->name);

        return (mprog_seval(context.target->name, context.opr, context.val) || mprog_seval(context.target->unique_name, context.opr, context.val));
    }
	
    if (context.prog->obj)
	    return (mprog_seval(context.prog->obj->name, context.opr, context.val));
	
    return -1;
}

PC_FUN(CheckLanguage) // language
{
	if (context.target)
	    return mprog_seval(lang_data[context.target->speaking].name, context.opr, context.val);
	else
	    return -1;
}

PC_FUN(CheckHasRelief) // hasrelief
{
	OBJ_DATA *relief = NULL;
	bool has1, has2, has3, has4;

	if (context.target)
	{
	    has1 = has2 = has3 = has4 = FALSE;
	    for (relief = context.target->carrying; relief; relief = relief->next_content)
	    {
		if (relief->pIndexData->vnum == OBJ_VNUM_RELIEF_1)
		    has1 = TRUE;
		if (relief->pIndexData->vnum == OBJ_VNUM_RELIEF_2)
		    has2 = TRUE;
		if (relief->pIndexData->vnum == OBJ_VNUM_RELIEF_3)
		    has3 = TRUE;
		if (relief->pIndexData->vnum == OBJ_VNUM_RELIEF_4)
		    has4 = TRUE;
	    }

	    if (has1 && has2 && has3 && has4)
		return 1;
	    else
		return 0;
	}
	else
	    return -1;
}

PC_FUN(CheckOwner) // isowner, isheld
{
	if (context.prog->obj)
	    if (context.target)
	        return (context.prog->obj->carried_by == context.target);
	    else
		return 0;
	else
	    return -1;
}

PC_FUN(CheckIsWorn) // isworn
{
	if (context.prog->obj)
	    if (context.target)
		return ((context.prog->obj->carried_by == context.target) && context.prog->obj->worn_on && !IS_SET(context.prog->obj->worn_on, WORN_CONCEAL1) && !IS_SET(context.prog->obj->worn_on, WORN_CONCEAL2));
	    else
		return 0;
	else
	    return -1;
}

PC_FUN(CheckInContainer) // incontainer
{
	int lhsvl = atoi(context.arg);
	if (context.prog->obj)
	{
	    if (lhsvl >= 1)
	    {
	        for (OBJ_DATA * pObj = context.prog->obj->contains; pObj; pObj = pObj->next_content)
		    if (pObj->pIndexData->vnum == lhsvl)
		        return 1;
	    }
	    else if (context.prog->obj->contains)
		return 1;

	    return 0;
	}
	else
	    return -1;
}

PC_FUN(CheckObjTargetHere) // objtargethere
{
	if (context.prog->actor && context.prog->obj)
	{
	    context.proctxt = one_argument(context.proctxt, context.buf);
	    context.proctxt = one_argument(context.proctxt, context.buf);

	    if (context.buf[0] == '\0')
		return 0;

	    return (get_obj_here(context.prog->actor, context.buf) == context.prog->obj);
	}
	else
	    return -1;
}

PC_FUN(CheckObjTargetCarry) // objtargetcarry
{
    if (context.prog->actor && context.prog->obj)
    {
        context.proctxt = one_argument(context.proctxt, context.buf);
        context.proctxt = one_argument(context.proctxt, context.buf);

        if (context.buf[0] == '\0')
        return 0;

        return (get_obj_carry(context.prog->actor, context.buf, context.prog->actor) == context.prog->obj);
    }
    else
        return -1;
}

PC_FUN(CheckObjTargetWear) // objtargetwear
{
	if (context.prog->actor && context.prog->obj)
	{
	    context.proctxt = one_argument(context.proctxt, context.buf);
	    context.proctxt = one_argument(context.proctxt, context.buf);

	    if (context.buf[0] == '\0')
		return 0;

	    return (get_obj_wear(context.prog->actor, context.buf) == context.prog->obj);
	}
	else
	    return -1;
}

PC_FUN(CheckMobTargetRoom) // mobtargetroom
{
	if (context.prog->actor && context.prog->mob)
	{
	    context.proctxt = one_argument(context.proctxt, context.buf);
	    context.proctxt = one_argument(context.proctxt, context.buf);

	    if (context.buf[0] == '\0')
		return 0;

	    return (get_char_room(context.prog->actor, context.buf) == context.prog->mob);
	}
	else
	    return -1;
}

PC_FUN(CheckMobTargetWorld) // mobtargetworld
{
	if (context.prog->actor && context.prog->mob)
	{
	    context.proctxt = one_argument(context.proctxt, context.buf);
	    context.proctxt = one_argument(context.proctxt, context.buf);

	    if (context.buf[0] == '\0')
		return 0;

	    return (get_char_world(context.prog->actor, context.buf) == context.prog->mob);
	}
	else
	    return -1;
}

PC_FUN(CheckLagged) // lagged
{
	if (context.target)
	    return (context.target->wait > 0) ? 1 : 0;
	else
	    return -1;
}

PC_FUN(CheckHasPath) // haspath
{
	if (context.target)
	   return context.target->path ? 1 : 0;
	else
	    return -1;
}

PC_FUN(CheckHometown) // ishometown
{
	if (context.target)
	{
	    if (context.target->recall_to == NULL)
		return 0;

	    for (int i = 0; i < MAX_HOMETOWN; i++)
		if (home_table[i].vnum == context.target->recall_to->vnum)
		    return mprog_seval(home_table[i].name, context.opr, context.val);

	    return 0;
	}
	else
	    return -1;
}

PC_FUN(CheckShopOpen) // shopopen    
{
	if (context.prog->mob && context.prog->mob->pIndexData->pShop)
	{
	    if (time_info.hour >= context.prog->mob->pIndexData->pShop->open_hour)
	        if (time_info.hour < context.prog->mob->pIndexData->pShop->close_hour)
		    return 1;
	    return 0;
	}        
    return -1;
}

PC_FUN(CheckFriend) // isfriend    
{
    if (context.target == NULL)
        return -1;
   
    if (context.val[0] == '\0')
    {
        if (context.prog->mob == NULL) return -1;
        return (FactionTable::CurrentStanding(*context.target, *context.prog->mob) == Rating_Friend ? 1 : 0);
    }

    if (!is_number(context.val))
    {
        bug("Invalid faction number in isfriend", 0);
        return -1;
    }

    return (FactionTable::CurrentStanding(*context.target, atoi(context.val)) == Rating_Friend ? 1 : 0);
}

PC_FUN(CheckEnemy) // isenemy    
{
    if (context.target == NULL)
        return -1;

    if (context.val[0] == '\0')
    {
        if (context.prog->mob == NULL) return -1;
        return (FactionTable::CurrentStanding(*context.target, *context.prog->mob) == Rating_Enemy ? 1 : 0);
    }

    if (!is_number(context.val))
    {
        bug("Invalid faction number in isenemy", 0);
        return -1;
    }

    return (FactionTable::CurrentStanding(*context.target, atoi(context.val)) == Rating_Enemy ? 1 : 0);
}

PC_FUN(CheckPermInvis) // perminvis
{
	if (context.prog->obj)
	    return IS_SET(context.prog->obj->pIndexData->extra_flags[0], ITEM_INVIS) ? 1 : 0;
	else
	    return -1;
}

PC_FUN(CheckHasSymbol) // hassymbol
{
	if (context.target)
	{
	    int rhsvl = atoi(context.val);

	    if ((rhsvl < 0) || (rhsvl >= 32))
		return -1;

	    if (IS_SET(context.target->symbols_known, inscribe_table[rhsvl].bit))
		return 1;
	    else
		return 0;
	}
	else
	    return -1;
}
   
PC_FUN(CheckHasTrait) // hastrait 
{
	if (context.target)
	{
	    if (IS_NPC(context.target))
		return 0;
	    
	    int c=0;
	    for (c=0;trait_table[c].name != NULL;c++)
		if (!strcmp(trait_table[c].name,context.val))
		    break;
	    if (trait_table[c].name == NULL)
		return 0;
	    if (BIT_GET(context.target->pcdata->traits,trait_table[c].bit))
		return 1;
	    else
		return 0;
	}
	else
	    return -1;
}

PC_FUN(CheckObjFrom) // objfrom
{
    if (context.prog->obj)
    {
        MOB_INDEX_DATA *pMobIndex;

        if ((pMobIndex = get_mob_index(context.prog->obj->value[0])) == NULL)
        return -1;

        if (!str_cmp(context.val, "animal"))
        {
        if (IS_SET(pMobIndex->act, ACT_ANIMAL)
         || IS_SET(pMobIndex->form, FORM_ANIMAL))
            return 1;
        else
            return 0;
        }

        if (!str_cmp(context.val, "celestial"))
        {
        if (pMobIndex->race == race_lookup("celestial"))
            return 1;
        else
            return 0;
        }
    }
    return -1;
}

PC_FUN(CheckLevel41CorpseHere) // level41corpsehere
{
	if (context.prog->mob && context.prog->mob->in_room)
	{
	    OBJ_DATA *corpse;

	    for (corpse = context.prog->mob->in_room->contents; corpse; corpse = corpse->next_content)
		if (((corpse->item_type == ITEM_CORPSE_PC) || (corpse->item_type == ITEM_CORPSE_NPC))
                 && (corpse->level >= 41))
		    return 1;

	     return 0;
	}
	else
	    return -1;
}

PC_FUN(CheckRecentOwner) // recentowner
{
	if (context.prog->obj)
    {
	    for (int i = 0; i < MAX_LASTOWNER; i++)
        {
            if (!str_cmp(context.prog->obj->lastowner[i], context.val))
                return 1;
        }
	
	    return 0;
	}
	else
	    return -1;
}

PC_FUN(CheckLuck) // luck
{
    if (context.target == NULL)
        return -1;

    // Get the argument
    char luckArg[MAX_INPUT_LENGTH];
    char * luckIter(one_argument(context.val, luckArg));
    Luck::Result result;
    if (!str_cmp(luckArg, "lucky")) result = Luck::Lucky;
    else if (!str_cmp(luckArg, "unlucky")) result = Luck::Unlucky;
    else if (!str_cmp(luckArg, "normal")) result = Luck::Normal;
    else
    {
        bug("Unrecognized luck check argument; expected 'lucky', 'normal', or 'unlucky'", 0);
        return -1;
    }

    // Now look for opposed checks
    luckIter = one_argument(luckIter, luckArg);
    if (luckArg[0] == '\0')
        return mprog_veval(Luck::Check(*context.target), context.opr, result);

    if (!str_cmp(luckArg, "vs"))
    {
        one_argument(luckIter, luckArg);
        CHAR_DATA * luckOpponent(NULL);
        deduce_char_arg(luckOpponent, context.prog, luckArg, context.vict, false);
        if (luckOpponent == NULL)
        {
            bug("No target found in opposed luck check", 0);
            return -1;
        }

        return mprog_veval(Luck::CheckOpposed(*context.target, *luckOpponent), context.opr, result);
    }

    bug("Unrecognized luck check argument; expected 'vs' or nothing", 0);
    return -1;
}

PC_FUN(CheckSkill) // skill
{
	if (context.target)
	{
	    int i, snum = 0;

	    for (i = 5; isdigit(context.buf[i]); i++)
	    {
            snum *= 10;
            snum += (context.buf[i] - 48);
	    }

	    int rhsvl = atoi(context.val);
	    return mprog_veval(get_skill(context.target, snum), context.opr, rhsvl);
	}
	else return -1;  
}

PC_FUN(CheckSaves) // saves
{
    if (context.target)
    {
        int i, snum = 0;

        for (i = 5; isdigit(context.buf[i]); i++)
        {
        snum *= 10;
        snum += (context.buf[i] - 48);
        }

        int rhsvl = atoi(context.val);
        return mprog_veval(saves_spell(rhsvl, NULL, context.target, snum) ? TRUE : FALSE, context.opr, TRUE);
    }
    else return -1;
}

const ProgConditionals::MapType & ProgConditionals::BuildConditions()
{
    static MapType conditions;
    #define PC_ADD(name, func) conditions.insert(std::make_pair((name), (func)))

    PC_ADD("rand", CheckRand);
    PC_ADD("compstr", CheckCompstr);
    PC_ADD("compx", CheckCompx);
    PC_ADD("regex", CheckRegex);
    PC_ADD("isexists", CheckExists);
    PC_ADD("acctpoints", CheckAccountPoints);
    PC_ADD("strength", CheckStrength);
    PC_ADD("dexterity", CheckDexterity);
    PC_ADD("constitution", CheckConstitution);
    PC_ADD("charisma", CheckCharisma);
    PC_ADD("intelligence", CheckIntelligence);
    PC_ADD("wisdom", CheckWisdom);
    PC_ADD("hp", CheckHP);
    PC_ADD("mana", CheckMana);
    PC_ADD("move", CheckMove);
    PC_ADD("size", CheckSize);
    PC_ADD("isrange", CheckRange);
    PC_ADD("isanypcarea", CheckAnyPCArea);
    PC_ADD("isanyobjhere", CheckAnyObjHere);
    PC_ADD("isanypchere", CheckAnyPCHere);
    PC_ADD("isanynpchere", CheckAnyNPCHere);
    PC_ADD("cansee", CheckCanSee);
    PC_ADD("isbitset", CheckBitSet);
    PC_ADD("iscriminal", CheckCriminal);
    PC_ADD("ispc", CheckPC);
    PC_ADD("isgroup", CheckGroup);
    PC_ADD("isfollowing", CheckFollowing);
    PC_ADD("isnpc", CheckNPC);
    PC_ADD("isplaying", CheckPlaying);
    PC_ADD("isharmonizing", CheckHarmonizing);
    PC_ADD("isopen", CheckOpen);
    PC_ADD("ismale", CheckMale);
    PC_ADD("isfemale", CheckFemale);
    PC_ADD("isneuter", CheckNeuter);
    PC_ADD("isneutral", CheckNeutral);
    PC_ADD("iswizi", CheckWizi);
    PC_ADD("israce", CheckRace);
    PC_ADD("auragrade", CheckAuraGrade);
    PC_ADD("port", CheckPort);
    PC_ADD("majorsphere", CheckMajorSphere);
    PC_ADD("minorsphere", CheckMinorSphere);
    PC_ADD("chosenpath", CheckChosenPath);
    PC_ADD("inhouse", CheckHouse);
    PC_ADD("fountainhere", CheckFountainHere);
    PC_ADD("isinhouse", CheckInHouse);
    PC_ADD("isastral", CheckAstral);
    PC_ADD("isghost", CheckGhost);
    PC_ADD("isevil", CheckEvil);
    PC_ADD("ischaotic", CheckChaotic);
    PC_ADD("isbalanced", CheckBalanced);
    PC_ADD("islawful", CheckLawful);
    PC_ADD("isflying", CheckFlying);
    PC_ADD("isoverlimit", CheckOverlimit);
    PC_ADD("isunderlimit", CheckUnderlimit);
    PC_ADD("islimited", CheckLimited);
    PC_ADD("mobvcheck", CheckMobVCheck);
    PC_ADD("remembers", CheckRemembers);
    PC_ADD("isfocused", CheckFocused);
    PC_ADD("value", CheckValue);
    PC_ADD("mobvalue", CheckMobValue);
    PC_ADD("objvalue", CheckObjValue);
    PC_ADD("roomvalue", CheckRoomValue);
    PC_ADD("cost", CheckCost);
    PC_ADD("demonstate", CheckDemonstate);
    PC_ADD("day", CheckDay);
    PC_ADD("month", CheckMonth);
    PC_ADD("isseason", CheckSeason);
    PC_ADD("isphase", CheckPhase);
    PC_ADD("rhosphase", CheckRhosPhase);
    PC_ADD("rhossize", CheckRhosSize);
    PC_ADD("cloudcover", CheckCloudCover);
    PC_ADD("precip", CheckPrecip);
    PC_ADD("isweather", CheckWeather);
    PC_ADD("islight", CheckLight);
    PC_ADD("isday", CheckIsDay);
    PC_ADD("isdayofweek", CheckDayOfWeek);
    PC_ADD("istime", CheckTime);
    PC_ADD("iswielding", CheckWielding);
    PC_ADD("iscarrying", CheckCarrying);
    PC_ADD("carrycount", CheckCarryCount);
    PC_ADD("allcarrycount", CheckAllCarryCount);
    PC_ADD("isgood", CheckGood);
    PC_ADD("isfight", CheckFighting);
    PC_ADD("isfighting", CheckFighting);
    PC_ADD("isimmort", CheckImmort);
    PC_ADD("ischarmed", CheckCharmed);
    PC_ADD("istracking", CheckTracking);
    PC_ADD("istrack", CheckTrack);
    PC_ADD("isfollow", CheckFollow);
    PC_ADD("iswearslotfull", CheckWearSlotFull);
    PC_ADD("isindoors", CheckIndoors);
    PC_ADD("ismaster", CheckMaster);
    PC_ADD("roomaffected", CheckRoomAffected);
    PC_ADD("snaffected", CheckSNAffected);
    PC_ADD("isaffected", CheckAffected);
    PC_ADD("isoaffected", CheckOAffected);
    PC_ADD("isnaffected", CheckNAffected);
    PC_ADD("ispaffected", CheckPAffected);
    PC_ADD("hitprcnt", CheckHitPercent);
    PC_ADD("objhere", CheckObjHere);
    PC_ADD("mobhere", CheckMobHere);
    PC_ADD("adrenaline", CheckAdrenaline);
    PC_ADD("ishere", CheckHere);
    PC_ADD("inroom", CheckInRoom);
    PC_ADD("inarea", CheckInArea);
    PC_ADD("roomflag", CheckRoomFlag);
    PC_ADD("hasboat", CheckHasBoat);
    PC_ADD("sector", CheckSector);
    PC_ADD("sex", CheckSex);
    PC_ADD("position", CheckPosition);
    PC_ADD("level", CheckLevel);
    PC_ADD("class", CheckClass);
    PC_ADD("newbie", CheckNewbie);
    PC_ADD("goldamt", CheckGoldAmount);
    PC_ADD("isanimal", CheckAnimal);
    PC_ADD("material", CheckMaterial);
    PC_ADD("objtype", CheckObjType);
    PC_ADD("objval0", CheckObjVal0);
    PC_ADD("objval1", CheckObjVal1);
    PC_ADD("objval2", CheckObjVal2);
    PC_ADD("objval3", CheckObjVal3);
    PC_ADD("number", CheckNumber);
    PC_ADD("weight", CheckWeight);
    PC_ADD("valname", CheckValName);
    PC_ADD("name", CheckName);
    PC_ADD("language", CheckLanguage);
    PC_ADD("hasrelief", CheckHasRelief);
    PC_ADD("isowner", CheckOwner);
    PC_ADD("isheld", CheckOwner);
    PC_ADD("isworn", CheckIsWorn);
    PC_ADD("incontainer", CheckInContainer);
    PC_ADD("objtargethere", CheckObjTargetHere);
    PC_ADD("objtargetcarry", CheckObjTargetCarry);
    PC_ADD("objtargetwear", CheckObjTargetWear);
    PC_ADD("mobtargetroom", CheckMobTargetRoom);
    PC_ADD("mobtargetworld", CheckMobTargetWorld);
    PC_ADD("lagged", CheckLagged);
    PC_ADD("haspath", CheckHasPath);
    PC_ADD("ishometown", CheckHometown);
    PC_ADD("shopopen", CheckShopOpen);
    PC_ADD("isfriend", CheckFriend);
    PC_ADD("isenemy", CheckEnemy);
    PC_ADD("perminvis", CheckPermInvis);
    PC_ADD("hassymbol", CheckHasSymbol);
    PC_ADD("hastrait", CheckHasTrait);
    PC_ADD("objfrom", CheckObjFrom);
    PC_ADD("level41corpsehere", CheckLevel41CorpseHere);
    PC_ADD("recentowner", CheckRecentOwner);
    PC_ADD("luck", CheckLuck);
    PC_ADD("tanking", CheckTanking);

    #undef PC_ADD
    return conditions;
}

ProgConditionals::ConditionalFun ProgConditionals::Lookup(const char * condition)
{
    // Process the checks which are mappable
    static const MapType & conditions(BuildConditions());
    MapType::const_iterator iter(conditions.find(condition));
    if (iter != conditions.end())
        return iter->second;

    // Process the rest of the checks
    #define PC_PREFIX(prefix, func)                                 \
    {                                                               \
        static const std::string checkVal(prefix);                  \
        if (checkVal.compare(0, checkVal.size(), condition, checkVal.size()) == 0)   \
            return (func);                                          \
    }

    PC_PREFIX("skill", CheckSkill);
    PC_PREFIX("saves", CheckSaves);

    #undef PC_PREFIX

    return NULL;
}

