/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2004 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include <stdio.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include "HtmlDocumentGenerator.h"
#include "TableStyle.h"
#include "TextRunStyle.h"

#include <libwps/libwps.h>

namespace HtmlDocumentGeneratorInternal
{
//
// the stream:
//

//! a abstratc stream
struct Stream
{
	//! constructor
	Stream(): m_delayedLabel("") {}
	//! destructor
	virtual ~Stream() { }
	//! add a label called on main and a label in this ( delayed to allow openParagraph to be called )
	void addLabel(std::ostream &main)
	{
		std::string lbl=label();
		if (!lbl.length())
			return;
		main << "<sup id=\"called" << lbl << "\"><a href=\"#data" << lbl << "\">" << lbl << "</a></sup>";
		flush();
		std::stringstream ss;
		ss << "<sup id=\"data" << lbl << "\"><a href=\"#called" << lbl << "\">" << lbl << "</a></sup>";
		m_delayedLabel=ss.str();
	}
	//! flush delayed label, ...
	void flush()
	{
		if (m_delayedLabel.length())
		{
			stream() << m_delayedLabel;
			m_delayedLabel="";
		}
	}
	//! return the stream
	virtual std::ostream &stream()= 0;
	//! send the data to the zone
	virtual void send()
	{
		WPS_DEBUG_MSG(("Zone::Stream::send: must not be called\n"));
		return;
	}
protected:
	//! return the stream label
	virtual std::string label() const
	{
		WPS_DEBUG_MSG(("Zone::Stream::label: must not be called\n"));
		return "";
	}
	//! the label
	std::string m_delayedLabel;
private:
	Stream(Stream const &orig);
};

//! a special stream for the main zone
struct MainStream : public Stream
{
public:
	//! constructor
	MainStream() : Stream(), m_mainStream()
	{
	}
	//! return the stream
	virtual std::ostream &stream()
	{
		return m_mainStream;
	}
	//! send the data to the zone
	void sendMain(std::ostream &out) const
	{
		out << m_mainStream.str() << "\n";
	}
protected:
	//! the main stream
	std::ostringstream m_mainStream;
};

//
// the zone and it stream:
//

//! the different zone
enum ZoneType { Z_Comment=0, Z_EndNote, Z_FootNote, Z_Main, Z_MetaData, Z_TextBox, Z_Unknown };

int const s_numZoneType=Z_Unknown+1;

//! a zone to regroup footnote/endnote,...
struct Zone
{
	struct ZoneStream;
	friend struct ZoneStream;

	//! constructor for basic stream
	Zone(ZoneType tp=Z_Unknown) : m_type(tp), m_actualId(0), m_stringList()
	{
	}
	//! the type
	ZoneType type() const
	{
		return m_type;
	}
	//! the type
	void setType(ZoneType tp)
	{
		m_type=tp;
	}
	//! returns a new stream corresponding to the main zone output
	static shared_ptr<Stream> getMainStream()
	{
		return shared_ptr<Stream>(new MainStream);
	}
	//! returns a new stream corresponding to this zone
	shared_ptr<Stream> getNewStream();
	//! returns true if there is no data
	bool isEmpty() const
	{
		for (size_t i = 0; i < m_stringList.size(); i++)
			if(m_stringList[i].size())
				return false;
		return true;
	}
	//! send the zone data
	void send(std::ostream &out) const
	{
		if (isEmpty() || m_type==Z_Unknown || m_type==Z_Main)
			return;
		if (m_type!=Z_MetaData)
			out << "<hr>\n";
		if (m_type==Z_MetaData)
		{
			for (size_t i = 0; i < m_stringList.size(); i++)
				out << m_stringList[i];
			return;
		}
		if (m_type==Z_TextBox)
		{
			out << "<p><b>TEXT BOXES</b></p><hr>\n";
			for (size_t i = 0; i < m_stringList.size(); i++)
				out << m_stringList[i] << "<hr>\n";
			return;
		}
		for (size_t i = 0; i < m_stringList.size(); i++)
		{
			std::string const &str=m_stringList[i];
			out << str << "\n";
			// check if we need to add a return line
			size_t lastComPos=str.rfind('<');
			if (lastComPos!=std::string::npos)
			{
				if (str.compare(lastComPos,4,"</p>")==0 ||
				        str.compare(lastComPos,5,"</ul>")==0 ||
				        str.compare(lastComPos,5,"</ol>")==0 ||
				        str.compare(lastComPos,4,"<br>")==0)
					continue;
			}
			out << "<br>\n";
		}
	}

