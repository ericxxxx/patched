/*
 * Copyright (C) 2003-2014 The Music Player Daemon Project
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

#ifndef MPD_UPDATE_SERVICE_HXX
#define MPD_UPDATE_SERVICE_HXX

#include "check.h"
#include "Queue.hxx"
#include "Walk.hxx"
#include "event/DeferredMonitor.hxx"
#include "thread/Thread.hxx"

class SimpleDatabase;
class DatabaseListener;

/**
 * This class manages the update queue and runs the update thread.
 */
class UpdateService final : DeferredMonitor {
	enum Progress {
		UPDATE_PROGRESS_IDLE = 0,
		UPDATE_PROGRESS_RUNNING = 1,
		UPDATE_PROGRESS_DONE = 2
	};

	SimpleDatabase &db;

	DatabaseListener &listener;

	Progress progress;

	bool modified;

	Thread update_thread;

	static const unsigned update_task_id_max = 1 << 15;

	unsigned update_task_id;

	UpdateQueue queue;

	UpdateQueueItem next;

	UpdateWalk walk;

public:
	UpdateService(EventLoop &_loop, SimpleDatabase &_db,
		      DatabaseListener &_listener);

	/**
	 * Returns a non-zero job id when we are currently updating
	 * the database.
	 */
	unsigned GetId() const {
		return next.id;
	}

	/**
	 * Add this path to the database update queue.
	 *
	 * @param path a path to update; if an empty string,
	 * the whole music directory is updated
	 * @return the job id, or 0 on error
	 */
	gcc_nonnull_all
	unsigned Enqueue(const char *path, bool discard);

private:
	/* virtual methods from class DeferredMonitor */
	virtual void RunDeferred() override;

	/* the update thread */
	void Task();
	static void Task(void *ctx);

	void StartThread(UpdateQueueItem &&i);

	unsigned GenerateId();
};

#endif