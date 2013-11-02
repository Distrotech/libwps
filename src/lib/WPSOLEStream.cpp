/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* POLE - Portable C++ library to access OLE Storage
   Copyright (C) 2002-2005 Ariya Hidayat <ariya@kde.org>

   Performance optimization: Dmitry Fedorov
   Copyright 2009 <www.bioimage.ucsb.edu> <www.dimin.net>

   Fix for more than 236 mbat block entries : Michel Boudinot
   Copyright 2010 <Michel.Boudinot@inaf.cnrs-gif.fr>

   Version: 0.4

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   * Neither the name of the authors nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
   THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 This file is largly inspirated from librevenge RVNGOLEStream.cpp
*/

#include <cstring>
#include <iostream>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <librevenge-stream/librevenge-stream.h>

#include "WPSDebug.h"

#include "WPSOLEStream.h"
namespace libwpsOLE
{
/** an internal class used to return the OLE InputStream */
class WPSStringStream: public RVNGInputStream
{
public:
	WPSStringStream(const unsigned char *data, const unsigned int dataSize) :
		buffer(dataSize), offset(0)
	{
		memcpy(&buffer[0], data, dataSize);
	}
	~WPSStringStream() { }

	const unsigned char *read(unsigned long numBytes, unsigned long &numBytesRead);
	long tell()
	{
		return offset;
	}
	int seek(long offset, RVNG_SEEK_TYPE seekType);
	bool atEOS()
	{
		return ((long)offset >= (long)buffer.size());
	}

	bool isOLEStream()
	{
		return false;
	}
	RVNGInputStream *getDocumentOLEStream(const char *)
	{
		return 0;
	};

private:
	std::vector<unsigned char> buffer;
	volatile long offset;
	WPSStringStream(const WPSStringStream &);
	WPSStringStream &operator=(const WPSStringStream &);
};

int WPSStringStream::seek(long _offset, RVNG_SEEK_TYPE seekType)
{
	if (seekType == RVNG_SEEK_CUR)
		offset += _offset;
	else if (seekType == RVNG_SEEK_END)
		offset = _offset + (long)buffer.size();
	else if (seekType == RVNG_SEEK_SET)
		offset = _offset;

	if (offset < 0)
	{
		offset = 0;
		return 1;
	}
	if ((long)offset > (long)buffer.size())
	{
		offset = long(buffer.size());
		return 1;
	}
	return 0;
}

const unsigned char *WPSStringStream::read(unsigned long numBytes, unsigned long &numBytesRead)
{
	numBytesRead = 0;

	if (numBytes == 0)
		return 0;

	unsigned long numBytesToRead;

	if (((unsigned long) offset+numBytes) < buffer.size())
		numBytesToRead = numBytes;
	else
		numBytesToRead = (unsigned long)buffer.size() -(unsigned long)offset;

	numBytesRead = numBytesToRead; // about as paranoid as we can be..

	if (numBytesToRead == 0)
		return 0;

	long oldOffset = offset;
	offset += numBytesToRead;

	return &buffer[size_t(oldOffset)];
}
}

namespace libwpsOLE
{
////////////////////////////////////////////////////////////
// internal: basic read/write functions and enum
////////////////////////////////////////////////////////////
static inline unsigned long readU16( const unsigned char *ptr )
{
	return (unsigned long)(ptr[0]+(ptr[1]<<8));
}

static inline unsigned long readU32( const unsigned char *ptr )
{
	return (unsigned long)(ptr[0]+(ptr[1]<<8)+(ptr[2]<<16)+(ptr[3]<<24));
}

static inline void writeU16( unsigned char *ptr, unsigned long data )
{
	ptr[0] = (unsigned char)(data & 0xff);
	ptr[1] = (unsigned char)((data >> 8) & 0xff);
}

static inline void writeU32( unsigned char *ptr, unsigned long data )
{
	ptr[0] = (unsigned char)(data & 0xff);
	ptr[1] = (unsigned char)((data >> 8) & 0xff);
	ptr[2] = (unsigned char)((data >> 16) & 0xff);
	ptr[3] = (unsigned char)((data >> 24) & 0xff);
}

enum { Avail = 0xffffffff, Eof = 0xfffffffe, Bat = 0xfffffffd, MetaBat = 0xfffffffc, NotFound=0xfffffff0 };

////////////////////////////////////////////////////////////
// internal: basic classes
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
//            =========== Header ==========
class Header
{
public:
	unsigned char m_magic[8];       // signature, or magic identifier
	unsigned m_revision;         // the file revision
	unsigned m_num_bat;          // blocks allocated for big bat
	unsigned m_start_dirent;     // starting block for directory info
	unsigned m_threshold;        // switch from small to big file (usually 4K)
	unsigned m_start_sbat;       // starting block index to store small bat
	unsigned m_num_sbat;         // blocks allocated for small bat
	unsigned m_shift_sbat;          // sbat->blockSize = 1 << m_shift_sbat
	unsigned m_size_sbat;		// the small block size, default 0x40
	unsigned m_shift_bbat;          // bbat->blockSize = 1 << m_shift_bbat
	unsigned m_size_bbat;		// the big block size, default 0x200
	unsigned m_start_mbat;       // starting block to store meta bat
	unsigned m_num_mbat;         // blocks allocated for meta bat
	unsigned long m_blocks_bbat[109];

	Header();
	void compute_block_size()
	{
		m_size_bbat = 1 << m_shift_bbat;
		m_size_sbat = 1 << m_shift_sbat;
	}
	bool valid_signature() const
	{
		for( unsigned i = 0; i < 8; i++ )
			if (m_magic[i] != s_ole_magic[i]) return false;
		return true;
	}
	bool valid();
	void load( const unsigned char *buffer, unsigned long size );

