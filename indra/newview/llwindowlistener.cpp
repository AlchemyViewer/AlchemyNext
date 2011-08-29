/** 
 * @file llwindowlistener.cpp
 * @brief EventAPI interface for injecting input into LLWindow
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llwindowlistener.h"

#include "llcoord.h"
#include "llfocusmgr.h"
#include "llkeyboard.h"
#include "llwindowcallbacks.h"
#include "llui.h"
#include "llview.h"
#include "llviewerwindow.h"
#include "llviewerkeyboard.h"
#include "llrootview.h"
#include <map>

LLWindowListener::LLWindowListener(LLViewerWindow *window, const KeyboardGetter& kbgetter)
	: LLEventAPI("LLWindow", "Inject input events into the LLWindow instance"),
	  mWindow(window),
	  mKbGetter(kbgetter)
{
	std::string keySomething =
		"Given [\"keysym\"], [\"keycode\"] or [\"char\"], inject the specified ";
	std::string keyExplain =
		"(integer keycode values, or keysym string from any addKeyName() call in\n"
		"http://hg.secondlife.com/viewer-development/src/tip/indra/llwindow/llkeyboard.cpp )\n";
	std::string mask =
		"Specify optional [\"mask\"] as an array containing any of \"CTL\", \"ALT\",\n"
		"\"SHIFT\" or \"MAC_CONTROL\"; the corresponding modifier bits will be combined\n"
		"to form the mask used with the event.";

	std::string given = "Given ";
	std::string mouseParams =
		"optional [\"path\"], optional [\"x\"] and [\"y\"], inject the requested mouse ";
	std::string buttonParams =
		std::string("[\"button\"], ") + mouseParams;
	std::string buttonExplain =
		"(button values \"LEFT\", \"MIDDLE\", \"RIGHT\")\n";
	std::string paramsExplain =
		"[\"path\"] is as for LLUI::resolvePath(), described in\n"
		"http://hg.secondlife.com/viewer-development/src/tip/indra/llui/llui.h\n"
		"If you omit [\"path\"], you must specify both [\"x\"] and [\"y\"].\n"
		"If you specify [\"path\"] without both [\"x\"] and [\"y\"], will synthesize (x, y)\n"
		"in the center of the LLView selected by [\"path\"].\n"
		"You may specify [\"path\"] with both [\"x\"] and [\"y\"], will use your (x, y).\n"
		"This may cause the LLView selected by [\"path\"] to reject the event.\n"
		"Optional [\"reply\"] requests a reply event on the named LLEventPump.\n"
		"reply[\"error\"] isUndefined (None) on success, else an explanatory message.\n";

	add("keyDown",
		keySomething + "keypress event.\n" + keyExplain + mask,
		&LLWindowListener::keyDown);
	add("keyUp",
		keySomething + "key release event.\n" + keyExplain + mask,
		&LLWindowListener::keyUp);
	add("mouseDown",
		given + buttonParams + "click event.\n" + buttonExplain + paramsExplain + mask,
		&LLWindowListener::mouseDown);
	add("mouseUp",
		given + buttonParams + "release event.\n" + buttonExplain + paramsExplain + mask,
		&LLWindowListener::mouseUp);
	add("mouseMove",
		given + mouseParams + "movement event.\n" + paramsExplain + mask,
		&LLWindowListener::mouseMove);
	add("mouseScroll",
		"Given an integer number of [\"clicks\"], inject the requested mouse scroll event.\n"
		"(positive clicks moves downward through typical content)",
		&LLWindowListener::mouseScroll);
}

template <typename MAPPED>
class StringLookup
{
private:
	std::string mDesc;
	typedef std::map<std::string, MAPPED> Map;
	Map mMap;

public:
	StringLookup(const std::string& desc): mDesc(desc) {}

	MAPPED lookup(const typename Map::key_type& key) const
	{
		typename Map::const_iterator found = mMap.find(key);
		if (found == mMap.end())
		{
			LL_WARNS("LLWindowListener") << "Unknown " << mDesc << " '" << key << "'" << LL_ENDL;
			return MAPPED();
		}
		return found->second;
	}

protected:
	void add(const typename Map::key_type& key, const typename Map::mapped_type& value)
	{
		mMap.insert(typename Map::value_type(key, value));
	}
};

namespace {

// helper for getMask()
MASK lookupMask_(const std::string& maskname)
{
	// It's unclear to me whether MASK_MAC_CONTROL is important, but it's not
	// supported by maskFromString(). Handle that specially.
	if (maskname == "MAC_CONTROL")
	{
		return MASK_MAC_CONTROL;
	}
	else
	{
		// In case of lookup failure, return MASK_NONE, which won't affect our
		// caller's OR.
		MASK mask(MASK_NONE);
		LLKeyboard::maskFromString(maskname, &mask);
		return mask;
	}
}

MASK getMask(const LLSD& event)
{
	LLSD masknames(event["mask"]);
	if (! masknames.isArray())
	{
		// If event["mask"] is a single string, perform normal lookup on it.
		return lookupMask_(masknames);
	}

	// Here event["mask"] is an array of mask-name strings. OR together their
	// corresponding bits.
	MASK mask(MASK_NONE);
	for (LLSD::array_const_iterator ai(masknames.beginArray()), aend(masknames.endArray());
		 ai != aend; ++ai)
	{
		mask |= lookupMask_(*ai);
	}
	return mask;
}

KEY getKEY(const LLSD& event)
{
    if (event.has("keysym"))
	{
		// Initialize to KEY_NONE; that way we can ignore the bool return from
		// keyFromString() and, in the lookup-fail case, simply return KEY_NONE.
		KEY key(KEY_NONE);
		LLKeyboard::keyFromString(event["keysym"], &key);
		return key;
	}
	else if (event.has("keycode"))
	{
		return KEY(event["keycode"].asInteger());
	}
	else
	{
		return KEY(event["char"].asString()[0]);
	}
}

} // namespace

void LLWindowListener::keyDown(LLSD const & evt)
{
	if (evt.has("path"))
	{
		LLView * target_view = 
			LLUI::resolvePath(gViewerWindow->getRootView(), evt["path"]);
		if ((target_view != 0) && target_view->isAvailable())
		{
			gFocusMgr.setKeyboardFocus(target_view);
			KEY key = getKEY(evt);
			MASK mask = getMask(evt);
			gViewerKeyboard.handleKey(key, mask, false);
			if(key < 0x80) mWindow->handleUnicodeChar(key, mask);
		}
		else 
		{
			; // TODO: Don't silently fail if target not available.
		}
	}
	else 
	{
		mKbGetter()->handleTranslatedKeyDown(getKEY(evt), getMask(evt));
	}
}

void LLWindowListener::keyUp(LLSD const & evt)
{
	if (evt.has("path"))
	{
		LLView * target_view = 
			LLUI::resolvePath(gViewerWindow->getRootView(), evt["path"]);
		if ((target_view != 0) && target_view->isAvailable())
		{
			gFocusMgr.setKeyboardFocus(target_view);
			mKbGetter()->handleTranslatedKeyUp(getKEY(evt), getMask(evt));
		}
		else 
		{
			; // TODO: Don't silently fail if target not available.
		}
	}
	else 
	{
		mKbGetter()->handleTranslatedKeyUp(getKEY(evt), getMask(evt));
	}
}

// for WhichButton
typedef BOOL (LLWindowCallbacks::*MouseFunc)(LLWindow *, LLCoordGL, MASK);
struct Actions
{
	Actions(const MouseFunc& d, const MouseFunc& u): down(d), up(u), valid(true) {}
	Actions(): valid(false) {}
	MouseFunc down, up;
	bool valid;
};

struct WhichButton: public StringLookup<Actions>
{
	WhichButton(): StringLookup<Actions>("mouse button")
	{
		add("LEFT",		Actions(&LLWindowCallbacks::handleMouseDown,
								&LLWindowCallbacks::handleMouseUp));
		add("RIGHT",	Actions(&LLWindowCallbacks::handleRightMouseDown,
								&LLWindowCallbacks::handleRightMouseUp));
		add("MIDDLE",	Actions(&LLWindowCallbacks::handleMiddleMouseDown,
								&LLWindowCallbacks::handleMiddleMouseUp));
	}
};
static WhichButton buttons;

static LLCoordGL getPos(const LLSD& event)
{
	return LLCoordGL(event["x"].asInteger(), event["y"].asInteger());
}

void LLWindowListener::mouseDown(LLSD const & evt)
{
	Actions actions(buttons.lookup(evt["button"]));
	if (actions.valid)
	{
		(mWindow->*(actions.down))(NULL, getPos(evt), getMask(evt));
	}
}

void LLWindowListener::mouseUp(LLSD const & evt)
{
	Actions actions(buttons.lookup(evt["button"]));
	if (actions.valid)
	{
		(mWindow->*(actions.up))(NULL, getPos(evt), getMask(evt));
	}
}

void LLWindowListener::mouseMove(LLSD const & evt)
{
	mWindow->handleMouseMove(NULL, getPos(evt), getMask(evt));
}

void LLWindowListener::mouseScroll(LLSD const & evt)
{
	S32 clicks = evt["clicks"].asInteger();

	mWindow->handleScrollWheel(NULL, clicks);
}

