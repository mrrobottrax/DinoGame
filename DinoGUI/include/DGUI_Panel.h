#pragma once

struct ID3D12GraphicsCommandList10;

class DGUI_API DGUI_Panel {
public:
  DGUI_Panel() {
    m_ChildCount = 0;
    m_ChildCapacity = 0;
    m_Children = nullptr;
  }

  virtual ~DGUI_Panel();

  virtual void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList);

  void add_child(DGUI_Panel *pPanel);
  void clear_children();
  void set_position(int x, int y);
  void set_size(int w, int h);

  uint32_t get_child_count() { return m_ChildCount; }
  DGUI_Panel *get_child(uint32_t index) {
    ASSERT(index < m_ChildCount);
    return m_Children[index];
  }

private:
  DGUI_Panel **m_Children;
  uint32_t m_ChildCapacity;
  uint32_t m_ChildCount;
};