	friend std::ostream &operator<<(std::ostream &o, Header const &h);
	// writer
	void save( unsigned char *buffer );

protected:
	static const unsigned char s_ole_magic[];
};

const unsigned char Header::s_ole_magic[] =
{ 0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1 };

Header::Header() :
	m_revision(0x3e), m_num_bat(0), m_start_dirent(0), m_threshold(4096),
	m_start_sbat(Eof), m_num_sbat(0), m_shift_sbat(6), m_size_sbat(0),
	m_shift_bbat(9), m_size_bbat(0),
	m_start_mbat(Eof), m_num_mbat(0)
{
	for( unsigned i = 0; i < 8; i++ )
		m_magic[i] = s_ole_magic[i];
	for( unsigned j=0; j<109; j++ )
		m_blocks_bbat[j] = Avail;
	compute_block_size();
}

bool Header::valid()
{
	if( m_threshold != 4096 ) return false;
	if( m_num_bat == 0 ) return false;
	if( (m_num_bat > 109) && (m_num_bat > (m_num_mbat * (m_size_bbat/4-1)) + 109)) return false;
	if( (m_num_bat <= 109) && (m_num_mbat != 0) ) return false;
	if( m_shift_sbat > m_shift_bbat ) return false;
	if( m_shift_bbat <= 6 ) return false;
	if( m_shift_bbat >=31 ) return false;

	return true;
}

void Header::load( const unsigned char *buffer, unsigned long size )
{
	if (size < 512)
		return;
	m_revision = (unsigned) readU16(buffer+0x18);
	m_shift_bbat      = (unsigned int) readU16( buffer + 0x1e );
	m_shift_sbat      = (unsigned int) readU16( buffer + 0x20 );
	m_num_bat      = (unsigned int) readU32( buffer + 0x2c );
	m_start_dirent = (unsigned int) readU32( buffer + 0x30 );
	m_threshold    = (unsigned int) readU32( buffer + 0x38 );
	m_start_sbat   = (unsigned int) readU32( buffer + 0x3c );
	m_num_sbat     = (unsigned int) readU32( buffer + 0x40 );
	m_start_mbat   = (unsigned int) readU32( buffer + 0x44 );
	m_num_mbat     = (unsigned int) readU32( buffer + 0x48 );

	for( unsigned i = 0; i < 8; i++ )
		m_magic[i] = buffer[i];
	for( unsigned j=0; j<109; j++ )
		m_blocks_bbat[j] = readU32( buffer + 0x4C+j*4 );
	compute_block_size();
}

void Header::save( unsigned char *buffer )
{
	memset( buffer, 0, 0x4c );
	memcpy( buffer, s_ole_magic, 8 );        // ole signature
	writeU32( buffer + 8, 0 );              // unknown
	writeU32( buffer + 12, 0 );             // unknown
	writeU32( buffer + 16, 0 );             // unknown
	writeU16( buffer + 24, m_revision);
	writeU16( buffer + 26, 3 );             // version ?
	writeU16( buffer + 28, 0xfffe );        // unknown
	writeU16( buffer + 0x1e, m_shift_bbat );
	writeU16( buffer + 0x20, m_shift_sbat );
	writeU32( buffer + 0x2c, m_num_bat );
	writeU32( buffer + 0x30, m_start_dirent );
	writeU32( buffer + 0x38, m_threshold );
	writeU32( buffer + 0x3c, m_start_sbat );
	writeU32( buffer + 0x40, m_num_sbat );
	writeU32( buffer + 0x44, m_start_mbat );
	writeU32( buffer + 0x48, m_num_mbat );

	for( unsigned i=0; i<109; i++ )
		writeU32( buffer + 0x4C+i*4, m_blocks_bbat[i] );
}

std::ostream &operator<<(std::ostream &o, Header const &h)
{
	if (h.m_revision != 0x3e)
		o << "revision=" << h.m_revision << ",";
	o << "blockSize=" << std::hex << (1<<h.m_shift_bbat) << std::dec << ",";
	o << "sBlockSize=" << std::hex << (1<<h.m_shift_sbat) << std::dec << ",";
	o << "numBigBat=" << h.m_num_bat << ",";
	o << "numSmallBat=" << h.m_num_sbat << "[" << h.m_start_sbat << "],";
	o << "numMetaBat=" << h.m_num_mbat << "[" << h.m_start_mbat << "],";
	o << "dirInfoBlock=" << h.m_start_dirent << ",";
	o << "m_threshold=" << std::hex << h.m_threshold << std::dec << ",";
	o << "bigBlock=[";
	for (unsigned i = 0; i < h.m_num_bat; i++)
	{
		if (i >= 109)
		{
			o << "...,";
			break;
		}
		o << std::hex << h.m_blocks_bbat[i] << std::dec << ",";
	}
	o << "]";
	return o;
}

////////////////////////////////////////////////////////////
//            =========== AllocTable ==========
class AllocTable
{
public:
	unsigned m_blockSize;
	AllocTable() : m_blockSize(4096), m_data()
	{
		resize(128); // initial size
	}
	void clear();
	unsigned long count() const
	{
		return (unsigned long) m_data.size();
	}
	void resize( unsigned long newsize )
	{
		m_data.resize(size_t(newsize), Avail);
	}
	void set( unsigned long index, unsigned long val )
	{
		if (index >= count())
			resize(index+1);
		m_data[size_t(index)] = val;
	}
	std::vector<unsigned long> follow( unsigned long start ) const;
	unsigned long operator[](unsigned long index ) const
	{
		return m_data[size_t(index)];
	}
	void load( const unsigned char *buffer, unsigned len )
	{
		resize( len / 4 );
		for( unsigned i = 0; i < count(); i++ )
			set( i, readU32( buffer + i*4 ) );
	}

	// write part
	void setChain( std::vector<unsigned long> chain, unsigned end);
	void save( unsigned char *buffer ) const
	{
		unsigned cnt=(unsigned) count();
		for( unsigned i = 0; i < cnt; i++ )
			writeU32( buffer + i*4, m_data[i] );
		unsigned lastFree = 128-(cnt%128);
		if (lastFree==128) return;
		for (unsigned i = 0; i < lastFree; i++)
			writeU32( buffer + (cnt+i)*4, Avail);
	}
	// return space required to save the allocation table
	unsigned saveSize() const
	{
		unsigned cnt=(unsigned) (((count()+127)/128)*128);
		return cnt * 4;
	}
private:
	std::vector<unsigned long> m_data;
	AllocTable( const AllocTable & );
	AllocTable &operator=( const AllocTable & );
};

std::vector<unsigned long> AllocTable::follow( unsigned long start ) const
{
	std::vector<unsigned long> chain;
	if( start >= count() ) return chain;

	std::set<unsigned long> seens;
	unsigned long p = start;
	while( p < count() )
	{
		if( p == Eof || p == Bat || p == MetaBat) break;
		if (seens.find(p) != seens.end()) break;
		seens.insert(p);
		chain.push_back( p );
		p = m_data[ p ];
	}

	return chain;
}

void AllocTable::setChain( std::vector<unsigned long> chain, unsigned end )
{
	if(!chain.size() ) return;

	for( unsigned i=0; i<chain.size()-1; i++ )
		set( chain[i], chain[i+1] );
	set( chain[ chain.size()-1 ], end );
}

//////////////////////////////////////////////////////////////////////
/**
   Internal and low level: class of libwpsOLE used to store and write
   a information find in a directory entry
*/
class DirInfo
{
public:
	//! constructor
	DirInfo()
	{
		for (int i=0; i < 4; i++)
			m_time[i]=0;
		for (int i=0; i < 4; i++)
			m_clsid[i]=0;
	}
	//! returns true if the clsid field is filed
	bool hasCLSId() const
	{
		for (int i=0; i < 4; i++)
			if (m_clsid[i]) return true;
		return false;
	}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, DirInfo const &inf)
	{
		for (int i = 0; i < 4; i++)
		{
			if (inf.m_time[i])
				o << "time" << i << "=" << inf.m_time[i] << ",";
		}
		if (inf.hasCLSId())
		{
			o << "cls=[" << std::hex;
			for (int i=0; i < 4; i++)
				o << inf.m_clsid[i] << ",";
			o << std::dec << "],";
		}
		return o;
	}
	/** four uint32_t : the first two used for creation, the last for modification time */
	unsigned m_time[4];
	/** four uint32_t: the clsid data */
	unsigned m_clsid[4];
};

//////////////////////////////////////////////////////////////////////
/**
   Internal and low level: class of libwpsOLE used to store and write
   a directory entry
*/
class DirEntry
{
public:
	enum { End= 0xffffffff };
	//! constructor
	DirEntry() : m_valid(false), m_macRootEntry(false), m_type(0), m_colour(0), m_size(0), m_start(0),
		m_right(End), m_left(End), m_child(End), m_info(), m_name("")
	{
	}
	//! returns true for a directory
	bool is_dir() const
	{
		return m_type==1 || m_type==5;
	}
	//! returns the simplified file name
	std::string name() const
	{
		if (m_name.length() && m_name[0]<32)
			return m_name.substr(1);
		return m_name;
	}
	/** returns the string which was store inside the file.

	\note: either name() or a index (unknown) followed by name() */
	std::string const &filename() const
	{
		return m_name;
	}
	/** sets the file name */
	void setName(std::string const &nm)
	{
		m_name=nm;
	}
	/** reads a entry content in buffer */
	void load( unsigned char *buffer, unsigned len );
	//! saves a entry content in buffer */
	void save( unsigned char *buffer ) const;
	//! returns space required to save a dir entry
	static unsigned saveSize()
	{
		return 128;
	}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, DirEntry const &e);

	bool m_valid;            /** false if invalid (should be skipped) */
	bool m_macRootEntry;      /** true if this is a classi mac directory entry */
	unsigned m_type;         /** the type */
	unsigned m_colour;       /** the red/black color: 0 means red */
	unsigned long m_size;    /** size (not valid if directory) */
	unsigned long m_start;   /** starting block */
	unsigned m_right;        /** previous sibling */
	unsigned m_left;         /** next sibling */
	unsigned m_child;        /** first child */

	DirInfo m_info; //! the file information
protected:
	std::string m_name;      /** the name, not in unicode anymore */
};

std::ostream &operator<<(std::ostream &o, DirEntry const &e)
{
	if (e.m_name.length()) o << "name=" << e.m_name << ",";
	if (e.m_type) o << "type=" << e.m_type << ",";
	if (e.m_colour) o << "black,";
	if (e.m_size) o << "sz=" << e.m_size << ",";
	if (e.m_start != DirEntry::End && (e.m_type!=1||e.m_start))
		o << "start=" << e.m_start << ":0x" << std::hex
		  << 0x200*(e.m_start+1) << std::dec << ",";
	if (e.m_left && e.m_left != DirEntry::End) o << "left=" << e.m_left << ",";
	if (e.m_right && e.m_right != DirEntry::End) o << "right=" << e.m_right << ",";
	if (e.m_child && e.m_child != DirEntry::End) o << "child=" << e.m_child << ",";
	o << e.m_info;
	if (!e.m_valid) o << "invalid*,";
	return o;
}

