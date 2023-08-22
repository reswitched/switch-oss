/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "EGLHelper.h"

#if USE(EGL)

#include "PlatformDisplay.h"
#include <opengl/GLPlatformContext.h>


namespace WebCore {

static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = 0;
static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = 0;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC eglImageTargetTexture2DOES = 0;

EGLDisplay EGLHelper::eglDisplay()
{
    PlatformDisplay::sharedDisplay().eglDisplay();
}

EGLDisplay EGLHelper::currentDisplay()
{
    EGLDisplay display = eglGetCurrentDisplay();

    if (display == EGL_NO_DISPLAY)
        display = EGLHelper::eglDisplay();

    return display;
}

void EGLHelper::resolveEGLBindings()
{
    static bool initialized = false;

    if (initialized)
        return;

    initialized = true;

    EGLDisplay display = currentDisplay();

    if (display == EGL_NO_DISPLAY)
        return;

    if (GLPlatformContext::supportsEGLExtension(display, "EGL_KHR_image") && GLPlatformContext::supportsEGLExtension(display, "EGL_KHR_image_pixmap") && GLPlatformContext::supportsGLExtension("GL_OES_EGL_image")) {
        eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
        eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");
        eglImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
    }
}

void EGLHelper::createEGLImage(EGLImageKHR* image, GLenum target, const EGLClientBuffer clientBuffer, const EGLint attributes[])
{
    EGLDisplay display = currentDisplay();

    if (display == EGL_NO_DISPLAY)
        return;

    EGLImageKHR tempHandle = EGL_NO_IMAGE_KHR;

    if (eglCreateImageKHR && eglImageTargetTexture2DOES && eglDestroyImageKHR)
        tempHandle = eglCreateImageKHR(display, EGL_NO_CONTEXT, target, clientBuffer, attributes);

    *image = tempHandle;
}

void EGLHelper::destroyEGLImage(const EGLImageKHR image)
{
    EGLDisplay display = currentDisplay();

    if (display == EGL_NO_DISPLAY)
        return;

    if (eglDestroyImageKHR)
        eglDestroyImageKHR(display, image);
}

void EGLHelper::imageTargetTexture2DOES(const EGLImageKHR image)
{
    eglImageTargetTexture2DOES(GL_TEXTURE_2D, static_cast<GLeglImageOES>(image));
}

}
#endif
