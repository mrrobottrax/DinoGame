#pragma once

typedef uint16_t UI_PanelFlags_t;

enum UI_PanelFlags : UI_PanelFlags_t {
  UI_PANEL_FLAG_NONE = 0,

  /// <summary>
  /// Dimensions start at 100% of parent, then this number is subtracted.
  /// </summary>
  UI_PANEL_FLAG_SUBTRACTIVE_SIZE_W = 1 << 0,
  UI_PANEL_FLAG_SUBTRACTIVE_SIZE_H = 1 << 1,

  UI_PANEL_FLAG_SUBTRACTIVE_SIZE_WH =
      UI_PANEL_FLAG_SUBTRACTIVE_SIZE_W | UI_PANEL_FLAG_SUBTRACTIVE_SIZE_H,

  /// <summary>
  /// Dimensions are not scaled with window size.
  /// </summary>
  UI_PANEL_FLAG_ABSOLUTE_SIZE_W = 1 << 2,
  UI_PANEL_FLAG_ABSOLUTE_SIZE_H = 1 << 3,

  UI_PANEL_FLAG_ABSOLUTE_SIZE_WH =
      UI_PANEL_FLAG_ABSOLUTE_SIZE_W | UI_PANEL_FLAG_ABSOLUTE_SIZE_H,

  /// <summary>
  /// Position is not scaled with window size.
  /// </summary>
  UI_PANEL_FLAG_ABSOLUTE_POSITION_X = 1 << 4,
  UI_PANEL_FLAG_ABSOLUTE_POSITION_Y = 1 << 5,

  UI_PANEL_FLAG_ABSOLUTE_POSITION_XY =
      UI_PANEL_FLAG_ABSOLUTE_POSITION_X | UI_PANEL_FLAG_ABSOLUTE_POSITION_Y,

  /// <summary>
  /// Dimensions are a fraction of the parent size.
  /// </summary>
  UI_PANEL_FLAG_RELATIVE_SIZE_W = 1 << 6,
  UI_PANEL_FLAG_RELATIVE_SIZE_H = 1 << 7,

  UI_PANEL_FLAG_RELATIVE_SIZE_WH =
      UI_PANEL_FLAG_RELATIVE_SIZE_W | UI_PANEL_FLAG_RELATIVE_SIZE_H,

  /// <summary>
  /// Position is a fraction of the parent size.
  /// </summary>
  UI_PANEL_FLAG_RELATIVE_POSITION_X = 1 << 8,
  UI_PANEL_FLAG_RELATIVE_POSITION_Y = 1 << 9,

  UI_PANEL_FLAG_RELATIVE_POSITION_XY =
      UI_PANEL_FLAG_RELATIVE_POSITION_X | UI_PANEL_FLAG_RELATIVE_POSITION_Y,

  UI_PANEL_FLAG_ABSOLUTE =
      UI_PANEL_FLAG_ABSOLUTE_SIZE_WH | UI_PANEL_FLAG_ABSOLUTE_POSITION_XY,

  UI_PANEL_FLAG_RELATIVE =
      UI_PANEL_FLAG_RELATIVE_SIZE_WH | UI_PANEL_FLAG_RELATIVE_POSITION_XY,
};

class DINO_API UI_Panel {
public:
  /// <summary>
  /// Position offset from the anchor point. X is right and Y is up in pixels
  /// based on 1920x1080.
  /// </summary>
  float Position[2]{};

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
                                   float px, float py, float pw, float ph);
  virtual void position_children(float w, float h);

  void set_position(float x, float y);
  void set_dimensions(float w, float h);
  void set_anchor(float x, float y);
  void set_pivot(float x, float y);

  void add_child(UI_Panel *pPanel);
  void delete_children();

  uint16_t get_child_count() const;
  UI_Panel *get_child(uint16_t index);

private:
  UI_Panel *m_Parent{};

  UI_Panel **m_Children{};
  uint16_t m_ChildCapacity{};
  uint16_t m_ChildCount{};
};

inline uint16_t UI_Panel::get_child_count() const { return m_ChildCount; }

inline UI_Panel *UI_Panel::get_child(uint16_t index) {
  ASSERT(index < m_ChildCount);
  return m_Children[index];
}

inline void UI_Panel::set_position(float x, float y) {
  Position[0] = x;
  Position[1] = y;
}

inline void UI_Panel::set_dimensions(float w, float h) {
  Dimensions[0] = w;
  Dimensions[1] = h;
}

inline void UI_Panel::set_anchor(float x, float y) {
  Anchor[0] = x;
  Anchor[1] = y;
}

inline void UI_Panel::set_pivot(float x, float y) {
  Pivot[0] = x;
  Pivot[1] = y;
}
