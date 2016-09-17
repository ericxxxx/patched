/*
 * Copyright 2003-2016 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "MmsInputPlugin.hxx"
#include "input/ThreadInputStream.hxx"
#include "input/InputPlugin.hxx"
#include "system/Error.hxx"
#include "util/StringCompare.hxx"

#include <libmms/mmsx.h>

#include <stdexcept>

static constexpr size_t MMS_BUFFER_SIZE = 256 * 1024;

class MmsInputStream final : public ThreadInputStream {
	mmsx_t *mms;

public:
	MmsInputStream(const char *_uri, Mutex &_mutex, Cond &_cond)
		:ThreadInputStream(input_plugin_mms.name, _uri, _mutex, _cond,
				   MMS_BUFFER_SIZE) {
	}

protected:
	virtual void Open() override;
	virtual size_t ThreadRead(void *ptr, size_t size) override;

	void Close() override {
		mmsx_close(mms);
	}
};

void
MmsInputStream::Open()
{
	Unlock();

	mms = mmsx_connect(nullptr, nullptr, GetURI(), 128 * 1024);
	if (mms == nullptr) {
		Lock();
		throw std::runtime_error("mmsx_connect() failed");
	}

	Lock();

	/* TODO: is this correct?  at least this selects the ffmpeg
	   decoder, which seems to work fine */
	SetMimeType("audio/x-ms-wma");
}

static InputStream *
input_mms_open(const char *url,
	       Mutex &mutex, Cond &cond)
{
	if (!StringStartsWith(url, "mms://") &&
	    !StringStartsWith(url, "mmsh://") &&
	    !StringStartsWith(url, "mmst://") &&
	    !StringStartsWith(url, "mmsu://"))
		return nullptr;

	auto m = new MmsInputStream(url, mutex, cond);
	m->Start();
	return m;
}

size_t
MmsInputStream::ThreadRead(void *ptr, size_t read_size)
{
	/* unfortunately, mmsx_read() blocks until the whole buffer
	   has been filled; to avoid big latencies, limit the size of
	   each chunk we read to a reasonable size */
	constexpr size_t MAX_CHUNK = 16384;
	if (read_size > MAX_CHUNK)
		read_size = MAX_CHUNK;

	int nbytes = mmsx_read(nullptr, mms, (char *)ptr, read_size);
	if (nbytes <= 0) {
		if (nbytes < 0)
			throw MakeErrno("mmsx_read() failed");
		return 0;
	}

	return (size_t)nbytes;
}

const InputPlugin input_plugin_mms = {
	"mms",
	nullptr,
	nullptr,
	input_mms_open,
};
