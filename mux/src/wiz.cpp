/*! \file wiz.cpp
 * \brief Wizard-only commands.
 *
 * $Id$
 *
 */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"
#include "externs.h"

#include "attrs.h"
#include "command.h"
#include "file_c.h"
#include "mathutil.h"
#include "powers.h"

static void do_teleport_single
(
    dbref executor,
    dbref caller,
    dbref enactor,
    int   key,
    dbref victim,
    UTF8 *to
)
{
    UNUSED_PARAMETER(caller);
    UNUSED_PARAMETER(enactor);

    dbref loc;
    int   hush = 0;

    // Validate type of victim.
    //
    if (  !Good_obj(victim)
       || isRoom(victim))
    {
        notify_quiet(executor, T("You can\xE2\x80\x99t teleport that."));
        return;
    }

    // Fail if we don't control the victim or the victim's location.
    //
    if (  !Controls(executor, victim)
       && !Controls(executor, isExit(victim) ? Exits(victim) : Location(victim))
       && !Tel_Anything(executor))
    {
        notify_quiet(executor, NOPERM_MESSAGE);
        return;
    }

    // Check for teleporting home
    //
    if (!string_compare(to, T("home")))
    {
        if (isExit(victim))
        {
            notify_quiet(executor, T("Bad destination."));
        }
        else
        {
            move_via_teleport(victim, HOME, enactor, 0);
        }
        return;
    }

    // Find out where to send the victim.
    //
    init_match(executor, to, NOTYPE);
    match_everything(0);
    dbref destination = match_result();

    switch (destination)
    {
    case NOTHING:

        notify_quiet(executor, T("No match."));
        return;

    case AMBIGUOUS:

        notify_quiet(executor, T("I don\xE2\x80\x99t know which destination you mean!"));
        return;

    default:

        if (victim == destination)
        {
            notify_quiet(executor, T("Bad destination."));
            return;
        }
    }

    // If fascist teleport is on, you must control the victim's ultimate
    // location (after LEAVEing any objects) or it must be JUMP_OK.
    //
    if (mudconf.fascist_tport)
    {
        if (isExit(victim))
        {
            loc = where_room(Home(victim));
        }
        else
        {
            loc = where_room(victim);
        }

        if (  !Good_obj(loc)
           || !isRoom(loc)
           || !( Controls(executor, loc)
              || Jump_ok(loc)
              || Tel_Anywhere(executor)))
        {
            notify_quiet(executor, NOPERM_MESSAGE);
            return;
        }
    }

    if (  isGarbage(destination)
       || (  Has_location(destination)
          && isGarbage(Location(destination))))
    {
        // @Teleporting into garbage is never permitted.
        //
        notify_quiet(executor, T("Bad destination."));
        return;
    }
    else if (Has_contents(destination))
    {
        // You must control the destination OR it must be a JUMP_OK where
        // the victim passes its TELEPORT lock (exit victims have the
        // additional requirement that the destination must be OPEN_OK and
        // the victim must pass the destination's OPEN lock) OR you must be
        // Tel_Anywhere.
        //
        // Only God may teleport exits into God.
        //
        if (  (  Controls(executor, destination)
              || Tel_Anywhere(executor)
              || (  Jump_ok(destination)
                 && could_doit(victim, destination, A_LTPORT)
                 && (  !isExit(victim)
                    || (  Open_ok(destination)
                       && could_doit(executor, destination, A_LOPEN)))))
           && (  !isExit(victim)
              || !God(destination)
              || God(executor)))
        {
            // We're OK, do the teleport.
            //
            if (key & TELEPORT_QUIET)
            {
                hush = HUSH_ENTER | HUSH_LEAVE;
            }

            if (move_via_teleport(victim, destination, enactor, hush))
            {
                if (executor != victim)
                {
                    if (!Quiet(executor))
                    {
                        notify_quiet(executor, T("Teleported."));
                    }
                }
            }
        }
        else
        {
            // Nope, report failure.
            //
            if (executor != victim)
            {
                notify_quiet(executor, NOPERM_MESSAGE);
            }
            did_it(victim, destination,
                   A_TFAIL, T("You can\xE2\x80\x99t teleport there!"),
                   A_OTFAIL, 0, A_ATFAIL, 0, nullptr, 0);
        }
    }
    else if (isExit(destination))
    {
        if (isExit(victim))
        {
            if (executor != victim)
            {
                notify_quiet(executor, T("Bad destination."));
            }
            did_it(victim, destination,
                   A_TFAIL, T("You can\xE2\x80\x99t teleport there!"),
                   A_OTFAIL, 0, A_ATFAIL, 0, nullptr, 0);
            return;
        }
        else
        {
            if (Exits(destination) == Location(victim))
            {
                move_exit(victim, destination, false, T("You can\xE2\x80\x99t go that way."), 0);
            }
            else
            {
                notify_quiet(executor, T("I can\xE2\x80\x99t find that exit."));
            }
        }
    }
}

