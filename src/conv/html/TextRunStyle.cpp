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

#include <string.h>

#include <cstring>
#include <iostream>
#include <sstream>

#include <librevenge/librevenge.h>

#include "TextRunStyle.h"

////////////////////////////////////////////////////////////
// list manager
////////////////////////////////////////////////////////////
void ListStyleManager::List::setLevel(int lvl, librevenge::RVNGPropertyList const &pList, bool ordered)
{
	if (lvl < 0 || lvl > 30)
	{
		WPS_DEBUG_MSG(("ListStyleManager::List::setLevel: level is %d\n", lvl));
		return;
	}
	if (size_t(lvl) >= m_contentsList.size())
		m_contentsList.resize(size_t(lvl)+1);

	std::stringstream s;
	// fixme: read text:space-before, text:min-label-width, text:min-label-distance
	if (!ordered)
	{
		// fixme read text:bullet-char
	}
	else
	{
		// fixme read style:num-prefix, style:num-suffix, text:start-value
		if (pList["style:num-format"])
		{
			if (strcmp(pList["style:num-format"]->getStr().cstr(),"a")==0)
				s << " list-style-type:lower-alpha;";
			else if (strcmp(pList["style:num-format"]->getStr().cstr(),"A")==0)
				s << " list-style-type:upper-alpha;";
			else if (strcmp(pList["style:num-format"]->getStr().cstr(),"i")==0)
				s << " list-style-type:lower-roman;";
			else if (strcmp(pList["style:num-format"]->getStr().cstr(),"I")==0)
				s << " list-style-type:upper-roman;";
			else if (strcmp(pList["style:num-format"]->getStr().cstr(),"1")==0)
				s << " list-style-type:decimal;";
			else
			{
				WPS_DEBUG_MSG(("ListStyleManager::List::setLevel: unknown format %s\n",
				               pList["style:num-format"]->getStr().cstr()));
			}
		}
	}
	s << "\n";
	m_contentsList[size_t(lvl)]=s.str();
}

std::string ListStyleManager::List::str() const
{
	if (m_level < 0 || size_t(m_level) >= m_contentsList.size())
	{
		WPS_DEBUG_MSG(("ListStyleManager::List::str: level %d is not set\n", m_level));
		return "";
	}
	return m_contentsList[size_t(m_level)];
}

void ListStyleManager::send(std::ostream &out)
{
	ParagraphStyleManager::send(out);
	std::map<std::string, std::string>::iterator it=m_levelNameMap.begin();
	while (it != m_levelNameMap.end())
	{
		out << "." << it->second << " {\n" << it->first << "}\n";
		++it;
	}
}

std::string ListStyleManager::getClass(librevenge::RVNGPropertyList const &pList, librevenge::RVNGPropertyListVector const &)
{
	std::string content=getContent(pList, true);
	std::map<std::string, std::string>::iterator it=m_contentNameMap.find(content);
	if (it != m_contentNameMap.end())
		return it->second;
	std::stringstream s;
	s << "listElt" << m_contentNameMap.size();
	m_contentNameMap[content]=s.str();
	return s.str();
}

void ListStyleManager::defineLevel(librevenge::RVNGPropertyList const &pList, bool ordered)
{
	if (!pList["libwpd:id"])
	{
		WPS_DEBUG_MSG(("ListStyleManager::defineLevel: can not find list id\n"));
		return;
	}
	int id = pList["libwpd:id"]->getInt();
	if (m_idListMap.find(id)==m_idListMap.end())
		m_idListMap[id]=List();
	if (!pList["libwpd:level"])
	{
		WPS_DEBUG_MSG(("ListStyleManager::defineLevel: can not find list level\n"));
		return;
	}
	m_idListMap.find(id)->second.setLevel(pList["libwpd:level"]->getInt(), pList, ordered);
}

std::string ListStyleManager::openLevel(librevenge::RVNGPropertyList const &pList, bool /*ordered*/)
{
	int id = -1;
	if (!pList["libwpd:id"])
	{
		WPS_DEBUG_MSG(("ListStyleManager::openLevel: can not find list id\n"));
	}
	else
		id = pList["libwpd:id"]->getInt();
	m_actualIdStack.push_back(id);

	std::string content=getContent(pList, true);
	if (id >= 0 && m_idListMap.find(id)!=m_idListMap.end())
	{
		m_idListMap.find(id)->second.openLevel();
		content+=m_idListMap.find(id)->second.str();
	}
	std::map<std::string, std::string>::iterator it=m_levelNameMap.find(content);
	if (it != m_levelNameMap.end())
		return it->second;
	std::stringstream s;
	s << "listLevel" << m_levelNameMap.size();
	m_levelNameMap[content]=s.str();

	return s.str();
}

void ListStyleManager::closeLevel()
{
	if (!m_actualIdStack.size())
	{
		WPS_DEBUG_MSG(("ListStyleManager::closeLevel: can not find level\n"));
		return;
	}
	int id=m_actualIdStack.back();
	if (id >= 0 && m_idListMap.find(id)!=m_idListMap.end())
		m_idListMap.find(id)->second.closeLevel();

	m_actualIdStack.pop_back();
}


