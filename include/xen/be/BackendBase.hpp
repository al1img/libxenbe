/*
 *  Xen backend base class
 *
 *  Based on
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

#ifndef INCLUDE_BACKENDBASE_HPP_
#define INCLUDE_BACKENDBASE_HPP_

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "FrontendHandlerBase.hpp"
#include "XenException.hpp"
#include "XenStore.hpp"
#include "XenStat.hpp"
#include "Log.hpp"

namespace XenBackend {

/***************************************************************************//**
 * @defgroup backend Backend
 * Contains classes and primitives to create the full featured Xen backend.
 ******************************************************************************/

/***************************************************************************//**
 * Exception generated by BackendBase.
 * @ingroup backend
 ******************************************************************************/
class BackendException : public XenException
{
	using XenException::XenException;
};

/***************************************************************************//**
 * Base class for a backend implementation.
 *
 * The main functionality is detecting a new frontend and deleting
 * terminated frontends.
 *
 * This class periodically checks if a new domain appears with Xen control API.
 * Then it looks up for a specific path in Xen store where the frontend
 * stores its state. If the path exists we assume that an expected frontend is
 * running or will be run on this domain. At same loop the backend class checks
 * FrontendHandlerBase::isTerminated() method and deleted the terminated
 * frontend. The frontend is terminated when during operation a critical
 * error occurs.
 *
 * Once the new frontend is detected it calls onNewFrontend() method.
 * This method should be overridden by the custom instance of BackendBase class.
 * It is expected that in this method a new instance of FrontendHandlerBase
 * class will be created and added with addFrontendHandler() method.
 * Adding the frontend handler is required to allow the backend class monitoring
 * terminated frontends.
 *
 * The client should create a class inherited from BackendBase and implement
 * onNewFrontend() method.
 *
 * Example of the client backend class:
 *
 * @snippet ExampleBackend.hpp ExampleBackend
 *
 * onNewFrontend() example:
 *
 * @snippet ExampleBackend.cpp onNewFrontend
 *
 * The client may change the new frontend detection algorithm. For this
 * reason it may override getNewFrontend() method.
 *
 * When the backend instance is created, it should be started by calling start()
 * method. The backend will process frontends till it is deleted or stop()
 * method is called. If the backend instance is used in the main loop,
 * waitForFinish() method may be used to block the main loop till it is
 * finished.
 *
 * @snippet ExampleBackend.cpp main
 *
 * @ingroup backend
 ******************************************************************************/
class BackendBase
{
public:
	/**
	 * @param[in] name       optional backend name
	 * @param[in] deviceName device name
	 * @param[in] domId      domain id
	 */
	BackendBase(const std::string& name, const std::string& deviceName,
				domid_t domId);
	virtual ~BackendBase();

	/**
	 * Starts backend.
	 */
	void start();

	/**
	 * Stops backend.
	 */
	void stop();

	/**
	 * Waits for backend is finished.
	 */
	void waitForFinish();

	/**
	 * Returns backend device name
	 */
	const std::string& getDeviceName() const { return mDeviceName; }

	/**
	 * Returns domain id
	 */
	domid_t getDomId() const { return mDomId; }

protected:

	/**
	 * Is called periodically to check if a new frontend's appeared.
	 * In order to change the way a new domain is detected the client may
	 * override this method.
	 *
	 * @param[out] domId domain id
	 * @param[out] devId device id
	 * @return <i>true</i> if new frontend is detected
	 */
	virtual bool getNewFrontend(domid_t& domId, uint16_t& devId);

	/**
	 * Is called when new frontend detected.
	 * Basically the client should create
	 * the instance of FrontendHandlerBase class and pass it to
	 * addFrontendHandler().
	 * @param[in] domId domain id
	 * @param[in] devId device id
	 */
	virtual void onNewFrontend(domid_t domId, uint16_t devId) = 0;

	/**
	 * Adds new frontend handler
	 * @param[in] frontendHandler frontend instance
	 */
	void addFrontendHandler(FrontendHandlerPtr frontendHandler);

private:

	const int cPollFrontendIntervalMs = 500; //!< Frontend poll interval in msec

	typedef std::pair<domid_t, uint16_t> FrontendKey;

	domid_t mDomId;
	std::string mDeviceName;
	XenStore mXenStore;
	XenStat mXenStat;

	std::map<FrontendKey, FrontendHandlerPtr> mFrontendHandlers;

	std::thread mThread;
	std::atomic_bool mTerminate;
	std::atomic_bool mTerminated;

	Log mLog;

	void run();
	void createFrontendHandler(const FrontendKey& ids);
	void checkTerminatedFrontends();
};

}

#endif /* INCLUDE_BACKENDBASE_HPP_ */
