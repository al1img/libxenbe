/*
 *  Xen evtchn wrapper
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Copyright (C) 2016 EPAM Systems Inc.
 */

#ifndef SRC_XEN_XENEVTCHN_HPP_
#define SRC_XEN_XENEVTCHN_HPP_

#include <atomic>
#include <thread>

extern "C" {
#include <xenevtchn.h>
}

#include "XenException.hpp"
#include "Log.hpp"

namespace XenBackend {

/***************************************************************************//**
 * @defgroup Xen
 * Contains classes, primitives and helpers for Xen tools.
 ******************************************************************************/

/***************************************************************************//**
 * Exception generated by XenEvtchn.
 * @ingroup Xen
 ******************************************************************************/
class XenEvtchnException : public XenException
{
	using XenException::XenException;
};

/***************************************************************************//**
 * Implements xen event channel.
 * @ingroup Xen
 ******************************************************************************/
class XenEvtchn
{
public:

	/**
	 * Callback which is called when the event channel is notified
	 */
	typedef std::function<void()> Callback;

	/**
	 * @param[in] domId domain id
	 * @param[in] port  event channel port number
	 * @param[in] callback callback which is called when the notification is
	 * received
	 * @param[in] errorCallback callback which is called when an error occurs
	 */
	XenEvtchn(int domId, int port, Callback callback,
			  ErrorCallback errorCallback = nullptr);
	XenEvtchn(const XenEvtchn&) = delete;
	XenEvtchn& operator=(XenEvtchn const&) = delete;
	~XenEvtchn();

	/**
	 * Starts listening to the event channel
	 */
	void start();

	/**
	 * Stops listening to the event channel
	 */
	void stop();

	/**
	 * Notifies the event channel
	 */
	void notify();

	/**
	 * Returns event channel port
	 */
	int getPort() const { return mPort; }

private:

	const int cPoolEventTimeoutMs = 100;

	int mPort;

	Callback mCallback;
	ErrorCallback mErrorCallback;

	xenevtchn_handle *mHandle;

	std::thread mThread;
	std::atomic_bool mTerminate;

	Log mLog;

	void init(int domId, int port);
	void release();
	void eventThread();
	bool waitEvent();
};

}

#endif /* SRC_XEN_XENEVTCHN_HPP_ */
