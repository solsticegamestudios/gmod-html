// Copied from https://github.com/chromium/chromium/blob/main/ui/events/keycodes/dom/dom_code.h
// Last Updated - August 7, 2024: https://github.com/chromium/chromium/commit/c0a18161acdf82d65faf66f964154cc2aeaee6fe

// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_KEYCODES_DOM_DOM_CODE_H_
#define UI_EVENTS_KEYCODES_DOM_DOM_CODE_H_

#include <cstdint>

namespace ui {

// Declares named values for each of the recognized DOM Code values.
#define DOM_CODE(usb, evdev, xkb, win, mac, code, id) id = usb
#define DOM_CODE_DECLARATION enum class DomCode : uint32_t
#include "dom_code_data.inc"
#undef DOM_CODE
#undef DOM_CODE_DECLARATION

}  // namespace ui

#endif  // UI_EVENTS_KEYCODES_DOM_DOM_CODE_H_
