/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Umang Jain <email@uajain.com>
 *
 * hotplug-cameras.cpp - Emulate cameraAdded/cameraRemoved signals in CameraManager
 */

#include <dirent.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/event_dispatcher.h>
#include <libcamera/timer.h>

#include "libcamera/internal/file.h"
#include "libcamera/internal/thread.h"

#include "test.h"

using namespace std;
using namespace libcamera;

class HotplugTest : public Test
{
protected:
	void cameraAddedHandler(std::shared_ptr<Camera> cam)
	{
		cameraAddedPass_ = true;
	}

	void cameraRemovedHandler(std::shared_ptr<Camera> cam)
	{
		cameraRemovedPass_ = true;
	}

	int init()
	{
		if (!File::exists("/sys/module/uvcvideo")) {
			std::cout << "uvcvideo driver is not loaded, skipping" << std::endl;
			return TestSkip;
		}

		if (geteuid() != 0) {
			std::cout << "This test requires root permissions, skipping" << std::endl;
			return TestSkip;
		}

		cm_ = new CameraManager();
		if (cm_->start()) {
			std::cout << "Failed to start camera manager" << std::endl;
			return TestFail;
		}

		cameraAddedPass_ = false;
		cameraRemovedPass_ = false;

		cm_->newCameraAdded.connect(this, &HotplugTest::cameraAddedHandler);
		cm_->cameraRemoved.connect(this, &HotplugTest::cameraRemovedHandler);

		uvc_toplevel_ = "/sys/module/uvcvideo/drivers/usb:uvcvideo/";

		return 0;
	}

	int run()
	{
		DIR *dir;
		struct dirent *dirent, *dirent2;
		std::string uvc_driver_dir;
		bool uvc_driver_found = false;

		dir = opendir(uvc_toplevel_.c_str());
		/* Find a UVC device driver symlink, which we can bind/unbind */
		while ((dirent = readdir(dir)) != nullptr) {
			if (dirent->d_type != DT_LNK)
				continue;

			std::string child_dir = uvc_toplevel_ + dirent->d_name;
			DIR *device_driver = opendir(child_dir.c_str());
			while ((dirent2 = readdir(device_driver)) != nullptr) {
				if (strncmp(dirent2->d_name, "video4linux", 11) == 0) {
					uvc_driver_dir = dirent->d_name;
					uvc_driver_found = true;
					break;
				}
			}
			closedir(device_driver);

			if (uvc_driver_found)
				break;
		}
		closedir(dir);

		/* If no UVC driver found, skip */
		if (!uvc_driver_found)
			return TestSkip;

		/* Unbind a camera, process events */
		int fd1 = open("/sys/module/uvcvideo/drivers/usb:uvcvideo/unbind", O_WRONLY);
		write(fd1, uvc_driver_dir.c_str(), uvc_driver_dir.size());
		close(fd1);
		Timer timer;
		timer.start(1000);
		while (timer.isRunning())
			Thread::current()->eventDispatcher()->processEvents();

		/* \todo: Fix this workaround of stopping and starting the camera-manager.
		 *  We need to do this, so that cm_ release all references to the uvc media symlinks.
		 */
		cm_->stop();
		if (cm_->start()) {
			std::cout << "Failed to restart camera-manager" << std::endl;
			return TestFail;
		}

		/* Bind the camera again, process events */
		int fd2 = open("/sys/module/uvcvideo/drivers/usb:uvcvideo/bind", O_WRONLY);
		write(fd2, uvc_driver_dir.c_str(), uvc_driver_dir.size());
		close(fd2);

		timer.start(1000);
		while (timer.isRunning())
			Thread::current()->eventDispatcher()->processEvents();

		if (cameraAddedPass_ && cameraRemovedPass_)
			return TestPass;
		else
			return TestFail;
	}

	void cleanup()
	{
		cm_->stop();
		delete cm_;
	}

private:
	CameraManager *cm_;
	std::string uvc_toplevel_;
	bool cameraRemovedPass_;
	bool cameraAddedPass_;
};

TEST_REGISTER(HotplugTest)