void DirEntry::load( unsigned char *buffer, unsigned len )
{
	if (len != 128)
	{
		WPS_DEBUG_MSG(("DirEntry::load: unexpected len for DirEntry::load\n"));
		*this=DirEntry();
		return;
	}
	// 2 = file (aka stream), 1 = directory (aka storage), 5 = root
	m_type = buffer[ 0x42];
	m_colour = buffer[0x43];

	// parse name of this entry, which stored as Unicode 16-bit
	m_name=std::string("");
	unsigned name_len = (unsigned) readU16( buffer + 0x40 );
	if( name_len > 64 ) name_len = 64;
	if (name_len==2 && m_type==5 && readU16(buffer)==0x5200)
	{
		// find in some mswork mac 4.0 file
		m_name="R";
		m_macRootEntry=true;
	}
	else
	{
		for( unsigned j=0; ( buffer[j]) && (j<name_len); j+= 2 )
			m_name.append( 1, char(buffer[j]) );
	}


	for (int i = 0; i < 4; i++)
		m_info.m_clsid[i]=(unsigned) readU32( buffer + 0x50 + 4*i);
	for (int i = 0; i < 4; i++)
		m_info.m_time[i]=(unsigned) readU32( buffer + 0x64 + 4*i);

	m_valid = true;
	m_start = (unsigned int) readU32( buffer + 0x74 );
	m_size = (unsigned int) readU32( buffer + 0x78 );
	m_left = (unsigned int) readU32( buffer + 0x44 );
	m_right = (unsigned int) readU32( buffer + 0x48 );
	m_child = (unsigned int) readU32( buffer + 0x4C );

	// sanity checks
	if( (m_type != 2) && (m_type != 1 ) && (m_type != 5 ) ) m_valid = false;
	if( name_len < 1 ) m_valid = false;
}

void DirEntry::save( unsigned char *buffer ) const
{
	for (int i = 0; i < 128; i++) buffer[i]=0;

	unsigned name_len = (unsigned) m_name.length();
	if (name_len>31) name_len = 31;
	if (name_len==2 && m_macRootEntry && m_type==5)
		buffer[1]='R';
	else
	{
		for (size_t i = 0; i < name_len; i++)
			writeU16(buffer+2*i, (unsigned) m_name[i]);
	}
	writeU16(buffer+0x40, 2*name_len+2);

	buffer[0x42]=(unsigned char) m_type;
	buffer[0x43]=(unsigned char) m_colour;
	for (int i = 0; i < 4; i++)
		writeU32(buffer + 0x50+4*i, m_info.m_clsid[i]);
	for (int i = 0; i < 4; i++)
		writeU32(buffer + 0x64+4*i, m_info.m_time[i]);
	writeU32(buffer + 0x74, m_start);
	writeU32(buffer + 0x78, m_size);
	writeU32(buffer + 0x44, m_left);
	writeU32(buffer + 0x48, m_right);
	writeU32(buffer + 0x4C, m_child);
}

//////////////////////////////////////////////////////////////////////
/**
   Internal and low level: class of libwpsOLE used to store and write
   a dirTree
*/
class DirTree
{
public:
	/** constructor */
	DirTree() : m_entries()
	{
		clear();
	}
	/** clear all entries, leaving only a root entries */
	void clear();
	/** set the root to a mac/pc root */
	void setRootType(bool pc=true);
	/** returns true if it is a pc file */
	bool hasRootTypePc() const
	{
		return !m_entries.size() || !m_entries[0].m_macRootEntry;
	}
	/** returns the number of entries */
	unsigned count() const
	{
		return unsigned(m_entries.size());
	}
	/** returns the entry with a given index */
	DirEntry const *entry( unsigned ind ) const
	{
		if( ind >= count() ) return 0;
		return &m_entries[ size_t(ind) ];
	}
	/** returns the entry with a given index */
	DirEntry *entry( unsigned ind )
	{
		if( ind >= count() ) return  0;
		return &m_entries[ size_t(ind) ];
	}
	/** returns the entry with a given name */
	DirEntry *entry( const std::string &name )
	{
		return entry(index(name));
	}
	/** given a fullname (e.g "/ObjectPool/_1020961869"), find the entry */
	unsigned index( const std::string &name, bool create=false );
	/** tries to find a child of ind with a given name */
	unsigned find_child( unsigned ind, const std::string &name ) const
	{
		DirEntry const *p = entry( ind );
		if (!p || !p->m_valid) return 0;
		std::vector<unsigned> siblingsList = get_siblings(p->m_child);
		for (size_t s=0; s < siblingsList.size(); s++)
		{
			p  = entry( siblingsList[s] );
			if (!p) continue;
			if (p->name()==name)
				return siblingsList[s];
		}
		return 0;
	}
	/** tries to read the different entries */
	void load( unsigned char *buffer, unsigned len );

	/** returns the list of ind substream */
	std::vector<std::string> getSubStreamList(unsigned ind=0, bool retrieveAll=false) const
	{
		std::vector<std::string> res;
		std::set<unsigned> seens;
		getSubStreamList(ind, retrieveAll, "", res, seens, true);
		return res;
	}

	// write part

	//! check/update so that the sibling are store with a red black tree
	void setInRedBlackTreeForm()
	{
		std::set<unsigned> seens;
		setInRedBlackTreeForm(0, seens);
	}
	//! return space required to save a dir entry
	unsigned saveSize() const
	{
		unsigned cnt = count();
		cnt = 4*((cnt+3)/4);
		return cnt*DirEntry::saveSize();
	}
	//! save the list of direntry in buffer
	void save( unsigned char *buffer ) const
	{
		unsigned entrySize=DirEntry::saveSize();
		size_t cnt = count();
		for (size_t i = 0; i < cnt; i++)
			m_entries[i].save(buffer+entrySize*i);
		if ((cnt%4)==0) return;
		DirEntry empty;
		while (cnt%4)
			empty.save(buffer+entrySize*cnt++);
	}

	//! a debug function used to print all siblings
	void print_all_siblings(std::ostream &o) const
	{
		std::set<unsigned> seens;
		print_all_siblings(0, o, seens);
	}
protected:
	//! returns a list of siblings corresponding to a node
	std::vector<unsigned> get_siblings(unsigned ind) const
	{
		std::set<unsigned> seens;
		get_siblings(ind, seens);
		return std::vector<unsigned>(seens.begin(), seens.end());
	}
	//! constructs the list of siblings ( by filling the seens set )
	void get_siblings(unsigned ind, std::set<unsigned> &seens) const
	{
		if (seens.find(ind) != seens.end())
			return;
		seens.insert(ind);
		DirEntry const *e = entry( ind );
		if (!e) return;
		unsigned cnt = count();
		if (e->m_left>0&& e->m_left < cnt)
			get_siblings(e->m_left, seens);
		if (e->m_right>0 && e->m_right < cnt)
			get_siblings(e->m_right, seens);
	}

	//! a debug function used to print all siblings
	void print_all_siblings(unsigned ind, std::ostream &o, std::set<unsigned> &seens) const;

	/** a debug function to print the siblings to try to give a sense to the sibling tree */
	void print_siblings(unsigned ind, std::ostream &o, std::set<unsigned> &seen) const;
	/** returns a substream list */
	void getSubStreamList(unsigned ind, bool all, const std::string &prefix,
	                      std::vector<std::string> &res, std::set<unsigned> &seen,
	                      bool isRoot=false) const;
	/** check that the subtrees of index is a red black tree, if not rebuild it */
	void setInRedBlackTreeForm(unsigned id, std::set<unsigned> &seen);
private:
	/** rebuild all the childs m_left, m_right index as a red black
		tree, returns the root index.

		\note: this function supposes that the childs list is already sorted
	*/
	unsigned setInRBTForm(std::vector<unsigned> const &childList,
	                      unsigned firstInd, unsigned lastInd,
	                      unsigned height);

