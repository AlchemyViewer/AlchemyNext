/**
 * @file aldiscordpresence.h
 * @brief Discord Rich Presence support
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * 2014-2018 Polarity Viewer Source Code
 * 2021-2022 Alchemy Viewer Source Code
 * Copyright (C) 2014-2021 XenHat (me@xenh.at)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * $/LicenseInfo$
 */

#pragma once

#ifndef AL_DISCORD_PRESENCE_H
#define AL_DISCORD_PRESENCE_H

#include "llerror.h"
#include "discord_rpc/discord_rpc.h"

struct DiscordUser;

class ALRichPresenceManager
{
	LOG_CLASS(ALRichPresenceManager);
public:
	ALRichPresenceManager();
	virtual ~ALRichPresenceManager();
	void discordReady();
	void setStatus(bool skip_update = false);
	void setGridName(bool visible);
	void setRegionData(bool skip_update = false);
	void updatePresence();
	static void startup();
	static void shutdown();
	bool isEnabled() { return mEnabled; };
	bool setEnabled(const bool enable);
	std::string getStateAsString();
private:
	void askRegionInfo();
	void regionChangedCallback();

	bool idleDiscordCallbacks();
	bool idleUpdateState();

public:
	// Static Callbacks
	static bool handleDiscordGridNameChanged(const LLSD& new_value);
	static bool handleDiscordPrivacyRegionChanged(const LLSD& new_value);
	static bool handleDiscordPrivacyStatusChanged(const LLSD& new_value);
	static bool handleDiscordEnabledChanged(const LLSD& new_value);

private:
	bool mEnabled = false;
	bool mDirty = false;
	boost::signals2::connection mRegionChangedConnection;
	DiscordRichPresence mPresenceData;
	static ALRichPresenceManager* sALRichPresence;
public:
	static ALRichPresenceManager* instance() { return sALRichPresence; }
};

#endif //AL_DISCORD_PRESENCE_H
