#pragma once

enum DGUI_PanelFlags : char {
  DGUI_PANEL_FLAGS_NONE = 0,

  /// <summary>
  /// Dimensions start at 100% of parent, then this number is subtracted.
  /// </summary>
  DGUI_PANEL_FLAG_SUBTRACTIVE_SIZE_X = 1 << 0,
  DGUI_PANEL_FLAG_SUBTRACTIVE_SIZE_Y = 1 << 1,
};

class DINO_API DGUI_Panel {
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

  DGUI_PanelFlags Flags;

  virtual ~DGUI_Panel();

  virtual void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   float x, float y, float w, float h);

  /// <summary>
  /// Pivot is used when positioning the object. 0-1.
  /// </summary>
  void set_position_dimensions(float x, float y, float w, float h, float pivotX,
                               float pivotY);
  void add_child(DGUI_Panel *pPanel);
  void delete_children();

  float calc_x();
  float calc_y();

  float calc_w();
  float calc_h();

  uint16_t get_child_count() const;
  DGUI_Panel *get_child(uint16_t index);

protected:
  DGUI_Panel *m_Parent;

private:
  DGUI_Panel **m_Children;
  uint16_t m_ChildCapacity;
  uint16_t m_ChildCount;
};

inline uint16_t DGUI_Panel::get_child_count() const { return m_ChildCount; }

inline DGUI_Panel *DGUI_Panel::get_child(uint16_t index) {
  ASSERT(index < m_ChildCount);
  return m_Children[index];
}

inline void DGUI_Panel::set_position_dimensions(float x, float y, float w,
                                                float h, float pivotX,
                                                float pivotY) {
  Dimensions[0] = w;
  Dimensions[1] = h;

  Position[0] = x - pivotX * w;
  Position[1] = y - pivotY * h;
}

inline float DGUI_Panel::calc_x() {
  const float anchorX = Anchor[0] * m_Parent->calc_w();
  return Position[0] + anchorX;
}

inline float DGUI_Panel::calc_y() {
  const float anchorY = Anchor[1] * m_Parent->calc_h();
  return Position[1] + anchorY;
}

inline float DGUI_Panel::calc_w() {
  if (Flags & DGUI_PANEL_FLAG_SUBTRACTIVE_SIZE_X) {
    return m_Parent->Dimensions[0] - Dimensions[0];
  }
  return Dimensions[0];
}

inline float DGUI_Panel::calc_h() {
  if (Flags & DGUI_PANEL_FLAG_SUBTRACTIVE_SIZE_Y) {
    return m_Parent->Dimensions[1] - Dimensions[1];
  }
  return Dimensions[1];
}