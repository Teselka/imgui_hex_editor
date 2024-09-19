#pragma once
#include <imgui.h>

enum ImGuiHexEditorHighlightFlags_ : int
{
	ImGuiHexEditorHighlightFlags_None = 0,
	ImGuiHexEditorHighlightFlags_Apply = 1 << 0,
	ImGuiHexEditorHighlightFlags_TextAutomaticContrast = 1 << 1,
	ImGuiHexEditorHighlightFlags_FullSized = 1 << 2, // Highlight entire byte space including it's container, has no effect on ascii
	ImGuiHexEditorHighlightFlags_Ascii = 1 << 3, // Highlight ascii (doesn't affect single byte highlighting)
	ImGuiHexEditorHighlightFlags_Border = 1 << 4,
	ImGuiHexEditorHighlightFlags_OverrideBorderColor = 1 << 5,
	ImGuiHexEditorHighlightFlags_BorderAutomaticContrast = 1 << 6,
};

typedef int ImGuiHexEditorHighlightFlags; // -> enum ImGuiHexEditorHighlightFlags_ // Flags: for ImGuiHexEditor callbacks

struct ImGuiHexEditorHighlightRange
{
	int From;
	int To;
	ImColor Color;
	ImColor BorderColor;
	ImGuiHexEditorHighlightFlags Flags;
};

enum ImGuiHexEditorClipboardFlags_ : int
{
	ImGuiHexEditorClipboardFlags_None = 0,
	ImGuiHexEditorClipboardFlags_Multiline = 1 << 0, // Separate resulting hex editor lines with carriage return
};

typedef int ImGuiHexEditorClipboardFlags; // -> enum ImGuiHexEditorClipboardFlags_

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
	ImVector<ImGuiHexEditorHighlightRange> HighlightRanges;
	bool EnableClipboard = true;
	ImGuiHexEditorClipboardFlags ClipboardFlags = ImGuiHexEditorClipboardFlags_Multiline;

	int(*ReadCallback)(ImGuiHexEditorState* state, int offset, void* buf, int size) = nullptr;
	int(*WriteCallback)(ImGuiHexEditorState* state, int offset, void* buf, int size) = nullptr;
	bool(*GetAddressNameCallback)(ImGuiHexEditorState* state, int offset, char* buf, int size) = nullptr;
	ImGuiHexEditorHighlightFlags(*SingleHighlightCallback)(ImGuiHexEditorState* state, int offset, ImColor* color, ImColor* text_color, ImColor* border_color) = nullptr;
	void(*HighlightRangesCallback)(ImGuiHexEditorState* state, int display_start, int display_end) = nullptr;

	int SelectStartByte = -1;
	int SelectStartSubByte = 0;
	int SelectEndByte = -1;
	int SelectEndSubByte = 0;
	int LastSelectedByte = -1;
	int SelectDragByte = -1;
	int SelectDragSubByte = 0;
	float SelectCursorAnimationTime = 0.f;

	ImGuiHexEditorHighlightFlags SelectionHighlightFlags = ImGuiHexEditorHighlightFlags_FullSized | ImGuiHexEditorHighlightFlags_Ascii;
};

namespace ImGui
{
	bool BeginHexEditor(const char* str_id, ImGuiHexEditorState* state, const ImVec2& size = { 0.f, 0.f }, ImGuiChildFlags child_flags = 0, ImGuiWindowFlags window_flags = 0);
	void EndHexEditor();

	// Helpers

	bool CalcHexEditorRowRange(int row_offset, int row_bytes_count, int range_min, int range_max, int* out_min, int* out_max);
}