	/** a comparison funcion of DirTree used to sort the entry by name */
	struct CompareEntryName
	{
		//! constructor
		CompareEntryName(DirTree &tree) : m_tree(tree)
		{
		}
		//! comparaison function
		bool operator()(unsigned ind1, unsigned ind2) const
		{
			DirEntry const *e1=m_tree.entry(ind1);
			DirEntry const *e2=m_tree.entry(ind2);
			if (!e1 && !e2) return false;
			if (!e1) return true;
			if (!e2) return false;
			std::string name1(e1->name()), name2(e2->name());
			size_t len1=name1.length();
			size_t len2=name2.length();
			if (len1 != len2) return len1 < len2;
			for (size_t c=0; c < len1; c++)
			{
				if (std::tolower(name1[c]) != std::tolower(name2[c]))
					return std::tolower(name1[c]) < std::tolower(name2[c]);
			}
			return ind1 < ind2;
		}

		//! the main tree
		DirTree &m_tree;
	};
	friend struct CompareEntryName;
	//! the list of entry
	std::vector<DirEntry> m_entries;
	DirTree( const DirTree & );
	DirTree &operator=( const DirTree & );
};

void DirTree::clear()
{
	m_entries.resize( 0 );
	setRootType(true);
}

void DirTree::setRootType(bool pc)
{
	if (!m_entries.size())
	{
		m_entries.resize( 1 );
		m_entries[0]=DirEntry();
		m_entries[0].m_valid = true;
		m_entries[0].setName("Root Entry");
		m_entries[0].m_type = 5;
	}
	if (pc)
		m_entries[0].setName("Root Entry");
	else
	{
		m_entries[0].setName("R");
		m_entries[0].m_macRootEntry = true;
	}
}

unsigned DirTree::index( const std::string &name, bool create )
{

	if( name.length()==0 ) return NotFound;

	// quick check for "/" (that's root)
	if( name == "/" ) return 0;

	// split the names, e.g  "/ObjectPool/_1020961869" will become:
	// "ObjectPool" and "_1020961869"
	std::list<std::string> names;
	std::string::size_type start = 0, end = 0;
	if( name[0] == '/' ) start++;
	while( start < name.length() )
	{
		end = name.find_first_of( '/', start );
		if( end == std::string::npos ) end = name.length();
		names.push_back( name.substr( start, end-start ) );
		start = end+1;
	}

	// start from root
	unsigned ind = 0 ;

	// trace one by one
	std::list<std::string>::const_iterator it;
	size_t depth = 0;

	for( it = names.begin(); it != names.end(); ++it, ++depth)
	{
		std::string childName(*it);
		if (childName.length() && childName[0]<32)
			childName= it->substr(1);

		unsigned child = find_child( ind, childName );
		// traverse to the child
		if( child > 0 )
		{
			ind = child;
			continue;
		}
		else if( !create ) return NotFound;

		// create a new entry
		unsigned parent = ind;
		m_entries.push_back( DirEntry() );
		ind = count()-1;
		DirEntry *e = entry( ind );
		e->m_valid = true;
		e->setName(*it);
		e->m_type = depth+1==names.size() ? 2 : 1;
		// e->m_start = Eof; CHECKME
		e->m_left = entry(parent)->m_child;
		entry(parent)->m_child = ind;
	}

	return ind;
}

void DirTree::load( unsigned char *buffer, unsigned size )
{
	m_entries.clear();

	for( unsigned i = 0; i < size/128; i++ )
	{
		DirEntry e;
		e.load(buffer+i*128, 128);
		m_entries.push_back( e );
	}
}

void DirTree::print_siblings(unsigned ind, std::ostream &o, std::set<unsigned> &seens) const
{
	if (seens.find(ind) != seens.end())
		return;
	seens.insert(ind);
	DirEntry const *e = entry( ind );
	if (!e) return;
	unsigned cnt = count();
	o << e->filename() << ":";
	if (e->m_colour)
		o << "black,";
	else
		o << "red,";
	if (e->m_left>0&& e->m_left < cnt)
	{
		o << "l=[";
		print_siblings(e->m_left, o, seens);
		o << "],";
	}
	if (e->m_right>0 && e->m_right < cnt)
	{
		o << "r=[";
		print_siblings(e->m_right, o, seens);
		o << "],";
	}
}

void DirTree::print_all_siblings(unsigned ind, std::ostream &o, std::set<unsigned> &seen) const
{
	if (seen.find(ind) != seen.end())
		return;
	seen.insert(ind);

	unsigned cnt = count();
	DirEntry const *p = entry( ind );
	if (!p || !p->m_valid || !p->is_dir())
		return;

	if (p->m_child == 0 || p->m_child > cnt)
		return;
	std::set<unsigned> child;
	o << "OLE(SIBLINGS):";
	print_siblings(p->m_child, o, child);
	o << "\n";
	for (std::set<unsigned>::iterator it = child.begin(); it != child.end(); ++it)
		print_all_siblings(*it, o, seen);
}

void DirTree::getSubStreamList(unsigned ind, bool all, const std::string &prefix,
                               std::vector<std::string> &res,
                               std::set<unsigned> &seen,
                               bool isRoot) const
{
	if (seen.find(ind) != seen.end())
		return;
	seen.insert(ind);
	unsigned cnt = count();
	DirEntry const *p = entry( ind );
	if (!p || !p->m_valid)
		return;
	std::string name(prefix);
	if (ind && !isRoot)
	{
		if (p->filename().length())
			name+= p->filename();
		else
			return;
	}
	if (!p->is_dir())
	{
		res.push_back(name);
		return;
	}
	if (ind)
		name += "/";
	if (all)
	{
		if (ind)
			res.push_back(name);
		else
			res.push_back("/");
	}
	if (p->m_child >= cnt)
		return;
	std::vector<unsigned> siblings=get_siblings(p->m_child);
	for (size_t s=0; s < siblings.size(); s++)
		getSubStreamList(siblings[s], all, name, res, seen);
}

void DirTree::setInRedBlackTreeForm(unsigned ind, std::set<unsigned> &seen)
{
	if (seen.find(ind) != seen.end())
		return;
	seen.insert(ind);

	DirEntry *p = entry( ind );
	if (!p || !p->m_valid)
		return;

	p->m_colour=1; // set all nodes blacks
	std::vector<unsigned> childs=get_siblings(p->m_child);
	size_t numChild=childs.size();
	for (size_t s=0; s < numChild; s++)
		setInRedBlackTreeForm(childs[s], seen);
	if (numChild <= 1)
		return;
	CompareEntryName compare(*this);
	std::set<unsigned,CompareEntryName> set(childs.begin(),childs.end(),compare);
	std::vector<unsigned> sortedChilds(set.begin(), set.end());
	if (sortedChilds.size() != numChild)
	{
		WPS_DEBUG_MSG(("DirTree::setInRedBlackTreeForm: OOPS pb with numChild\n"));
		return;
	}
	unsigned h=1;
	unsigned hNumChild=1;
	while (2*hNumChild+1<=numChild)
	{
		hNumChild=2*hNumChild+1;
		h++;
	}
	p->m_child=setInRBTForm(sortedChilds, 0, unsigned(numChild-1), h);
}

unsigned DirTree::setInRBTForm(std::vector<unsigned> const &childs,
                               unsigned firstInd, unsigned lastInd,
                               unsigned height)
{
	unsigned middle = (firstInd+lastInd)/2;
	unsigned ind=childs[middle];
	DirEntry *p = entry( ind );
	if (!p)
	{
		WPS_DEBUG_MSG(("DirTree::setInRBTForm: OOPS can not find tree to modified\n"));
		throw libwps::GenericException();
	}
	unsigned newH = height==0 ? 0 : height-1;
	if (height==0)
	{
		p->m_colour = 0;
		if (firstInd!=middle || lastInd!=middle)
		{
			WPS_DEBUG_MSG(("DirTree::setInRBTForm: OOPS problem setting RedBlack colour\n"));
		}
	}
	if (firstInd!=middle)
		p->m_left=setInRBTForm(childs, firstInd,unsigned(middle-1),newH);
	else
		p->m_left=DirEntry::End;
	if (lastInd!=middle)
		p->m_right=setInRBTForm(childs, unsigned(middle+1),lastInd,newH);
	else
		p->m_right=DirEntry::End;
	return ind;
}

//////////////////////////////////////////////////////////////////////

