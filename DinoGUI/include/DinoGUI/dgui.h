#pragma once

class DGUI_Panel;

DGUI_API void DGUI_init(ID3D12Device9 *pDevice);
DGUI_API void DGUI_stop();
DGUI_API DGUI_Panel *DGUI_get_top_panel();
DGUI_API void DGUI_clear_all();
