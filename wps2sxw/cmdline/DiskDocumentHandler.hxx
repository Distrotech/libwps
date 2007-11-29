/* 
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2004-2007 Fridrich Strba (fridrich.strba@bluewin.ch)
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
  *
 * For further information visit http://libwpd.sourceforge.net
 *
 */

#ifndef _DISKDOCUMENTHANDLER_H
#define _DISKDOCUMENTHANDLER_H
#include "DocumentElement.hxx"
#include <libwpd/libwpd.h>
#include <iostream>
#include <sstream>

class DiskDocumentHandler : public DocumentHandler
{
  public:
        DiskDocumentHandler(std::ostringstream &contentStream);
        virtual void startDocument() {}
        virtual void endDocument();
        virtual void startElement(const char *psName, const WPXPropertyList &xPropList);
        virtual void endElement(const char *psName);
        virtual void characters(const WPXString &sCharacters);

  private:
	std::ostringstream &mContentStream;
	bool mbIsTagOpened;
	WPXString msOpenedTagName;
};
#endif