	struct ZoneStream : public Stream
	{
		//! constructor
		ZoneStream(Zone &zone) : Stream(), m_zone(zone), m_stream(), m_id(zone.m_actualId++)
		{
		}
		//! destructor
		virtual ~ZoneStream() { }
		//! return the stream label
		virtual std::string label() const
		{
			return m_zone.label(m_id);
		}
		//! return the stream
		virtual std::ostream &stream()
		{
			return m_stream;
		}
		//! send the data to the zone
		virtual void send()
		{
			flush();
			if (m_zone.m_stringList.size() <= size_t(m_id))
				m_zone.m_stringList.resize(size_t(m_id)+1);
			m_zone.m_stringList[size_t(m_id)]=m_stream.str();
		}
	protected:
		//! a reference to the main zone
		Zone &m_zone;
		//! the stream
		std::ostringstream m_stream;
		//! the id
		int m_id;
	private:
		ZoneStream(ZoneStream const &orig);
		ZoneStream operator=(ZoneStream const &orig);
	};
protected:
	//! return a label corresponding to the zone
	std::string label(int id) const;
	//! the zone type
	ZoneType m_type;
	//! the actual id
	mutable int m_actualId;
	//! the list of data string
	std::vector<std::string> m_stringList;
private:
	Zone(Zone const &orig);
	Zone operator=(Zone const &orig);
};

shared_ptr<Stream> Zone::getNewStream()
{
	if (m_type==Z_Main)
	{
		WPS_DEBUG_MSG(("Zone::getNewStream: must not be called on the main zone\n"));
		return shared_ptr<Stream>();
	}
	return shared_ptr<Stream>(new Zone::ZoneStream(*this));
}

std::string Zone::label(int id) const
{
	char c=0;
	switch(m_type)
	{
	case Z_Comment:
		c='C';
		break;
	case Z_EndNote:
		c='E';
		break;
	case Z_FootNote:
		c='F';
		break;
	case Z_TextBox:
		c='T';
		break;
	case Z_Main:
	case Z_MetaData:
	case Z_Unknown:
	default:
		break;
	}
	if (c==0)
		return "";
	std::stringstream s;
	s << c << id+1;
	return s.str();
}

//! the internal state of a html document generator
struct State
{
	//! constructor
	State() : m_actualPage(0), m_ignore(false), m_actualStream(), m_streamStack(), m_listManager(), m_paragraphManager(), m_spanManager(), m_tableManager()
	{
		for (int i = 0; i < s_numZoneType; i++)
			m_zones[i].setType(ZoneType(i));
		m_actualStream=Zone::getMainStream();
		m_paragraphManager.reset(new ParagraphStyleManager);
		m_listManager.reset(new ListStyleManager);
		m_spanManager.reset(new SpanStyleManager);
		m_tableManager.reset(new TableStyleManager);
	};
	//! returns the actual output ( sending delayed data if needed)
	std::ostream &output(bool sendDelayed=true)
	{
		if (sendDelayed)
			m_actualStream->flush();
		return m_actualStream->stream();
	}
	//! returns the actual stream
	Stream &stream()
	{
		return *m_actualStream;
	}
	// access to the paragraph manager
	ListStyleManager &listManager()
	{
		return *m_listManager;
	}
	// access to the paragraph manager
	ParagraphStyleManager &paragraphManager()
	{
		return *m_paragraphManager;
	}
	// access to the span manager
	SpanStyleManager &spanManager()
	{
		return *m_spanManager;
	}
	// access to the table manager
	TableStyleManager &tableManager()
	{
		return *m_tableManager;
	}
	void push(ZoneType type)
	{
		m_streamStack.push_back(m_actualStream);
		if (type==Z_Main)
		{
			WPS_DEBUG_MSG(("State::push: can not push main zone\n"));
			type=Z_Unknown;
		}
		m_actualStream=m_zones[type].getNewStream();
	}
	void pop()
	{
		if (m_streamStack.size() <= 0)
		{
			WPS_DEBUG_MSG(("HtmlDocumentGenerator::pop: can not pop stream\n"));
			return;
		}
		m_actualStream->send();
		m_actualStream = m_streamStack.back();
		m_streamStack.pop_back();
	}
	void sendMetaData(std::ostream &out)
	{
		m_zones[Z_MetaData].send(out);
	}
	void flushUnsent(std::ostream &out)
	{
		if (m_streamStack.size())
		{
			WPS_DEBUG_MSG(("HtmlDocumentGenerator::flushUnsent: the stream stack is not empty\n"));
			while (m_streamStack.size())
				pop();
		}
		// first send the main data
		if (!m_actualStream || !dynamic_cast<MainStream *>(m_actualStream.get()))
		{
			WPS_DEBUG_MSG(("HtmlDocumentGenerator::flushUnsent: can not find the main block\n"));
		}
		else
			dynamic_cast<MainStream *>(m_actualStream.get())->sendMain(out);
		m_zones[Z_Comment].send(out);
		m_zones[Z_FootNote].send(out);
		m_zones[Z_EndNote].send(out);
		m_zones[Z_TextBox].send(out);
	}
	int m_actualPage;
	bool m_ignore;
protected:
	shared_ptr<Stream> m_actualStream;
	std::vector<shared_ptr<Stream> > m_streamStack;
	shared_ptr<ListStyleManager> m_listManager;
	shared_ptr<ParagraphStyleManager> m_paragraphManager;
	shared_ptr<SpanStyleManager> m_spanManager;
	shared_ptr<TableStyleManager> m_tableManager;

