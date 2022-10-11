/*
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2011 ProFUSION embedded systems
 * Copyright (C) 2019 Apple Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if USE(DIRECT2D)

#include "COMPtr.h"
#include "GraphicsTypes.h"
#include "IntSize.h"
#include "PlatformExportMacros.h"

interface ID2D1Bitmap;
interface ID2D1BitmapRenderTarget;
interface ID2D1DCRenderTarget;
interface ID2D1RenderTarget;
interface ID3D11Device1;
interface ID3D11DeviceContext1;
interface ID3D11RenderTargetView;
interface IDXGIDevice1;
interface IDXGIFactory2;
interface IDXGISwapChain;
interface IDXGISurface1;
interface IWICBitmapSource;
interface IWICBitmap;

struct D2D1_BITMAP_PROPERTIES;
struct D2D1_PIXEL_FORMAT;
struct D2D1_RENDER_TARGET_PROPERTIES;
struct D3D11_VIEWPORT;

namespace WebCore {

class FloatPoint;
class FloatSize;
class IntRect;
class IntSize;

namespace Direct2D {

GUID wicBitmapFormat();
D2D1_PIXEL_FORMAT pixelFormat(); // BGRA
D2D1_PIXEL_FORMAT pixelFormatForSoftwareManipulation(); // RGBA
D2D1_BITMAP_PROPERTIES bitmapProperties();
D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties();

void inPlaceSwizzle(uint8_t* byteData, unsigned length, bool applyPremultiplication = false);

IntSize bitmapSize(IWICBitmapSource*);
FloatSize bitmapSize(ID2D1Bitmap*);
FloatSize bitmapResolution(IWICBitmapSource*);
FloatSize bitmapResolution(ID2D1Bitmap*);
FloatSize bitmapResolution(ID2D1RenderTarget*);
unsigned bitsPerPixel(GUID);
COMPtr<ID2D1Bitmap> createBitmap(ID2D1RenderTarget*, const IntSize&);
COMPtr<IWICBitmap> createWicBitmap(const IntSize&);
COMPtr<IWICBitmap> createDirect2DImageSurfaceWithData(void* data, const IntSize&, unsigned stride);
COMPtr<ID2D1RenderTarget> createRenderTargetFromWICBitmap(IWICBitmap*);
COMPtr<ID2D1BitmapRenderTarget> createBitmapRenderTargetOfSize(const IntSize&, ID2D1RenderTarget* = nullptr, float deviceScaleFactor = 1.0);
COMPtr<ID2D1BitmapRenderTarget> createBitmapRenderTarget(ID2D1RenderTarget* = nullptr);
COMPtr<ID2D1DCRenderTarget> createGDIRenderTarget();
COMPtr<IDXGISurface1> createDXGISurfaceOfSize(const IntSize&, ID3D11Device1*, bool crossProcess);
COMPtr<ID2D1RenderTarget> createSurfaceRenderTarget(IDXGISurface1*);
COMPtr<ID2D1Bitmap> createBitmapCopyFromContext(ID2D1BitmapRenderTarget*);

void copyRectFromOneSurfaceToAnother(ID2D1Bitmap* from, ID2D1Bitmap* to, const IntSize& sourceOffset, const IntRect&, const IntSize& destOffset = IntSize());

WEBCORE_EXPORT ID3D11DeviceContext1* dxgiImmediateContext();
WEBCORE_EXPORT ID3D11Device1* defaultDirectXDevice();
WEBCORE_EXPORT bool createDeviceAndContext(COMPtr<ID3D11Device1>&, COMPtr<ID3D11DeviceContext1>&);
WEBCORE_EXPORT COMPtr<IDXGIDevice1> toDXGIDevice(const COMPtr<ID3D11Device1>&);
WEBCORE_EXPORT COMPtr<IDXGIFactory2> factoryForDXGIDevice(const COMPtr<IDXGIDevice1>&);
WEBCORE_EXPORT COMPtr<IDXGISwapChain> swapChainOfSizeForWindowAndDevice(const WebCore::IntSize&, HWND, const COMPtr<ID3D11Device1>&);

void writeDiagnosticPNGToPath(ID2D1RenderTarget*, ID2D1Bitmap*, LPCWSTR fileName);

} // namespace Direct2D

} // namespace WebCore

#endif // USE(DIRECT2D)
