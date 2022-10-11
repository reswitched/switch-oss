/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora, Ltd.  All rights reserved.
 * All rights reserved.
 * Copyright (c) 2010-2022 ACCESS CO., LTD. All rights reserved.
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

#include "wtf/HexNumber.h"

namespace WebCore {

WKC_DEFINE_GLOBAL_TYPE_ZERO(OptionSet<PlatformEvent::Modifier>*, gCurrentModifiers);

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

    return makeString("U+", pad('0', 4, hex(toASCIIUpper(keyCode))));
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

static String keyForKeyboardEvent(int keyCode, unsigned short in_char)
{
    switch (keyCode) {
    case WKC::EKeyBack:
        return "Backspace";
    case WKC::EKeyTab:
        return "Tab";
    case WKC::EKeyClear:
        return "NumpadClear";
    case WKC::EKeyReturn:
        return "Enter";
    case WKC::EKeyShift:
        return "Shift";
    case WKC::EKeyControl:
        return "Control";
    case WKC::EKeyMenu:
        return "Alt";
    case WKC::EKeyPause:
        return "Pause";
    case WKC::EKeyCapital:
        return "CapsLock";
    case WKC::EKeyKana:
        //    case WKC::EKeyHangul:
        return "KanaMode";
    case WKC::EKeyJunja:
    case WKC::EKeyFinal:
        return "Unidentified";
    case WKC::EKeyHanja:
        //    case WKC::EKeyKanji:
        return "Lang2";
    case WKC::EKeyEscape:
        return "Escape";
    case WKC::EKeyConvert:
        return "Convert";
    case WKC::EKeyNonConvert:
        return "NonConvert";
    case WKC::EKeyAccept:
        return "Unidentified";
    case WKC::EKeyModeChange:
        return "ModeChange";
    case WKC::EKeySpace:
        return singleCharacterString(in_char);
    case WKC::EKeyPrior:
        return "PageUp";
    case WKC::EKeyNext:
        return "PageDown";
    case WKC::EKeyEnd:
        return "End";
    case WKC::EKeyHome:
        return "Home";
    case WKC::EKeyLeft:
        return "ArrowLeft";
    case WKC::EKeyUp:
        return "ArrowUp";
    case WKC::EKeyRight:
        return "ArrowRight";
    case WKC::EKeyDown:
        return "ArrowDown";
    case WKC::EKeySelect:
        return "Select";
    case WKC::EKeyPrint:
    case WKC::EKeyExecute:
        return "Unidentified";
    case WKC::EKeySnapShot:
        return "PrintScreen";
    case WKC::EKeyInsert:
        return "Insert";
    case WKC::EKeyDelete:
        return "Delete";
    case WKC::EKeyHelp:
        return "Help";

    case WKC::EKey0:
    case WKC::EKey1:
    case WKC::EKey2:
    case WKC::EKey3:
    case WKC::EKey4:
    case WKC::EKey5:
    case WKC::EKey6:
    case WKC::EKey7:
    case WKC::EKey8:
    case WKC::EKey9:
    case WKC::EKeyA:
    case WKC::EKeyB:
    case WKC::EKeyC:
    case WKC::EKeyD:
    case WKC::EKeyE:
    case WKC::EKeyF:
    case WKC::EKeyG:
    case WKC::EKeyH:
    case WKC::EKeyI:
    case WKC::EKeyJ:
    case WKC::EKeyK:
    case WKC::EKeyL:
    case WKC::EKeyM:
    case WKC::EKeyN:
    case WKC::EKeyO:
    case WKC::EKeyP:
    case WKC::EKeyQ:
    case WKC::EKeyR:
    case WKC::EKeyS:
    case WKC::EKeyT:
    case WKC::EKeyU:
    case WKC::EKeyV:
    case WKC::EKeyW:
    case WKC::EKeyX:
    case WKC::EKeyY:
    case WKC::EKeyZ:
        return singleCharacterString(in_char);

    case WKC::EKeyLWin:
        return "MetaLeft";
    case WKC::EKeyRWin:
        return "MetaRight";
    case WKC::EKeyApps:
        return "ContextMenu";
    case WKC::EKeySleep:
        return "Sleep";
    case WKC::EKeyNumPad0:
        return "Numpad0";
    case WKC::EKeyNumPad1:
        return "Numpad1";
    case WKC::EKeyNumPad2:
        return "Numpad2";
    case WKC::EKeyNumPad3:
        return "Numpad3";
    case WKC::EKeyNumPad4:
        return "Numpad4";
    case WKC::EKeyNumPad5:
        return "Numpad5";
    case WKC::EKeyNumPad6:
        return "Numpad6";
    case WKC::EKeyNumPad7:
        return "Numpad7";
    case WKC::EKeyNumPad8:
        return "Numpad8";
    case WKC::EKeyNumPad9:
        return "Numpad9";
    case WKC::EKeyMultiply:
        return "NumpadMultiply";
    case WKC::EKeyAdd:
        return "NumpadAdd";
    case WKC::EKeySeparator:
        return "Unidentified";
    case WKC::EKeySubtract:
        return "NumpadSubtract";
    case WKC::EKeyDecimal:
        return "NumpadDecimal";
    case WKC::EKeyDivide:
        return "NumpadDivide";
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
    case WKC::EKeyNumLock:
        return "NumLock";
    case WKC::EKeyScroll:
        return "ScrollLock";
    case WKC::EKeyLShift:
        return "ShiftLeft";
    case WKC::EKeyRShift:
        return "ShiftRight";
    case WKC::EKeyLControl:
        return "ControlLeft";
    case WKC::EKeyRControl:
        return "ControlRight";
    case WKC::EKeyLMenu:
        return "MetaLeft";
    case WKC::EKeyRMenu:
        return "MetaRight";
    case WKC::EKeyBrowserBack:
        return "BrowserBack";
    case WKC::EKeyBrowserForward:
        return "BrowserForward";
    case WKC::EKeyBrowserRefresh:
        return "BrowserRefresh";
    case WKC::EKeyBrowserStop:
        return "BrowserStop";
    case WKC::EKeyBrowserSearch:
        return "BrowserSearch";
    case WKC::EKeyBrowserFavorites:
        return "BrowserFavorites";
    case WKC::EKeyBrowserHome:
        return "BrowserHome";
    case WKC::EKeyVolumeMute:
        return "AudioVolumeMute";
    case WKC::EKeyVolumeDown:
        return "AudioVolumeDown";
    case WKC::EKeyVolumeUp:
        return "AudioVolumeUp";
    case WKC::EKeyMediaNextTrack:
        return "MediaTrackNext";
    case WKC::EKeyMediaPrevTrack:
        return "MediaTrackPrevious";
    case WKC::EKeyMediaStop:
        return "MediaStop";
    case WKC::EKeyMediaPlayPause:
        return "MediaPlayPause";
    case WKC::EKeyMediaLaunchMail:
        return "LaunchMail";
    case WKC::EKeyMediaLaunchMediaselect:
        return "MediaSelect";
    case WKC::EKeyMediaLaunchApp1:
        return "LaunchApp1";
    case WKC::EKeyMediaLaunchApp2:
        return "LaunchApp2";

    case WKC::EKeyOem1:
    case WKC::EKeyOemPlus:
    case WKC::EKeyOemMinus:
    case WKC::EKeyOemComma:
    case WKC::EKeyOemPeriod:
    case WKC::EKeyOem2:
    case WKC::EKeyOem3:
    case WKC::EKeyOem4:
    case WKC::EKeyOem5:
    case WKC::EKeyOem6:
    case WKC::EKeyOem7:
        return singleCharacterString(in_char);
    case WKC::EKeyOem8:
        return "Unidentified";

    case WKC::EKeyOem102:
        return singleCharacterString(in_char);

    case WKC::EKeyPacket:
    case WKC::EKeyOemReset: //VK_OEM_RESET;
    case WKC::EKeyOemJump: //VK_OEM_JUMP:
    case WKC::EKeyOemPA1: //VK_OEM_PA1;
    case WKC::EKeyOemPA2: //VK_OEM_PA2;
    case WKC::EKeyOemPA3: //VK_OEM_PA3;
    case WKC::EKeyOemWsctrl: //VK_OEM_WSCTRL;
    case WKC::EKeyOemCusel: //VK_OEM_CUSEL;
    case WKC::EKeyOemAttn: //VK_OEM_ATTN;
    case WKC::EKeyOemFinish: //VK_OEM_FINISH;
    case WKC::EKeyOemCopy: //VK_OEM_COPY;
    case WKC::EKeyOemAuto: //VK_OEM_AUTO;
    case WKC::EKeyOemEnlw: //VK_OEM_ENLW;
    case WKC::EKeyOemBackTab: //VK_OEM_BACKTAB;
    case WKC::EKeyAttn:
    case WKC::EKeyCrSel:
    case WKC::EKeyExSel:
    case WKC::EKeyErEOF:
    case WKC::EKeyZoom:
    case WKC::EKeyNoName:
    case WKC::EKeyPA1:
    case WKC::EKeyOemClear:
        return "Unidentified";

    default:
        break;
    }
    return "Unidentified";
}

static String codeForKeyboardEvent(int keyCode)
{
    switch (keyCode) {
    case WKC::EKeyBack:
        return "Backspace";
    case WKC::EKeyTab:
        return "Tab";
    case WKC::EKeyClear:
        return "NumpadClear";
    case WKC::EKeyReturn:
        return "Enter";
    case WKC::EKeyShift:
        return "Shift";
    case WKC::EKeyControl:
        return "Control";
    case WKC::EKeyMenu:
        return "Alt";
    case WKC::EKeyPause:
        return "Pause";
    case WKC::EKeyCapital:
        return "CapsLock";
    case WKC::EKeyKana:
        //    case WKC::EKeyHangul:
        return "KanaMode";
    case WKC::EKeyJunja:
    case WKC::EKeyFinal:
        return "Unidentified";
    case WKC::EKeyHanja:
        //    case WKC::EKeyKanji:
        return "Lang2";
    case WKC::EKeyEscape:
        return "Escape";
    case WKC::EKeyConvert:
        return "Convert";
    case WKC::EKeyNonConvert:
        return "NonConvert";
    case WKC::EKeyAccept:
        return "Unidentified";
    case WKC::EKeyModeChange:
        return "ModeChange";
    case WKC::EKeySpace:
        return "Space";
    case WKC::EKeyPrior:
        return "PageUp";
    case WKC::EKeyNext:
        return "PageDown";
    case WKC::EKeyEnd:
        return "End";
    case WKC::EKeyHome:
        return "Home";
    case WKC::EKeyLeft:
        return "ArrowLeft";
    case WKC::EKeyUp:
        return "ArrowUp";
    case WKC::EKeyRight:
        return "ArrowRight";
    case WKC::EKeyDown:
        return "ArrowDown";
    case WKC::EKeySelect:
        return "Select";
    case WKC::EKeyPrint:
    case WKC::EKeyExecute:
        return "Unidentified";
    case WKC::EKeySnapShot:
        return "PrintScreen";
    case WKC::EKeyInsert:
        return "Insert";
    case WKC::EKeyDelete:
        return "Delete";
    case WKC::EKeyHelp:
        return "Help";

    case WKC::EKey0:
        return "Digit0";
    case WKC::EKey1:
        return "Digit1";
    case WKC::EKey2:
        return "Digit2";
    case WKC::EKey3:
        return "Digit3";
    case WKC::EKey4:
        return "Digit4";
    case WKC::EKey5:
        return "Digit5";
    case WKC::EKey6:
        return "Digit6";
    case WKC::EKey7:
        return "Digit7";
    case WKC::EKey8:
        return "Digit8";
    case WKC::EKey9:
        return "Digit9";
    case WKC::EKeyA:
        return "KeyA";
    case WKC::EKeyB:
        return "KeyB";
    case WKC::EKeyC:
        return "KeyC";
    case WKC::EKeyD:
        return "KeyD";
    case WKC::EKeyE:
        return "KeyE";
    case WKC::EKeyF:
        return "KeyF";
    case WKC::EKeyG:
        return "KeyG";
    case WKC::EKeyH:
        return "KeyH";
    case WKC::EKeyI:
        return "KeyI";
    case WKC::EKeyJ:
        return "KeyJ";
    case WKC::EKeyK:
        return "KeyK";
    case WKC::EKeyL:
        return "KeyL";
    case WKC::EKeyM:
        return "KeyM";
    case WKC::EKeyN:
        return "KeyN";
    case WKC::EKeyO:
        return "KeyO";
    case WKC::EKeyP:
        return "KeyP";
    case WKC::EKeyQ:
        return "KeyQ";
    case WKC::EKeyR:
        return "KeyR";
    case WKC::EKeyS:
        return "KeyS";
    case WKC::EKeyT:
        return "KeyT";
    case WKC::EKeyU:
        return "KeyU";
    case WKC::EKeyV:
        return "KeyV";
    case WKC::EKeyW:
        return "KeyW";
    case WKC::EKeyX:
        return "KeyX";
    case WKC::EKeyY:
        return "KeyY";
    case WKC::EKeyZ:
        return "KeyZ";

    case WKC::EKeyLWin:
        return "MetaLeft";
    case WKC::EKeyRWin:
        return "MetaRight";
    case WKC::EKeyApps:
        return "ContextMenu";
    case WKC::EKeySleep:
        return "Sleep";
    case WKC::EKeyNumPad0:
        return "Numpad0";
    case WKC::EKeyNumPad1:
        return "Numpad1";
    case WKC::EKeyNumPad2:
        return "Numpad2";
    case WKC::EKeyNumPad3:
        return "Numpad3";
    case WKC::EKeyNumPad4:
        return "Numpad4";
    case WKC::EKeyNumPad5:
        return "Numpad5";
    case WKC::EKeyNumPad6:
        return "Numpad6";
    case WKC::EKeyNumPad7:
        return "Numpad7";
    case WKC::EKeyNumPad8:
        return "Numpad8";
    case WKC::EKeyNumPad9:
        return "Numpad9";
    case WKC::EKeyMultiply:
        return "NumpadMultiply";
    case WKC::EKeyAdd:
        return "NumpadAdd";
    case WKC::EKeySeparator:
        return "Unidentified";
    case WKC::EKeySubtract:
        return "NumpadSubtract";
    case WKC::EKeyDecimal:
        return "NumpadDecimal";
    case WKC::EKeyDivide:
        return "NumpadDivide";
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
    case WKC::EKeyNumLock:
        return "NumLock";
    case WKC::EKeyScroll:
        return "ScrollLock";
    case WKC::EKeyLShift:
        return "ShiftLeft";
    case WKC::EKeyRShift:
        return "ShiftRight";
    case WKC::EKeyLControl:
        return "ControlLeft";
    case WKC::EKeyRControl:
        return "ControlRight";
    case WKC::EKeyLMenu:
        return "MetaLeft";
    case WKC::EKeyRMenu:
        return "MetaRight";
    case WKC::EKeyBrowserBack:
        return "BrowserBack";
    case WKC::EKeyBrowserForward:
        return "BrowserForward";
    case WKC::EKeyBrowserRefresh:
        return "BrowserRefresh";
    case WKC::EKeyBrowserStop:
        return "BrowserStop";
    case WKC::EKeyBrowserSearch:
        return "BrowserSearch";
    case WKC::EKeyBrowserFavorites:
        return "BrowserFavorites";
    case WKC::EKeyBrowserHome:
        return "BrowserHome";
    case WKC::EKeyVolumeMute:
        return "AudioVolumeMute";
    case WKC::EKeyVolumeDown:
        return "AudioVolumeDown";
    case WKC::EKeyVolumeUp:
        return "AudioVolumeUp";
    case WKC::EKeyMediaNextTrack:
        return "MediaTrackNext";
    case WKC::EKeyMediaPrevTrack:
        return "MediaTrackPrevious";
    case WKC::EKeyMediaStop:
        return "MediaStop";
    case WKC::EKeyMediaPlayPause:
        return "MediaPlayPause";
    case WKC::EKeyMediaLaunchMail:
        return "LaunchMail";
    case WKC::EKeyMediaLaunchMediaselect:
        return "MediaSelect";
    case WKC::EKeyMediaLaunchApp1:
        return "LaunchApp1";
    case WKC::EKeyMediaLaunchApp2:
        return "LaunchApp2";

    case WKC::EKeyOem1:
        return "Quote";
    case WKC::EKeyOemPlus:
        return "Semicolon";
    case WKC::EKeyOemComma:
        return "Comma";
    case WKC::EKeyOemMinus:
        return "Minus";
    case WKC::EKeyOemPeriod:
        return "Period";
    case WKC::EKeyOem2:
        return "Slash";
    case WKC::EKeyOem3:
        return "BracketLeft";
    case WKC::EKeyOem4:
        return "BracketRight";
    case WKC::EKeyOem5:
        return "IntlYen";
    case WKC::EKeyOem6:
        return "Backslash";
    case WKC::EKeyOem7:
        return "Equal";
    case WKC::EKeyOem8:
        return "Unidentified";

    case WKC::EKeyOem102:
        return "IntlRo";
    case WKC::EKeyPacket:
    case WKC::EKeyOemReset: //VK_OEM_RESET;
    case WKC::EKeyOemJump: //VK_OEM_JUMP:
    case WKC::EKeyOemPA1: //VK_OEM_PA1;
    case WKC::EKeyOemPA2: //VK_OEM_PA2;
    case WKC::EKeyOemPA3: //VK_OEM_PA3;
    case WKC::EKeyOemWsctrl: //VK_OEM_WSCTRL;
    case WKC::EKeyOemCusel: //VK_OEM_CUSEL;
    case WKC::EKeyOemAttn: //VK_OEM_ATTN;
    case WKC::EKeyOemFinish: //VK_OEM_FINISH;
    case WKC::EKeyOemCopy: //VK_OEM_COPY;
    case WKC::EKeyOemAuto: //VK_OEM_AUTO;
    case WKC::EKeyOemEnlw: //VK_OEM_ENLW;
    case WKC::EKeyOemBackTab: //VK_OEM_BACKTAB;
    case WKC::EKeyAttn:
    case WKC::EKeyCrSel:
    case WKC::EKeyExSel:
    case WKC::EKeyErEOF:
    case WKC::EKeyPlay:
    case WKC::EKeyZoom:
    case WKC::EKeyNoName:
    case WKC::EKeyPA1:
    case WKC::EKeyOemClear:
        return "Unidentified";

    default:
        break;
    }
    return "Unidentified";
}

PlatformKeyboardEvent::PlatformKeyboardEvent(const void* event)
{
    const WKC::WKCKeyEvent* ev = (const WKC::WKCKeyEvent *)event;

    m_autoRepeat = ev->m_autoRepeat;

    switch (ev->m_type) {
    case WKC::EKeyEventDown:
        m_type = RawKeyDown;
        break;
    case WKC::EKeyEventUp:
        m_type = KeyUp;
        break;
    case WKC::EKeyEventPress:
        m_type = Char;
        break;
    case WKC::EKeyEventAccessKey:
        m_type = KeyDown;
        break;
    default:
        m_type = KeyUp;
    }

    m_isSystemKey = false;

    if (ev->m_modifiers & WKC::EModifierIME) {
        m_key = "Process";
    } else {
        m_key = keyForKeyboardEvent(ev->m_key, ev->m_char);
    }

    m_code = codeForKeyboardEvent(ev->m_key);

    if (ev->m_modifiers & WKC::EModifierShift) {
        m_modifiers.add(PlatformEvent::Modifier::ShiftKey);
    }
    if (ev->m_modifiers & WKC::EModifierCtrl) {
        m_modifiers.add(PlatformEvent::Modifier::ControlKey);
    }
    if (ev->m_modifiers & WKC::EModifierAlt) {
        m_modifiers.add(PlatformEvent::Modifier::AltKey);
    }
    if (ev->m_modifiers & WKC::EModifierMod1) {
        m_modifiers.add(PlatformEvent::Modifier::MetaKey);
    }

    if (m_type==Char) {
        m_keyIdentifier = String();
        m_windowsVirtualKeyCode = 0;
        m_text = singleCharacterString(ev->m_char);
        m_unmodifiedText = m_text;
        m_isKeypad = false;
        m_isSystemKey = false;
        return;
    }

    m_keyIdentifier = keyIdentifierForWKCKeyCode(ev->m_key);
    m_windowsVirtualKeyCode = windowsKeyCodeForKeyEvent(ev->m_key);
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

    if (!gCurrentModifiers) {
        gCurrentModifiers = new OptionSet<PlatformEvent::Modifier>();
    }

    if (ev->m_modifiers & WKC::EModifierShift) {
        if (m_type == KeyUp)
            gCurrentModifiers->remove(PlatformEvent::Modifier::ShiftKey);
        else
            gCurrentModifiers->add(PlatformEvent::Modifier::ShiftKey);
    }
    if (ev->m_modifiers & WKC::EModifierCtrl) {
        if (m_type == KeyUp)
            gCurrentModifiers->remove(PlatformEvent::Modifier::ControlKey);
        else
            gCurrentModifiers->add(PlatformEvent::Modifier::ControlKey);
    }
    if (ev->m_modifiers & WKC::EModifierAlt) {
        if (m_type == KeyUp)
            gCurrentModifiers->remove(PlatformEvent::Modifier::AltKey);
        else
            gCurrentModifiers->add(PlatformEvent::Modifier::AltKey);
    }
    if (ev->m_modifiers & WKC::EModifierMod1) {
        if (m_type == KeyUp)
            gCurrentModifiers->remove(PlatformEvent::Modifier::MetaKey);
        else
            gCurrentModifiers->add(PlatformEvent::Modifier::MetaKey);
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
    if (!gCurrentModifiers) {
        shiftKey = false;
        ctrlKey = false;
        altKey = false;
        metaKey = false;
        return;
    }
    shiftKey = gCurrentModifiers->contains(PlatformEvent::Modifier::ShiftKey);
    ctrlKey = gCurrentModifiers->contains(PlatformEvent::Modifier::ControlKey);
    altKey = gCurrentModifiers->contains(PlatformEvent::Modifier::AltKey);
    metaKey = gCurrentModifiers->contains(PlatformEvent::Modifier::MetaKey);
}

}
