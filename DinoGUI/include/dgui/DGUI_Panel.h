#pragma once

typedef uint8_t DGUI_PanelFlags_t;

enum DGUI_PanelFlags : DGUI_PanelFlags_t {
  DGUI_PANEL_FLAGS_NONE = 0,

  /// <summary>
  /// Dimensions start at 100% of parent, then this number is subtracted.
  /// </summary>
  DGUI_PANEL_FLAG_SUBTRACTIVE_SIZE_X = 1 << 0,
  DGUI_PANEL_FLAG_SUBTRACTIVE_SIZE_Y = 1 << 1,
};

class DGUI_API DGUI_Panel {
public:
  /// <summary>
  /// Position offset from the anchor point. X is right and Y is up in pixels
  /// based on 1920x1080; Z is depth (0-1).
  /// </summary>
  float Position[3];

  /// <summary>
  /// Width and height in pixels based on 1920x1080.
  /// </summary>
  float Dimensions[2];

  /// <summary>
  /// Anchor point to start from based on parent dimensions. Normal range is
  /// 0-1.
  /// </summary>
  float Anchor[2];

  DGUI_PanelFlags_t Flags;

  virtual ~DGUI_Panel();

  virtual void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   float x, float y);

  /// <summary>
  /// Pivot is used when positioning the object. 0-1.
  /// </summary>
  void set_position_dimensions(float x, float y, float w, float h, float pivotX,
                               float pivotY);
  void add_child(DGUI_Panel *pPanel);
  void delete_children();

  float calc_x(float baseX);
  float calc_y(float baseY);

  float calc_w();
  float calc_h();

  mat4_t get_matrix(float baseX, float baseY);

  uint16_t get_child_count() { return m_ChildCount; }
  DGUI_Panel *get_child(uint16_t index) {
    ASSERT(index < m_ChildCount);
    return m_Children[index];
  }

protected:
  DGUI_Panel *m_Parent;

private:
  DGUI_Panel **m_Children;
  uint16_t m_ChildCapacity;
  uint16_t m_ChildCount;
};