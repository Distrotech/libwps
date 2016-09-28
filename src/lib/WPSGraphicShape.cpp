/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002, 2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002, 2004 Marc Maurer (uwog@uwog.net)
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

#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <librevenge/librevenge.h>

#include "libwps_internal.h"

#include "WPSGraphicShape.h"

////////////////////////////////////////////////////////////
// WPSGraphicShape::PathData
////////////////////////////////////////////////////////////
std::ostream &operator<<(std::ostream &o, WPSGraphicShape::PathData const &path)
{
	o << path.m_type;
	switch (path.m_type)
	{
	case 'H':
		o << ":" << path.m_x[0];
		break;
	case 'V':
		o << ":" << path.m_x[1];
		break;
	case 'M':
	case 'L':
	case 'T':
		o << ":" << path.m_x;
		break;
	case 'Q':
	case 'S':
		o << ":" << path.m_x << ":" << path.m_x1;
		break;
	case 'C':
		o << ":" << path.m_x << ":" << path.m_x1 << ":" << path.m_x2;
		break;
	case 'A':
		o << ":" << path.m_x << ":r=" << path.m_r;
		if (path.m_largeAngle) o << ":largeAngle";
		if (path.m_sweep) o << ":sweep";
		if (path.m_rotate<0 || path.m_rotate>0) o << ":rot=" << path.m_rotate;
	case 'Z':
		break;
	default:
		o << "###";
	}
	return o;
}

void WPSGraphicShape::PathData::translate(Vec2f const &decal)
{
	if (m_type=='Z')
		return;
	m_x += decal;
	if (m_type=='H' || m_type=='V' || m_type=='M' || m_type=='L' || m_type=='T' || m_type=='A')
		return;
	m_x1 += decal;
	if (m_type=='Q' || m_type=='S')
		return;
	m_x2 += decal;
}

void WPSGraphicShape::PathData::scale(Vec2f const &scaling)
{
	if (m_type=='Z')
		return;
	m_x = Vec2f(m_x[0]*scaling[0], m_x[1]*scaling[1]);
	if (m_type=='H' || m_type=='V' || m_type=='M' || m_type=='L' || m_type=='T' || m_type=='A')
		return;
	m_x1 = Vec2f(m_x1[0]*scaling[0], m_x1[1]*scaling[1]);
	if (m_type=='Q' || m_type=='S')
		return;
	m_x2 = Vec2f(m_x2[0]*scaling[0], m_x2[1]*scaling[1]);
}

void WPSGraphicShape::PathData::rotate(float angle, Vec2f const &decal)
{
	if (m_type=='Z')
		return;
	float angl=angle*float(M_PI/180.);
	m_x = Vec2f(std::cos(angl)*m_x[0]-std::sin(angl)*m_x[1],
	            std::sin(angl)*m_x[0]+std::cos(angl)*m_x[1])+decal;
	if (m_type=='A')
	{
		m_rotate += angle;
		return;
	}
	if (m_type=='H' || m_type=='V' || m_type=='M' || m_type=='L' || m_type=='T')
		return;
	m_x1 = Vec2f(std::cos(angl)*m_x1[0]-std::sin(angl)*m_x1[1],
	             std::sin(angl)*m_x1[0]+std::cos(angl)*m_x1[1])+decal;
	if (m_type=='Q' || m_type=='S')
		return;
	m_x2 = Vec2f(std::cos(angl)*m_x2[0]-std::sin(angl)*m_x2[1],
	             std::sin(angl)*m_x2[0]+std::cos(angl)*m_x2[1])+decal;
}

void WPSGraphicShape::PathData::transform(WPSTransformation const &matrix, float rotation)
{
	if (m_type=='Z')
		return;
	m_x = matrix*m_x;
	if (m_type=='A')
	{
		m_rotate += rotation;
		return;
	}
	if (m_type=='H' || m_type=='V' || m_type=='M' || m_type=='L' || m_type=='T')
		return;
	m_x1 = matrix*m_x1;
	if (m_type=='Q' || m_type=='S')
		return;
	m_x2 = matrix*m_x2;
}

