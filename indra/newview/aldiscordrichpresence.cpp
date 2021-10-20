/**
 * @file aldiscordpresence.cpp
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

#include "llviewerprecompiledheaders.h"
#include "aldiscordrichpresence.h"

#include "llagent.h"
#include "llcallbacklist.h"
#include "llappviewer.h"
#include "llviewernetwork.h" // for LLGridManager
#include "llstartup.h"
#include "llviewermessage.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llregioninfomodel.h"
#include "llversioninfo.h"

ALRichPresenceManager* ALRichPresenceManager::sALRichPresence = nullptr;

constexpr F32 STATE_POLL_UPDATE_TIME = 20.f;

namespace DiscordDataStrings
{
	constexpr char statusStringDND[] = "🔕";
	constexpr char statusStringAway[] = "💤";
	constexpr char statusStringFlying[] = "🚁";
	constexpr char statusStringChatting[] = "💬";
	constexpr char statusStringIdle[] = "Idle";
}

static void handleDiscordReady(const DiscordUser* connectedUser)
{
#if 0
	LL_INFOS("DiscordRPC") << llformat("Discord: connected to user %s#%s - %s",
		connectedUser->username,
		connectedUser->discriminator,
		connectedUser->userId) << LL_ENDL;
#endif
	LL_INFOS("DiscordRPC") << "Discord: ready" << LL_ENDL;
	ALRichPresenceManager::instance()->discordReady();
}

static void handleDiscordError(int errcode, const char* message)
{
	LL_WARNS("DiscordRPC") << "Discord: error (" << errcode << ": " << message << ")" << LL_ENDL;
}

static void handleDiscordDisconnected(int errcode, const char* message)
{
	LL_WARNS("DiscordRPC") << "Discord: disconnected (" << errcode << ": " << message << ")" << LL_ENDL;
}

ALRichPresenceManager::ALRichPresenceManager()
{
	memset(&mPresenceData, 0, sizeof(mPresenceData));
	switch (LLVersionInfo::instance().getViewerMaturity())
	{
		case LLVersionInfo::TEST_VIEWER:
		/* Fall through */
		default:
			mPresenceData.largeImageKey = "channel_test";
			break;
		case LLVersionInfo::PROJECT_VIEWER:
			mPresenceData.largeImageKey = "channel_project";
			break;
		case LLVersionInfo::BETA_VIEWER:
			mPresenceData.largeImageKey = "channel_beta";
			break;
		case LLVersionInfo::RELEASE_VIEWER:
			mPresenceData.largeImageKey = "channel_release";
			break;
	}
	mPresenceData.state = "Idle";
	mPresenceData.details = "";
	gSavedSettings.getControl("DiscordRichPresenceEnabled")->getSignal()->connect(boost::bind(&ALRichPresenceManager::handleDiscordEnabledChanged, _2));
	gSavedSettings.getControl("DiscordRichPresenceShowGridName")->getSignal()->connect(boost::bind(&ALRichPresenceManager::handleDiscordGridNameChanged, _2));
	gSavedSettings.getControl("DiscordRichPresenceShowRegion")->getSignal()->connect(boost::bind(&ALRichPresenceManager::handleDiscordPrivacyRegionChanged, _2));
	gSavedSettings.getControl("DiscordRichPresenceShowStatus")->getSignal()->connect(boost::bind(&ALRichPresenceManager::handleDiscordPrivacyStatusChanged, _2));
}

ALRichPresenceManager::~ALRichPresenceManager()
{
	Discord_Shutdown();
}

void ALRichPresenceManager::discordReady()
{
	setGridName(gSavedSettings.getBOOL("DiscordRichPresenceShowGridName"));
	setRegionData(true);
	setStatus(true);
	updatePresence();

	doPeriodically(boost::bind(&ALRichPresenceManager::idleUpdateState, this), STATE_POLL_UPDATE_TIME);
	mRegionChangedConnection = gAgent.addRegionChangedCallback(boost::bind(&ALRichPresenceManager::regionChangedCallback, this));
}

