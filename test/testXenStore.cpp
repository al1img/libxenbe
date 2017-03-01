/*
 * testXenStore.cpp
 *
 *  Created on: Feb 28, 2017
 *      Author: al1
 */

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <catch.hpp>

#include "mocks/XenStoreMock.hpp"
#include "XenStore.hpp"

using std::chrono::milliseconds;
using std::condition_variable;
using std::exception;
using std::find;
using std::mutex;
using std::string;
using std::unique_lock;
using std::unique_ptr;
using std::vector;

using XenBackend::XenStore;
using XenBackend::XenStoreException;

mutex gMutex;
condition_variable gCondVar;

int gNumErrors = 0;
bool gWatchCbk1 = false;
bool gWatchCbk2 = false;

void errorHandling(const exception& e)
{
	gNumErrors++;
}

void watchCbk1()
{
	unique_lock<mutex> lock(gMutex);

	gWatchCbk1 = true;

	gCondVar.notify_all();
}

void watchCbk2()
{
	unique_lock<mutex> lock(gMutex);

	gWatchCbk2 = true;

	gCondVar.notify_all();
}

void waitForWatch()
{
	unique_lock<mutex> lock(gMutex);

	gCondVar.wait_for(lock, milliseconds(100));
}

TEST_CASE("XenStore", "[xenstore]")
{
	XenStore xenStore(errorHandling);
	auto mock = XenStoreMock::getInstance();

	SECTION("Check getting domain path")
	{
		string path = "/local/domain/3/";

		mock->setDomainPath(3, path);

		REQUIRE_THAT(xenStore.getDomainPath(3), Equals(path));
	}

	SECTION("Check read/write")
	{
		string path = "/local/domain/3/value";
		int intVal = -34567;
		unsigned int uintVal = 23567;
		string strVal = "This is string value";

		xenStore.writeInt(path, intVal);
		REQUIRE(xenStore.readInt(path) == intVal);

		xenStore.writeUint(path, uintVal);
		REQUIRE(xenStore.readUint(path) == uintVal);

		xenStore.writeString(path, strVal);
		REQUIRE(xenStore.readString(path) == strVal);

		REQUIRE_THROWS_AS(xenStore.readInt("/non/exist/entry"),
						  const XenStoreException&);
	}

	SECTION("Check exist/remove")
	{
		string path = "/local/domain/3/exist";

		xenStore.writeString(path, "This entry exists");
		REQUIRE(xenStore.checkIfExist(path));

		xenStore.removePath(path);
		REQUIRE_FALSE(xenStore.checkIfExist(path));
	}

	SECTION("Check read directory")
	{
		string path = "/local/domain/3/directory/";
		vector<string> items = {"Item0", "Item1", "SubDir0", "SubDir1"};


		xenStore.writeString(path + items[0], "Entry 0");
		xenStore.writeString(path + items[1], "Entry 1");
		xenStore.writeString(path + items[2] + "/entry0", "Entry 0");
		xenStore.writeString(path + items[2] + "/entry1", "Entry 0");
		xenStore.writeString(path + items[3] + "/entry0", "Entry 0");
		xenStore.writeString(path + items[3] + "/entry1", "Entry 0");

		auto result = xenStore.readDirectory(path);

		for(auto item : items)
		{
			auto it = find(result.begin(), result.end(), item);

			REQUIRE(it != result.end());

			result.erase(it);
		}

		REQUIRE(result.size() == 0);

		result = xenStore.readDirectory("/non/exist/dir");

		REQUIRE(result.size() == 0);
	}

	SECTION("Check watches")
	{
		string path = "/local/domain/3/watch1";
		string value = "Value1";

		xenStore.setWatch(path, watchCbk1);

//		std::this_thread::sleep_for(milliseconds(100));

		mock->writeValue(path, "Changed");

		waitForWatch();

		REQUIRE(gWatchCbk1);

		xenStore.clearWatch(path);

		path = "/local/domain/3/watch2";
		value = "Value2";

		xenStore.setWatch(path, watchCbk2);

		mock->writeValue(path, value);

		waitForWatch();

		REQUIRE(gWatchCbk2);

		xenStore.clearWatch(path);
	}
}