////////////////////////////////////////////////////////////
// paragraph manager
////////////////////////////////////////////////////////////
std::string ParagraphStyleManager::getClass(librevenge::RVNGPropertyList const &pList, librevenge::RVNGPropertyListVector const &)
{
	std::string content=getContent(pList, false);
	std::map<std::string, std::string>::iterator it=m_contentNameMap.find(content);
	if (it != m_contentNameMap.end())
		return it->second;
	std::stringstream s;
	s << "para" << m_contentNameMap.size();
	m_contentNameMap[content]=s.str();
	return s.str();
}

void ParagraphStyleManager::send(std::ostream &out)
{
	std::map<std::string, std::string>::iterator it=m_contentNameMap.begin();
	while (it != m_contentNameMap.end())
	{
		out << "." << it->second << " {\n" << it->first << "}\n";
		++it;
	}
}

std::string ParagraphStyleManager::getContent(librevenge::RVNGPropertyList const &pList, bool isList) const
{
	std::stringstream s;
	if (pList["fo:text-align"])
	{
		if (pList["fo:text-align"]->getStr() == librevenge::RVNGString("end")) // stupid OOo convention..
			s << " text-align:right;";
		else
			s << " text-align:" << pList["fo:text-align"]->getStr().cstr() << ";";
	}
	// the margins
	s << " margin:";
	if (pList["fo:margin-top"])
		s << " " << pList["fo:margin-top"]->getStr().cstr();
	else
		s << " 0px";
	if (pList["fo:margin-right"])
		s << " " << pList["fo:margin-right"]->getStr().cstr();
	else
		s << " 0px";
	if (pList["fo:margin-bottom"])
		s << " " << pList["fo:margin-bottom"]->getStr().cstr();
	else
		s << " 0px";
	if (isList)
	{
		double val=0, tmp;
		if (pList["fo:margin-left"]
		        && wps2html::getPointValue(*pList["fo:margin-left"], tmp))
		{
			val += tmp;
		}
		if (pList["fo:text-indent"]
		        && wps2html::getPointValue(*pList["fo:text-indent"], tmp))
		{
			val += tmp;
		}
		val -= 10; // checkme: seems to big, so decrease it
		s << " " << val << "px";
	}
	else if (pList["fo:margin-left"])
		s << " " << pList["fo:margin-left"]->getStr().cstr();
	else
		s << " 0px";
	s << ";";
	if (pList["fo:text-indent"])
	{
		s << " text-indent:" << pList["fo:text-indent"]->getStr().cstr() << ";";
		if (isList && pList["fo:text-indent"]->getStr().cstr()[0]=='-')
			s << " padding-left:" << pList["fo:text-indent"]->getStr().cstr()+1 << ";";
	}
	// line height
	if (pList["fo:line-height"] && (pList["fo:line-height"]->getDouble()<0.999||pList["fo:line-height"]->getDouble()>1.001))
		s << " line-height:" << pList["fo:line-height"]->getStr().cstr() << ";";
	if (pList["style:line-height-at-least"] && (pList["style:line-height-at-least"]->getDouble()<0.999||pList["style:line-height-at-least"]->getDouble()>1.001))
		s << " min-height:" << pList["style:line-height-at-least"]->getStr().cstr() << ";";
	// other: background, border
	if (pList["fo:background-color"])
		s << " background-color: " << pList["fo:background-color"]->getStr().cstr() << ";";
	parseBorders(pList, s);
	s << "\n";
	return s.str();
}

void ParagraphStyleManager::parseBorders(librevenge::RVNGPropertyList const &pList, std::ostream &out) const
{
	static char const *(type[]) = {"border", "border-left", "border-top", "border-right", "border-bottom" };
	for (int i = 0; i < 5; i++)
	{
		std::string field("fo:");
		field+=type[i];
		if (!pList[field.c_str()])
			continue;
		out << " " << type[i] << ": " << pList[field.c_str()]->getStr().cstr() << ";";
		// does not seems to works with negative text-indent, so add a padding
		if (i<=1 && pList["fo:text-indent"] && pList["fo:text-indent"]->getDouble()<0 &&
		        pList["fo:text-indent"]->getStr().cstr()[0]=='-')
			out << "\tpadding-left:" << pList["fo:text-indent"]->getStr().cstr()+1 << ";\n";
	}
}


////////////////////////////////////////////////////////////
// span manager
////////////////////////////////////////////////////////////
std::string SpanStyleManager::getClass(librevenge::RVNGPropertyList const &pList)
{
	std::string content=getContent(pList);
	std::map<std::string, std::string>::iterator it=m_contentNameMap.find(content);
	if (it != m_contentNameMap.end())
		return it->second;
	std::stringstream s;
	s << "span" << m_contentNameMap.size();
	m_contentNameMap[content]=s.str();
	return s.str();
}

void SpanStyleManager::send(std::ostream &out)
{
	std::map<std::string, std::string>::iterator it=m_contentNameMap.begin();
	while (it != m_contentNameMap.end())
	{
		out << "." << it->second << " " << it->first << "\n";
		++it;
	}
}

