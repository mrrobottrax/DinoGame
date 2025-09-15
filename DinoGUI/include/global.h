#pragma once

class DGUI_Panel;

struct ID3D12GraphicsCommandList;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

DGUI_API void dgui_init();
DGUI_API void dgui_stop();
DGUI_API DGUI_Panel *dgui_get_top_panel();
DGUI_API void dgui_clear_all();
DGUI_API void dgui_add_render_commands(ID3D12GraphicsCommandList10 * pCommandList);