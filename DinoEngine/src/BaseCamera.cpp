#include "pch.h"

#include "BaseCamera.h"
#include "CameraSystem.h"

void BaseCamera::spawn() { g_CameraSystem.set_main_camera(this); }