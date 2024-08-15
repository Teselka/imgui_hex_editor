My version of hexadecimal editor for the Dear ImGui

https://github.com/user-attachments/assets/69057bdd-333a-411b-9cef-24eb602221b4

Features
1. Automatically adjust visible bytes count depending on the window width
2. Read-only mode
3. Optional ascii display
4. Separators support (customizable)
5. Support for lowercase bytes
6. Keyboard navigation
7. Custom read/write/name callbacks
8. Render zeroes as disabled (idea from the ocronut's hex editor version)

Example:
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

hex_state.Bytes = (void*)&ImGui::GetIO();
hex_state.MaxBytes = sizeof(ImGuiIO) + 0x1000;

ImGui::BeginHexEditor("##HexEditor", &hex_state);
ImGui::EndHexEditor();
```

TODO:
- [ ] Support for any amount of address chars for automatical bytes count selection.
- [ ] Highlighting support.
