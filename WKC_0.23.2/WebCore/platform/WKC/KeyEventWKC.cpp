/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora, Ltd.  All rights reserved.
 * All rights reserved.
 * Copyright (c) 2010-2012 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformKeyboardEvent.h"
#include "WindowsKeyboardCodes.h"

#include "NotImplemented.h"
#include "TextEncoding.h"

#include "WKCPlatformEvents.h"

namespace WebCore {

WKC_DEFINE_GLOBAL_UINT(gCurrentModifiers, 0);

static String keyIdentifierForWKCKeyCode(int keyCode)
{
    switch (keyCode) {
    case WKC::EKeyBack:
        return "U+0008";
    case WKC::EKeyTab:
        return "U+0009";
    case WKC::EKeyClear:
        return "Clear";
    case WKC::EKeyReturn:
        return "Enter";
    case WKC::EKeyMenu:
        return "Alt";
    case WKC::EKeyPause:
        return "Pause";
    case WKC::EKeyPrior:
        return "PageUp";
    case WKC::EKeyNext:
        return "PageDown";
    case WKC::EKeyEnd:
        return "End";
    case WKC::EKeyHome:
        return "Home";
    case WKC::EKeyLeft:
        return "Left";
    case WKC::EKeyUp:
        return "Up";
    case WKC::EKeyRight:
        return "Right";
    case WKC::EKeyDown:
        return "Down";
    case WKC::EKeySelect:
        return "Select";
    case WKC::EKeyExecute:
        return "Execute";
    case WKC::EKeySnapShot:
        return "PrintScreen";
    case WKC::EKeyInsert:
        return "Insert";
    case WKC::EKeyDelete:
        return "U+007F";
    case WKC::EKeyHelp:
        return "Help";

    case WKC::EKeyF1:
        return "F1";
    case WKC::EKeyF2:
        return "F2";
    case WKC::EKeyF3:
        return "F3";
    case WKC::EKeyF4:
        return "F4";
    case WKC::EKeyF5:
        return "F5";
    case WKC::EKeyF6:
        return "F6";
    case WKC::EKeyF7:
        return "F7";
    case WKC::EKeyF8:
        return "F8";
    case WKC::EKeyF9:
        return "F9";
    case WKC::EKeyF10:
        return "F10";
    case WKC::EKeyF11:
        return "F11";
    case WKC::EKeyF12:
        return "F12";
    case WKC::EKeyF13:
        return "F13";
    case WKC::EKeyF14:
        return "F14";
    case WKC::EKeyF15:
        return "F15";
    case WKC::EKeyF16:
        return "F16";
    case WKC::EKeyF17:
        return "F17";
    case WKC::EKeyF18:
        return "F18";
    case WKC::EKeyF19:
        return "F19";
    case WKC::EKeyF20:
        return "F20";
    case WKC::EKeyF21:
        return "F21";
    case WKC::EKeyF22:
        return "F22";
    case WKC::EKeyF23:
        return "F23";
    case WKC::EKeyF24:
        return "F24";

    default:
        break;
    }

    return String::format("U+%04X", toASCIIUpper(keyCode));
}

static int windowsKeyCodeForKeyEvent(int keyCode)
{
    switch (keyCode) {
    case WKC::EKeyBack:
        return VK_BACK;
    case WKC::EKeyTab:
        return VK_TAB;
    case WKC::EKeyClear:
        return VK_CLEAR;
    case WKC::EKeyReturn:
        return VK_RETURN;
    case WKC::EKeyShift:
        return VK_SHIFT;
    case WKC::EKeyControl:
        return VK_CONTROL;
    case WKC::EKeyMenu:
        return VK_MENU;
    case WKC::EKeyPause:
        return VK_PAUSE;
    case WKC::EKeyCapital:
        return VK_CAPITAL;
    case WKC::EKeyKana:
//    case WKC::EKeyHangul:
        return VK_KANA;
    case WKC::EKeyJunja:
        return VK_JUNJA;
    case WKC::EKeyFinal:
        return VK_FINAL;
    case WKC::EKeyHanja:
//    case WKC::EKeyKanji:
        return VK_HANJA;
    case WKC::EKeyEscape:
        return VK_ESCAPE;
    case WKC::EKeyConvert:
        return VK_CONVERT;
    case WKC::EKeyNonConvert:
        return VK_NONCONVERT;
    case WKC::EKeyAccept:
        return VK_ACCEPT;
    case WKC::EKeyModeChange:
        return VK_MODECHANGE;
    case WKC::EKeySpace:
        return VK_SPACE;
    case WKC::EKeyPrior:
        return VK_PRIOR;
    case WKC::EKeyNext:
        return VK_NEXT;
    case WKC::EKeyEnd:
        return VK_END;
    case WKC::EKeyHome:
        return VK_HOME;
    case WKC::EKeyLeft:
        return VK_LEFT;
    case WKC::EKeyUp:
        return VK_UP;
    case WKC::EKeyRight:
        return VK_RIGHT;
    case WKC::EKeyDown:
        return VK_DOWN;
    case WKC::EKeySelect:
        return VK_SELECT;
    case WKC::EKeyPrint:
        return VK_PRINT;
    case WKC::EKeyExecute:
        return VK_EXECUTE;
    case WKC::EKeySnapShot:
        return VK_SNAPSHOT;
    case WKC::EKeyInsert:
        return VK_INSERT;
    case WKC::EKeyDelete:
        return VK_DELETE;
    case WKC::EKeyHelp:
        return VK_HELP;

    case WKC::EKey0:
        return VK_0;
    case WKC::EKey1:
        return VK_1;
    case WKC::EKey2:
        return VK_2;
    case WKC::EKey3:
        return VK_3;
    case WKC::EKey4:
        return VK_4;
    case WKC::EKey5:
        return VK_5;
    case WKC::EKey6:
        return VK_6;
    case WKC::EKey7:
        return VK_7;
    case WKC::EKey8:
        return VK_8;
    case WKC::EKey9:
        return VK_9;
    case WKC::EKeyA:
        return VK_A;
    case WKC::EKeyB:
        return VK_B;
    case WKC::EKeyC:
        return VK_C;
    case WKC::EKeyD:
        return VK_D;
    case WKC::EKeyE:
        return VK_E;
    case WKC::EKeyF:
        return VK_F;
    case WKC::EKeyG:
        return VK_G;
    case WKC::EKeyH:
        return VK_H;
    case WKC::EKeyI:
        return VK_I;
    case WKC::EKeyJ:
        return VK_J;
    case WKC::EKeyK:
        return VK_K;
    case WKC::EKeyL:
        return VK_L;
    case WKC::EKeyM:
        return VK_M;
    case WKC::EKeyN:
        return VK_N;
    case WKC::EKeyO:
        return VK_O;
    case WKC::EKeyP:
        return VK_P;
    case WKC::EKeyQ:
        return VK_Q;
    case WKC::EKeyR:
        return VK_R;
    case WKC::EKeyS:
        return VK_S;
    case WKC::EKeyT:
        return VK_T;
    case WKC::EKeyU:
        return VK_U;
    case WKC::EKeyV:
        return VK_V;
    case WKC::EKeyW:
        return VK_W;
    case WKC::EKeyX:
        return VK_X;
    case WKC::EKeyY:
        return VK_Y;
    case WKC::EKeyZ:
        return VK_Z;

    case WKC::EKeyLWin:
        return VK_LWIN;
    case WKC::EKeyRWin:
        return VK_RWIN;
    case WKC::EKeyApps:
        return VK_APPS;
    case WKC::EKeySleep:
        return VK_SLEEP;
    case WKC::EKeyNumPad0:
        return VK_NUMPAD0;
    case WKC::EKeyNumPad1:
        return VK_NUMPAD1;
    case WKC::EKeyNumPad2:
        return VK_NUMPAD2;
    case WKC::EKeyNumPad3:
        return VK_NUMPAD3;
    case WKC::EKeyNumPad4:
        return VK_NUMPAD4;
    case WKC::EKeyNumPad5:
        return VK_NUMPAD5;
    case WKC::EKeyNumPad6:
        return VK_NUMPAD6;
    case WKC::EKeyNumPad7:
        return VK_NUMPAD7;
    case WKC::EKeyNumPad8:
        return VK_NUMPAD8;
    case WKC::EKeyNumPad9:
        return VK_NUMPAD9;
    case WKC::EKeyMultiply:
        return VK_MULTIPLY;
    case WKC::EKeyAdd:
        return VK_ADD;
    case WKC::EKeySeparator:
        return VK_SEPARATOR;
    case WKC::EKeySubtract:
        return VK_SUBTRACT;
    case WKC::EKeyDecimal:
        return VK_DECIMAL;
    case WKC::EKeyDivide:
        return VK_DIVIDE;
    case WKC::EKeyF1:
        return VK_F1;
    case WKC::EKeyF2:
        return VK_F2;
    case WKC::EKeyF3:
        return VK_F3;
    case WKC::EKeyF4:
        return VK_F4;
    case WKC::EKeyF5:
        return VK_F5;
    case WKC::EKeyF6:
        return VK_F6;
    case WKC::EKeyF7:
        return VK_F7;
    case WKC::EKeyF8:
        return VK_F8;
    case WKC::EKeyF9:
        return VK_F9;
    case WKC::EKeyF10:
        return VK_F10;
    case WKC::EKeyF11:
        return VK_F11;
    case WKC::EKeyF12:
        return VK_F12;
    case WKC::EKeyF13:
        return VK_F13;
    case WKC::EKeyF14:
        return VK_F14;
    case WKC::EKeyF15:
        return VK_F15;
    case WKC::EKeyF16:
        return VK_F16;
    case WKC::EKeyF17:
        return VK_F17;
    case WKC::EKeyF18:
        return VK_F18;
    case WKC::EKeyF19:
        return VK_F19;
    case WKC::EKeyF20:
        return VK_F20;
    case WKC::EKeyF21:
        return VK_F21;
    case WKC::EKeyF22:
        return VK_F22;
    case WKC::EKeyF23:
        return VK_F23;
    case WKC::EKeyF24:
        return VK_F24;
    case WKC::EKeyNumLock:
        return VK_NUMLOCK;
    case WKC::EKeyScroll:
        return VK_SCROLL;
    case WKC::EKeyLShift:
        return VK_LSHIFT;
    case WKC::EKeyRShift:
        return VK_RSHIFT;
    case WKC::EKeyLControl:
        return VK_LCONTROL;
    case WKC::EKeyRControl:
        return VK_RCONTROL;
    case WKC::EKeyLMenu:
        return VK_LMENU;
    case WKC::EKeyRMenu:
        return VK_RMENU;
    case WKC::EKeyBrowserBack:
        return VK_BROWSER_BACK;
    case WKC::EKeyBrowserForward:
        return VK_BROWSER_FORWARD;
    case WKC::EKeyBrowserRefresh:
        return VK_BROWSER_REFRESH;
    case WKC::EKeyBrowserStop:
        return VK_BROWSER_STOP;
    case WKC::EKeyBrowserSearch:
        return VK_BROWSER_SEARCH;
    case WKC::EKeyBrowserFavorites:
        return VK_BROWSER_FAVORITES;
    case WKC::EKeyBrowserHome:
        return VK_BROWSER_HOME;
    case WKC::EKeyVolumeMute:
        return VK_VOLUME_MUTE;
    case WKC::EKeyVolumeDown:
        return VK_VOLUME_DOWN;
    case WKC::EKeyVolumeUp:
        return VK_VOLUME_UP;
    case WKC::EKeyMediaNextTrack:
        return VK_MEDIA_NEXT_TRACK;
    case WKC::EKeyMediaPrevTrack:
        return VK_MEDIA_PREV_TRACK;
    case WKC::EKeyMediaStop:
        return VK_MEDIA_STOP;
    case WKC::EKeyMediaPlayPause:
        return VK_MEDIA_PLAY_PAUSE;
    case WKC::EKeyMediaLaunchMail:
        return VK_MEDIA_LAUNCH_MAIL;
    case WKC::EKeyMediaLaunchMediaselect:
        return VK_MEDIA_LAUNCH_MEDIA_SELECT;
    case WKC::EKeyMediaLaunchApp1:
        return VK_MEDIA_LAUNCH_APP1;
    case WKC::EKeyMediaLaunchApp2:
        return VK_MEDIA_LAUNCH_APP2;

    case WKC::EKeyOem1:
        return VK_OEM_1;
    case WKC::EKeyOemPlus:
        return VK_OEM_PLUS;
    case WKC::EKeyOemComma:
        return VK_OEM_COMMA;
    case WKC::EKeyOemMinus:
        return VK_OEM_MINUS;
    case WKC::EKeyOemPeriod:
        return VK_OEM_PERIOD;
    case WKC::EKeyOem2:
        return VK_OEM_2;
    case WKC::EKeyOem3:
        return VK_OEM_3;
    case WKC::EKeyOem4:
        return VK_OEM_4;
    case WKC::EKeyOem5:
        return VK_OEM_5;
    case WKC::EKeyOem6:
        return VK_OEM_6;
    case WKC::EKeyOem7:
        return VK_OEM_7;
    case WKC::EKeyOem8:
        return VK_OEM_8;

    case WKC::EKeyOem102:
        return VK_OEM_102;
    case WKC::EKeyProcessKey:
        return VK_PROCESSKEY;
    case WKC::EKeyPacket:
        return VK_PACKET;

    case WKC::EKeyOemReset:
        return 0xe9; //VK_OEM_RESET;
    case WKC::EKeyOemJump:
        return 0xea; //VK_OEM_JUMP:
    case WKC::EKeyOemPA1:
        return 0xeb; //VK_OEM_PA1;
    case WKC::EKeyOemPA2:
        return 0xec; //VK_OEM_PA2;
    case WKC::EKeyOemPA3:
        return 0xed; //VK_OEM_PA3;
    case WKC::EKeyOemWsctrl:
        return 0xee; //VK_OEM_WSCTRL;
    case WKC::EKeyOemCusel:
        return 0xef; //VK_OEM_CUSEL;
    case WKC::EKeyOemAttn:
        return 0xf0; //VK_OEM_ATTN;
    case WKC::EKeyOemFinish:
        return 0xf1; //VK_OEM_FINISH;
    case WKC::EKeyOemCopy:
        return 0xf2; //VK_OEM_COPY;
    case WKC::EKeyOemAuto:
        return 0xf3; //VK_OEM_AUTO;
    case WKC::EKeyOemEnlw:
        return 0xf4; //VK_OEM_ENLW;
    case WKC::EKeyOemBackTab:
        return 0xf5; //VK_OEM_BACKTAB;

    case WKC::EKeyAttn:
        return VK_ATTN;
    case WKC::EKeyCrSel:
        return VK_CRSEL;
    case WKC::EKeyExSel:
        return VK_EXSEL;
    case WKC::EKeyErEOF:
        return VK_EREOF;
    case WKC::EKeyPlay:
        return VK_PLAY;
    case WKC::EKeyZoom:
        return VK_ZOOM;
    case WKC::EKeyNoName:
        return VK_NONAME;
    case WKC::EKeyPA1:
        return VK_PA1;
    case WKC::EKeyOemClear:
        return VK_OEM_CLEAR;

    default:
        break;
    }
    return 0;
}

static String
singleCharacterString(unsigned int keyChar)
{
    UChar buf[2];
    buf[0] = (UChar)keyChar;
    buf[1] = 0;
    return String(buf, 1);
}

PlatformKeyboardEvent::PlatformKeyboardEvent(const void* event)
{
    const WKC::WKCKeyEvent* ev = (const WKC::WKCKeyEvent *)event;

    m_autoRepeat = ev->m_autoRepeat;

    switch (ev->m_type) {
    case WKC::EKeyEventPressed:
        m_type = RawKeyDown;
        break;
    case WKC::EKeyEventReleased:
        m_type = KeyUp;
        break;
    case WKC::EKeyEventChar:
        m_type = Char;
        break;
    case WKC::EKeyEventAccessKey:
        m_type = KeyDown;
        break;
    default:
        m_type = KeyUp;
    }

    m_modifiers = 0;
    m_isSystemKey = false;
    m_macCharCode = 0;

    if (m_type==Char) {
        m_keyIdentifier = String();
        m_windowsVirtualKeyCode = 0;
        m_nativeVirtualKeyCode = 0;
        m_text = singleCharacterString(ev->m_char);
        m_unmodifiedText = m_text;
        m_isKeypad = false;
        m_isSystemKey = false;
        m_macCharCode = 0;
        return;
    }

    m_keyIdentifier = keyIdentifierForWKCKeyCode(ev->m_key);
    m_windowsVirtualKeyCode = windowsKeyCodeForKeyEvent(ev->m_key);
    m_nativeVirtualKeyCode = m_windowsVirtualKeyCode;
    if (ev->m_type==WKC::EKeyEventAccessKey) {
        m_text = singleCharacterString(ev->m_char);
        m_unmodifiedText = m_text;
    } else {
        m_text = String();
        m_unmodifiedText = String();
    }

    switch (ev->m_key) {
    case WKC::EKeyNumPad0:
    case WKC::EKeyNumPad1:
    case WKC::EKeyNumPad2:
    case WKC::EKeyNumPad3:
    case WKC::EKeyNumPad4:
    case WKC::EKeyNumPad5:
    case WKC::EKeyNumPad6:
    case WKC::EKeyNumPad7:
    case WKC::EKeyNumPad8:
    case WKC::EKeyNumPad9:
    case WKC::EKeyMultiply:
    case WKC::EKeyAdd:
    case WKC::EKeySeparator:
    case WKC::EKeySubtract:
    case WKC::EKeyDecimal:
    case WKC::EKeyDivide:
        m_isKeypad = true;
        break;
    default:
        m_isKeypad = false;
    }

    if (ev->m_modifiers & WKC::EModifierShift) {
        m_modifiers |= ShiftKey;
        if (m_type == KeyUp)
            gCurrentModifiers &= ~ShiftKey;
        else
            gCurrentModifiers |= ShiftKey;
    }
    if (ev->m_modifiers & WKC::EModifierCtrl) {
        m_modifiers |= CtrlKey;
        if (m_type == KeyUp)
            gCurrentModifiers &= ~CtrlKey;
        else
            gCurrentModifiers |= CtrlKey;
    }
    if (ev->m_modifiers & WKC::EModifierAlt) {
        m_modifiers |= AltKey;
        if (m_type == KeyUp)
            gCurrentModifiers &= ~AltKey;
        else
            gCurrentModifiers |= AltKey;
    }
    if (ev->m_modifiers & WKC::EModifierMod1) {
        m_modifiers |= MetaKey;
        if (m_type == KeyUp)
            gCurrentModifiers &= ~MetaKey;
        else
            gCurrentModifiers |= MetaKey;
    }
}

void PlatformKeyboardEvent::disambiguateKeyDownEvent(Type type, bool backwardCompatibilityMode)
{
    // Can only change type from KeyDown to RawKeyDown or Char, as we lack information for other conversions.
    ASSERT(m_type == KeyDown);
    m_type = type;
    if (type == RawKeyDown) {
        m_text = String();
        m_unmodifiedText = String();
    } else {
        m_keyIdentifier = String();
        m_windowsVirtualKeyCode = 0;
    }
}

bool PlatformKeyboardEvent::currentCapsLockState()
{
    return false;
}

void PlatformKeyboardEvent::getCurrentModifierState(bool& shiftKey, bool& ctrlKey, bool& altKey, bool& metaKey)
{
    shiftKey = (gCurrentModifiers&ShiftKey);
    ctrlKey = (gCurrentModifiers&CtrlKey);
    altKey = (gCurrentModifiers&AltKey);
    metaKey = (gCurrentModifiers&MetaKey);
}

}