bool WPSGraphicShape::PathData::get(librevenge::RVNGPropertyList &list, Vec2f const &orig) const
{
	list.clear();
	std::string type("");
	type += m_type;
	list.insert("librevenge:path-action", type.c_str());
	if (m_type=='Z')
		return true;
	if (m_type=='H')
	{
		list.insert("svg:x",m_x[0]-orig[0], librevenge::RVNG_POINT);
		return true;
	}
	if (m_type=='V')
	{
		list.insert("svg:y",m_x[1]-orig[1], librevenge::RVNG_POINT);
		return true;
	}
	list.insert("svg:x",m_x[0]-orig[0], librevenge::RVNG_POINT);
	list.insert("svg:y",m_x[1]-orig[1], librevenge::RVNG_POINT);
	if (m_type=='M' || m_type=='L' || m_type=='T')
		return true;
	if (m_type=='A')
	{
		list.insert("svg:rx",m_r[0], librevenge::RVNG_POINT);
		list.insert("svg:ry",m_r[1], librevenge::RVNG_POINT);
		list.insert("librevenge:large-arc", m_largeAngle);
		list.insert("librevenge:sweep", m_sweep);
		list.insert("librevenge:rotate", m_rotate, librevenge::RVNG_GENERIC);
		return true;
	}
	list.insert("svg:x1",m_x1[0]-orig[0], librevenge::RVNG_POINT);
	list.insert("svg:y1",m_x1[1]-orig[1], librevenge::RVNG_POINT);
	if (m_type=='Q' || m_type=='S')
		return true;
	list.insert("svg:x2",m_x2[0]-orig[0], librevenge::RVNG_POINT);
	list.insert("svg:y2",m_x2[1]-orig[1], librevenge::RVNG_POINT);
	if (m_type=='C')
		return true;
	WPS_DEBUG_MSG(("WPSGraphicShape::PathData::get: unknown command %c\n", m_type));
	list.clear();
	return false;
}

int WPSGraphicShape::PathData::cmp(WPSGraphicShape::PathData const &a) const
{
	if (m_type < a.m_type) return 1;
	if (m_type > a.m_type) return 1;
	int diff = m_x.cmp(a.m_x);
	if (diff) return diff;
	diff = m_x1.cmp(a.m_x1);
	if (diff) return diff;
	diff = m_x2.cmp(a.m_x2);
	if (diff) return diff;
	diff = m_r.cmp(a.m_r);
	if (diff) return diff;
	if (m_rotate < a.m_rotate) return 1;
	if (m_rotate > a.m_rotate) return -1;
	if (m_largeAngle != a.m_largeAngle)
		return m_largeAngle ? 1 : -1;
	if (m_sweep != a.m_sweep)
		return m_sweep ? 1 : -1;
	return 0;
}

////////////////////////////////////////////////////////////
// WPSGraphicShape
////////////////////////////////////////////////////////////
WPSGraphicShape WPSGraphicShape::line(Vec2f const &orig, Vec2f const &dest)
{
	WPSGraphicShape res;
	res.m_type = WPSGraphicShape::Line;
	res.m_vertices.resize(2);
	res.m_vertices[0]=orig;
	res.m_vertices[1]=dest;
	Vec2f minPt(orig), maxPt(orig);
	for (int c=0; c<2; ++c)
	{
		if (orig[c] < dest[c])
			maxPt[c]=dest[c];
		else
			minPt[c]=dest[c];
	}
	res.m_bdBox=WPSBox2f(minPt,maxPt);
	return res;
}

