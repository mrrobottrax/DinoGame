#pragma once

typedef char UI_PanelFlags_t;

enum UI_PanelFlags : UI_PanelFlags_t {
  UI_PANEL_FLAGS_NONE = 0,

  /// <summary>
  /// Dimensions start at 100% of parent, then this number is subtracted.
  /// </summary>
  UI_PANEL_FLAG_SUBTRACTIVE_SIZE_X = 1 << 0,
  UI_PANEL_FLAG_SUBTRACTIVE_SIZE_Y = 1 << 1,
};

class DINO_API UI_Panel {
public:
  /// <summary>
  /// Position offset from the anchor point. X is right and Y is up in pixels
  /// based on 1920x1080; Z is depth (0-1).
  /// </summary>
  float Position[3]{};

  /// <summary>
  /// Width and height in pixels based on 1920x1080.
  /// </summary>
  float Dimensions[2]{};

  /// <summary>
  /// Anchor point to start from based on parent dimensions. Normal range is
  /// 0-1.
  /// </summary>
  float Anchor[2]{};

  /// <summary>
  /// Pivot point based on internal dimensions. Normal range is
  /// 0-1.
  /// </summary>
  float Pivot[2]{};

  UI_PanelFlags_t Flags{};

  UI_Panel() = default;
  UI_Panel(const UI_Panel &) = delete;
  virtual ~UI_Panel();

  virtual void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   float x, float y, float w, float h);

  /// <summary>
  /// Pivot is used when positioning the object. 0-1.
  /// </summary>
  void set_position_dimensions(float x, float y, float w, float h, float pivotX,
                               float pivotY);
  void add_child(UI_Panel *pPanel);
  void delete_children();

  float calc_x(float parentW) const;
  float calc_y(float parentH) const;

  float calc_w(float parentW) const;
  float calc_h(float parentH) const;

  uint16_t get_child_count() const;
  UI_Panel *get_child(uint16_t index);

protected:
  UI_Panel *m_Parent{};

private:
  UI_Panel **m_Children{};
  uint16_t m_ChildCapacity{};
  uint16_t m_ChildCount{};
};

inline uint16_t UI_Panel::get_child_count() const { return m_ChildCount; }

inline UI_Panel *UI_Panel::get_child(uint16_t index) {
  ASSERT(index < m_ChildCount);
  return m_Children[index];
}

inline void UI_Panel::set_position_dimensions(float x, float y, float w,
                                              float h, float pivotX,
                                              float pivotY) {
  Dimensions[0] = w;
  Dimensions[1] = h;

  Position[0] = x - pivotX * w;
  Position[1] = y - pivotY * h;
}

inline float UI_Panel::calc_x(float parentW) const {
  const float anchorX = Anchor[0] * parentW;
  return Position[0] + anchorX;
}

inline float UI_Panel::calc_y(float parentH) const {
  const float anchorY = Anchor[1] * parentH;
  return Position[1] + anchorY;
}

inline float UI_Panel::calc_w(float parentW) const {
  if (Flags & UI_PANEL_FLAG_SUBTRACTIVE_SIZE_X) {
    return parentW - Dimensions[0];
  }
  return Dimensions[0];
}

inline float UI_Panel::calc_h(float parentH) const {
  if (Flags & UI_PANEL_FLAG_SUBTRACTIVE_SIZE_Y) {
    return parentH - Dimensions[1];
  }
  return Dimensions[1];
}