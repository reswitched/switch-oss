/*
 *  WKCEnumKeys.h
 *
 *  Copyright (c) 2010, 2012 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#ifndef WKCEnumKeys_h
#define WKCEnumKeys_h

namespace WKC {

/*@{*/

/** @brief Key code type. */
enum Key {
    EKeyBack = 0x08,
    EKeyTab = 0x09,
    EKeyClear = 0x0c,
    EKeyReturn = 0x0d,
    EKeyShift = 0x10,
    EKeyControl = 0x11,
    EKeyMenu = 0x12,
    EKeyPause = 0x13,
    EKeyCapital = 0x14,
    EKeyKana = 0x15,
    EKeyHangul = 0x15,
    EKeyJunja = 0x17,
    EKeyFinal = 0x18,
    EKeyHanja = 0x19,
    EKeyKanji = 0x19,
    EKeyEscape = 0x1b,
    EKeyConvert = 0x1c,
    EKeyNonConvert = 0x1d,
    EKeyAccept = 0x1e,
    EKeyModeChange = 0x1f,
    EKeySpace = 0x20,
    EKeyPrior = 0x21,
    EKeyNext = 0x22,
    EKeyEnd = 0x23,
    EKeyHome = 0x24,
    EKeyLeft = 0x25,
    EKeyUp = 0x26,
    EKeyRight = 0x27,
    EKeyDown = 0x28,
    EKeySelect = 0x29,
    EKeyPrint = 0x2a,
    EKeyExecute = 0x2b,
    EKeySnapShot = 0x2c,
    EKeyInsert = 0x2d,
    EKeyDelete = 0x2e,
    EKeyHelp = 0x2f,

    EKey0 = 0x30,
    EKey1 = 0x31,
    EKey2 = 0x32,
    EKey3 = 0x33,
    EKey4 = 0x34,
    EKey5 = 0x35,
    EKey6 = 0x36,
    EKey7 = 0x37,
    EKey8 = 0x38,
    EKey9 = 0x39,
    EKeyA = 0x41,
    EKeyB = 0x42,
    EKeyC = 0x43,
    EKeyD = 0x44,
    EKeyE = 0x45,
    EKeyF = 0x46,
    EKeyG = 0x47,
    EKeyH = 0x48,
    EKeyI = 0x49,
    EKeyJ = 0x4a,
    EKeyK = 0x4b,
    EKeyL = 0x4c,
    EKeyM = 0x4d,
    EKeyN = 0x4e,
    EKeyO = 0x4f,
    EKeyP = 0x50,
    EKeyQ = 0x51,
    EKeyR = 0x52,
    EKeyS = 0x53,
    EKeyT = 0x54,
    EKeyU = 0x55,
    EKeyV = 0x56,
    EKeyW = 0x57,
    EKeyX = 0x58,
    EKeyY = 0x59,
    EKeyZ = 0x5a,

    EKeyLWin = 0x5b,
    EKeyRWin = 0x5c,
    EKeyApps = 0x5d,
    EKeySleep = 0x5f,
    EKeyNumPad0 = 0x60,
    EKeyNumPad1 = 0x61,
    EKeyNumPad2 = 0x62,
    EKeyNumPad3 = 0x63,
    EKeyNumPad4 = 0x64,
    EKeyNumPad5 = 0x65,
    EKeyNumPad6 = 0x66,
    EKeyNumPad7 = 0x67,
    EKeyNumPad8 = 0x68,
    EKeyNumPad9 = 0x69,
    EKeyMultiply = 0x6a,
    EKeyAdd = 0x6b,
    EKeySeparator = 0x6c,
    EKeySubtract = 0x6d,
    EKeyDecimal = 0x6e,
    EKeyDivide = 0x6f,
    EKeyF1 = 0x70,
    EKeyF2 = 0x71,
    EKeyF3 = 0x72,
    EKeyF4 = 0x73,
    EKeyF5 = 0x74,
    EKeyF6 = 0x75,
    EKeyF7 = 0x76,
    EKeyF8 = 0x77,
    EKeyF9 = 0x78,
    EKeyF10 = 0x79,
    EKeyF11 = 0x7a,
    EKeyF12 = 0x7b,
    EKeyF13 = 0x7c,
    EKeyF14 = 0x7d,
    EKeyF15 = 0x7e,
    EKeyF16 = 0x7f,
    EKeyF17 = 0x80,
    EKeyF18 = 0x81,
    EKeyF19 = 0x82,
    EKeyF20 = 0x83,
    EKeyF21 = 0x84,
    EKeyF22 = 0x85,
    EKeyF23 = 0x86,
    EKeyF24 = 0x87,
    EKeyNumLock = 0x90,
    EKeyScroll = 0x91,
    EKeyLShift = 0xa0,
    EKeyRShift = 0xa1,
    EKeyLControl = 0xa2,
    EKeyRControl = 0xa3,
    EKeyLMenu = 0xa4,
    EKeyRMenu = 0xa5,
    EKeyBrowserBack = 0xa6,
    EKeyBrowserForward = 0xa7,
    EKeyBrowserRefresh = 0xa8,
    EKeyBrowserStop = 0xa9,
    EKeyBrowserSearch = 0xaa,
    EKeyBrowserFavorites = 0xab,
    EKeyBrowserHome = 0xac,
    EKeyVolumeMute = 0xad,
    EKeyVolumeDown = 0xae,
    EKeyVolumeUp = 0xaf,
    EKeyMediaNextTrack = 0xb0,
    EKeyMediaPrevTrack = 0xb1,
    EKeyMediaStop = 0xb2,
    EKeyMediaPlayPause = 0xb3,
    EKeyMediaLaunchMail = 0xb4,
    EKeyMediaLaunchMediaselect = 0xb5,
    EKeyMediaLaunchApp1 = 0xb6,
    EKeyMediaLaunchApp2 = 0xb7,

    EKeyOem1 = 0xba,
    EKeyOemPlus = 0xbb,
    EKeyOemComma = 0xbc,
    EKeyOemMinus = 0xbd,
    EKeyOemPeriod = 0xbe,
    EKeyOem2 = 0xbf,
    EKeyOem3 = 0xc0,
    EKeyOem4 = 0xdb,
    EKeyOem5 = 0xdc,
    EKeyOem6 = 0xdd,
    EKeyOem7 = 0xde,
    EKeyOem8 = 0xdf,

    EKeyOem102 = 0xe2,
    EKeyProcessKey = 0xe5,
    EKeyPacket = 0xe7,

    EKeyOemReset = 0xe9,
    EKeyOemJump = 0xea,
    EKeyOemPA1 = 0xeb,
    EKeyOemPA2 = 0xec,
    EKeyOemPA3 = 0xed,
    EKeyOemWsctrl = 0xee,
    EKeyOemCusel = 0xef,
    EKeyOemAttn = 0xf0,
    EKeyOemFinish = 0xf1,
    EKeyOemCopy = 0xf2,
    EKeyOemAuto = 0xf3,
    EKeyOemEnlw = 0xf4,
    EKeyOemBackTab = 0xf5,

    EKeyAttn = 0xf6,
    EKeyCrSel = 0xf7,
    EKeyExSel = 0xf8,
    EKeyErEOF = 0xf9,
    EKeyPlay = 0xfa,
    EKeyZoom = 0xfb,
    EKeyNoName = 0xfc,
    EKeyPA1 = 0xfd,
    EKeyOemClear = 0xfe,

    EKeyUnknown,

    EKeyEnter = EKeyReturn,

    EKeys
};

/*@}*/

} // namespace

#endif // WKCEnumKeys_h