std::ostream &operator<<(std::ostream &o, WPSGraphicShape const &sh)
{
	o << "box=" << sh.m_bdBox << ",";
	switch (sh.m_type)
	{
	case WPSGraphicShape::Line:
		o << "line,";
		if (sh.m_vertices.size()!=2)
			o << "###pts,";
		else
			o << "pts=" << sh.m_vertices[0] << "<->" << sh.m_vertices[1] << ",";
		break;
	case WPSGraphicShape::Rectangle:
		o << "rect,";
		if (sh.m_formBox!=sh.m_bdBox)
			o << "box[rect]=" << sh.m_formBox << ",";
		if (sh.m_cornerWidth!=Vec2f(0,0))
			o << "corners=" << sh.m_cornerWidth << ",";
		break;
	case WPSGraphicShape::Circle:
		o << "circle,";
		break;
	case WPSGraphicShape::Arc:
	case WPSGraphicShape::Pie:
		o << (sh.m_type == WPSGraphicShape::Arc ? "arc," : "pie,");
		o << "box[ellipse]=" << sh.m_formBox << ",";
		o << "angle=" << sh.m_arcAngles << ",";
		break;
	case WPSGraphicShape::Polygon:
		o << "polygons,pts=[";
		for (size_t pt=0; pt < sh.m_vertices.size(); ++pt)
			o << sh.m_vertices[pt] << ",";
		o << "],";
		break;
	case WPSGraphicShape::Path:
		o << "path,pts=[";
		for (size_t pt=0; pt < sh.m_path.size(); ++pt)
			o << sh.m_path[pt] << ",";
		o << "],";
		break;
	case WPSGraphicShape::ShapeUnknown:
	default:
		o << "###unknwown[shape],";
		break;
	}
	o << sh.m_extra;
	return o;
}

int WPSGraphicShape::cmp(WPSGraphicShape const &a) const
{
	if (m_type < a.m_type) return 1;
	if (m_type > a.m_type) return -1;
	int diff = m_bdBox.cmp(a.m_bdBox);
	if (diff) return diff;
	diff = m_formBox.cmp(a.m_formBox);
	if (diff) return diff;
	diff = m_cornerWidth.cmp(a.m_cornerWidth);
	if (diff) return diff;
	diff = m_arcAngles.cmp(a.m_arcAngles);
	if (diff) return diff;
	if (m_vertices.size()<a.m_vertices.size()) return -1;
	if (m_vertices.size()>a.m_vertices.size()) return -1;
	for (size_t pt=0; pt < m_vertices.size(); ++pt)
	{
		diff = m_vertices[pt].cmp(a.m_vertices[pt]);
		if (diff) return diff;
	}
	if (m_path.size()<a.m_path.size()) return -1;
	if (m_path.size()>a.m_path.size()) return -1;
	for (size_t pt=0; pt < m_path.size(); ++pt)
	{
		diff = m_path[pt].cmp(a.m_path[pt]);
		if (diff) return diff;
	}
	return 0;
}

void WPSGraphicShape::translate(Vec2f const &decal)
{
	if (decal==Vec2f(0,0))
		return;
	m_bdBox=WPSBox2f(m_bdBox.min()+decal, m_bdBox.max()+decal);
	m_formBox=WPSBox2f(m_formBox.min()+decal, m_formBox.max()+decal);
	for (size_t pt=0; pt<m_vertices.size(); ++pt)
		m_vertices[pt]+=decal;
	for (size_t pt=0; pt<m_path.size(); ++pt)
		m_path[pt].translate(decal);
}

void WPSGraphicShape::scale(Vec2f const &scaling)
{
	m_bdBox=WPSBox2f(Vec2f(scaling[0]*m_bdBox.min()[0],scaling[1]*m_bdBox.min()[1]),
	                 Vec2f(scaling[0]*m_bdBox.max()[0],scaling[1]*m_bdBox.max()[1]));
	m_formBox=WPSBox2f(Vec2f(scaling[0]*m_formBox.min()[0],scaling[1]*m_formBox.min()[1]),
	                   Vec2f(scaling[0]*m_formBox.max()[0],scaling[1]*m_formBox.max()[1]));
	for (size_t pt=0; pt<m_vertices.size(); ++pt)
		m_vertices[pt]=Vec2f(scaling[0]*m_vertices[pt][0],
		                     scaling[1]*m_vertices[pt][1]);
	for (size_t pt=0; pt<m_path.size(); ++pt)
		m_path[pt].scale(scaling);
}

