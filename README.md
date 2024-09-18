My version of hexadecimal editor for the Dear ImGui

![image](https://github.com/user-attachments/assets/d95e778f-d157-4abc-94ad-2acd7890f4c9)

Features
1. Automatically adjust visible bytes count depending on the window width
2. Read-only mode
3. Optional ascii display
4. Separators support
5. Support for lowercase bytes
6. Keyboard navigation
7. Custom read/write/name callbacks
8. Render zeroes as disabled (idea from the ocronut's hex editor version)
9. Custom highlighting (with automatic text contrast selection)
10. Clipboard support

Example:

> [!NOTE]  
> Hex editor doesn't require you to implement callbacks at all.
```cpp
static ImGuiHexEditorState hex_state;

hex_state.ReadCallback = [](ImGuiHexEditorState* state, size_t offset, void* buf, size_t size) -> size_t {
  SIZE_T read;
  ReadProcessMemory(GetCurrentProcess(), (char*)state->Bytes + offset, buf, size, &read);
  return read;
};

hex_state.WriteCallback = [](ImGuiHexEditorState* state, size_t offset, void* buf, size_t size) -> size_t {
  SIZE_T write;
  WriteProcessMemory(GetCurrentProcess(), (char*)state->Bytes + offset, buf, size, &write);
  return write;
};

hex_state.GetAddressNameCallback = [](ImGuiHexEditorState* state, size_t offset, char* buf, size_t size) -> bool
{
  if (offset >= 0 && offset < sizeof(ImGuiIO))
  {
    snprintf(buf, size, "io+%0.*zX", 4, offset);
    return true;
  }

  return false;
};

hex_state.SingleHighlightCallback = [](ImGuiHexEditorState* state, int offset, ImColor* color, ImColor* text_color, ImColor* border_color) -> ImGuiHexEditorHighlightFlags
{
  if (offset >= 100 && offset <= 150)
  {
    *color = ImColor(user_highlight_color);
    return ImGuiHexEditorHighlightFlags_Apply | ImGuiHexEditorHighlightFlags_TextAutomaticContrast | ImGuiHexEditorHighlightFlags_Ascii
      | ImGuiHexEditorHighlightFlags_BorderAutomaticContrast;
  }

  return ImGuiHexEditorHighlightFlags_None;
};

hex_state.HighlightRanges.clear();

{
    ImGuiHexEditorHighlightRange range;
    range.From = 200;
    range.To = 250;
    range.Color = ImColor(user_highlight_color);
    range.Flags = ImGuiHexEditorHighlightFlags_TextAutomaticContrast | ImGuiHexEditorHighlightFlags_FullSized 
        | ImGuiHexEditorHighlightFlags_Ascii | ImGuiHexEditorHighlightFlags_Border | ImGuiHexEditorHighlightFlags_BorderAutomaticContrast;
    hex_state.HighlightRanges.push_back(range);
}

hex_state.Bytes = (void*)&ImGui::GetIO();
hex_state.MaxBytes = sizeof(ImGuiIO) + 0x1000;

ImGui::BeginHexEditor("##HexEditor", &hex_state);
ImGui::EndHexEditor();
```

TODO:
- [ ] Support for any amount of address chars for automatical bytes count selection.