void do_teleport
(
    dbref executor,
    dbref caller,
    dbref enactor,
    int   eval,
    int   key,
    int   nargs,
    UTF8 *arg1,
    UTF8 *arg2,
    const UTF8 *cargs[],
    int   ncargs
)
{
    UNUSED_PARAMETER(eval);
    UNUSED_PARAMETER(cargs);
    UNUSED_PARAMETER(ncargs);

    if (  (  Fixed(executor)
          || Fixed(Owner(executor)))
       && !Tel_Anywhere(executor))
    {
        notify(executor, mudconf.fixed_tel_msg);
        return;
    }

    // Get victim.
    //
    if (nargs == 1)
    {
        // Teleport executor to given destination.
        //
        do_teleport_single(executor, caller, enactor, key, executor, arg1);
    }
    else if (nargs == 2)
    {
        // Teleport 3rd part(y/ies) to given destination.
        //
        if (key & TELEPORT_LIST)
        {
            // We have a space-delimited list of victims.
            //
            UTF8 *p;
            MUX_STRTOK_STATE tts;
            mux_strtok_src(&tts, arg1);
            mux_strtok_ctl(&tts, T(" "));
            for (p = mux_strtok_parse(&tts); p; p = mux_strtok_parse(&tts))
            {
                init_match(executor, p, NOTYPE);
                match_everything(0);
                dbref victim = noisy_match_result();

                if (Good_obj(victim))
                {
                    do_teleport_single(executor, caller, enactor, key, victim,
                        arg2);
                }
            }
        }
        else
        {
            init_match(executor, arg1, NOTYPE);
            match_everything(0);
            dbref victim = noisy_match_result();

            if (Good_obj(victim))
            {
                do_teleport_single(executor, caller, enactor, key, victim,
                    arg2);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// do_force_prefixed: Interlude to do_force for the # command
//
void do_force_prefixed
(
    dbref executor,
    dbref caller,
    dbref enactor,
    int   eval,
    int   key,
    UTF8 *command,
    const UTF8 *cargs[],
    int ncargs
)
{
    UTF8 *cp = parse_to(&command, ' ', 0);
    if (!command)
    {
        return;
    }
    while (mux_isspace(*command))
    {
        command++;
    }
    if (*command)
    {
        do_force(executor, caller, enactor, eval, key, 2, cp, command, cargs, ncargs);
    }
}

// ---------------------------------------------------------------------------
// do_force: Force an object to do something.
//
void do_force
(
    dbref executor,
    dbref caller,
    dbref enactor,
    int   eval,
    int   key,
    int   nargs,
    UTF8 *arg1,
    UTF8 *arg2,
    const UTF8 *cargs[],
    int ncargs
)
{
    UNUSED_PARAMETER(enactor);
    UNUSED_PARAMETER(key);
    UNUSED_PARAMETER(nargs);

    dbref victim = match_controlled(executor, arg1);
    if (victim != NOTHING)
    {
        // Force victim to do command.
        //
        CLinearTimeAbsolute lta;
        wait_que(victim, caller, executor, eval, false, lta, NOTHING, 0,
            arg2,
            ncargs, cargs,
            mudstate.global_regs);
    }
}

// ---------------------------------------------------------------------------
// do_toad: Turn a player into an object.
//
void do_toad
(
    dbref executor,
    dbref caller,
    dbref enactor,
    int   eval,
    int   key,
    int   nargs,
    UTF8 *toad,
    UTF8 *newowner,
    const UTF8 *cargs[],
    int   ncargs
)
{
    UNUSED_PARAMETER(caller);
    UNUSED_PARAMETER(enactor);
    UNUSED_PARAMETER(eval);
    UNUSED_PARAMETER(cargs);
    UNUSED_PARAMETER(ncargs);

    dbref victim, recipient, loc, aowner;
    UTF8 *buf;
    int count, aflags;

    init_match(executor, toad, TYPE_PLAYER);
    match_neighbor();
    match_absolute();
    match_player();
    if (!Good_obj(victim = noisy_match_result()))
    {
        return;
    }

    if (!isPlayer(victim))
    {
        notify_quiet(executor, T("Try @destroy instead."));
        return;
    }
    if (No_Destroy(victim))
    {
        notify_quiet(executor, T("You can\xE2\x80\x99t toad that player."));
        return;
    }
    if (  nargs == 2
       && *newowner )
    {
        init_match(executor, newowner, TYPE_PLAYER);
        match_neighbor();
        match_absolute();
        match_player();
        if ((recipient = noisy_match_result()) == NOTHING)
        {
            return;
        }
    }
    else
    {
        if (mudconf.toad_recipient == NOTHING)
        {
            recipient = executor;
        }
        else
        {
            recipient = mudconf.toad_recipient;
        }
    }

    STARTLOG(LOG_WIZARD, "WIZ", "TOAD");
    log_name_and_loc(victim);
    log_text(T(" was @toaded by "));
    log_name(executor);
    ENDLOG;

    // Clear everything out.
    //
    if (key & TOAD_NO_CHOWN)
    {
        count = -1;
    }
    else
    {
        // You get it.
        //
        count = chown_all(victim, recipient, executor, CHOWN_NOZONE);
        s_Owner(victim, recipient);
        s_Zone(victim, NOTHING);
    }
    s_Flags(victim, FLAG_WORD1, TYPE_THING | HALT);
    s_Flags(victim, FLAG_WORD2, 0);
    s_Flags(victim, FLAG_WORD3, 0);
    s_Pennies(victim, 1);

    // Notify people.
    //
    loc = Location(victim);
    buf = alloc_mbuf("do_toad");
    const UTF8 *pVictimMoniker = Moniker(victim);
    const UTF8 *pVictimName = Name(victim);
    mux_sprintf(buf, MBUF_SIZE, T("%s has been turned into a slimy toad!"),
        pVictimMoniker);
    notify_except2(loc, executor, victim, executor, buf);
    mux_sprintf(buf, MBUF_SIZE, T("You toaded %s! (%d objects @chowned)"),
        pVictimMoniker, count + 1);
    notify_quiet(executor, buf);

    // Zap the name from the name hash table.
    //
    mux_sprintf(buf, MBUF_SIZE, T("a slimy toad named %s"), pVictimMoniker);
    delete_player_name(victim, pVictimName, false);
    s_Name(victim, buf);
    free_mbuf(buf);

    // Zap the alias, too.
    //
    buf = atr_pget(victim, A_ALIAS, &aowner, &aflags);
    delete_player_name(victim, buf, true);
    free_lbuf(buf);

    // Boot off.
    //
    count = boot_off(victim, T("You have been turned into a slimy toad!"));

    // Release comsys and @mail resources.
    //
    ReleaseAllResources(victim);

    buf = tprintf(T("%d connection%s closed."), count, (count == 1 ? "" : "s"));
    notify_quiet(executor, buf);
}

void do_newpassword
(
    dbref executor,
    dbref caller,
    dbref enactor,
    int   eval,
    int   key,
    int   nargs,
    UTF8 *name,
    UTF8 *password,
    const UTF8 *cargs[],
    int   ncargs
)
{
    UNUSED_PARAMETER(caller);
    UNUSED_PARAMETER(enactor);
    UNUSED_PARAMETER(eval);
    UNUSED_PARAMETER(key);
    UNUSED_PARAMETER(nargs);
    UNUSED_PARAMETER(cargs);
    UNUSED_PARAMETER(ncargs);

    dbref victim = lookup_player(executor, name, false);
    if (victim == NOTHING)
    {
        notify_quiet(executor, T("No such player."));
        return;
    }
    const UTF8 *pmsg;
    if (  *password != '\0'
       && !ok_password(password, &pmsg))
    {
        // Can set null passwords, but not bad passwords.
        //
        notify_quiet(executor, pmsg);
        return;
    }
    if (God(victim))
    {
        bool bCan = true;
        if (God(executor))
        {
            // God can change her own password if it's missing.
            //
            int   aflags;
            dbref aowner;
            UTF8 *target = atr_get("do_newpassword.474", executor, A_PASS, &aowner, &aflags);
            if (target[0] != '\0')
            {
                bCan = false;
            }
            free_lbuf(target);
        }
        else
        {
            bCan = false;
        }
        if (!bCan)
        {
            notify_quiet(executor, T("You cannot change that player\xE2\x80\x99s password."));
            return;
        }
    }
    STARTLOG(LOG_WIZARD, "WIZ", "PASS");
    log_name(executor);
    log_text(T(" changed the password of "));
    log_name(victim);
    ENDLOG;

    // It's ok, do it.
    //
    ChangePassword(victim, password);
    notify_quiet(executor, T("Password changed."));
    UTF8 *buf = alloc_lbuf("do_newpassword");
    UTF8 *bp = buf;
    safe_tprintf_str(buf, &bp, T("Your password has been changed by %s."), Moniker(executor));
    notify_quiet(victim, buf);
    free_lbuf(buf);
}

void do_boot(dbref executor, dbref caller, dbref enactor, int eval, int key, UTF8 *name, const UTF8 *cargs[], int ncargs)
{
    UNUSED_PARAMETER(caller);
    UNUSED_PARAMETER(enactor);
    UNUSED_PARAMETER(eval);
    UNUSED_PARAMETER(cargs);
    UNUSED_PARAMETER(ncargs);

    if (!Can_Boot(executor))
    {
        notify(executor, NOPERM_MESSAGE);
        return;
    }

    dbref victim;
    int count;
    if (key & BOOT_PORT)
    {
        if (is_integer(name, nullptr))
        {
            victim = mux_atol(name);
        }
        else
        {
            notify_quiet(executor, T("That\xE2\x80\x99s not a number!"));
            return;
        }
        STARTLOG(LOG_WIZARD, "WIZ", "BOOT");
        log_printf(T("Port %d"), victim);
        log_text(T(" was @booted by "));
        log_name(executor);
        ENDLOG;
    }
    else
    {
        init_match(executor, name, TYPE_PLAYER);
        match_neighbor();
        match_absolute();
        match_player();
        if ((victim = noisy_match_result()) == NOTHING)
        {
            return;
        }

        if (God(victim))
        {
            notify_quiet(executor, T("You cannot boot that player!"));
            return;
        }
        if (  (  !isPlayer(victim)
              && !God(executor))
           || executor == victim)
        {
            notify_quiet(executor, T("You can only boot off other players!"));
            return;
        }
        STARTLOG(LOG_WIZARD, "WIZ", "BOOT");
        log_name_and_loc(victim);
        log_text(T(" was @booted by "));
        log_name(executor);
        ENDLOG;
        notify_quiet(executor, tprintf(T("You booted %s off!"), Moniker(victim)));
    }

    const UTF8 *buf;
    if (key & BOOT_QUIET)
    {
        buf = nullptr;
    }
    else
    {
        buf = tprintf(T("%s gently shows you the door."), Moniker(executor));
    }

    if (key & BOOT_PORT)
    {
        count = boot_by_port(victim, God(executor), buf);
    }
    else
    {
        count = boot_off(victim, buf);
    }
    notify_quiet(executor, tprintf(T("%d connection%s closed."), count, (count == 1 ? "" : "s")));
}

// ---------------------------------------------------------------------------
// do_poor: Reduce the wealth of anyone over a specified amount.
//
void do_poor(dbref executor, dbref caller, dbref enactor, int eval, int key, UTF8 *arg1, const UTF8 *cargs[], int ncargs)
{
    UNUSED_PARAMETER(executor);
    UNUSED_PARAMETER(caller);
    UNUSED_PARAMETER(enactor);
    UNUSED_PARAMETER(eval);
    UNUSED_PARAMETER(key);
    UNUSED_PARAMETER(cargs);
    UNUSED_PARAMETER(ncargs);

    if (!is_rational(arg1))
    {
        return;
    }

    int amt = mux_atol(arg1);
    int curamt;
    dbref a;
    DO_WHOLE_DB(a)
    {
        if (isPlayer(a))
        {
            curamt = Pennies(a);
            if (amt < curamt)
            {
                s_Pennies(a, amt);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// do_cut: Chop off a contents or exits chain after the named item.
//
void do_cut(dbref executor, dbref caller, dbref enactor, int eval, int key, UTF8 *thing, const UTF8 *cargs[], int ncargs)
{
    UNUSED_PARAMETER(caller);
    UNUSED_PARAMETER(enactor);
    UNUSED_PARAMETER(eval);
    UNUSED_PARAMETER(key);
    UNUSED_PARAMETER(cargs);
    UNUSED_PARAMETER(ncargs);

    dbref object = match_controlled(executor, thing);
    if (Good_obj(object))
    {
        s_Next(object, NOTHING);
        notify_quiet(executor, T("Cut."));
    }
}

// --------------------------------------------------------------------------
// do_motd: Wizard-settable message of the day (displayed on connect)
//
void do_motd(dbref executor, dbref caller, dbref enactor, int eval, int key, UTF8 *message, const UTF8 *cargs[], int ncargs)
{
    UNUSED_PARAMETER(caller);
    UNUSED_PARAMETER(enactor);
    UNUSED_PARAMETER(eval);
    UNUSED_PARAMETER(cargs);
    UNUSED_PARAMETER(ncargs);

    bool is_brief = false;

    if (key & MOTD_BRIEF)
    {
        is_brief = true;
        key = key & ~MOTD_BRIEF;
        if (key == MOTD_ALL)
            key = MOTD_LIST;
        else if (key != MOTD_LIST)
            key |= MOTD_BRIEF;
    }

    switch (key)
    {
    case MOTD_ALL:

        mux_strncpy(mudconf.motd_msg, message, sizeof(mudconf.motd_msg)-1);
        if (!Quiet(executor))
        {
            notify_quiet(executor, T("Set: MOTD."));
        }
        break;

    case MOTD_WIZ:

        mux_strncpy(mudconf.wizmotd_msg, message, sizeof(mudconf.wizmotd_msg)-1);
        if (!Quiet(executor))
        {
            notify_quiet(executor, T("Set: Wizard MOTD."));
        }
        break;

    case MOTD_DOWN:

        mux_strncpy(mudconf.downmotd_msg, message, sizeof(mudconf.downmotd_msg)-1);
        if (!Quiet(executor))
        {
            notify_quiet(executor, T("Set: Down MOTD."));
        }
        break;

    case MOTD_FULL:

        mux_strncpy(mudconf.fullmotd_msg, message, sizeof(mudconf.fullmotd_msg)-1);
        if (!Quiet(executor))
        {
            notify_quiet(executor, T("Set: Full MOTD."));
        }
        break;

    case MOTD_LIST:

        if (Wizard(executor))
        {
            if (!is_brief)
            {
                notify_quiet(executor, T("----- motd file -----"));
                fcache_send(executor, FC_MOTD);
                notify_quiet(executor, T("----- wizmotd file -----"));
                fcache_send(executor, FC_WIZMOTD);
                notify_quiet(executor, T("----- motd messages -----"));
            }
            notify_quiet(executor, tprintf(T("MOTD: %s"), mudconf.motd_msg));
            notify_quiet( executor,
                          tprintf(T("Wizard MOTD: %s"), mudconf.wizmotd_msg) );
            notify_quiet( executor,
                          tprintf(T("Down MOTD: %s"), mudconf.downmotd_msg) );
            notify_quiet( executor,
                          tprintf(T("Full MOTD: %s"), mudconf.fullmotd_msg) );
        }
        else
        {
            if (Guest(executor))
            {
                fcache_send(executor, FC_CONN_GUEST);
            }
            else
            {
                fcache_send(executor, FC_MOTD);
            }
            notify_quiet(executor, mudconf.motd_msg);
        }
        break;

    default:

        notify_quiet(executor, T("Illegal combination of switches."));
    }
}

// ---------------------------------------------------------------------------
// do_enable: enable or disable global control flags
//
NAMETAB enable_names[] =
{
    {T("building"),        1,  CA_PUBLIC,  CF_BUILD},
    {T("checkpointing"),   2,  CA_PUBLIC,  CF_CHECKPOINT},
    {T("cleaning"),        2,  CA_PUBLIC,  CF_DBCHECK},
    {T("dequeueing"),      1,  CA_PUBLIC,  CF_DEQUEUE},
    {T("idlechecking"),    2,  CA_PUBLIC,  CF_IDLECHECK},
    {T("interpret"),       2,  CA_PUBLIC,  CF_INTERP},
    {T("logins"),          3,  CA_PUBLIC,  CF_LOGIN},
    {T("guests"),          2,  CA_PUBLIC,  CF_GUEST},
    {T("eventchecking"),   2,  CA_PUBLIC,  CF_EVENTCHECK},
    {(UTF8 *) nullptr,     0,  0,          0}
};

void do_global(dbref executor, dbref caller, dbref enactor, int eval, int key, UTF8 *flag, const UTF8 *cargs[], int ncargs)
{
    UNUSED_PARAMETER(caller);
    UNUSED_PARAMETER(enactor);
    UNUSED_PARAMETER(eval);
    UNUSED_PARAMETER(cargs);
    UNUSED_PARAMETER(ncargs);

    // Set or clear the indicated flag.
    //
    int flagvalue;
    if (!search_nametab(executor, enable_names, flag, &flagvalue))
    {
        notify_quiet(executor, T("I don\xE2\x80\x99t know about that flag."));
    }
    else if (key == GLOB_ENABLE)
    {
        // Check for CF_DEQUEUE
        //
        if (flagvalue == CF_DEQUEUE)
        {
            scheduler.SetMinPriority(PRIORITY_CF_DEQUEUE_ENABLED);
        }
        mudconf.control_flags |= flagvalue;
        STARTLOG(LOG_CONFIGMODS, "CFG", "GLOBAL");
        log_name(executor);
        log_text(T(" enabled: "));
        log_text(flag);
        ENDLOG;
        if (!Quiet(executor))
        {
            notify_quiet(executor, T("Enabled."));
        }
    }
    else if (key == GLOB_DISABLE)
    {
        if (flagvalue == CF_DEQUEUE)
        {
            scheduler.SetMinPriority(PRIORITY_CF_DEQUEUE_DISABLED);
        }
        mudconf.control_flags &= ~flagvalue;
        STARTLOG(LOG_CONFIGMODS, "CFG", "GLOBAL");
        log_name(executor);
        log_text(T(" disabled: "));
        log_text(flag);
        ENDLOG;
        if (!Quiet(executor))
        {
            notify_quiet(executor, T("Disabled."));
        }
    }
    else
    {
        notify_quiet(executor, T("Illegal combination of switches."));
    }
}