WPSGraphicShape WPSGraphicShape::rotate(float angle, Vec2f const &center) const
{
	while (angle >= 360) angle -= 360;
	while (angle <= -360) angle += 360;
	if (angle >= -1e-3 && angle <= 1e-3) return *this;
	float angl=angle*float(M_PI/180.);
	Vec2f decal=center-Vec2f(std::cos(angl)*center[0]-std::sin(angl)*center[1],
	                         std::sin(angl)*center[0]+std::cos(angl)*center[1]);
	WPSBox2f fBox;
	for (int i=0; i < 4; ++i)
	{
		Vec2f pt=Vec2f(m_bdBox[i%2][0],m_bdBox[i/2][1]);
		pt = Vec2f(std::cos(angl)*pt[0]-std::sin(angl)*pt[1],
		           std::sin(angl)*pt[0]+std::cos(angl)*pt[1])+decal;
		if (i==0) fBox=WPSBox2f(pt,pt);
		else fBox=fBox.getUnion(WPSBox2f(pt,pt));
	}
	WPSGraphicShape res = path(fBox);
	res.m_path=getPath(false);
	for (size_t p=0; p < res.m_path.size(); p++)
		res.m_path[p].rotate(angle, decal);
	return res;
}

WPSGraphicShape WPSGraphicShape::transform(WPSTransformation const &matrix) const
{
	if (matrix.isIdentity()) return *this;
	if (matrix[0][1]<=0 && matrix[0][1]>=0 && matrix[1][0]<=0 && matrix[1][0]>=0)
	{
		WPSGraphicShape res=*this;
		if (matrix[0][0]<1 || matrix[0][0]>1 || matrix[1][1]<1 || matrix[1][1]>1)
			res.scale(Vec2f(matrix[0][0], matrix[1][1]));
		res.translate(Vec2f(matrix[0][2],matrix[1][2]));
		return res;
	}

	WPSBox2f fBox;
	for (int i=0; i < 4; ++i)
	{
		Vec2f pt = matrix*Vec2f(m_bdBox[i%2][0],m_bdBox[i/2][1]);
		if (i==0) fBox=WPSBox2f(pt,pt);
		else fBox=fBox.getUnion(WPSBox2f(pt,pt));
	}
	WPSGraphicShape res = path(fBox);
	res.m_path=getPath(true);

	WPSTransformation transf;
	float rotation=0;
	Vec2f shearing;
	if (!matrix.decompose(rotation,shearing,transf,fBox.center()))
		rotation=0;
	for (size_t p=0; p < res.m_path.size(); p++)
		res.m_path[p].transform(matrix, rotation);
	return res;
}