class IStream;
/**
   Internal and low level: class of libwpsOLE used to read an OLE
   with various functions to retrieve the OLE content
*/
class IStorage
{
	friend class IStream;
public:
	//! constructor
	IStorage( shared_ptr<RVNGInputStream> is );
	//! destructor
	~IStorage() { }
	//! returns a directory entry corresponding to a index
	DirEntry *entry(unsigned ind)
	{
		load();
		return m_dirtree.entry(ind);
	}
	//! returns a directory entry corresponding to a name
	DirEntry *entry(const std::string &name)
	{
		if( !name.length() ) return 0;
		load();
		return m_dirtree.entry(name);
	}
	//! returns a directory entry corresponding to a index
	unsigned index(const std::string &name)
	{
		if( !name.length() ) return NotFound;
		load();
		return m_dirtree.index( name );
	}
	/** returns the OLE revision */
	unsigned revision() const
	{
		return m_header.m_revision;
	}
	/** returns true if it is a pc file */
	bool hasRootTypePc() const
	{
		return m_dirtree.hasRootTypePc();
	}
	//! returns true if the entry is an OLE document
	bool isStructuredDocument();
	//! returns true if the entry exists in the OLE, if so fills isDir
	bool isSubStream(const std::string &name, bool &isDir);
	//! returns the list of subStream given a dirEntry index
	std::vector<std::string> getSubStreamList(unsigned ind=0, bool retrieveAll=false)
	{
		load();
		return m_dirtree.getSubStreamList(ind, retrieveAll);
	}
	//! returns the current results
	Storage::Result result() const
	{
		return m_result;
	}

protected:
	AllocTable &sbat()
	{
		return m_sbat;
	}
	AllocTable const &sbat() const
	{
		return m_sbat;
	}
	AllocTable &bbat()
	{
		return m_bbat;
	}
	AllocTable const &bbat() const
	{
		return m_bbat;
	}

	bool use_big_block_for(unsigned long size) const
	{
		return size >= m_header.m_threshold;
	}
	unsigned long loadBigBlocks( std::vector<unsigned long> const &blocks, unsigned char *buffer, unsigned long maxlen );

	unsigned long loadBigBlock( unsigned long block, unsigned char *buffer, unsigned long maxlen );

	unsigned long loadSmallBlocks( std::vector<unsigned long> const &blocks, unsigned char *buffer, unsigned long maxlen );

	unsigned long loadSmallBlock( unsigned long block, unsigned char *buffer, unsigned long maxlen );

	/** define a message to add when reading a small block */
	void setDebugMessage(char const *msg) const
	{
		m_debugMessage = msg ? msg : "";
	}
	/** mark a list of block with a debugging message */
	void markDebug(std::vector<unsigned long> const &blocks, char const *msg);
protected:
	/** add main debugging information (header and directory entries ) */
	void addDebugInfo(std::vector<unsigned long> const &directoryBlocks);
	/** load the file structure and fill m_result */
	void load();

	shared_ptr<RVNGInputStream> m_input; // the input file
	Header m_header;           // storage header
	DirTree m_dirtree;         // directory tree
	AllocTable m_bbat;         // allocation table for big blocks
	AllocTable m_sbat;         // allocation table for small blocks

	std::vector<unsigned long> m_sb_blocks; // blocks for "small" files
	bool m_isLoad;             // true if we have load the structure file
	Storage::Result m_result;              // result of operation

private:
	// no copy or assign
	IStorage( const IStorage & );
	IStorage &operator=( const IStorage & );

	//! a debug message
	mutable std::string m_debugMessage;
	//! the debug file
	libwps::DebugFile m_asciiFile;
};


IStorage::IStorage( shared_ptr<RVNGInputStream> is ) :
	m_input( is ),
	m_header(), m_dirtree(), m_bbat(), m_sbat(), m_sb_blocks(),
	m_isLoad(false), m_result(Storage::Ok),
	m_debugMessage(""), m_asciiFile()
{
	m_bbat.m_blockSize = m_header.m_size_bbat;
	m_sbat.m_blockSize = m_header.m_size_sbat;
}

bool IStorage::isStructuredDocument()
{
	if (!m_input) return false;
	long actPos = m_input->tell();
	load();
	m_input->seek(actPos, RVNG_SEEK_SET);
	return (m_result == Storage::Ok);
}

bool IStorage::isSubStream(const std::string &name, bool &isDir)
{
	if (!name.length()) return false;
	load();
	// search in the entries
	DirEntry *e = m_dirtree.entry(name);
	if( !e) return false;
	isDir = e->is_dir();
	return true;
}

unsigned long IStorage::loadSmallBlock
( unsigned long block, unsigned char *data, unsigned long maxlen )
{
	// sentinel
	if( !data ) return 0;

	// wraps call for loadSmallBlocks
	std::vector<unsigned long> blocks;
	blocks.resize( 1 );
	blocks.assign( 1, block );

	return loadSmallBlocks( blocks, data, maxlen );
}

unsigned long IStorage::loadBigBlock
( unsigned long block, unsigned char *data, unsigned long maxlen )
{
	// sentinel
	if( !data ) return 0;

	// wraps call for loadBigBlocks
	std::vector<unsigned long> blocks;
	blocks.resize( 1 );
	blocks[ 0 ] = block;

	return loadBigBlocks( blocks, data, maxlen );
}

unsigned long IStorage::loadBigBlocks
( std::vector<unsigned long>  const &blocks, unsigned char *data, unsigned long maxlen )
{
	// sentinel
	if( !data ) return 0;
	if( blocks.size() < 1 ) return 0;
	if( maxlen == 0 ) return 0;

	// read block one by one, seems fast enough
	unsigned long bytes = 0;
	for( unsigned long i=0; (i < blocks.size() ) & ( bytes<maxlen ); i++ )
	{
		unsigned long block = blocks[i];
		unsigned long pos =  m_bbat.m_blockSize * ( block+1 );
		unsigned long p = (m_bbat.m_blockSize < maxlen-bytes) ? m_bbat.m_blockSize : maxlen-bytes;

		m_input->seek(long(pos), RVNG_SEEK_SET);
		unsigned long numBytesRead = 0;
		const unsigned char *buf = m_input->read(p, numBytesRead);
		memcpy(data+bytes, buf, numBytesRead);
		bytes += numBytesRead;
	}

	return bytes;
}

// return number of bytes which has been read
unsigned long IStorage::loadSmallBlocks
( std::vector<unsigned long>  const &blocks, unsigned char *data, unsigned long maxlen )
{
	// sentinel
	if( !data  || blocks.size() < 1 ||  maxlen == 0 ) return 0;

	// our own local buffer
	std::vector<unsigned char> tmpBuf( m_bbat.m_blockSize );

	// read small block one by one
	libwps::DebugStream f;
	unsigned long bytes = 0;
	for( unsigned long i=0; ( i<blocks.size() ) & ( bytes<maxlen ); i++ )
	{
		unsigned long block = blocks[i];

		// find where the small-block exactly is
		unsigned long pos = block * m_sbat.m_blockSize;
		unsigned long bbindex = pos / m_bbat.m_blockSize;
		if( bbindex >= m_sb_blocks.size() ) break;

		loadBigBlock( m_sb_blocks[ bbindex ], &tmpBuf[0], m_bbat.m_blockSize );

		// copy the data
		unsigned offset = unsigned(pos % m_bbat.m_blockSize);
		unsigned long p = (maxlen-bytes < m_bbat.m_blockSize-offset ) ? maxlen-bytes :  m_bbat.m_blockSize-offset;
		p = (m_sbat.m_blockSize<p ) ? m_sbat.m_blockSize : p;
		memcpy( data + bytes, &tmpBuf[offset], p );
		bytes += p;

		f << "OLE(SmallBock" << block << "-" << i << ")[" << m_debugMessage << "]:";
		m_asciiFile.addPos(long(m_bbat.m_blockSize*(m_sb_blocks[ bbindex ]+1)+offset));
		m_asciiFile.addNote(f.str().c_str());
		m_asciiFile.addPos(long(m_bbat.m_blockSize*(m_sb_blocks[ bbindex ]+1)+offset+p));
		m_asciiFile.addNote("_");
	}

	return bytes;
}

