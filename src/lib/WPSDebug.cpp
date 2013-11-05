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

#include <iomanip>
#include <sstream>

#include <librevenge/librevenge.h>
#include <librevenge-stream/librevenge-stream.h>


#include "WPSDebug.h"

#if defined(DEBUG_WITH_FILES)
namespace libwps
{
bool DebugFile::open(std::string const &filename)
{
	std::string name=Debug::flattenFileName(filename);
	name += ".ascii";
	m_file.open(name.c_str());
	return m_on = m_file.is_open();
}

void DebugFile::addPos(long pos)
{
	if (!m_on) return;
	m_actOffset = pos;
}

void DebugFile::addNote(char const *note)
{
	if (!m_on || note == 0L) return;

	size_t numNotes = m_notes.size();

	if (!numNotes || m_notes[numNotes-1].m_pos != m_actOffset)
	{
		std::string empty("");
		m_notes.push_back(NotePos(m_actOffset, empty));
		numNotes++;
	}

	m_notes[numNotes-1].m_text += std::string(note);
}

void DebugFile::addDelimiter(long pos, char c)
{
	if (!m_on) return;
	std::string s;
	s+=c;
	m_notes.push_back(NotePos(pos,s,false));
}

void DebugFile::sort()
{
	if (!m_on) return;
	size_t numNotes = m_notes.size();

	if (m_actOffset >= 0 && (numNotes == 0 || m_notes[numNotes-1].m_pos != m_actOffset))
	{
		std::string empty("");
		m_notes.push_back(NotePos(m_actOffset, empty));
		numNotes++;
	}

	NotePos::Map map;
	for (size_t i = 0; i < numNotes; i++) map[m_notes[i]] = 0;

	size_t i = 0;
	for (NotePos::Map::iterator it = map.begin(); it != map.end(); i++, it++)
		m_notes[i] = it->first;
	if (i != numNotes) m_notes.resize(i);

	Vec2i::MapX sMap;
	size_t numSkip = m_skipZones.size();
	for (size_t s = 0; s < numSkip; s++) sMap[m_skipZones[s]] = 0;

	i = 0;
	for (Vec2i::MapX::iterator it = sMap.begin();
	        it != sMap.end(); i++, it++)
		m_skipZones[i] = it->first;
	if (i < numSkip) m_skipZones.resize(i);
}

void DebugFile::write()
{
	if (!m_on || m_input.get() == 0) return;

	sort();

	long readPos = m_input->tell();

	std::vector<NotePos>::const_iterator noteIter = m_notes.begin();

	//! write the notes which does not have any position
	while(noteIter != m_notes.end() && noteIter->m_pos < 0)
	{
		if (!noteIter->m_text.empty())
			std::cerr << "DebugFile::write: skipped: " << noteIter->m_text << std::endl;
		++noteIter;
	}

	long actualPos = 0;
	int numSkip = int(m_skipZones.size()), actSkip = (numSkip == 0) ? -1 : 0;
	long actualSkipEndPos = (numSkip == 0) ? -1 : m_skipZones[0].x();

	m_input->seek(0,librevenge::RVNG_SEEK_SET);
	m_file << std::hex << std::right << std::setfill('0') << std::setw(6) << 0 << " ";

	do
	{
		bool printAdr = false;
		bool stop = false;
		while (actualSkipEndPos != -1 && actualPos >= actualSkipEndPos)
		{
			printAdr = true;
			actualPos = m_skipZones[size_t(actSkip)].y()+1;
			m_file << "\nSkip : " << std::hex << std::setw(6) << actualSkipEndPos << "-"
			       << actualPos-1 << "\n\n";
			m_input->seek(actualPos, librevenge::RVNG_SEEK_SET);
			stop = m_input->isEnd();
			actSkip++;
			actualSkipEndPos = (actSkip < numSkip) ? m_skipZones[size_t(actSkip)].x() : -1;
		}
		if (stop) break;
		while(noteIter != m_notes.end() && noteIter->m_pos < actualPos)
		{
			if (!noteIter->m_text.empty())
				m_file << "Skipped: " << noteIter->m_text << std::endl;
			++noteIter;
		}
		bool printNote = noteIter != m_notes.end() && noteIter->m_pos == actualPos;
		if (printAdr || (printNote && noteIter->m_breaking))
			m_file << "\n" << std::setw(6) << actualPos << " ";
		while(noteIter != m_notes.end() && noteIter->m_pos == actualPos)
		{
			if (noteIter->m_text.empty())
			{
				++noteIter;
				continue;
			}
			if (noteIter->m_breaking)
				m_file << "[" << noteIter->m_text << "]";
			else
				m_file << noteIter->m_text;
			++noteIter;
		}

		long ch = libwps::readU8(m_input);
		m_file << std::setw(2) << ch;
		actualPos++;

	}
	while (!m_input->isEnd());

	m_file << "\n\n";

	m_input->seek(readPos,librevenge::RVNG_SEEK_SET);

	m_actOffset=-1;
	m_notes.resize(0);
}

////////////////////////////////////////////////////////////
//
// save librevenge::RVNGBinaryData in a file
//
////////////////////////////////////////////////////////////
namespace Debug
{
bool dumpFile(librevenge::RVNGBinaryData &data, char const *fileName)
{
	if (!fileName) return false;
	if (!data.size() || !data.getDataBuffer())
	{
		WPS_DEBUG_MSG(("Debug::dumpFile: can not find data for %s\n", fileName));
		return false;
	}
	std::string fName = Debug::flattenFileName(fileName);
	FILE *file = fopen(fName.c_str(), "wb");
	if (!file) return false;
	fwrite(data.getDataBuffer(), data.size(), 1, file);
	fclose(file);
	return true;
}

std::string flattenFileName(std::string const &name)
{
	std::string res;
	for (size_t i = 0; i < name.length(); i++)
	{
		char c = name[i];
		switch(c)
		{
		case '\0':
		case '/':
		case '\\':
		case ':': // potential file system separator
		case ' ':
		case '\t':
		case '\n': // potential text separator
			res += '_';
			break;
		default:
			if (c <= 28) res += '#'; // potential trouble potential char
			else res += c;
		}
	}
	return res;
}
}

}
#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