	Zone m_zones[s_numZoneType];
private:
	State(State const &orig);
	State operator=(State const &orig);
};
}

HtmlDocumentGenerator::HtmlDocumentGenerator(char const *fName) :
	m_state(), m_output()
{
	if (fName==0)
		m_output.reset(&std::cout,WPS_shared_ptr_noop_deleter<std::ostream>());
	else
	{
		shared_ptr<std::ofstream> file(new std::ofstream(fName));
		if (!file->good())
			throw WPSResult(WPS_FILE_ACCESS_ERROR);
		m_output=file;
	}
	m_state.reset(new HtmlDocumentGeneratorInternal::State());
}

HtmlDocumentGenerator::~HtmlDocumentGenerator()
{
}

void HtmlDocumentGenerator::setDocumentMetaData(const librevenge::RVNGPropertyList &propList)
{
	m_state->push(HtmlDocumentGeneratorInternal::Z_MetaData);
	std::ostream &meta=m_state->output();
	static char const *metaFields[]= { "author", "subject", "publisher", "keywords", "language",
	                                   "abstract", "descriptive-name", "descriptive-type"
	                                 };
	for (int i = 0; i < 8; i++)
	{
		if (!propList[metaFields[i]])
			continue;
		meta << "<meta name=\"" << metaFields[i] << "\" content=\"" << propList[metaFields[i]]->getStr().cstr() << "\">" << std::endl;
	}
	m_state->pop();
}

void HtmlDocumentGenerator::startDocument()
{
}

void HtmlDocumentGenerator::endDocument()
{
	*m_output << "<!DOCTYPE HTML>" << std::endl;
	*m_output << "<html>" << std::endl;
	*m_output << "<head>" << std::endl;
	*m_output << "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\" >" << std::endl;
	m_state->sendMetaData(*m_output);
	*m_output << "<style>" << std::endl;
	m_state->listManager().send(*m_output);
	m_state->paragraphManager().send(*m_output);
	m_state->spanManager().send(*m_output);
	m_state->tableManager().send(*m_output);
	*m_output << "</style>" << std::endl;
	*m_output << "<title></title>" << std::endl;
	*m_output << "</head>" << std::endl;
	*m_output << "<body>" << std::endl;
	m_state->flushUnsent(*m_output);
	*m_output << "</body>" << std::endl;
	*m_output << "</html>" << std::endl;
}