void IStorage::load()
{
	if (m_isLoad)
		return;
	m_isLoad = true;
	m_result = Storage::NotOLE;
	if (!m_input)
		return;

	std::vector<unsigned long> blocks, metablocks;

	// load header
	unsigned long numBytesRead = 0;
	m_input->seek(0, RVNG_SEEK_SET);
	const unsigned char *buf = m_input->read(512, numBytesRead);

	if (numBytesRead < 512)
		return;

	m_header.load( buf, numBytesRead );

	// check OLE magic id
	if (!m_header.valid_signature())
		return;

#if DEBUG_OLE
	WPSInputStreamPtr oleStream(new WPSInputStream(m_input, true));
	m_asciiFile.setStream(oleStream);
	m_asciiFile.open("OLE");
#endif

	// sanity checks
	m_result = Storage::BadOLE;
	if( !m_header.valid() ) return;
	if( m_header.m_threshold != 4096 ) return;

	// important block size
	m_bbat.m_blockSize = m_header.m_size_bbat;
	m_sbat.m_blockSize = m_header.m_size_sbat;

	// find blocks allocated to store big bat
	// the first 109 blocks are in header, the rest in meta bat
	blocks.clear();
	blocks.resize( m_header.m_num_bat );
	for( unsigned j = 0; j < 109; j++ )
		if( j >= m_header.m_num_bat ) break;
		else blocks[j] = m_header.m_blocks_bbat[j];
	if( (m_header.m_num_bat > 109) && (m_header.m_num_mbat > 0) )
	{
		std::vector<unsigned char> buffer2( m_bbat.m_blockSize );
		unsigned k = 109;
		unsigned long sector;
		for( unsigned r = 0; r < m_header.m_num_mbat; r++ )
		{
			if(r == 0) // 1st meta bat location is in file header.
				sector = m_header.m_start_mbat;
			else      // next meta bat location is the last current block value.
				sector = blocks[--k];
			metablocks.push_back(sector);
			loadBigBlock( sector, &buffer2[0], m_bbat.m_blockSize );
			for( unsigned s=0; s < m_bbat.m_blockSize; s+=4 )
			{
				if( k >= m_header.m_num_bat ) break;
				else  blocks[k++] = readU32( &buffer2[s] );
			}
		}
		markDebug(metablocks,"MetaBlock");
	}

	// load big bat
	if( blocks.size()*m_bbat.m_blockSize > 0 )
	{
		std::vector<unsigned char> buffer( blocks.size()*m_bbat.m_blockSize );
		loadBigBlocks( blocks, &buffer[0], buffer.size() );
		m_bbat.load( &buffer[0], (unsigned int)buffer.size() );
	}
	markDebug(blocks,"BigBlock[allocTable]");

	// load small bat
	blocks.clear();
	blocks = m_bbat.follow( m_header.m_start_sbat );
	if( blocks.size()*m_bbat.m_blockSize > 0 )
	{
		std::vector<unsigned char> buffer( blocks.size()*m_bbat.m_blockSize );
		loadBigBlocks( blocks, &buffer[0], buffer.size() );
		m_sbat.load( &buffer[0], (unsigned int) buffer.size() );
	}
	markDebug(blocks,"SmallBlock[allocTable]");

	// load directory tree
	blocks.clear();
	blocks = m_bbat.follow( m_header.m_start_dirent );
	if (blocks.size()*m_bbat.m_blockSize)
	{
		std::vector<unsigned char> buffer(blocks.size()*m_bbat.m_blockSize);
		loadBigBlocks( blocks, &buffer[0], buffer.size() );
		m_dirtree.load( &buffer[0], (unsigned int) buffer.size() );
		if (buffer.size() >= 0x74 + 4)
		{
			unsigned sb_start = (unsigned) readU32( &buffer[0x74] );
			addDebugInfo(blocks);

			// fetch block chain as data for small-files
			m_sb_blocks = m_bbat.follow( sb_start ); // small files

			// so far so good
			m_result = Storage::Ok;
#if defined(DEBUG) && DEBUG_OLE
			m_dirtree.print_all_siblings(std::cout);
#endif
		}
		else
		{
			WPS_DEBUG_MSG(("IStorage::load: can not find small block entry\n"));
		}
	}
	else
	{
		WPS_DEBUG_MSG(("IStorage::load: can not find dirent block\n"));
	}
}

void IStorage::markDebug(std::vector<unsigned long> const &blocks, char const *msg)
{
	libwps::DebugStream f;
	for (size_t i = 0; i < blocks.size(); i++)
	{
		f.str("");
		f << msg;
		if (i) f << "[part" << i << "]";
		m_asciiFile.addPos(long(m_bbat.m_blockSize*(blocks[i]+1)));
		m_asciiFile.addNote(f.str().c_str());
		m_asciiFile.addPos(long(m_bbat.m_blockSize*(blocks[i]+2)));
		m_asciiFile.addNote("_");
	}
}

void IStorage::addDebugInfo(std::vector<unsigned long> const &blocks)
{
	libwps::DebugStream f;
	f << "OLE(Header):" << m_header << ",";
	m_asciiFile.addPos(0);
	m_asciiFile.addNote(f.str().c_str());
	m_asciiFile.addPos(512);
	m_asciiFile.addNote("_");

	markDebug(blocks,"Directory");
	unsigned entryByBlock=m_bbat.m_blockSize/128;
	unsigned numBlocks=(unsigned) blocks.size();
	long pos=0;
	for (unsigned i=0; i < m_dirtree.count(); i++)
	{
		if (entryByBlock==0 || (i%entryByBlock)==0)
		{
			if (i >= entryByBlock*numBlocks)
			{
				WPS_DEBUG_MSG(("IStorage::addDebugInfo: oops find to many entry\n"));
				return;
			}
			pos = long(m_bbat.m_blockSize*(blocks[i/entryByBlock]+1));
		}
		DirEntry const *e = m_dirtree.entry(i);
		f.str("");
		f << "DirEntry" << int(i) << ":";
		if (!e) f << "###";
		else f << *e;
		m_asciiFile.addPos(pos);
		m_asciiFile.addNote(f.str().c_str());
		pos += 128;
	}
}

////////////////////////////////////////////////////////////
//            =========== IStream ==========
class IStream
{
public:
	//! constructor
	IStream( IStorage *io, std::string const &name );
	//! destructor
	~IStream() { }
	//! return the stream size
	unsigned long size() const
	{
		return m_size;
	}
	//! return the actual position in the stream
	unsigned long tell() const
	{
		return m_pos;
	}
	//! try to read maxlen bytes
	unsigned long read( unsigned char *data, unsigned long maxlen )
	{
		unsigned long bytes = read( tell(), data, maxlen );
		m_pos += bytes;
		return bytes;
	}

private:
	unsigned long read( unsigned long pos, unsigned char *data, unsigned long maxlen );

	// no copy or assign
	IStream( const IStream & );
	IStream &operator=( const IStream & );

	//! the main storage
	IStorage *m_io;
	//! the stream size
	unsigned long m_size;
	//! the full stream name
	std::string m_name;
	//! the list of block in the stream
	std::vector<unsigned long> m_blocks;
	//! pointer to the actual position
	unsigned long m_pos;

};

IStream::IStream( IStorage *s, std::string const &name) :
	m_io(s),
	m_size(0),
	m_name(name),
	m_blocks(),
	m_pos(0)
{
	if( !name.length() || !s) return;
	DirEntry *e = s->entry( name );
	if (!e || e->is_dir()) return;
	m_size = e->m_size;
	if( m_io->use_big_block_for(m_size) )
		m_blocks = m_io->bbat().follow( e->m_start );
	else
		m_blocks = m_io->sbat().follow( e->m_start );
}

unsigned long IStream::read( unsigned long pos, unsigned char *data, unsigned long maxlen )
{
	// sanity checks
	if( !data ) return 0;
	if( maxlen == 0 ) return 0;

	unsigned long totalbytes = 0;
	if( !m_io->use_big_block_for(m_size) )
	{
		m_io->setDebugMessage("DataFile");
		// small file
		unsigned sBlockSize = m_io->sbat().m_blockSize;
		unsigned long index = pos / sBlockSize;

		if( index >= m_blocks.size() ) return 0;

		std::vector<unsigned char> buf( sBlockSize );
		unsigned long offset = pos % sBlockSize;
		while( totalbytes < maxlen )
		{
			if( index >= m_blocks.size() ) break;
			m_io->loadSmallBlock( m_blocks[index], &buf[0], m_io->bbat().m_blockSize );
			unsigned long count = sBlockSize - offset;
			if( count > maxlen-totalbytes ) count = maxlen-totalbytes;
			memcpy( data+totalbytes, &buf[offset], count );
			totalbytes += count;
			offset = 0;
			index++;
		}
	}
	else
	{
		libwps::DebugStream f;
		f << "DataFile[" << m_name << "]";
		m_io->markDebug(m_blocks, f.str().c_str());
		// big file
		unsigned bBlockSize = m_io->bbat().m_blockSize;
		unsigned long index = pos / bBlockSize;

		if( index >= m_blocks.size() ) return 0;

		std::vector<unsigned char> buf( bBlockSize );
		unsigned long offset = pos % bBlockSize;
		while( totalbytes < maxlen )
		{
			if( index >= m_blocks.size() ) break;
			m_io->loadBigBlock( m_blocks[index], &buf[0], bBlockSize );
			unsigned long count = bBlockSize - offset;
			if( count > maxlen-totalbytes ) count = maxlen-totalbytes;
			memcpy( data+totalbytes, &buf[offset], count );
			totalbytes += count;
			index++;
			offset = 0;
		}
	}

	return totalbytes;
}

