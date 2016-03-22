#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include "merc.h"

DECLARE_DO_FUN(do_assume);

void do_dismount(CHAR_DATA *ch, char *argument)
{
    if (!ch->mount)
    {
	send_to_char("You are not currently riding anything.\n\r", ch);
	return;
    }

    act("You dismount from $N.", ch, NULL, ch->mount, TO_CHAR);
    act("$n dismounts from $N.", ch, NULL, ch->mount, TO_ROOM);

    unmount(ch);
}

void do_mount(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *mount;
    AFFECT_DATA *paf;

    if (argument[0] == '\0') 
    {
	if (ch->mount)
	    act("You are currently mounted upon $N.", ch, NULL, ch->mount, TO_CHAR);
	else
	    send_to_char("What do you wish to mount?\n\r", ch);
	return;
    }

    if ((mount = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("You do not see that here.\n\r", ch);
	return;
    }

    if (ch->mount)
    {
	send_to_char("You are already upon a mount.\n\r", ch);
	return;
    }

    if (!IS_NPC(mount))
    {
	send_to_char("Only creatures may be ridden.\n\r", ch);
	return;
    }

    if (!IS_SET(mount->nact, ACT_MOUNT))
    {
	send_to_char("That creature may not be ridden.\n\r", ch);
	return;
    }

    if (mount->rider)
    {
	send_to_char("That creature is already being ridden.\n\r", ch);
	return;
    }

    if (!IS_AFFECTED(mount, AFF_CHARM) || mount->master != ch)
    {
	act("$N forcefully resists your attempts to mount it.", ch, NULL, mount, TO_CHAR);
	return;
    }

    for (paf = ch->affected; paf; paf = paf->next)
	if (paf->location == APPLY_FORM)
	{
	    do_assume(ch, "none");
	    break;
	}

    if (ch->guarding)
    {
	act("You stop protecting $N.", ch, NULL, ch->guarding, TO_CHAR);
	act("$N stops protecting you.", ch->guarding, NULL, ch, TO_CHAR);
	ch->guarding = NULL;
    }

    uncamo_char(ch);
    unhide_char(ch);

    act("You carefully mount $N, preparing to ride.", ch, NULL, mount, TO_CHAR);
    act("$n carefully mounts $N, preparing to ride.", ch, NULL, mount, TO_ROOM);

    ch->mount = mount;
    mount->rider = ch;
    
    return;
}

// Note: ch may be mount -or- rider
void unmount(CHAR_DATA *ch)
{
    if (ch->rider)
    {
	ch->rider->mount = FALSE;
        ch->rider = NULL;
    }
 
    if (ch->mount)
    {
	ch->mount->rider = FALSE;
        ch->mount = NULL;
    }
}