void HtmlDocumentGenerator::openPageSpan(const librevenge::RVNGPropertyList & /* propList */)
{
	m_state->m_actualPage++;
}

void HtmlDocumentGenerator::closePageSpan()
{
}

void HtmlDocumentGenerator::openHeader(const librevenge::RVNGPropertyList & /* propList */)
{
	m_state->m_ignore = true;
}

void HtmlDocumentGenerator::closeHeader()
{
	m_state->m_ignore = false;
}


void HtmlDocumentGenerator::openFooter(const librevenge::RVNGPropertyList & /* propList */)
{
	m_state->m_ignore = true;
}

void HtmlDocumentGenerator::closeFooter()
{
	m_state->m_ignore = false;
}

void HtmlDocumentGenerator::openParagraph(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &tabStops)
{
	if (m_state->m_ignore)
		return;

	m_state->output(false) << "<p class=\"" << m_state->paragraphManager().getClass(propList, tabStops) << "\">";
}

void HtmlDocumentGenerator::closeParagraph()
{
	if (m_state->m_ignore)
		return;

	m_state->output() << "</p>" << std::endl;
}

void HtmlDocumentGenerator::openSpan(const librevenge::RVNGPropertyList &propList)
{
	if (m_state->m_ignore)
		return;

	m_state->output() << "<span class=\"" << m_state->spanManager().getClass(propList) << "\">";
}

void HtmlDocumentGenerator::closeSpan()
{
	if (m_state->m_ignore)
		return;
	m_state->output() << "</span>" << std::endl;
}

void HtmlDocumentGenerator::insertTab()
{
	if (m_state->m_ignore)
		return;

	// Does not have a lot of effect since tabs in html are ignorable white-space
	m_state->output() << "\t";
}

void HtmlDocumentGenerator::insertLineBreak()
{
	if (m_state->m_ignore)
		return;
	m_state->output() << "<br>" << std::endl;
}

void HtmlDocumentGenerator::insertText(const librevenge::RVNGString &text)
{
	if (m_state->m_ignore)
		return;
	librevenge::RVNGString tempUTF8(text, true);
	m_state->output() << tempUTF8.cstr();
}

void HtmlDocumentGenerator::insertSpace()
{
	if (m_state->m_ignore)
		return;
	m_state->output() << "&nbsp;";
}

void HtmlDocumentGenerator::defineOrderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	if (m_state->m_ignore)
		return;
	m_state->listManager().defineLevel(propList, true);
}

void HtmlDocumentGenerator::defineUnorderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	if (m_state->m_ignore)
		return;
	m_state->listManager().defineLevel(propList, false);
}

void HtmlDocumentGenerator::openOrderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	if (m_state->m_ignore)
		return;
	// fixme: if level is > 1, we must first insert a div here
	m_state->output(false) << "<ol class=\"" << m_state->listManager().openLevel(propList, true) << "\">\n";
}

void HtmlDocumentGenerator::closeOrderedListLevel()
{
	if (m_state->m_ignore)
		return;
	m_state->listManager().closeLevel();
	m_state->output() << "</ol>" << std::endl;
}

void HtmlDocumentGenerator::openUnorderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	if (m_state->m_ignore)
		return;
	// fixme: if level is > 1, we must first insert a div here
	m_state->output(false) << "<ul class=\"" << m_state->listManager().openLevel(propList, false) << "\">\n";
}

void HtmlDocumentGenerator::closeUnorderedListLevel()
{
	if (m_state->m_ignore)
		return;
	m_state->listManager().closeLevel();
	m_state->output() << "</ul>" << std::endl;
}


