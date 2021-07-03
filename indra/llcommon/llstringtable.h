/** 
 * @file llstringtable.h
 * @brief The LLStringTable class provides a _fast_ method for finding
 * unique copies of strings.
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

#ifndef LL_STRING_TABLE_H
#define LL_STRING_TABLE_H

#include "lldefs.h"
#include "llstl.h"

#include <absl/container/node_hash_set.h>

constexpr U32 MAX_STRINGS_LENGTH = 256;

class LL_COMMON_API LLStringTableEntry
{
public:
    LLStringTableEntry() = default;
	LLStringTableEntry(std::string_view str);
	LLStringTableEntry(const LLStringTableEntry& other)
	{
        mString = strdup(other.mString);
        incRef();
	}

    LLStringTableEntry(LLStringTableEntry&& other) noexcept
	{
		mString       = other.mString;
        other.mString = nullptr;
        mRef          = other.mRef;
        other.mRef    = 0;
	}
	~LLStringTableEntry();

	LLStringTableEntry& operator=(const LLStringTableEntry& rhs)
	{
        if (this != &rhs)
        {
            if (mString)
            {
                free(mString);
            }
            mString = strdup(rhs.mString);
            mRef    = 1;
            return *this;
        }
	}

	LLStringTableEntry& operator=(LLStringTableEntry&& rhs) noexcept
	{
        if (this != &rhs)
        {
            if (mString)
            {
                free(mString);
            }
            mString     = rhs.mString;
            rhs.mString = nullptr;
            mRef        = rhs.mRef;
            rhs.mRef    = 0;
            return *this;
        }
	}

	void incRef() const { ++mRef; }
    S32  decRef() const { return --mRef; }
    S32  numRefs() const { return mRef; }

    friend bool operator==(const LLStringTableEntry& lhs, const LLStringTableEntry& rhs);

	template <typename H> 
	friend H AbslHashValue(H h, const LLStringTableEntry& m);

	struct StringPoolEntryHash
    {
	  using is_transparent = void;

	  size_t operator()(std::string_view v) const {
		return absl::Hash<std::string_view>{}(v);
	  }
      size_t operator()(const LLStringTableEntry& v) const {
		return absl::Hash<LLStringTableEntry>{}(v);
	  }
	};

	// Supports heterogeneous lookup for string-like elements.
	struct StringPoolEntryEq {
		using is_transparent = void;
		bool operator()(std::string_view lhs, std::string_view rhs) const 
		{
		  return lhs == rhs;
		}
        bool operator()(const LLStringTableEntry& lhs, const LLStringTableEntry& rhs) const 
		{
            return strcmp(lhs.mString, rhs.mString) == 0;
		}
        bool operator()(const LLStringTableEntry& lhs, std::string_view rhs) const 
		{
		  return lhs.mString == rhs;
		}
        bool operator()(std::string_view lhs, const LLStringTableEntry& rhs) const 
		{
		  return lhs == rhs.mString;
		}
	};

	char* mString = nullptr;
    mutable S32 mRef = 0;
};

inline bool operator==(const LLStringTableEntry& lhs, const LLStringTableEntry& rhs)
{
    return strcmp(lhs.mString, rhs.mString) == 0;
}

template <typename H>
inline H AbslHashValue(H h, const LLStringTableEntry& m) 
{
	return H::combine(std::move(h), std::string_view(m.mString));
}

class LL_COMMON_API ALStringTable
{
  public:
    ALStringTable(size_t tablesize) { mStringHash.reserve(tablesize); };
    ~ALStringTable() { mStringHash.clear(); };

    char* checkString(std::string_view str)
    {
        auto entry = checkStringEntry(str);
        if (entry)
        {
            return entry->mString;
        }
        else
        {
            return nullptr;
        }
    }

    LLStringTableEntry* checkStringEntry(std::string_view str)
    {
        if (!str.empty())
        {
            auto it = mStringHash.find(str);
            if (it != mStringHash.end())
            {
                return const_cast<LLStringTableEntry*>(&(*it));
            }
        }
        return nullptr;
    }

    char* addString(std::string_view str)
    {
        auto entry = addStringEntry(str);
        if (entry)
        {
            return entry->mString;
        }
        else
        {
            return nullptr;
        }
    }

    LLStringTableEntry* addStringEntry(std::string_view str)
    {
        if (str.empty())
        {
            return nullptr;
        }

        auto it = mStringHash.find(str);
        if (it != mStringHash.end())
        {
            it->incRef();
            return const_cast<LLStringTableEntry*>(&(*it));
        }
        else
        {
            mUniqueEntries++;

            auto ret = mStringHash.emplace(str);
            return const_cast<LLStringTableEntry*>(&(*ret.first));
        }
    }

    void removeString(std::string_view str)
    {
        if (!str.empty())
        {
            auto it = mStringHash.find(str);
            if (it != mStringHash.end())
            {
                if (it->decRef() <= 0)
                {
                    mStringHash.erase(it);
                    mUniqueEntries--;
                    if (mUniqueEntries < 0)
                    {
                        LL_ERRS() << "LLStringTable:removeString trying to remove too many strings!" << LL_ENDL;
                    }
                }
            }
        }
    }

  private:
    S64 mUniqueEntries = 0;

    absl::node_hash_set<LLStringTableEntry, LLStringTableEntry::StringPoolEntryHash, LLStringTableEntry::StringPoolEntryEq> mStringHash;
};

//============================================================================

// This class is designed to be used locally,
// e.g. as a member of an LLXmlTree
// Strings can be inserted only, then quickly looked up

typedef const std::string* LLStdStringHandle;

class LL_COMMON_API LLStdStringTable
{
public:
	LLStdStringTable(S32 tablesize = 0)
	{
		if (tablesize == 0)
		{
			tablesize = 256; // default
		}
		// Make sure tablesize is power of 2
		for (S32 i = 31; i>0; i--)
		{
			if (tablesize & (1<<i))
			{
				if (tablesize >= (3<<(i-1)))
					tablesize = (1<<(i+1));
				else
					tablesize = (1<<i);
				break;
			}
		}
        mStringList.reserve(tablesize);
	}
	~LLStdStringTable()
	{
		cleanup();
	}
	void cleanup()
	{
		// remove strings
        mStringList.clear();
	}

	LLStdStringHandle lookup(std::string_view s)
	{
        string_set_t::iterator iter = mStringList.find(s);  // compares actual strings
        if (iter != mStringList.end())
        {
            return &(*iter);
        }
        else
        {
            return NULL;
        };
	}
	
	LLStdStringHandle checkString(std::string_view s)
	{
		return lookup(s);
	}

	LLStdStringHandle insert(std::string_view s)
	{
		LLStdStringHandle result = lookup(s);
		if (result == NULL)
		{
			auto it = mStringList.emplace(std::string(s));
            result  = &(*it.first);
		}
		return result;
	}
    LLStdStringHandle addString(std::string_view s)
	{
		return insert(s);
	}
	
private:
	typedef absl::node_hash_set<std::string> string_set_t;
	string_set_t mStringList;
};

extern LL_COMMON_API ALStringTable gStringTable;

#endif
