#pragma once

class DGUI_API DGUI_Panel {
public:
  DGUI_Panel() {
    m_ChildCount = 0;
    m_ChildCapacity = 0;
    m_Children = nullptr;
  }

  virtual ~DGUI_Panel();

  virtual void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   unsigned int x, unsigned int y);

  void add_child(DGUI_Panel *pPanel);
  void clear_children();
  void set_position(unsigned int x, unsigned int y);
  void set_size(unsigned int w, unsigned int h);

  unsigned int get_position_x() { return m_Position[0]; }
  unsigned int get_position_y() { return m_Position[1]; }

  uint32_t get_child_count() { return m_ChildCount; }
  DGUI_Panel *get_child(uint32_t index) {
    ASSERT(index < m_ChildCount);
    return m_Children[index];
  }

protected:
  unsigned int m_Position[2];
  unsigned int m_Dimensions[2];

private:
  DGUI_Panel **m_Children;
  uint32_t m_ChildCapacity;
  uint32_t m_ChildCount;
};