void HtmlDocumentGenerator::openListElement(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &tabStops)
{
	if (m_state->m_ignore)
		return;
	m_state->output(false) << "<li class=\"" << m_state->listManager().getClass(propList, tabStops) << "\">";
}

void HtmlDocumentGenerator::closeListElement()
{
	if (m_state->m_ignore)
		return;
	m_state->output() << "</li>" << std::endl;
}

void HtmlDocumentGenerator::openFootnote(const librevenge::RVNGPropertyList &)
{
	if (m_state->m_ignore)
		return;
	std::ostream &out=m_state->output();
	m_state->push(HtmlDocumentGeneratorInternal::Z_FootNote);
	m_state->stream().addLabel(out);
}

void HtmlDocumentGenerator::closeFootnote()
{
	if (m_state->m_ignore)
		return;
	m_state->pop();
}

void HtmlDocumentGenerator::openEndnote(const librevenge::RVNGPropertyList &)
{
	if (m_state->m_ignore)
		return;
	std::ostream &out=m_state->output();
	m_state->push(HtmlDocumentGeneratorInternal::Z_EndNote);
	m_state->stream().addLabel(out);
}

void HtmlDocumentGenerator::closeEndnote()
{
	if (m_state->m_ignore)
		return;
	m_state->pop();
}

void HtmlDocumentGenerator::openComment(const librevenge::RVNGPropertyList & /*propList*/)
{
	if (m_state->m_ignore)
		return;
	std::ostream &out=m_state->output();
	m_state->push(HtmlDocumentGeneratorInternal::Z_Comment);
	m_state->stream().addLabel(out);
}

void HtmlDocumentGenerator::closeComment()
{
	if (m_state->m_ignore)
		return;
	m_state->pop();
}

void HtmlDocumentGenerator::openTextBox(const librevenge::RVNGPropertyList & /*propList*/)
{
	if (m_state->m_ignore)
		return;
	std::ostream &out=m_state->output();
	m_state->push(HtmlDocumentGeneratorInternal::Z_TextBox);
	m_state->stream().addLabel(out);
}

void HtmlDocumentGenerator::closeTextBox()
{
	if (m_state->m_ignore)
		return;
	m_state->pop();
}

void HtmlDocumentGenerator::openTable(const librevenge::RVNGPropertyList & /* propList */, const librevenge::RVNGPropertyListVector &columns)
{
	if (m_state->m_ignore)
		return;
	m_state->tableManager().openTable(columns);
	m_state->output() << "<table>" << std::endl;
	m_state->output() << "<tbody>" << std::endl;
}

void HtmlDocumentGenerator::openTableRow(const librevenge::RVNGPropertyList &propList)
{
	if (m_state->m_ignore)
		return;
	m_state->output() << "<tr class=\"" << m_state->tableManager().getRowClass(propList) << "\">\n";
}

void HtmlDocumentGenerator::closeTableRow()
{
	if (m_state->m_ignore)
		return;
	m_state->output() << "</tr>" << std::endl;
}

void HtmlDocumentGenerator::openTableCell(const librevenge::RVNGPropertyList &propList)
{
	if (m_state->m_ignore)
		return;
	std::ostream &out=m_state->output();
	out << "<td class=\"" << m_state->tableManager().getCellClass(propList) << "\"";
	if (propList["table:number-columns-spanned"])
		out << " colspan=\"" << propList["table:number-columns-spanned"]->getInt() << "\"";
	if (propList["table:number-rows-spanned"])
		out << " rowspan=\"" << propList["table:number-rows-spanned"]->getInt() << "\"";
	out << ">" << std::endl;

}

void HtmlDocumentGenerator::closeTableCell()
{
	if (m_state->m_ignore)
		return;
	m_state->output() << "</td>" << std::endl;
}

void HtmlDocumentGenerator::closeTable()
{
	if (m_state->m_ignore)
		return;
	m_state->output() << "</tbody>" << std::endl;
	m_state->output() << "</table>" << std::endl;
	m_state->tableManager().closeTable();
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