bool ALRichPresenceManager::setEnabled(const bool enable)
{
	// TODO: Ensure clean shutdown and startup of rich presence system when toggling
	if (LLStartUp::getStartupState() == STATE_STARTED)
	{
		LL_INFOS("DiscordRPC") << "Setting Enabled : " << enable << LL_ENDL;
		if (!mEnabled && enable)
		{
			mEnabled = true;
			DiscordEventHandlers handlers;
			memset(&handlers, 0, sizeof(handlers));
			handlers.ready = handleDiscordReady;
			handlers.disconnected = handleDiscordDisconnected;
			handlers.errored = handleDiscordError;
			Discord_Initialize(std::to_string(DISCORD_APP_ID).c_str(), &handlers, 1, 0);

			doOnIdleRepeating(boost::bind(&ALRichPresenceManager::idleDiscordCallbacks, this));
			LL_INFOS() << "Discord Rich Presence Initialized" << LL_ENDL;
		}
		else if (!enable)
		{
			mEnabled = false;

			gAgent.removeRegionChangedCallback(mRegionChangedConnection);
			Discord_ClearPresence();
			Discord_Shutdown();
			LL_INFOS() << "Shutting down Discord presence" << LL_ENDL;
		}
	}
	return mEnabled;
}

//static
void ALRichPresenceManager::shutdown()
{
	if(sALRichPresence)
	{
		sALRichPresence->setEnabled(false);
	}
	delete sALRichPresence;
	sALRichPresence = nullptr;
}

//static
void ALRichPresenceManager::startup()
{
	if(!sALRichPresence)
	{
		sALRichPresence = new ALRichPresenceManager();
	}

	if (sALRichPresence)
	{
		sALRichPresence->setEnabled(gSavedSettings.getBOOL("DiscordRichPresenceEnabled"));
	}
}

std::string ALRichPresenceManager::getStateAsString()
{
return "";
}

void ALRichPresenceManager::setStatus(bool skip_update/* = false*/)
{
	// FIXME: Status has garbage in it from time to time. Maybe due to emoji conversion.
	bool show = gSavedSettings.getBOOL("DiscordRichPresenceShowStatus");
	const auto oldStatus = mPresenceData.details;
	// TODO: Scripting, Building detection
	if (show && gAgent.isDoNotDisturb())
	{
		mPresenceData.details = DiscordDataStrings::statusStringDND; // Note: Storing the c_str() result seems to produce garbage.
	}
	else if (show && gAgent.getAFK())
	{
		mPresenceData.details = DiscordDataStrings::statusStringAway;
	}
	else if (show && gAgent.getFlying()) // TODO: Check and report velocity and altitude if sitting
	{
		mPresenceData.details = DiscordDataStrings::statusStringFlying;
	}
	else if (show && gAgent.getRenderState() & AGENT_STATE_TYPING)
	{
		mPresenceData.details = DiscordDataStrings::statusStringChatting;
	}
	else
	{
		mPresenceData.details = DiscordDataStrings::statusStringIdle;
	}
	if (mPresenceData.details != oldStatus)
	{
		mDirty = true;
		if (!skip_update)
			updatePresence();
	}
}

void ALRichPresenceManager::setGridName(bool visible)
{
	if (LLStartUp::getStartupState() == STATE_STARTED)
	{
		// TODO: Implement and check grid privacy
		if(visible)
		{
			std::string grid_name = LLGridManager::getInstance()->getGridLabel();
			const bool is_secondlife = grid_name.find("Second Life") != std::string::npos;
			if (is_secondlife)
			{
				LL_DEBUGS("DiscordRPC") << "Grid is Second Life!" << LL_ENDL;
				mPresenceData.largeImageText = "Grid: Second Life";
			}
			else
			{
				// Until I add per-grid setting, only broadcast Second Life or Metaverse
				mPresenceData.largeImageText = "Grid: Other";
			}
			LL_INFOS("DiscordRPC") << "Setting Rich Presence Grid Name: '" << mPresenceData.largeImageText << "'" << LL_ENDL;
		}
		else
		{
			mPresenceData.largeImageText = std::string("").c_str();
		}
	}
}

