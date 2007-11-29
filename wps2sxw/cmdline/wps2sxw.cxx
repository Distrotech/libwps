/* wps2sxw:
 *
 * Copyright (C) 2002 Jon K Hellan (hellan@acm.org)
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2003-2004 Net Integration Technologies (http://www.net-itech.com)
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <libwps/libwps.h>
#include <libwps/WPSStreamImplementation.h>

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>

#include "MSWorksCollector.hxx"
#include "DiskDocumentHandler.hxx"
#include "StdOutHandler.hxx"
#include "femtozip.hxx"

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		std::cout << "usage: wps2sxw <MS Works source document> <sxw target document>\n";
		std::cout << "       or\n";
		std::cout << "       wps2sxw <MS Works source document> (to output to standard output)\n";
		return -1;
	}

	const char* wpsfile = argv[1];

	WPSInputStream* input = new WPSFileStream(wpsfile);

	WPSConfidence confidence = WPSDocument::isFileFormatSupported(input, false);
	if (confidence != WPS_CONFIDENCE_EXCELLENT && confidence != WPS_CONFIDENCE_GOOD)
	{
		std::cout << "ERROR: Unsupported file format!" << std::endl;
		delete input;
		return 0;
	}

	bool retVal;
	if (argc < 3)
	{
		StdOutHandler tmpHandler;
		MSWorksCollector collector(input, &tmpHandler);
		retVal = collector.filter();
		if (retVal)
			return 0;
		else
			return -1;
	}

	const char* sxwfile = argv[2];

	const char mimetypeStr[] = "application/vnd.sun.xml.writer";

	FemtoZip zip(sxwfile);
	zip.createEntry("mimetype", 0);
	zip.writeString(mimetypeStr);
	zip.closeEntry();

	const char manifestStr[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<!DOCTYPE manifest:manifest PUBLIC \"-//OpenOffice.org//DTD Manifest 1.0//EN\" \"Manifest.dtd\">"
	"<manifest:manifest xmlns:manifest=\"http://openoffice.org/2001/manifest\">"
 	"<manifest:file-entry manifest:media-type=\"application/vnd.sun.xml.writer\" manifest:full-path=\"/\"/>"
 	"<manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"content.xml\"/>"
 	"<manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"styles.xml\"/>"
	"</manifest:manifest>";

	zip.createEntry("META-INF/manifest.xml", 0);
	zip.writeString(manifestStr);
	zip.closeEntry();

	const char stylesStr[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<!DOCTYPE office:document-styles PUBLIC \"-//OpenOffice.org//DTD OfficeDocument 1.0//EN\" \"office.dtd\">"
	"<office:document-styles xmlns:office=\"http://openoffice.org/2000/office\" xmlns:style=\"http://openoffice.org/2000/style\""
	" xmlns:text=\"http://openoffice.org/2000/text\" xmlns:table=\"http://openoffice.org/2000/table\""
	" xmlns:draw=\"http://openoffice.org/2000/drawing\" xmlns:fo=\"http://www.w3.org/1999/XSL/Format\""
	" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:number=\"http://openoffice.org/2000/datastyle\""
	" xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns:chart=\"http://openoffice.org/2000/chart\" xmlns:dr3d=\"http://openoffice.org/2000/dr3d\""
	" xmlns:math=\"http://www.w3.org/1998/Math/MathML\" xmlns:form=\"http://openoffice.org/2000/form\""
	" xmlns:script=\"http://openoffice.org/2000/script\" office:version=\"1.0\">"
	"<office:styles>"
	"<style:default-style style:family=\"paragraph\">"
	"<style:properties style:use-window-font-color=\"true\" style:text-autospace=\"ideograph-alpha\""
	" style:punctuation-wrap=\"hanging\" style:line-break=\"strict\" style:writing-mode=\"page\"/>"
	"</style:default-style>"
	"<style:default-style style:family=\"table\"/>"
	"<style:default-style style:family=\"table-row\"/>"
	"<style:default-style style:family=\"table-column\"/>"
	"<style:style style:name=\"Standard\" style:family=\"paragraph\" style:class=\"text\"/>"
	"<style:style style:name=\"Text body\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"text\"/>"
	"<style:style style:name=\"List\" style:family=\"paragraph\" style:parent-style-name=\"Text body\" style:class=\"list\"/>"
	"<style:style style:name=\"Header\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>"
	"<style:style style:name=\"Footer\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>"
	"<style:style style:name=\"Caption\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>"
	"<style:style style:name=\"Footnote\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>"
	"<style:style style:name=\"Endnote\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>"
	"<style:style style:name=\"Index\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"index\"/>"
	"<style:style style:name=\"Footnote Symbol\" style:family=\"text\">"
	"<style:properties style:text-position=\"super 58%\"/>"
	"</style:style>"
	"<style:style style:name=\"Endnote Symbol\" style:family=\"text\">"
	"<style:properties style:text-position=\"super 58%\"/>"
	"</style:style>"
	"<style:style style:name=\"Footnote anchor\" style:family=\"text\">"
	"<style:properties style:text-position=\"super 58%\"/>"
	"</style:style>"
	"<style:style style:name=\"Endnote anchor\" style:family=\"text\">"
	"<style:properties style:text-position=\"super 58%\"/>"
	"</style:style>"
	"<text:footnotes-configuration text:citation-style-name=\"Footnote Symbol\" text:citation-body-style-name=\"Footnote anchor\""
	" style:num-format=\"1\" text:start-value=\"0\" text:footnotes-position=\"page\" text:start-numbering-at=\"document\"/>"
	"<text:endnotes-configuration text:citation-style-name=\"Endnote Symbol\" text:citation-body-style-name=\"Endnote anchor\""
	" text:master-page-name=\"Endnote\" style:num-format=\"i\" text:start-value=\"0\"/>"
	"<text:linenumbering-configuration text:number-lines=\"false\" text:offset=\"0.1965inch\" style:num-format=\"1\""
	" text:number-position=\"left\" text:increment=\"5\"/>"
	"</office:styles>"
	"<office:automatic-styles>"
	"<style:page-master style:name=\"PM0\">"
	"<style:properties fo:margin-bottom=\"1.0000inch\" fo:margin-left=\"1.0000inch\" fo:margin-right=\"1.0000inch\" fo:margin-top=\"1.0000inch\""
	" fo:page-height=\"11.0000inch\" fo:page-width=\"8.5000inch\" style:print-orientation=\"portrait\">"
	"<style:footnote-sep style:adjustment=\"left\" style:color=\"#000000\" style:distance-after-sep=\"0.0398inch\""
	" style:distance-before-sep=\"0.0398inch\" style:rel-width=\"25%\" style:width=\"0.0071inch\"/>"
	"</style:properties>"
	"</style:page-master>"
	"<style:page-master style:name=\"PM1\">"
	"<style:properties fo:margin-bottom=\"1.0000inch\" fo:margin-left=\"1.0000inch\" fo:margin-right=\"1.0000inch\" fo:margin-top=\"1.0000inch\""
	" fo:page-height=\"11.0000inch\" fo:page-width=\"8.5000inch\" style:print-orientation=\"portrait\">"
	"<style:footnote-sep style:adjustment=\"left\" style:color=\"#000000\" style:rel-width=\"25%\"/>"
	"</style:properties>"
	"</style:page-master>"
	"</office:automatic-styles>"
	"<office:master-styles>"
	"<style:master-page style:name=\"Standard\" style:page-master-name=\"PM0\"/>"
	"<style:master-page style:name=\"Endnote\" style:page-master-name=\"PM1\"/>"
	"</office:master-styles>"
	"</office:document-styles>";

	zip.createEntry("styles.xml", 0);
	zip.writeString(stylesStr);
	zip.closeEntry();

	std::ostringstream tmpStringStream;
	DiskDocumentHandler tmpHandler(tmpStringStream);
	MSWorksCollector collector(input, &tmpHandler);
	retVal = collector.filter();
	
	zip.createEntry("content.xml", 0);
	zip.writeString(tmpStringStream.str().c_str());
	zip.closeEntry();

	if (retVal)
		return 0;
	else
		return -1;
}
