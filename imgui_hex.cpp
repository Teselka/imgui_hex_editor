#include "imgui_hex.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <ctype.h>

static char HalfByteToPrintable(unsigned char half_byte, bool lower)
{
	IM_ASSERT(!(half_byte & 0xf0));
	return half_byte <= 9 ? '0' + half_byte : (lower ? 'a' : 'A') + half_byte - 10;
}

static unsigned char KeyToHalfByte(ImGuiKey key)
{
	IM_ASSERT((key >= ImGuiKey_A && key <= ImGuiKey_F) || (key >= ImGuiKey_0 && key <= ImGuiKey_9));
	return (key >= ImGuiKey_A && key <= ImGuiKey_F) ? (char)(key - ImGuiKey_A) + 10 : (char)(key - ImGuiKey_0);
}

static bool HasAsciiRepresentation(unsigned char byte)
{
	return (byte >= '!' && byte <= '~');
}

static int CalcBytesPerLine(float bytes_avail_x, const ImVec2& byte_size, const ImVec2& spacing, bool show_ascii, const ImVec2& char_size, int separators)
{
	const float byte_width = byte_size.x + spacing.x + (show_ascii ? char_size.x : 0.f);
	int bytes_per_line = (int)(bytes_avail_x / byte_width);
	bytes_per_line = bytes_per_line <= 0 ? 1 : bytes_per_line;

	int actual_separators = separators > 0 ? (int)(bytes_per_line / separators) : 0;
	if (actual_separators != 0 && separators > 0 && bytes_per_line > actual_separators && (bytes_per_line - 1) % actual_separators == 0)
		--actual_separators;
	
	return separators > 0 ? CalcBytesPerLine(bytes_avail_x - (actual_separators * spacing.x), byte_size, spacing, show_ascii, char_size, 0) : bytes_per_line;
}

static ImColor CalcContrastColor(ImColor color)
{
#ifdef IMGUI_USE_BGRA_PACKED_COLOR
	const float l = (0.299f * (color.Value.z * 255.f) + 0.587f * (color.Value.y * 255.f) + 0.114f * (color.Value.x * 255.f)) / 255.f;
#else
	const float l = (0.299f * (color.Value.x * 255.f) + 0.587f * (color.Value.y * 255.f) + 0.114f * (color.Value.z * 255.f)) / 255.f;
#endif
	const int c = l > 0.5f ? 0 : 255;
	return IM_COL32(c, c, c, 255);
}

