#pragma once

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

	size_t(*ReadCallback)(ImGuiHexEditorState* state, size_t offset, void* buf, size_t size) = nullptr;
	size_t(*WriteCallback)(ImGuiHexEditorState* state, size_t offset, void* buf, size_t size) = nullptr;
	bool(*GetAddressNameCallback)(ImGuiHexEditorState* state, size_t offset, char* buf, size_t size) = nullptr;

	// [Internal]
	size_t SelectStartByte = -1;
	int SelectStartSubByte = 0;
	size_t SelectEndByte = -1;
	int SelectEndSubByte = 0;
	size_t LastSelectedByte = -1;
};

namespace ImGui
{
	bool BeginHexEditor(const char* str_id, ImGuiHexEditorState* state);
	void EndHexEditor();
}