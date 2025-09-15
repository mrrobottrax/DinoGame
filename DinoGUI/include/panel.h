#pragma once

class DGUI_API DGUI_Panel {
public:
  void add_child(DGUI_Panel *pPanel);
  void set_position(int x, int y);
  void set_size(int w, int h);
};