#pragma once

inline unsigned int g_ScreenDimensions[2];

/// <summary>
/// Height over width ratio
/// </summary>
inline float g_ScreenRatio = 1;
inline float g_InvScreenRatio = 1;

constexpr size_t DGUI_PIXEL_BASIS = 1080;
constexpr float DGUI_PIXEL_SCALE = 1.0f / DGUI_PIXEL_BASIS;
constexpr float DGUI_2PIXEL_SCALE = 2 * DGUI_PIXEL_SCALE;