////////////////////////////////////////////////////////////
//            =========== OStorage ==========
class OStorage
{
public:
	//! constructor
	OStorage(unsigned long minSize=0) : m_header(), m_dirtree(), m_bbat(), m_num_bbat(0), m_sbat(), m_num_sbat(0), m_sb_blocks(), m_data()
	{
		// update data to contain at least, header
		m_data.resize(minSize>512 ? minSize : 512);
	}
	//! destructor
	~OStorage() { }

	//! try to return a stream containing the ole file
	shared_ptr<RVNGInputStream> getStream()
	{
		shared_ptr<WPSStringStream> res;
		try
		{
			if (!updateToSave())
				return res;
			res.reset(new WPSStringStream(&m_data[0], (unsigned int) m_data.size()));
		}
		catch(...)
		{
			res.reset();
		}
		return res;
	}

	//! function to retrieve the list of actual direntry
	std::vector<std::string> getSubStreamList(unsigned ind=0, bool all=false)
	{
		return m_dirtree.getSubStreamList(ind, all);
	}
	/** set the OLE revision */
	void setRevision(unsigned rev)
	{
		m_header.m_revision=rev;
	}
	//! set the root to a mac/pc root */
	void setRootType(bool pc)
	{
		m_dirtree.setRootType(pc);
	}
	//! add a new stream knowing its data
	bool addStream(std::string const &name, unsigned char const *buffer, unsigned long len);
	//! add a new directory (usefull to create empty leaf dir )
	bool addDirectory(std::string const &dir);
	//! set a node information
	void setInformation(std::string const &name, DirInfo const &info)
	{
		DirEntry *e = m_dirtree.entry(name);
		if (!e)
		{
			WPS_DEBUG_MSG(("libwpsOLE::OStorage::setInformation: can not find entry %s!!!\n", name.c_str()));
			return;
		}
		e->m_info = info;
	}

protected:
	//! add stream data in a file, returns the first index
	unsigned insertData(unsigned char const *buffer, unsigned long len, bool useBigBlock, unsigned end=Eof);

	//! return true if we need to use big block
	bool useBigBlockFor(unsigned long size) const
	{
		return size >= m_header.m_threshold;
	}
	//! returns the maximum size of a big/small block
	static unsigned long getMaximumSize(bool isBig)
	{
		return isBig ? 512 : 64;
	}
	//! returns the address of a big/small block
	size_t getDataAddress(unsigned block, bool isBig) const
	{
		if (isBig) return size_t((block+1)*512);
		size_t bId=size_t(block/8);
		if (bId >= m_sb_blocks.size()) throw libwps::GenericException();
		return size_t((m_sb_blocks[bId]+1)*512+64*(block%8));
	}

	//! create a new big block, resize m_data; ... and return is identifier
	unsigned newBBlock()
	{
		unsigned res = m_num_bbat++;
		if (m_data.size() < (res+2)*512) m_data.resize((res+2)*512, 0);
		m_bbat.resize(res+1);
		return res;
	}
	//! create a new small block, ... and returns is identifier
	unsigned newSBlock()
	{
		unsigned res = m_num_sbat++;
		if ((res%8)==0)
			m_sb_blocks.push_back(newBBlock());
		m_sbat.resize(res+1);
		return res;
	}

	//! return a new dir entry, if it does not exists
	DirEntry *createEntry(std::string const &name)
	{
		if (name.length()==0)
		{
			WPS_DEBUG_MSG(("libwpsOLE::OStorage::createEntry: called with no name\n"));
			return 0;
		}
		if (m_dirtree.index(name)!=NotFound)
		{
			WPS_DEBUG_MSG(("libwpsOLE::OStorage::createEntry: entry %s already exists\n", name.c_str()));
			return 0;
		}
		unsigned index = m_dirtree.index(name,true);
		if (index == NotFound) return 0;

		return m_dirtree.entry(index);
	}

	//! finish to update the file ( note: it is better to call this function only one time )
	bool updateToSave();

	Header m_header;           // storage header
	DirTree m_dirtree;         // directory tree
	AllocTable m_bbat;         // allocation table for big blocks
	unsigned m_num_bbat;       // the number of bbat
	AllocTable m_sbat;         // allocation table for small blocks
	unsigned m_num_sbat;       // the number of sbat

	std::vector<unsigned long> m_sb_blocks; // blocks for "small" files

	std::vector<unsigned char> m_data;
private:
	// no copy or assign
	OStorage( const OStorage & );
	OStorage &operator=( const OStorage & );
};

bool OStorage::addDirectory(std::string const &dir)
{
	DirEntry *e=createEntry(dir);
	if (!e) return false;

	e->m_type=1;
	return true;
}

bool OStorage::addStream(std::string const &name, unsigned char const *buffer, unsigned long len)
{
	DirEntry *e=createEntry(name);
	if (!e) return false;

	if (!len)
	{
		WPS_DEBUG_MSG(("libwpsOLE::OStorage::addStream: call to create an empty file!!!\n"));
		return true;
	}
	e->m_start = insertData(buffer, len, useBigBlockFor(len));
	e->m_size = len;
	return true;
}

bool OStorage::updateToSave()
{
	unsigned long dirSize = m_dirtree.saveSize();
	DirEntry *rEntry = m_dirtree.entry(0);
	if (!dirSize || !rEntry)
	{
		WPS_DEBUG_MSG(("libwpsOLE::OStorage::updateToSave: can not find dir tree size!!!\n"));
		return false;
	}
	m_dirtree.setInRedBlackTreeForm();

	std::vector<unsigned char> buffer;
	// first add the small block allocation table
	unsigned long sbatSize = m_sbat.saveSize();
	if (!sbatSize)
		m_header.m_start_sbat = Bat;
	else
	{
		// FIXME: set m_header.m_start_sbat
		buffer.resize(sbatSize);
		m_sbat.save(&buffer[0]);
		m_header.m_num_sbat = (unsigned) (sbatSize+511)/512;
		m_header.m_start_sbat = insertData(&buffer[0], sbatSize, true, unsigned(Eof));
		if (m_sb_blocks.size())
		{
			rEntry->m_start =(unsigned) m_sb_blocks[0];
			m_bbat.setChain(m_sb_blocks, unsigned(Eof));
		}
		rEntry->m_size = 64*m_num_sbat;
	}
	// then add dirtree
	buffer.resize(dirSize);
	m_dirtree.save(&buffer[0]);
	m_header.m_start_dirent=insertData(&buffer[0], dirSize, true, unsigned(Eof));

	// now update the big alloc table to add size for self and for the meta data
	unsigned numBBlock = m_num_bbat;
	if (numBBlock==0)
	{
		WPS_DEBUG_MSG(("libwpsOLE::OStorage::updateToSave: can not find any big block!!!\n"));
		return false;
	}
	unsigned numBAlloc = (numBBlock+127)/128;
	unsigned numMAlloc = (numBAlloc+17)/127; // ie. 109->0, 110->1, ...
	while (numBBlock+numBAlloc+numMAlloc > numBAlloc*128)
	{
		numBAlloc++;
		numMAlloc = (numBAlloc+17)/127;
	}

	std::vector<unsigned long> mainBlocks(numBAlloc);
	for (unsigned b = 0; b < numBAlloc; b++)
	{
		mainBlocks[b]=numBBlock+b;
		m_bbat.set(mainBlocks[b], unsigned(Bat));
	}
	if (numMAlloc)
	{
		for (unsigned b = 0; b < numMAlloc; b++)
			m_bbat.set(numBBlock+numBAlloc+b, unsigned(MetaBat));
	}

	// now add the small block allocation table
	unsigned long bbatSize = m_bbat.saveSize();
	if (bbatSize)
	{
		buffer.resize(bbatSize);
		m_bbat.save(&buffer[0]);
		insertData(&buffer[0], bbatSize, true, unsigned(Bat));
	}

	for (unsigned b = 0; b < numBAlloc; b++)
	{
		if (b >= 109)
			break;
		m_header.m_blocks_bbat[b]=mainBlocks[b];
	}
	if (numMAlloc)
	{
		buffer.resize(numMAlloc*512,0);
		size_t wPos=0;
		for (unsigned b=109; b < numBAlloc; b++)
		{
			if ((wPos%512)==508)
			{
				writeU32(&buffer[wPos],numBBlock+numBAlloc+(wPos+4)/512);
				wPos+=4;
			}
			writeU32(&buffer[wPos],mainBlocks[b]);
			wPos+=4;
		}
		while (wPos%512)
		{
			writeU32(&buffer[wPos], Avail);
			wPos+=4;
		}
		m_header.m_start_mbat = insertData((unsigned char const *)&buffer[0], 512*numMAlloc, true, unsigned(Eof));
		m_header.m_start_mbat = numBBlock+numBAlloc;
	}
	m_header.m_num_bat = (m_num_bbat+127)/128;
	m_header.m_num_mbat = numMAlloc;

	m_data.resize((m_num_bbat+1)*512,0);
	// save the header
	m_header.save(&m_data[0]);
	return true;
}

