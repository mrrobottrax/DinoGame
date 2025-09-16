#pragma once

class DGUI_Panel;

DGUI_API void dgui_init(ID3D12Device9 *pDevice);
DGUI_API void dgui_stop();
DGUI_API DGUI_Panel *dgui_get_top_panel();
DGUI_API void dgui_clear_all();
DGUI_API void
dgui_add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                         unsigned int w, unsigned int h);
