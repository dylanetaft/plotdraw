#pragma once

#include <string>

void clipboard_set_text(const std::string& text);

// The text ImGui should paste on Ctrl+V. Native: the system clipboard. Web: whatever the last
// clipboard_request_paste retrieved, since nothing can read the browser clipboard unprompted.
// Points at storage owned by this module, valid until the next clipboard call.
const char* clipboard_get_text();

// Starts a clipboard read. On the web this is asynchronous and prompts for permission, so the
// text is not available when this returns -- poll clipboard_take_paste on a later frame. Must be
// called from a user gesture (a button press), or the browser rejects it.
void clipboard_request_paste();

// Hands over text from a completed request and clears it. False, leaving out untouched, when
// nothing is pending -- including when the user denied the permission prompt.
bool clipboard_take_paste(std::string& out);
