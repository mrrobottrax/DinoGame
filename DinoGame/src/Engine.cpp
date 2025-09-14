#include "pch.h"

#include "Engine.h"

#include "RenderingSystem.h"
#include "WindowSystem.h"

void Engine::init() {
  g_WindowSystem.init("DinoEngine Test");
  g_RenderingSystem.init();
}

void Engine::loop() {
  MSG msg;
  while (true) {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

      if (msg.message == WM_QUIT) {
        return;
      }

      g_RenderingSystem.frame();
    }
  }
}

void Engine::stop() {
  g_RenderingSystem.stop();
  g_WindowSystem.stop();
}