unsigned OStorage::insertData(unsigned char const *buffer, unsigned long len, bool useBigBlock, unsigned end)
{
	if (buffer==0 || len == 0)
	{
		WPS_DEBUG_MSG(("libwpsOLE::OStorage::insertData: call with no data\n"));
		return 0;
	}

	unsigned long bSize = getMaximumSize(useBigBlock);
	size_t nBlock = ((len+bSize-1)/bSize);
	std::vector<unsigned long> chain;
	for (size_t b=0; b < nBlock; b++)
	{
		unsigned block=useBigBlock ? newBBlock() : newSBlock();
		chain.push_back(block);
		size_t wPos = getDataAddress(block, useBigBlock);
		unsigned long wSize = len > bSize ? bSize : len;
		memcpy(&m_data[wPos], &buffer[0], wSize);
		buffer += bSize;
		len -= wSize;
	}
	if (useBigBlock)
		m_bbat.setChain(chain, end);
	else
		m_sbat.setChain(chain, end);
	return (unsigned) chain[0];
}

////////////////////////////////////////////////////////////
//                       Storage
////////////////////////////////////////////////////////////
Storage::Storage( shared_ptr<RVNGInputStream> is ) : m_io(0)
{
	m_io = new IStorage( is );
}

Storage::~Storage()
{
	delete m_io;
}

bool Storage::isStructuredDocument()
{
	return m_io->isStructuredDocument();
}

std::vector<std::string> Storage::getSubStreamList(std::string dir, bool onlyFiles)
{

	std::vector<std::string> res;
	unsigned index = m_io->index(dir);

	if (index==NotFound)  return res;
	res=m_io->getSubStreamList(index, !onlyFiles);

	// time to do some cleaning ( remove ^A, ^B, ... )
	for (size_t i = 0; i < res.size(); i++)
	{
		std::string str=res[i], finalStr("");
		for (size_t s=0; s<str.length(); s++)
			if (str[s]>=32)
				finalStr+=str[s];
		res[i]=finalStr;
	}
	return res;
}

bool Storage::isSubStream(const std::string &name)
{
	if (!isStructuredDocument() || !name.length())
		return false;
	bool isDir;
	if (!m_io->isSubStream(name, isDir))
		return false;
	return !isDir;
}

bool Storage::isDirectory(const std::string &name)
{
	if (!isStructuredDocument() || !name.length())
		return false;
	bool isDir;
	if (!m_io->isSubStream(name, isDir))
		return false;
	return isDir;
}

shared_ptr<RVNGInputStream> Storage::getSubStream(const std::string &name)
{
	shared_ptr<RVNGInputStream> res;
	if (!isStructuredDocument() || !name.length())
		return res;
	if (isDirectory(name))
		return getSubStreamForDirectory(name);
	IStream stream(m_io, name);
	unsigned long sz = stream.size();
	if (!sz)
		return res;

	unsigned char *buf = new unsigned char[sz];
	if (buf == 0) return res;

	unsigned long oleLength = stream.read(buf, sz);
	if (oleLength != sz)
	{
		WPS_DEBUG_MSG(("Storage::getSubStream: Ole=%s expected length %ld but read %ld\n",
		               name.c_str(), sz, oleLength));

		// Checkme: we ignore this error, if we read a ole in the root
		// directory and we read at least 50% of the data. This may
		// help to read a damaged file.
		bool rootDir = name.find('/', 0) == std::string::npos;
		if (!rootDir || oleLength <= (sz+1)/2)
		{
			delete [] buf;
			return res;
		}
		// continue
		WPS_DEBUG_MSG(("libwpsOLE::Storage::getSubStream: tries to use damaged OLE: %s\n", name.c_str()));
	}

	res.reset(new WPSStringStream(buf, (unsigned int) oleLength));
	delete [] buf;
	return res;
}

shared_ptr<RVNGInputStream> Storage::getSubStreamForDirectory(const std::string &name)
{
	shared_ptr<RVNGInputStream> res;
	if (!isStructuredDocument() || !name.length() || !isDirectory(name))
		return res;

	unsigned index = m_io->index(name);
	if (index==NotFound)  return res;
	std::vector<std::string> nodes=m_io->getSubStreamList(index, true);
	if (nodes.size() <= 1)
	{
		WPS_DEBUG_MSG(("libwpsOLE::Storage::getSubStreamForDirectory: can not find any child for %s\n", name.c_str()));
		return res;
	}

	try
	{
		std::string dir(name);
		size_t len=dir.length();
		if (len && dir[len-1]=='/') dir.resize(len-1);
		std::vector<unsigned char> buffer;

		// try to compute a minimum data size
		unsigned long minimalSize = 512;
		for (size_t l=0; l < nodes.size(); l++)
		{
			std::string fullName=dir+nodes[l];
			DirEntry *entry=m_io->entry(fullName);
			if (!entry || !entry->m_valid)
				continue;
			minimalSize += 128; // DirEntry
			if (!entry->is_dir() && entry->m_size < 50000000)
				minimalSize+=((entry->m_size+63)/64)*64;
		}

		OStorage storage(minimalSize);
		storage.setRevision(m_io->revision());
		if (!m_io->hasRootTypePc())
			storage.setRootType(false);
		for (size_t l=0; l < nodes.size(); l++)
		{
			std::string fullName=dir+nodes[l];
			DirEntry *entry=m_io->entry(fullName);
			if (!entry)
			{
				WPS_DEBUG_MSG(("libwpsOLE::Storage::getSubStreamForDirectory: can not find child for %s\n", fullName.c_str()));
				continue;
			}
			if (entry->is_dir())
			{
				if (entry->m_child == DirEntry::End)
					storage.addDirectory(nodes[l]);
				continue;
			}
			IStream leafStream(m_io,fullName);
			unsigned long sz=leafStream.size();
			bool ok = true;
			if (sz)
			{
				// a test because empty file are rare but seems to exists
				buffer.resize(sz);
				/* a strict test which explains that dir subwith broken
				   OLE file will not be retrieved  */
				ok = leafStream.read(&buffer[0], sz) == sz;
			}
			if (ok)
				ok=storage.addStream(nodes[l], &buffer[0], sz);
			if (!ok)
			{
				WPS_DEBUG_MSG(("libwpsOLE::Storage::getSubStreamForDirectory: can not read %s\n", fullName.c_str()));
				return res;
			}
		}
		// finally try to update the storage info
		std::vector<std::string> resNodes=storage.getSubStreamList(0, true);
		for (size_t l=0; l < resNodes.size(); l++)
		{
			std::string fullName=dir+resNodes[l];
			DirEntry *entry=m_io->entry(fullName);
			if (!entry) continue;
			if (resNodes[l].length())
				storage.setInformation(resNodes[l], entry->m_info);
		}
		return storage.getStream();
	}
	catch(...)
	{
	}
	return res;
}
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