void ALRichPresenceManager::setRegionData(bool skip_update/* = false*/)
{
	if (LLStartUp::getStartupState() == STATE_STARTED)
	{
		auto show_region = gSavedSettings.getBOOL("DiscordRichPresenceShowRegion");
		bool change = false;
		if(show_region)
		{
			LLViewerRegion* region = gAgent.getRegion();
			// Note: This skirts on abuse of resources, but I don't know any other way to refresh this
			askRegionInfo();
			if (region)
			{
				const char* name = region->getName().c_str();
				if (name != mPresenceData.state)
				{
					mPresenceData.state = name;
					// TODO: Check user preference if specific region is not set to be hidden from API
					switch (region->getSimAccess())
					{
					case SIM_ACCESS_PG:
					{
						mPresenceData.smallImageKey = "region_general";
						mPresenceData.smallImageText = "PG region";
						break;
					}
					case SIM_ACCESS_MATURE:
					{
						mPresenceData.smallImageKey = "region_mature";
						mPresenceData.smallImageText = "Mature region";
						break;
					}
					case SIM_ACCESS_ADULT:
					{
						mPresenceData.smallImageKey = "region_adult";
						mPresenceData.smallImageText = "Adult region";
						break;
					}
					default:
					{
						mPresenceData.smallImageKey = "region_unknown";
						mPresenceData.smallImageText = "Unknown";
						break;
					}
					}
					LL_INFOS("DiscordRPC") << "Updating Rich Presence with Region Name: '" << mPresenceData.state << "'" << LL_ENDL;
				}

				auto newCount = region->mMapAvatarIDs.size() + 1;
				if (newCount != mPresenceData.partySize)
				{
					LL_INFOS("DiscordRPC") << "Updating Rich Presence agent count " << mPresenceData.partySize << " -> " << newCount << LL_ENDL;
					mPresenceData.partySize = newCount;
					change = true;
				}
				auto max_agents = LLRegionInfoModel::instance().mAgentLimit;
				if(max_agents != mPresenceData.partyMax)
				{
					mPresenceData.partyMax = max_agents;
					change = true; // only update region name once we have the agent limit
				}
			}
		}
		else
		{
			if(!mPresenceData.state == '\0')
			{
				change = true;
			}
			mPresenceData.smallImageKey = "";
			mPresenceData.smallImageText = "";
			mPresenceData.state = "";
			mPresenceData.partySize = 0;
		}
		if (change)
		{
			mDirty = true;
			if(!skip_update)
				updatePresence();
		}
	}
}

void ALRichPresenceManager::askRegionInfo()
{
	// Ask the region about some info. Required for agent limit.
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("RequestRegionInfo");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	gAgent.sendReliableMessage();
}

void ALRichPresenceManager::regionChangedCallback()
{
	setRegionData();
}

void ALRichPresenceManager::updatePresence()
{
	mDirty = false;
	if (ALRichPresenceManager::isEnabled())
	{

		//
		// Reminder: NEVER call setEnabled within this function.
		// This will cause an infinite loop.
		if (!gSavedSettings.getBOOL("DiscordRichPresenceEnabled"))
		{
			LL_WARNS("DiscordRPC") << "Internal state is enabled but setting is disabled!!!" << LL_ENDL;
			return;
		}
		Discord_UpdatePresence(&mPresenceData);
		LL_INFOS("DiscordRPC") << "Updating presence!" << LL_ENDL;
	}
}
bool ALRichPresenceManager::handleDiscordGridNameChanged(const LLSD& new_value)
{
	ALRichPresenceManager::instance()->setGridName(new_value.asBoolean());
	return true;
}

bool ALRichPresenceManager::handleDiscordPrivacyRegionChanged(const LLSD& new_value)
{
	ALRichPresenceManager::instance()->setRegionData();
	return true;
}

bool ALRichPresenceManager::handleDiscordPrivacyStatusChanged(const LLSD& new_value)
{
	ALRichPresenceManager::instance()->setStatus();
	return true;
}

bool ALRichPresenceManager::handleDiscordEnabledChanged(const LLSD& new_value)
{
	ALRichPresenceManager::instance()->setEnabled(new_value.asBoolean());
	return true;
}

bool ALRichPresenceManager::idleDiscordCallbacks()
{
	if (!mEnabled)
		return true;

	Discord_RunCallbacks();
	return false;
}

bool ALRichPresenceManager::idleUpdateState()
{
	if (!mEnabled)
		return true;

	if(LLAppViewer::isQuitting())
	{
		ALRichPresenceManager::shutdown();
		return true;
	}
	setRegionData(true);
	setStatus(true);
	if (mDirty)
	{
		updatePresence();
	}
	return false;
}