std::string SpanStyleManager::getContent(librevenge::RVNGPropertyList const &pList) const
{
	std::stringstream s;
	s << "{\n";
	if (pList["fo:background-color"])
		s << " background-color: " << pList["fo:background-color"]->getStr().cstr() << ";";
	if (pList["fo:color"])
		s << " color: " << pList["fo:color"]->getStr().cstr() << ";";
	if (pList["fo:font-size"])
		s << " font-size: " << pList["fo:font-size"]->getStr().cstr() << ";";
	if (pList["fo:font-style"])
		s << " font-style: " << pList["fo:font-style"]->getStr().cstr() << ";";
	if (pList["fo:font-variant"])
		s << " font-variant: " << pList["fo:font-variant"]->getStr().cstr() << ";";
	if (pList["fo:font-weight"])
		s << " font-weight: " << pList["fo:font-weight"]->getStr().cstr() << ";";
	if (pList["fo:letter-spacing"])
		s << " letter-spacing: " << pList["fo:letter-spacing"]->getStr().cstr() << ";";
	if (pList["fo:text-shadow"])
		s << " text-shadow: 1px 1px 1px #666666;";
	if (pList["fo:text-transform"])
		s << " text-transform: " << pList["fo:text-transform"]->getStr().cstr() << ";";

	if (pList["style:font-name"])
		s << " font-family: \'" << pList["style:font-name"]->getStr().cstr() << "\';";
	if (pList["style:text-blinking"])
		s << " text-decoration: blink;";
	parseDecorations(pList, s);
	if (pList["style:text-position"])
		parseTextPosition(pList["style:text-position"]->getStr().cstr(),s);

	if (pList["text:display"])
		s << " display: " << pList["text:display"]->getStr().cstr() << ";";

	// checkme not working with Safari 6.02...
	if (pList["style:font-relief"] && pList["style:font-relief"]->getStr().cstr())
	{
		if (strcmp(pList["style:font-relief"]->getStr().cstr(),"embossed")==0)
			s << " font-effect: emboss;";
		else if (strcmp(pList["style:font-relief"]->getStr().cstr(),"engraved")==0)
			s << " font-effect: engrave;";
	}
	if (pList["style:text-outline"])
		s << " font-effect: outline;";

	if (pList["style:text-scale"])
	{
		if (pList["style:text-scale"]->getDouble() < 0.2)
			s << " font-stretch: ultra-condensed;";
		else if (pList["style:text-scale"]->getDouble() < 0.4)
			s << " font-stretch: extra-condensed;";
		else if (pList["style:text-scale"]->getDouble() < 0.6)
			s << " font-stretch: condensed;";
		else if (pList["style:text-scale"]->getDouble() < 0.8)
			s << " font-stretch: semi-condensed;";
		else if (pList["style:text-scale"]->getDouble() > 2.0)
			s << " font-stretch: ultra-expanded;";
		else if (pList["style:text-scale"]->getDouble() > 1.6)
			s << " font-stretch: extra-expanded;";
		else if (pList["style:text-scale"]->getDouble() > 1.4)
			s << " font-stretch: expanded;";
		else if (pList["style:text-scale"]->getDouble() > 1.2)
			s << " font-stretch: semi-expanded;";

	}
	s << "\n}";

	return s.str();
}

void SpanStyleManager::parseDecorations(librevenge::RVNGPropertyList const &pList, std::ostream &out) const
{
	// replaceme by text-decoration-line when its implementation will appear in browser
	std::stringstream s;

	if (pList["style:text-line-through-style"] || pList["style:text-line-through-type"])
		s << " line-through";
	if (pList["style:text-overline-style"] || pList["style:text-overline-type"])
		s << " overline";
	if (pList["style:text-underline-style"] || pList["style:text-underline-type"])
		s << " underline";
	if (s.str().length())
		out << " text-decoration:" << s.str() << ";";
}

void SpanStyleManager::parseTextPosition(char const *value, std::ostream &out) const
{
	if (!value) return;
	// first try to retrieve the position
	std::stringstream s("");
	double pos=0;
	if (strncmp(value,"super", 5)==0)
	{
		pos=33;
		s << value+5;
	}
	else if (strncmp(value,"sub", 3)==0)
	{
		pos=-33;
		s << value+3;
	}
	else
	{
		s << value;
		s >> pos;
		if (!s.good())
		{
			WPS_DEBUG_MSG(("SpanStyleManager::parseTextPosition: can not read %s\n", value));
			return;
		}
		if (s.peek()=='%')
		{
			char c;
			s >> c;
		}
	}
	double percent;
	s >> percent;
	if (!s.good() || percent <= 0 || percent > 100)
	{
		WPS_DEBUG_MSG(("SpanStyleManager::parseTextPosition: can not read percent: find %s\n", s.str().c_str()));
		percent=100;
	}
	if (pos <= 0 && pos >= 0) return;
	out << " vertical-align: baseline;";
	out << " position: relative;";
	out << " top: " << -pos/100. << "em;";
	if (percent < 100)
		out << " font-size:" << percent/100. << "em;";
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