WPSGraphicShape::Command WPSGraphicShape::addTo(Vec2f const &orig, bool asSurface, librevenge::RVNGPropertyList &propList) const
{
	Vec2f pt;
	librevenge::RVNGPropertyList list;
	librevenge::RVNGPropertyListVector vect;
	Vec2f decal=orig-m_bdBox[0];
	switch (m_type)
	{
	case Line:
		if (m_vertices.size()!=2) break;
		pt=m_vertices[0]+decal;
		list.insert("svg:x",pt.x(), librevenge::RVNG_POINT);
		list.insert("svg:y",pt.y(), librevenge::RVNG_POINT);
		vect.append(list);
		pt=m_vertices[1]+decal;
		list.insert("svg:x",pt.x(), librevenge::RVNG_POINT);
		list.insert("svg:y",pt.y(), librevenge::RVNG_POINT);
		vect.append(list);
		propList.insert("svg:points", vect);
		return C_Polyline;
	case Rectangle:
		if (m_cornerWidth[0] > 0 && m_cornerWidth[1] > 0)
		{
			propList.insert("svg:rx",double(m_cornerWidth[0]), librevenge::RVNG_POINT);
			propList.insert("svg:ry",double(m_cornerWidth[1]), librevenge::RVNG_POINT);
		}
		pt=m_formBox[0]+decal;
		propList.insert("svg:x",pt.x(), librevenge::RVNG_POINT);
		propList.insert("svg:y",pt.y(), librevenge::RVNG_POINT);
		pt=m_formBox.size();
		propList.insert("svg:width",pt.x(), librevenge::RVNG_POINT);
		propList.insert("svg:height",pt.y(), librevenge::RVNG_POINT);
		return C_Rectangle;
	case Circle:
		pt=0.5*(m_formBox[0]+m_formBox[1])+decal;
		propList.insert("svg:cx",pt.x(), librevenge::RVNG_POINT);
		propList.insert("svg:cy",pt.y(), librevenge::RVNG_POINT);
		pt=0.5*(m_formBox[1]-m_formBox[0]);
		propList.insert("svg:rx",pt.x(), librevenge::RVNG_POINT);
		propList.insert("svg:ry",pt.y(), librevenge::RVNG_POINT);
		return C_Ellipse;
	case Arc:
	case Pie:
	{
		Vec2f center=0.5*(m_formBox[0]+m_formBox[1])+decal;
		Vec2f rad=0.5*(m_formBox[1]-m_formBox[0]);
		float angl0=m_arcAngles[0];
		float angl1=m_arcAngles[1];
		if (rad[1]<0)
		{
			static bool first=true;
			if (first)
			{
				WPS_DEBUG_MSG(("WPSGraphicShape::addTo: oops radiusY for arc is negative, inverse it\n"));
				first=false;
			}
			rad[1]=-rad[1];
		}
		while (angl1<angl0)
			angl1+=360.f;
		while (angl1>angl0+360.f)
			angl1-=360.f;
		if (angl1-angl0>=180.f && angl1-angl0<=180.f)
			angl1+=0.01f;
		float angl=angl0*float(M_PI/180.);
		bool addCenter=m_type==Pie && asSurface;
		if (addCenter)
		{
			pt=center;
			list.insert("librevenge:path-action", "M");
			list.insert("svg:x",pt.x(), librevenge::RVNG_POINT);
			list.insert("svg:y",pt.y(), librevenge::RVNG_POINT);
			vect.append(list);
		}
		list.clear();
		pt=center+Vec2f(std::cos(angl)*rad[0],-std::sin(angl)*rad[1]);
		list.insert("librevenge:path-action", addCenter ? "L" : "M");
		list.insert("svg:x",pt.x(), librevenge::RVNG_POINT);
		list.insert("svg:y",pt.y(), librevenge::RVNG_POINT);
		vect.append(list);

		list.clear();
		angl=angl1*float(M_PI/180.);
		pt=center+Vec2f(std::cos(angl)*rad[0],-std::sin(angl)*rad[1]);
		list.insert("librevenge:path-action", "A");
		list.insert("librevenge:large-arc", !(angl1-angl0<180.f));
		list.insert("librevenge:sweep", false);
		list.insert("svg:rx",rad.x(), librevenge::RVNG_POINT);
		list.insert("svg:ry",rad.y(), librevenge::RVNG_POINT);
		list.insert("svg:x",pt.x(), librevenge::RVNG_POINT);
		list.insert("svg:y",pt.y(), librevenge::RVNG_POINT);
		vect.append(list);
		if (asSurface)
		{
			list.clear();
			list.insert("librevenge:path-action", "Z");
			vect.append(list);
		}

		propList.insert("svg:d", vect);
		return C_Path;
	}
	case Polygon:
	{
		size_t n=m_vertices.size();
		if (n<2) break;
		for (size_t i = 0; i < n; ++i)
		{
			list.clear();
			pt=m_vertices[i]+decal;
			list.insert("svg:x", pt.x(), librevenge::RVNG_POINT);
			list.insert("svg:y", pt.y(), librevenge::RVNG_POINT);
			vect.append(list);
		}
		propList.insert("svg:points", vect);
		return asSurface ? C_Polygon : C_Polyline;
	}
	case Path:
	{
		size_t n=m_path.size();
		if (!n) break;
		for (size_t c=0; c < n; ++c)
		{
			list.clear();
			if (m_path[c].get(list, -1.0f*decal))
				vect.append(list);
		}
		if (asSurface && m_path[n-1].m_type != 'Z')
		{
			// odg need a closed path to draw surface, so ...
			list.clear();
			list.insert("librevenge:path-action", "Z");
			vect.append(list);
		}
		propList.insert("svg:d", vect);
		return C_Path;
	}
	case ShapeUnknown:
	default:
		break;
	}
	WPS_DEBUG_MSG(("WPSGraphicShape::addTo: can not send a shape with type=%d\n", int(m_type)));
	return C_Bad;
}

