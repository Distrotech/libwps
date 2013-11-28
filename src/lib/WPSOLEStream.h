/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* POLE - Portable C++ library to access OLE Storage
   Copyright (C) 2002-2005 Ariya Hidayat <ariya@kde.org>

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

#ifndef WPSOLESTREAM_H
#define WPSOLESTREAM_H

#include <string>
#include <fstream>
#include <vector>

#include "libwps_internal.h"

class WPXInputStream;

namespace libwpsOLE
{

class IStorage;
class IStream;

/** class used to read/parse an OLE file */
class Storage
{
	friend class Stream;

public:

	// for Storage::result()
	enum Result { Ok, OpenFailed, NotOLE, BadOLE, UnknownError };

	/**
	 * Constructs a storage with data.
	 **/
	Storage(shared_ptr<WPXInputStream> is);

	/**
	 * Destroys the storage.
	 **/
	~Storage();

	/**
	 * Checks whether the storage is OLE2 storage.
	 **/
	bool isStructuredDocument();

	/**
	 * Returns the list of all ole leaves names
	 **/
	std::vector<std::string> getSubStreamList(std::string dir="/", bool onlyFiles=true);

	/**
	 * Returns true if name corresponds to a sub stream
	 **/
	bool isSubStream(const std::string &name);

	/**
	 * Returns true if name corresponds to a directory
	 **/
	bool isDirectory(const std::string &name);

	/**
	 * Returns a WPXInputStream corresponding to a leaf/directory substream
	 **/
	shared_ptr<WPXInputStream> getSubStream(const std::string &name);

private:
	/**
	 * Returns a WPXInputStream corresponding to a directory substream
	 **/
	shared_ptr<WPXInputStream> getSubStreamForDirectory(const std::string &name);

	//! the main data storage
	IStorage *m_io;

	// no copy or assign
	Storage(const Storage &);
	Storage &operator=(const Storage &);

};

}  // namespace libwps

#endif // WPXOLESTREAM_H
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
