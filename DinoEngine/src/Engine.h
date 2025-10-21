#pragma once

class Engine {
public:
  void parse_argv(wchar_t **argv, int nArgs);
  void start();
  void loop();
  void stop();

private:
  wchar_t *m_GameName;
};

inline Engine g_Engine{};