/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2009, 2011 Alonso Laurent (alonso@loria.fr)
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libwps.sourceforge.net
 */

#ifndef WPS_DEBUG
#  define WPS_DEBUG

#include <string>

#include "libwps_internal.h"

class WPXBinaryData;

#  if defined(DEBUG_WITH_FILES)
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

//! some  basic tools
namespace libwps
{
//! debugging tools
namespace Debug
{
//! a debug function to store in a datafile in the current directory
//! WARNING: this function erase the file fileName if it exists
//! (if debug_with_files is not defined, does nothing)
bool dumpFile(WPXBinaryData &data, char const *fileName);

//! returns a file name from an ole/... name
std::string flattenFileName(std::string const &name);
}

//! a basic stream (if debug_with_files is not defined, does nothing)
typedef std::stringstream DebugStream;

//! an interface used to insert comment in a binary file,
//! written in ascii form (if debug_with_files is not defined, does nothing)
class DebugFile
{
public:
	//! constructor given the input file
	DebugFile(WPXInputStreamPtr ip=WPXInputStreamPtr())
		: m_file(), m_on(false), m_input(ip), m_actOffset(-1), m_notes(), m_skipZones() { }

	//! resets the input
	void setStream(WPXInputStreamPtr ip)
	{
		m_input = ip;
	}
	//! destructor
	~DebugFile()
	{
		reset();
	}
	//! opens/creates a file to store a result in the current directory
	bool open(std::string const &filename);
	//! writes the current file and reset to zero
	void reset()
	{
		write();
		m_file.close();
		m_on = false;
		m_notes.resize(0);
		m_skipZones.resize(0);
		m_actOffset = -1;
	}
	//! adds a new position in the file
	void addPos(long pos);
	//! adds a note in the file, in actual position
	void addNote(char const *note);
	//! adds a not breaking delimiter in position \a pos
	void addDelimiter(long pos, char c);

	//! skips the file zone defined by beginPos-endPos
	void skipZone(long beginPos, long endPos)
	{
		if (m_on) m_skipZones.push_back(Vec2<long>(beginPos, endPos));
	}

protected:
	//! flushes the file
	void write();

	//! sorts the position/note date
	void sort();

	//! a stream which is open to write the file
	mutable std::ofstream m_file;
	//! a flag to know if the result stream is open or note
	mutable bool m_on;

	//! the input
	WPXInputStreamPtr m_input;

	//! \brief a note and its position (used to sort all notes)
	struct NotePos
	{
		//! empty constructor used by std::vector
		NotePos() : m_pos(-1), m_text(""), m_breaking(false) { }

		//! constructor: given position and note
		NotePos(long p, std::string const &n, bool br=true) : m_pos(p), m_text(n), m_breaking(br) {}
		//! note offset
		long m_pos;
		//! note text
		std::string m_text;
		//! flag to indicate a non breaking note
		bool m_breaking;

		//! comparison operator based on the position
		bool operator<(NotePos const &p) const
		{
			long diff = m_pos-p.m_pos;
			if (diff) return (diff < 0) ? true : false;
			if (m_breaking != p.m_breaking) return m_breaking;
			return m_text < p.m_text;
		}
		/*! \struct NotePosLt
		 * \brief internal struct used to sort the notes, sorted by position
		 */
		struct NotePosLt
		{
			//! comparison operator
			bool operator()(NotePos const &s1, NotePos const &s2) const
			{
				return s1 < s2;
			}
		};
		/*! \typedef Map
		 *  \brief map of notes
		 */
		typedef std::map<NotePos, int,struct NotePosLt> Map;
	};

	//! the actual offset (used to store note)
	long m_actOffset;
	//! list of notes
	std::vector<NotePos> m_notes;
	//! list of skipZone
	std::vector<Vec2<long> > m_skipZones;
};
}
#  else
namespace libwps
{
namespace Debug
{
inline bool dumpFile(WPXBinaryData &, char const *)
{
	return true;
}

inline std::string flattenFileName(std::string const &name)
{
	return name;
}
}

class DebugStream
{
public:
	template <class T>
	DebugStream &operator<<(T const &)
	{
		return *this;
	}

	static std::string str()
	{
		return std::string("");
	}
	static void str(std::string const &) { }
};

class DebugFile
{
public:
	DebugFile(WPXInputStreamPtr) {}
	DebugFile() {}
	static void setStream(WPXInputStreamPtr) {  }
	~DebugFile() { }

	static bool open(std::string const &)
	{
		return true;
	}

	static void addPos(long ) {}
	static void addNote(char const *) {}
	static void addDelimiter(long, char) {}

	static void reset() { }

	static void skipZone(long , long ) {}
};
}
#  endif

#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