bool ImGui::BeginHexEditor(const char* str_id, ImGuiHexEditorState* state, const ImVec2& size, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags)
{
	if (!ImGui::BeginChild(str_id, size, child_flags, window_flags))
		return false;

	const ImVec2 char_size = ImGui::CalcTextSize("0");
	const ImVec2 byte_size = { char_size.x * 2.f, char_size.y };

	const ImGuiStyle& style = ImGui::GetStyle();
	const ImVec2 spacing = style.ItemSpacing;

	ImVec2 content_avail = ImGui::GetContentRegionAvail();

	float address_max_size;
	int address_max_chars;
	if (state->ShowAddress)
	{
		int address_chars = state->AddressChars;
		if (address_chars == -1)
			address_chars = ImFormatString(nullptr, 0, "%zX", (size_t)state->MaxBytes) + 1;

		address_max_chars = address_chars + 1;
		address_max_size = char_size.x * address_max_chars + spacing.x * 0.5f;
	}
	else
	{
		address_max_size = 0.f;
		address_max_chars = 0;
	}

	float bytes_avail_x = content_avail.x - address_max_size;
	if (ImGui::GetScrollMaxY() > 0.f)
		bytes_avail_x -= style.ScrollbarSize;

	bytes_avail_x = bytes_avail_x < 0.f ? 0.f : bytes_avail_x;

	const bool show_ascii = state->ShowAscii;

	int bytes_per_line;

	if (state->BytesPerLine == -1)
	{
		bytes_per_line = CalcBytesPerLine(bytes_avail_x, byte_size, spacing, show_ascii, char_size, state->Separators);
	}
	else
	{
		bytes_per_line = state->BytesPerLine;
	}

	int actual_separators = (int)(bytes_per_line / state->Separators);
	if (bytes_per_line % state->Separators == 0)
		--actual_separators;
	
	int lines_count;
	if (bytes_per_line != 0)
	{
		lines_count = state->MaxBytes / bytes_per_line;
		if (lines_count * bytes_per_line < state->MaxBytes)
		{
			++lines_count;
		}
	}
	else
		lines_count = 0;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImColor text_color = ImGui::GetColorU32(ImGuiCol_Text);
	const ImColor text_disabled_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
	const ImColor text_selected_bg_color = ImGui::GetColorU32(ImGuiCol_TextSelectedBg);
	const ImColor separator_color = ImGui::GetColorU32(ImGuiCol_Separator);

	const bool lowercase_bytes = state->LowercaseBytes;

	const int select_start_byte = state->SelectStartByte;
	const int select_start_subbyte = state->SelectStartSubByte;
	const int select_end_byte = state->SelectEndByte;
	const int select_end_subbyte = state->SelectEndSubByte;
	const int last_selected_byte = state->LastSelectedByte;

	int next_select_start_byte = select_start_byte;
	int next_select_start_subbyte = select_start_subbyte;
	int next_select_end_byte = select_end_byte;
	int next_select_end_subbyte = select_end_subbyte;
	int next_last_selected_byte = last_selected_byte;

	if (last_selected_byte != -1)
	{
		bool any_pressed = false;
		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
		{
			if (!select_start_subbyte)
			{
				if (last_selected_byte == 0)
				{
					next_last_selected_byte = 0;
				}
				else
				{
					next_last_selected_byte = last_selected_byte - 1;
					next_select_start_subbyte = 1;
				}
			}
			else
				next_select_start_subbyte = 0;

			any_pressed = true;
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
		{
			if (select_start_subbyte)
			{
				if (last_selected_byte >= state->MaxBytes - 1)
				{
					next_last_selected_byte = state->MaxBytes - 1;
				}
				else
				{
					next_last_selected_byte = last_selected_byte + 1;
					next_select_start_subbyte = 0;
				}
			}
			else
				next_select_start_subbyte = 1;

			any_pressed = true;
		}
		else if (bytes_per_line != 0)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			{
				if (last_selected_byte >= bytes_per_line)
				{
					next_last_selected_byte = last_selected_byte - bytes_per_line;
				}

				any_pressed = true;
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			{
				if (last_selected_byte < state->MaxBytes - bytes_per_line)
				{
					next_last_selected_byte = last_selected_byte + bytes_per_line;
				}

				any_pressed = true;
			}
		}

		if (any_pressed)
		{
			next_select_start_byte = next_last_selected_byte;
			next_select_end_byte = next_last_selected_byte;
		}
	}

	ImGuiKey hex_key_pressed = ImGuiKey_None;

	for (ImGuiKey key = ImGuiKey_A; key != ImGuiKey_G; key = (ImGuiKey)((int)key + 1))
	{
		if (ImGui::IsKeyPressed(key))
		{
			hex_key_pressed = key;
			break;
		}
	}

	if (hex_key_pressed == ImGuiKey_None)
	{
		for (ImGuiKey key = ImGuiKey_0; key != ImGuiKey_A; key = (ImGuiKey)((int)key + 1))
		{
			if (ImGui::IsKeyPressed(key))
			{
				hex_key_pressed = key;
				break;
			}
		}
	}

	unsigned char stack_line_buf[128];
	unsigned char* line_buf = bytes_per_line <= sizeof(stack_line_buf) ? stack_line_buf : (unsigned char*)ImGui::MemAlloc(bytes_per_line);
	if (!line_buf)
		return true;

	char stack_address_buf[32];
	char* address_buf = address_max_chars <= sizeof(stack_address_buf) ? stack_address_buf : (char*)ImGui::MemAlloc(address_max_chars);
	if (!address_buf)
	{
		if (line_buf != stack_line_buf)
			ImGui::MemFree(line_buf);

		return true;
	}

	memset(line_buf, 0, bytes_per_line);

	const ImVec2 mouse_pos = ImGui::GetMousePos();

	ImGuiListClipper clipper;
	clipper.Begin(lines_count, byte_size.y + spacing.y);
	while (clipper.Step())
	{
		ImVec2 cursor = ImGui::GetCursorScreenPos();

		if (show_ascii)
		{
			ImVec2 ascii_cursor = { cursor.x + address_max_size + (bytes_per_line * (byte_size.x + spacing.x)) + (actual_separators * spacing.x), cursor.y };
			draw_list->AddLine(ascii_cursor, { ascii_cursor.x, ascii_cursor.y + (clipper.DisplayEnd - clipper.DisplayStart) * (byte_size.y + spacing.y) }, separator_color);
		}

		for (int n = clipper.DisplayStart; n != clipper.DisplayEnd; n++)
		{
			const int line_base = n * bytes_per_line;
			if (state->ShowAddress)
			{
				if (!state->GetAddressNameCallback || !state->GetAddressNameCallback(state, line_base, address_buf, address_max_chars))
					ImFormatString(address_buf, (size_t)address_max_chars, "%0.*zX", address_max_chars - 1, (size_t)line_base);

				const ImVec2 text_size = ImGui::CalcTextSize(address_buf);
				draw_list->AddText(cursor, text_color, address_buf);
				draw_list->AddText({ cursor.x + text_size.x, cursor.y }, text_disabled_color, ":");
				cursor.x += address_max_size;
			}

			int max_bytes_per_line = line_base;
			max_bytes_per_line = max_bytes_per_line > state->MaxBytes ? max_bytes_per_line - state->MaxBytes : bytes_per_line;

			int bytes_read;
			if (!state->ReadCallback)
			{
				memcpy(line_buf, (char*)state->Bytes + line_base, max_bytes_per_line);
				bytes_read = max_bytes_per_line;
			}
			else
				bytes_read = state->ReadCallback(state, line_base, line_buf, max_bytes_per_line);

			int row_highlight_min = -1;
			int row_highlight_max = -1;
			ImColor row_highlight_color;
			ImColor row_text_color;
			bool row_highlight_full_sized;
			bool row_highlight_ascii;

			if (state->MultipleHighlightCallback)
			{
				ImGuiHexEditorHighlightFlags flags = state->MultipleHighlightCallback(state, line_base, bytes_per_line, &row_highlight_min, &row_highlight_max, &row_highlight_color, &row_text_color);
				if (flags & ImGuiHexEditorHighlightFlags_Apply)
				{
					row_highlight_full_sized = flags & ImGuiHexEditorHighlightFlags_FullSized;
					row_highlight_ascii = flags & ImGuiHexEditorHighlightFlags_Ascii;

					if (flags & ImGuiHexEditorHighlightFlags_TextAutomaticContrast)
						row_text_color = CalcContrastColor(row_highlight_color);
				}
			}

			for (int i = 0; i != bytes_per_line; i++)
			{
				const ImRect byte_bb = { { cursor.x, cursor.y }, { cursor.x + byte_size.x, cursor.y + byte_size.y } };

				ImRect item_bb = byte_bb;
				if (i != 0)
					item_bb.Min.x -= spacing.x * 0.5f;

				if (n != clipper.DisplayStart)
					item_bb.Min.y -= spacing.y * 0.5f;

				item_bb.Max.x += spacing.x * 0.5f;
				item_bb.Max.y += spacing.y * 0.5f;

				const int offset = bytes_per_line * n + i;
				unsigned char byte;

				char text[3];
				if (offset < state->MaxBytes && i < bytes_read)
				{
					byte = line_buf[i];

					text[0] = HalfByteToPrintable((byte & 0xf0) >> 4, lowercase_bytes);
					text[1] = HalfByteToPrintable(byte & 0x0f, lowercase_bytes);
					text[2] = '\0';
				}
				else
				{
					byte = 0x00;

					text[0] = '?';
					text[1] = '?';
					text[2] = '\0';
				}

				const ImGuiID id = ImGui::GetID(offset);
				if (!ImGui::ItemAdd(item_bb, id, 0, ImGuiItemFlags_Inputable))
					continue;

				ImColor highlight_color;
				ImColor byte_text_color = (offset >= state->MaxBytes || (state->RenderZeroesDisabled && byte == 0x00) || i >= bytes_read) ? text_disabled_color : text_color;

				if (offset >= select_start_byte && offset <= select_end_byte)
				{
					if ((offset > select_start_byte && offset < select_end_byte)
						|| (select_start_byte != select_end_byte && offset == select_start_byte && !select_start_subbyte)
						|| (select_start_byte != select_end_byte && offset == select_end_byte && select_end_subbyte))
					{
						draw_list->AddRectFilled(byte_bb.Min, byte_bb.Max, text_selected_bg_color);
					}
					else
					{
						const int subbyte = offset == select_start_byte ? select_start_subbyte : select_end_subbyte;

						ImVec2 min = byte_bb.Min;

						if (subbyte)
							min.x += byte_size.x * 0.5f;

						const ImVec2 max = { min.x + byte_size.x * 0.5f, byte_bb.Max.y };
						draw_list->AddRectFilled(min, max, text_selected_bg_color);
					}
				}
				else
				{
					bool single_highlight = false;

					if (state->SingleHighlightCallback)
					{
						ImGuiHexEditorHighlightFlags flags = state->SingleHighlightCallback(state, offset, &highlight_color, &byte_text_color);
						if (flags & ImGuiHexEditorHighlightFlags_Apply)
						{
							single_highlight = true;

							if (flags & ImGuiHexEditorHighlightFlags_FullSized)
								draw_list->AddRectFilled(item_bb.Min, item_bb.Max, highlight_color);
							else
								draw_list->AddRectFilled(byte_bb.Min, byte_bb.Max, highlight_color);

							if (flags & ImGuiHexEditorHighlightFlags_TextAutomaticContrast)
								byte_text_color = CalcContrastColor(highlight_color);
						}
					}

					if (!single_highlight && row_highlight_min != -1 && i >= row_highlight_min && i < row_highlight_max)
					{
						if (row_highlight_full_sized)
							draw_list->AddRectFilled(item_bb.Min, item_bb.Max, row_highlight_color);
						else
							draw_list->AddRectFilled(byte_bb.Min, byte_bb.Max, row_highlight_color);

						byte_text_color = row_text_color;
					}
				}

				draw_list->AddText(byte_bb.Min, byte_text_color, text);

				const bool hovered = ItemHoverable(item_bb, id, ImGuiItemFlags_Inputable);

				if (hovered && !(offset >= state->MaxBytes))
				{
					ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);

					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					{
						next_select_start_byte = offset;
						next_select_end_byte = offset;
						next_select_start_subbyte = mouse_pos.x > item_bb.GetCenter().x;
						next_last_selected_byte = offset;
						ImGui::SetKeyboardFocusHere();
					}
					else if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && select_start_byte != -1)
					{
						if (offset < next_select_start_byte)
						{
							const int prev_byte = next_select_end_byte > next_select_start_byte ? next_select_end_byte : next_select_start_byte;
							const int prev_subbyte = next_select_end_byte > next_select_start_byte ? next_select_end_subbyte : next_select_start_subbyte;

							next_select_start_byte = offset;
							next_select_start_subbyte = mouse_pos.x > item_bb.GetCenter().x;

							next_select_end_byte = prev_byte;
							next_select_end_subbyte = prev_subbyte;

							next_last_selected_byte = offset;
						}
						else if (offset != select_start_byte)
						{
							next_select_end_byte = offset;
							next_select_end_subbyte = mouse_pos.x > item_bb.GetCenter().x;
						}

						ImGui::SetKeyboardFocusHere();
					}
				}

				if (offset == next_last_selected_byte && last_selected_byte != next_last_selected_byte)
				{
					ImGui::SetKeyboardFocusHere();
				}

				if (offset == last_selected_byte && !state->ReadOnly && hex_key_pressed != ImGuiKey_None)
				{
					IM_ASSERT(offset == select_start_byte || offset == select_end_byte);
					const int subbyte = offset == select_start_byte ? select_start_subbyte : select_end_subbyte;

					unsigned char wbyte;
					if (subbyte)
						wbyte = (byte & 0xf0) | KeyToHalfByte(hex_key_pressed);
					else
						wbyte = (KeyToHalfByte(hex_key_pressed) << 4) | (byte & 0x0f);

					if (!state->WriteCallback)
						*(unsigned char*)((char*)state->Bytes + n * bytes_per_line + i) = wbyte;
					else
						state->WriteCallback(state, n * bytes_per_line + i, &wbyte, sizeof(wbyte));

					int* next_subbyte = (int*)(offset == select_start_byte ? &next_select_start_subbyte : &next_select_end_subbyte);
					if (!subbyte)
					{
						next_select_start_byte = offset;
						next_select_end_byte = offset;
						*next_subbyte = 1;
					}
					else
					{
						next_last_selected_byte = offset + 1;
						if (next_last_selected_byte >= state->MaxBytes - 1)
							next_last_selected_byte = state->MaxBytes - 1;
						else
							*next_subbyte = 0;

						next_select_start_byte = next_last_selected_byte;
						next_select_end_byte = next_last_selected_byte;
					}
				}

				cursor.x += byte_size.x + spacing.x;
				if (i > 0 && state->Separators > 0 && (i + 1) % state->Separators == 0
					&& i != bytes_per_line - 1)
					cursor.x += spacing.x;

				ImGui::SetCursorScreenPos(cursor);
			}

			if (show_ascii)
			{
				cursor.x += spacing.x;

				for (int i = 0; i != bytes_per_line; i++)
				{
					const int offset = (int)bytes_per_line * (int)n + (int)i;

					unsigned char byte;
					if (offset < state->MaxBytes)
						byte = line_buf[i];
					else
						byte = 0x00;

					bool has_ascii = HasAsciiRepresentation(byte);

					const ImRect char_bb = { cursor,  { cursor.x + char_size.x, cursor.y + char_size.y } };
					ImColor char_color = (offset >= state->MaxBytes || !has_ascii) ? text_disabled_color : text_color;

					if (offset >= select_start_byte && offset <= select_end_byte)
					{
						draw_list->AddRectFilled(char_bb.Min, char_bb.Max, text_selected_bg_color);
					}
					else
					{
						if (row_highlight_min != -1 && i >= row_highlight_min && i < row_highlight_max && row_highlight_ascii)
						{
							draw_list->AddRectFilled(char_bb.Min, char_bb.Max, row_highlight_color);
							char_color = row_text_color;
						}
					}
						
					char text[2];
					text[0] = has_ascii ? *(char*)&byte : '.';
					text[1] = '\0';

					draw_list->AddText(cursor, char_color, text);

					cursor.x += char_size.x;
					ImGui::SetCursorScreenPos(cursor);
				}
			}

			ImGui::NewLine();
			cursor = ImGui::GetCursorScreenPos();
		}
	}

	state->SelectStartByte = next_select_start_byte;
	state->SelectStartSubByte = next_select_start_subbyte;
	state->SelectEndByte = next_select_end_byte;
	state->SelectEndSubByte = next_select_end_subbyte;	
	state->LastSelectedByte = next_last_selected_byte;

	if (line_buf != stack_line_buf)
		ImGui::MemFree(line_buf);

	if (address_buf != stack_address_buf)
		ImGui::MemFree(address_buf);
	
	return true;
}

void ImGui::EndHexEditor()
{
	ImGui::EndChild();
}

static bool RangeRangeIntersection(int a_min, int a_max, int b_min, int b_max, int* out_min, int* out_max)
{
	if (a_max < b_min || b_max < a_min)
		return false;

	*out_min = ImMax(a_min, b_min);
	*out_max = ImMin(a_max, b_max);

	if (*out_min <= *out_max)
		return true;

	return false;
}

bool ImGui::CalcHexEditorRowRange(int row_offset, int row_bytes_count, int range_min, int range_max, int* out_min, int* out_max)
{
	int abs_min;
	int abs_max;

	if (RangeRangeIntersection(row_offset, row_offset + row_bytes_count, range_min, range_max, &abs_min, &abs_max))
	{
		*out_min = abs_min - row_offset;
		*out_max = abs_max - row_offset;
		return true;
	}

	return false;
}