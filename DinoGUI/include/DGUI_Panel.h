#pragma once

class DGUI_API DGUI_Panel {
public:
  void add_child(DGUI_Panel *pPanel);
  void clear_children();
  void set_position(int x, int y);
  void set_size(int w, int h);

  DGUI_Panel() {
    m_ChildCount = 0;
    m_ChildCapacity = 0;
    m_Children = nullptr;
  }

  virtual ~DGUI_Panel();

private:
  DGUI_Panel **m_Children;
  uint32_t m_ChildCapacity;
  uint32_t m_ChildCount;
};