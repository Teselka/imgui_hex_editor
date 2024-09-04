#pragma once
#include <imgui.h>

enum ImGuiHexEditorHighlightFlags_ : int
{
	ImGuiHexEditorHighlightFlags_None = 0,
	ImGuiHexEditorHighlightFlags_Apply = 1 << 0,
	ImGuiHexEditorHighlightFlags_TextAutomaticContrast = 1 << 1,
	ImGuiHexEditorHighlightFlags_FullSized = 1 << 2, // Highlight entire byte space including it's container, has no effect on ascii
	ImGuiHexEditorHighlightFlags_Ascii = 1 << 3 // Highlight ascii (doesn't affect single byte highlighting)
};

typedef int ImGuiHexEditorHighlightFlags; // -> enum ImGuiHexEditorHighlightFlags_ // Flags: for ImGuiHexEditor callbacks

struct ImGuiHexEditorState
{
	void* Bytes;
	int MaxBytes;
	int BytesPerLine = -1;
	bool ShowPrintable = false;
	bool LowercaseBytes = false;
	bool RenderZeroesDisabled = true;
	bool ShowAddress = true;
	int AddressChars = -1;
	bool ShowAscii = true;
	bool ReadOnly = false;
	int Separators = 8;
	void* UserData = nullptr;

	int(*ReadCallback)(ImGuiHexEditorState* state, int offset, void* buf, int size) = nullptr;
	int(*WriteCallback)(ImGuiHexEditorState* state, int offset, void* buf, int size) = nullptr;
	bool(*GetAddressNameCallback)(ImGuiHexEditorState* state, int offset, char* buf, int size) = nullptr;
	ImGuiHexEditorHighlightFlags(*SingleHighlightCallback)(ImGuiHexEditorState* state, int offset, ImColor* color, ImColor* text_color) = nullptr;
	ImGuiHexEditorHighlightFlags(*MultipleHighlightCallback)(ImGuiHexEditorState* state, int row_offset, int row_bytes_count, int* highlight_min, int* highlight_max, ImColor* color, ImColor* text_color) = nullptr;

	// [Internal]
	int SelectStartByte = -1;
	int SelectStartSubByte = 0;
	int SelectEndByte = -1;
	int SelectEndSubByte = 0;
	int LastSelectedByte = -1;
};

namespace ImGui
{
	bool BeginHexEditor(const char* str_id, ImGuiHexEditorState* state);
	void EndHexEditor();

	// Helpers
	bool CalcHexEditorRowRange(int row_offset, int row_bytes_count, int range_min, int range_max, int* out_min, int* out_max);
}