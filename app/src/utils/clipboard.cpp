#include "clipboard.h"

#include <SDL3/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace {

// Backs the const char* handed to ImGui. ImGui reads it immediately, but it must outlive the
// call, so it cannot be a local.
std::string g_clipboard_text;

// Text from a completed read, waiting for a frame to pick it up.
std::string g_pending_paste;
bool g_paste_ready = false;

}  // namespace

bool clipboard_take_paste(std::string& out) {
    if (!g_paste_ready) return false;
    out = g_pending_paste;
    g_pending_paste.clear();
    g_paste_ready = false;
    return true;
}

#ifdef __EMSCRIPTEN__

// SDL3 has no clipboard on this target at all: src/video/emscripten/ contains no clipboard code,
// so SDL_VideoDevice::SetClipboardData is never populated and SDL_SetClipboardText falls through
// to storing the string in wasm memory (see SDL_clipboard.c, "if (_this->SetClipboardData)").
// Copy and paste therefore appear to work *within* the app while never touching the browser,
// which is exactly why this looked fine for so long. Tracked upstream as libsdl-org/SDL#13785,
// milestone 3.6.0 -- we build against 3.4.2, so there is nothing to upgrade to.

extern "C" EMSCRIPTEN_KEEPALIVE void plotdraw_on_clipboard_read(const char* text) {
    g_pending_paste = text ? text : "";
    g_paste_ready = true;
    // Also what a subsequent Ctrl+V pastes, since ImGui's hook cannot fetch anything itself here.
    g_clipboard_text = g_pending_paste;
}

// Fire-and-forget rather than EM_ASYNC_JS. -sASYNCIFY is enabled and could await this directly,
// but the call originates in a tool's on_action, which runs mid-ImGui-frame; suspending there
// while requestAnimationFrame keeps dispatching risks corrupting ImGui's frame state. The
// callback only stores the text, and a later frame drains it.
//
// readText is the only way in: listening for the browser's paste event does not work here,
// because SDL's keydown handler treats every Ctrl+key as a nav key
// (SDL_emscriptenevents.c, is_nav_key) and calls preventDefault on it -- and generating the
// paste event is the default action being cancelled.
EM_JS(void, plotdraw_clipboard_read, (), {
    if (!navigator.clipboard || !navigator.clipboard.readText) return;
    navigator.clipboard.readText()
        .then(text => Module.ccall('plotdraw_on_clipboard_read', null, ['string'], [text]))
        .catch(() => {});   // denied or unsupported -- leave the shape alone
});

// writeText can be rejected when the browser decides the call is too far from a user gesture --
// we reach here from the main loop's requestAnimationFrame tick rather than from the DOM handler
// itself. execCommand is deprecated but has no such requirement, so it covers the rejection. The
// textarea is created and destroyed inside this call; it is not an overlay over the canvas.
EM_JS(void, plotdraw_clipboard_write, (const char* ptr), {
    const text = UTF8ToString(ptr);
    const fallback = () => {
        const ta = document.createElement('textarea');
        ta.value = text;
        ta.style.position = 'fixed';
        ta.style.opacity = '0';
        document.body.appendChild(ta);
        ta.select();
        try { document.execCommand('copy'); } catch (e) { /* nothing left to try */ }
        document.body.removeChild(ta);
    };
    if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(text).catch(fallback);
    } else {
        fallback();
    }
});

void clipboard_set_text(const std::string& text) {
    plotdraw_clipboard_write(text.c_str());
}

void clipboard_request_paste() {
    plotdraw_clipboard_read();
}

const char* clipboard_get_text() {
    // The last text a Paste press retrieved. Ctrl+V cannot fetch anything itself here.
    return g_clipboard_text.c_str();
}

#else

void clipboard_set_text(const std::string& text) {
    SDL_SetClipboardText(text.c_str());
}

void clipboard_request_paste() {
    // Synchronous natively, but routed through the same pending buffer so both platforms deliver
    // through on_paste on a later frame and the tools need only one code path.
    char* text = SDL_GetClipboardText();
    g_pending_paste = text ? text : "";
    g_paste_ready = true;
    if (text) SDL_free(text);
}

const char* clipboard_get_text() {
    char* text = SDL_GetClipboardText();
    g_clipboard_text = text ? text : "";
    if (text) SDL_free(text);
    return g_clipboard_text.c_str();
}

#endif