std::vector<WPSGraphicShape::PathData> WPSGraphicShape::getPath(bool forTransformation) const
{
	std::vector<WPSGraphicShape::PathData> res;
	float const delta=0.55228f;
	switch (m_type)
	{
	case Line:
	case Polygon:
	{
		size_t n=m_vertices.size();
		if (n<2) break;
		res.push_back(PathData('M',m_vertices[0]));
		for (size_t i = 1; i < n; ++i)
			res.push_back(PathData('L', m_vertices[i]));
		break;
	}
	case Rectangle:
		if (m_cornerWidth[0] > 0 && m_cornerWidth[1] > 0)
		{
			WPSBox2f box=m_formBox;
			Vec2f c=m_cornerWidth;
			if (2*c[0]>box.size()[0]) c[0]=0.5f*box.size()[0];
			if (2*c[1]>box.size()[1]) c[1]=0.5f*box.size()[1];
			if (forTransformation)
			{
				Vec2f pt0(box[1][0]-c[0],box[0][1]);
				res.push_back(PathData('M',pt0));
				Vec2f pt1(box[1][0],box[0][1]+c[1]);
				res.push_back(PathData('C',pt1,pt0+Vec2f(delta*c[0],0),pt1-Vec2f(0,delta*c[1])));
				pt0=Vec2f(box[1][0],box[1][1]-c[1]);
				res.push_back(PathData('L',pt0));
				pt1=Vec2f(box[1][0]-c[0],box[1][1]);
				res.push_back(PathData('C',pt1,pt0+Vec2f(0,delta*c[1]),pt1+Vec2f(delta*c[0],0)));
				pt0=Vec2f(box[0][0]+c[0],box[1][1]);
				res.push_back(PathData('L',pt0));
				pt1=Vec2f(box[0][0],box[1][1]-c[1]);
				res.push_back(PathData('C',pt1,pt0-Vec2f(delta*c[0],0),pt1+Vec2f(0,delta*c[1])));
				pt0=Vec2f(box[0][0],box[0][1]+c[1]);
				res.push_back(PathData('L',pt0));
				pt1=Vec2f(box[0][0]+c[0],box[0][1]);
				res.push_back(PathData('C',pt1,pt0-Vec2f(0,delta*c[1]),pt1-Vec2f(delta*c[0],0)));
			}
			else
			{
				res.push_back(PathData('M',Vec2f(box[1][0]-c[0],box[0][1])));
				PathData data('A',Vec2f(box[1][0],box[0][1]+c[1]));
				data.m_r=c;
				data.m_sweep=true;
				res.push_back(data);
				res.push_back(PathData('L',Vec2f(box[1][0],box[1][1]-c[1])));
				data.m_x=Vec2f(box[1][0]-c[0],box[1][1]);
				res.push_back(data);
				res.push_back(PathData('L',Vec2f(box[0][0]+c[0],box[1][1])));
				data.m_x=Vec2f(box[0][0],box[1][1]-c[1]);
				res.push_back(data);
				res.push_back(PathData('L',Vec2f(box[0][0],box[0][1]+c[1])));
				data.m_x=Vec2f(box[0][0]+c[0],box[0][1]);
				res.push_back(data);
			}
			res.push_back(PathData('Z'));
			break;
		}
		res.push_back(PathData('M',m_formBox[0]));
		res.push_back(PathData('L',Vec2f(m_formBox[0][0],m_formBox[1][1])));
		res.push_back(PathData('L',m_formBox[1]));
		res.push_back(PathData('L',Vec2f(m_formBox[1][0],m_formBox[0][1])));
		res.push_back(PathData('Z'));
		break;
	case Circle:
	{
		if (forTransformation)
		{
			Vec2f center=m_formBox.center();
			Vec2f dir=0.5f*delta*(m_formBox[1]-m_formBox[0]);
			Vec2f pt0(m_formBox[0][0],center[1]);
			res.push_back(PathData('M',pt0));
			Vec2f pt1(center[0],m_formBox[0][1]);
			res.push_back(PathData('C',pt1, pt0-Vec2f(0,dir[1]), pt1-Vec2f(dir[0],0)));
			pt0=Vec2f(m_formBox[1][0],center[1]);
			res.push_back(PathData('C',pt0, pt1+Vec2f(dir[0],0), pt0-Vec2f(0,dir[1])));
			pt1=Vec2f(center[0],m_formBox[1][1]);
			res.push_back(PathData('C',pt1, pt0+Vec2f(0,dir[1]), pt1+Vec2f(dir[0],0)));
			pt0=Vec2f(m_formBox[0][0],center[1]);
			res.push_back(PathData('C',pt0, pt1-Vec2f(dir[0],0), pt0+Vec2f(0,dir[1])));
			res.push_back(PathData('Z'));
		}
		else
		{
			Vec2f pt0 = Vec2f(m_formBox[0][0],0.5f*(m_formBox[0][1]+m_formBox[1][1]));
			Vec2f pt1 = Vec2f(m_formBox[1][0],pt0[1]);
			res.push_back(PathData('M',pt0));
			PathData data('A',pt1);
			data.m_r=0.5*(m_formBox[1]-m_formBox[0]);
			data.m_largeAngle=true;
			res.push_back(data);
			data.m_x=pt0;
			res.push_back(data);
			break;
		}
	}
	case Arc:
	case Pie:
	{
		Vec2f center=0.5*(m_formBox[0]+m_formBox[1]);
		Vec2f rad=0.5*(m_formBox[1]-m_formBox[0]);
		float angl0=m_arcAngles[0];
		float angl1=m_arcAngles[1];
		if (rad[1]<0)
		{
			static bool first=true;
			if (first)
			{
				WPS_DEBUG_MSG(("WPSGraphicShape::getPath: oops radiusY for arc is negative, inverse it\n"));
				first=false;
			}
			rad[1]=-rad[1];
		}
		while (angl1<angl0)
			angl1+=360.f;
		while (angl1>angl0+360.f)
			angl1-=360.f;
		if (angl1-angl0>=180.f && angl1-angl0<=180.f)
			angl1+=0.01f;
		float angl=angl0*float(M_PI/180.);
		bool addCenter=m_type==Pie;
		if (addCenter)
			res.push_back(PathData('M', center));
		Vec2f pt=center+Vec2f(std::cos(angl)*rad[0],-std::sin(angl)*rad[1]);
		res.push_back(PathData(addCenter ? 'L' : 'M', pt));
		if (!forTransformation)
		{
			angl=angl1*float(M_PI/180.);
			pt=center+Vec2f(std::cos(angl)*rad[0],-std::sin(angl)*rad[1]);
			PathData data('A',pt);
			data.m_largeAngle=(angl1-angl0>=180.f);
			data.m_r=rad;
			res.push_back(data);
		}
		else
		{
			int N=int(angl1-angl0)/90;
			float dAngle=float(angl1-angl0)/float(N+1);
			for (int i=0; i<=N; ++i)
			{
				float newAngl= i==N ? angl1 : angl0+float(i+1)*dAngle;
				newAngl*=float(M_PI/180.);
				Vec2f newPt=center+Vec2f(std::cos(angl1)*rad[0],-std::sin(angl1)*rad[1]);
				Vec2f dir(-std::sin(angl)*rad[0],-std::cos(angl)*rad[1]);
				Vec2f newDir(-std::sin(newAngl)*rad[0],-std::cos(newAngl)*rad[1]);
				float deltaDir=4/3.f*std::tan((newAngl-angl)/4);
				res.push_back(PathData('C',newPt,pt+deltaDir*dir,newPt-deltaDir*newDir));
				pt=newPt;
				angl=newAngl;
			}
			if (m_type==Pie) res.push_back(PathData('Z'));
		}
		break;
	}
	case Path:
		return m_path;
	case ShapeUnknown:
	default:
		WPS_DEBUG_MSG(("WPSGraphicShape::getPath: unexpected type\n"));
		break;
	}
	return res;
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
