/*
 * Extended ban: "in channel?" (+b ~c:#chan)
 * (C) Copyright 2003-.. Bram Matthys (Syzop) and the UnrealIRCd team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "unrealircd.h"

ModuleHeader MOD_HEADER
= {
	"extbans/inchannel",
	"4.2",
	"ExtBan ~c - banned when in specified channel",
	"UnrealIRCd Team",
	"unrealircd-6",
};

/* Forward declarations */
int extban_inchannel_is_ok(BanContext *b);
char *extban_inchannel_conv_param(BanContext *b, Extban *extban);
int extban_inchannel_is_banned(BanContext *b);

/** Called upon module init */
MOD_INIT()
{
	ExtbanInfo req;
	
	req.letter = 'c';
	req.is_ok = extban_inchannel_is_ok;
	req.conv_param = extban_inchannel_conv_param;
	req.is_banned = extban_inchannel_is_banned;
	req.options = EXTBOPT_INVEX; /* for +I too */
	if (!ExtbanAdd(modinfo->handle, req))
	{
		config_error("could not register extended ban type");
		return MOD_FAILED;
	}

	MARK_AS_OFFICIAL_MODULE(modinfo);
	
	return MOD_SUCCESS;
}

/** Called upon module load */
MOD_LOAD()
{
	return MOD_SUCCESS;
}

/** Called upon unload */
MOD_UNLOAD()
{
	return MOD_SUCCESS;
}

char *extban_inchannel_conv_param(BanContext *b, Extban *extban)
{
	static char retbuf[CHANNELLEN+6];
	char *chan, *p, symbol='\0';

	strlcpy(retbuf, b->banstr, sizeof(retbuf));
	chan = retbuf;

	if ((*chan == '+') || (*chan == '%') || (*chan == '%') ||
	    (*chan == '@') || (*chan == '&') || (*chan == '~'))
	    chan++;

	if ((*chan != '#') && (*chan != '*') && (*chan != '?'))
		return NULL;

	if (!valid_channelname(chan))
		return NULL;

	p = strchr(chan, ':'); /* ~r:#chan:*.blah.net is not allowed (for now) */
	if (p)
		*p = '\0';

	/* on a sidenote '#' is allowed because it's a valid channel (atm) */
	return retbuf;
}

/* The only purpose of this function is a temporary workaround to prevent a desync.. pfff */
int extban_inchannel_is_ok(BanContext *b)
{
	char *p = b->banstr;

	if ((b->is_ok_checktype == EXBCHK_PARAM) && MyUser(b->client) && (b->what == MODE_ADD) && (strlen(b->banstr) > 3))
	{
		if ((*p == '+') || (*p == '%') || (*p == '%') ||
		    (*p == '@') || (*p == '&') || (*p == '~'))
		    p++;

		if (*p != '#')
		{
			sendnotice(b->client, "Please use a # in the channelname (eg: ~c:#*blah*)");
			return 0;
		}
	}
	return 1;
}

static int extban_inchannel_compareflags(char symbol, int flags)
{
	int require=0;

	if (symbol == '+')
		require = CHFL_VOICE|CHFL_HALFOP|CHFL_CHANOP|CHFL_CHANADMIN|CHFL_CHANOWNER;
	else if (symbol == '%')
		require = CHFL_HALFOP|CHFL_CHANOP|CHFL_CHANADMIN|CHFL_CHANOWNER;
	else if (symbol == '@')
		require = CHFL_CHANOP|CHFL_CHANADMIN|CHFL_CHANOWNER;
	else if (symbol == '&')
		require = CHFL_CHANADMIN|CHFL_CHANOWNER;
	else if (symbol == '~')
		require = CHFL_CHANOWNER;

	if (flags & require)
		return 1;

	return 0;
}

int extban_inchannel_is_banned(BanContext *b)
{
	Membership *lp;
	char *p = b->banstr;
	char symbol = '\0';

	if (*p != '#')
	{
		symbol = *p;
		p++;
	}

	for (lp = b->client->user->channel; lp; lp = lp->next)
	{
		if (match_esc(p, lp->channel->name))
		{
			/* Channel matched, check symbol if needed (+/%/@/etc) */
			if (symbol)
			{
				if (extban_inchannel_compareflags(symbol, lp->flags))
					return 1;
			} else
				return 1;
		}
	}

	return 0;
}

