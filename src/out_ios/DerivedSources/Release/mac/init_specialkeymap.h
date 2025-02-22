// Copyright 2009 Google Inc.  All Rights Reserved.
// Author: mukai
//
// This file is automatically generated by
// generate_maping.py.
// Do not edit directly and do not include this from any file other
// than KeyCodeMap.mm

namespace {
static map<unsigned short, KeyEvent::SpecialKey> *kSpecialKeyMap = NULL;
static map<unsigned short, KeyEvent::SpecialKey> *kSpecialKeyMapShift = NULL;
static once_t kOnceForSpecialKeyMap = MOZC_ONCE_INIT;
void InitSpecialKeyMap() {
  if (kSpecialKeyMap != NULL || kSpecialKeyMapShift != NULL) {
    return;
  }
  kSpecialKeyMap = new(nothrow)map<unsigned short, KeyEvent::SpecialKey>;
  if (kSpecialKeyMap == NULL) {
    return;
  }
  kSpecialKeyMapShift = new(nothrow)map<unsigned short, KeyEvent::SpecialKey>;
  if (kSpecialKeyMapShift == NULL) {
    delete kSpecialKeyMap;
    kSpecialKeyMap = NULL;
    return;
  }

  (*kSpecialKeyMap)[kVK_Return] = KeyEvent::ENTER;
  (*kSpecialKeyMap)[kVK_Tab] = KeyEvent::TAB;
  (*kSpecialKeyMap)[kVK_Space] = KeyEvent::SPACE;
  (*kSpecialKeyMap)[kVK_Delete] = KeyEvent::BACKSPACE;
  (*kSpecialKeyMap)[kVK_ForwardDelete] = KeyEvent::DEL;
  (*kSpecialKeyMap)[kVK_Escape] = KeyEvent::ESCAPE;
  (*kSpecialKeyMap)[kVK_Home] = KeyEvent::HOME;
  (*kSpecialKeyMap)[kVK_PageUp] = KeyEvent::PAGE_UP;
  (*kSpecialKeyMap)[kVK_PageDown] = KeyEvent::PAGE_DOWN;
  (*kSpecialKeyMap)[kVK_End] = KeyEvent::END;
  (*kSpecialKeyMap)[kVK_LeftArrow] = KeyEvent::LEFT;
  (*kSpecialKeyMap)[kVK_RightArrow] = KeyEvent::RIGHT;
  (*kSpecialKeyMap)[kVK_DownArrow] = KeyEvent::DOWN;
  (*kSpecialKeyMap)[kVK_UpArrow] = KeyEvent::UP;
  (*kSpecialKeyMap)[kVK_F1] = KeyEvent::F1;
  (*kSpecialKeyMap)[kVK_F2] = KeyEvent::F2;
  (*kSpecialKeyMap)[kVK_F3] = KeyEvent::F3;
  (*kSpecialKeyMap)[kVK_F4] = KeyEvent::F4;
  (*kSpecialKeyMap)[kVK_F5] = KeyEvent::F5;
  (*kSpecialKeyMap)[kVK_F6] = KeyEvent::F6;
  (*kSpecialKeyMap)[kVK_F7] = KeyEvent::F7;
  (*kSpecialKeyMap)[kVK_F8] = KeyEvent::F8;
  (*kSpecialKeyMap)[kVK_F9] = KeyEvent::F9;
  (*kSpecialKeyMap)[kVK_F10] = KeyEvent::F10;
  (*kSpecialKeyMap)[kVK_F11] = KeyEvent::F11;
  (*kSpecialKeyMap)[kVK_F12] = KeyEvent::F12;
  (*kSpecialKeyMap)[kVK_F13] = KeyEvent::F13;
  (*kSpecialKeyMap)[kVK_F14] = KeyEvent::F14;
  (*kSpecialKeyMap)[kVK_F15] = KeyEvent::F15;
  (*kSpecialKeyMap)[kVK_F16] = KeyEvent::F16;
  (*kSpecialKeyMap)[kVK_F17] = KeyEvent::F17;
  (*kSpecialKeyMap)[kVK_F18] = KeyEvent::F18;
  (*kSpecialKeyMap)[kVK_F19] = KeyEvent::F19;
  (*kSpecialKeyMap)[kVK_F20] = KeyEvent::F20;
  (*kSpecialKeyMap)[kVK_ANSI_KeypadDecimal] = KeyEvent::DECIMAL;
  (*kSpecialKeyMap)[kVK_ANSI_KeypadMultiply] = KeyEvent::MULTIPLY;
  (*kSpecialKeyMap)[kVK_ANSI_KeypadPlus] = KeyEvent::ADD;
  (*kSpecialKeyMap)[kVK_ANSI_KeypadDivide] = KeyEvent::DIVIDE;
  (*kSpecialKeyMap)[kVK_ANSI_KeypadEnter] = KeyEvent::SEPARATOR;
  (*kSpecialKeyMap)[kVK_ANSI_KeypadMinus] = KeyEvent::SUBTRACT;
  (*kSpecialKeyMap)[kVK_ANSI_KeypadEquals] = KeyEvent::EQUALS;
  (*kSpecialKeyMap)[kVK_ANSI_Keypad0] = KeyEvent::NUMPAD0;
  (*kSpecialKeyMap)[kVK_ANSI_Keypad1] = KeyEvent::NUMPAD1;
  (*kSpecialKeyMap)[kVK_ANSI_Keypad2] = KeyEvent::NUMPAD2;
  (*kSpecialKeyMap)[kVK_ANSI_Keypad3] = KeyEvent::NUMPAD3;
  (*kSpecialKeyMap)[kVK_ANSI_Keypad4] = KeyEvent::NUMPAD4;
  (*kSpecialKeyMap)[kVK_ANSI_Keypad5] = KeyEvent::NUMPAD5;
  (*kSpecialKeyMap)[kVK_ANSI_Keypad6] = KeyEvent::NUMPAD6;
  (*kSpecialKeyMap)[kVK_ANSI_Keypad7] = KeyEvent::NUMPAD7;
  (*kSpecialKeyMap)[kVK_ANSI_Keypad8] = KeyEvent::NUMPAD8;
  (*kSpecialKeyMap)[kVK_ANSI_Keypad9] = KeyEvent::NUMPAD9;
  (*kSpecialKeyMap)[kVK_JIS_Eisu] = KeyEvent::OFF;
  (*kSpecialKeyMap)[kVK_JIS_Kana] = KeyEvent::ON;
  (*kSpecialKeyMap)[kVK_JIS_KeypadComma] = KeyEvent::COMMA;
}
}  // anonymous namespace

