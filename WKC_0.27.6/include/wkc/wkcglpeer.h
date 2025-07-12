/*
 *  wkcglpeer.h
 *
 *  Copyright(c) 2009-2017 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKC_GL_PEER_H_
#define _WKC_GL_PEER_H_

#include <wkc/wkcbase.h>
#include <stdint.h>

/**
   @file
   @brief OpenGL / WebGL related peers.
   @details
   All typedefs of wkcGLXXXX and functions wkcGLXXXXPeer() which are not described in this document should be
   1-1 matching for each identical OpenGL ES 2.0 APIs.@n
   And also refer GraphicsContext3D::getExtensions() for all wkcGLExtXXXPeer().
 */
/*@{*/

WKC_BEGIN_C_LINKAGE

typedef unsigned int wkcGLenum;
typedef unsigned char wkcGLboolean;
typedef unsigned int wkcGLbitfield;
typedef signed char wkcGLbyte;
typedef unsigned char wkcGLubyte;
typedef short wkcGLshort;
typedef unsigned short wkcGLushort;
typedef int wkcGLint;
typedef int wkcGLsizei;
typedef unsigned int wkcGLuint;
typedef float wkcGLfloat;
typedef float wkcGLclampf;
typedef intptr_t wkcGLintptr;
typedef intptr_t wkcGLsizeiptr;
typedef wkcGLuint wkcGLobject;

/** @brief Structure that stores GL attributes */
struct WKCGLAttributes_ {
    bool fAlpha;
    bool fDepth;
    bool fStencil;
    bool fAntialias;
    bool fPremultipliedAlpha;
    bool fPreserveDrawingBuffer;
    bool fNoExtensions;
    bool fShareResources;
    bool fPreferLowPowerToHighPerformance;
    bool fForceSoftwareRenderer;
    bool fFailIfMajorPerformanceCaveat;
    bool fUseGLES3;
    float fDevicePixelRatio;
};
/** @brief Type definitions of WKCGLAttributes */
typedef struct WKCGLAttributes_ WKCGLAttributes;


/** @brief callback definitions for wkcGLRegisterTextureCallbacksPeer */
typedef bool (*wkcGLTextureMakeProc)(int num, unsigned int* out_textures);
typedef void (*wkcGLTextureDeleteProc)(int num, unsigned int* in_textures);
typedef bool (*wkcGLTextureChangeProc)(int num, unsigned int* inout_textures);

/** @brief callback definitions for wkcGLRegisterOffscreenBufferCallbacksPeer */
typedef bool (*wkcGLOffscreenBufferMakeProc)(int in_width, int in_height, int in_bpp, void** out_buffer);
typedef void (*wkcGLOffscreenBufferDeleteProc)(void* in_buffer);
typedef bool (*wkcGLOffscreenBufferChangeProc)(int in_width, int in_height, int in_bpp, void** inout_buffer);

/*
  initialize / finalize / force-terminate GL peers
*/
/**
@brief Initializes GL peer
@retval !=false Succeeded
@retval ==false Failed
@details
Write the necessary processes for initializing the GL peer layer.
*/
WKC_PEER_API bool wkcGLInitializePeer(void);

/**
@brief Enable GL peer
@retval !=false Succeeded
@retval ==false Failed
@details
This API must be called after wkcGLInitializePeer
*/
WKC_PEER_API bool wkcGLActivatePeer();

/**
@brief Registeres callbacks for texture creation in peers
@details
This API must call before loading any WebGL contents.
*/
WKC_PEER_API void wkcGLRegisterTextureCallbacksPeer(
        wkcGLTextureMakeProc in_texture_maker_proc,
        wkcGLTextureDeleteProc in_texture_deleter_proc,
        wkcGLTextureChangeProc in_texture_changer_proc
        );

/**
@brief Registeres callbacks for offscreen buffer for GL peers
@details
This API must call before loading any WebGL contents.
This buffer is used for showing gl texture without AC layers.
*/
WKC_PEER_API void wkcGLRegisterOffscreenBufferCallbacksPeer(
        wkcGLOffscreenBufferMakeProc in_buf_creater_proc,
        wkcGLOffscreenBufferDeleteProc in_buf_deleter_proc,
        wkcGLOffscreenBufferChangeProc in_buf_changer_proc
        );

/**
@brief Finalizes GL peer
@details
Write the necessary processes for finalizing the GL peer layer.
*/
WKC_PEER_API void wkcGLFinalizePeer(void);
/**
@brief Forcibly terminates GL peer
@details
Write the necessary processes for forcibly terminating the GL peer layer.
*/
WKC_PEER_API void wkcGLForceTerminatePeer(void);

/**
@brief Registers HW device lock / unlock procs
@param in_lock callback for locking device
@param in_unlock callback for unlocking device
@param in_opaque opaque value for each callbacks
@details
See WKCWebKitSetHWOffscreenDeviceParams().
*/
WKC_PEER_API void wkcGLRegisterDeviceLockProcsPeer(void(*in_lock)(void*), void(*in_unlock)(void*), void* in_opaque);

/**
@brief Create GL peer instances
@param inout_attr HW attributes
@param in_hostwindow WebCore::HostWindow itself
@param in_layer WKCGenericLayer
@details
Creates GL context.
*/
WKC_PEER_API void* wkcGLCreateContextPeer(WKCGLAttributes* inout_attr, void* in_hostwindow, void* in_layer);
/**
@brief Delete GL peer instances
@param in_context context
@details
Deletes GL context.
*/
WKC_PEER_API void wkcGLDeleteContextPeer(void* in_context);

/**
@brief Make context as current context
@param in_context context
@details
It will be called before each GL APIs.
*/
WKC_PEER_API void wkcGLMakeContextCurrentPeer(void* in_context);

/**
@brief Determine whether HW is GLES 2.0 compliant
@param in_context context
@retval !=false Supported
@retval ==false Not supported
@attention
webcore won't support non-GLES2.0 compliant devices.@n
all implementations must return true at this time...
*/
WKC_PEER_API bool wkcGLIsGLES20CompliantPeer(void* in_context);

/**
@brief Sets error code for next calling of glGetError
@param in_context context
@param error error code
*/
WKC_PEER_API void wkcGLSynthesizeGLErrorPeer(void* in_context, wkcGLenum error);

/**
@brief Returns internal frame-buffer size
@param in_context context
@param out_size size of internal frame-buffer
*/
WKC_PEER_API void wkcGLGetInternalFramebufferSizePeer(void* in_context, WKCSize* out_size);

/**
@brief Determine whether rendered image should be transfer to canvas
@param in_context context
@retval !=false transfer to canvas
@retval ==false not transfer
*/
WKC_PEER_API bool wkcGLPaintsIntoCanvasBufferPeer(void* in_context);

/**
@brief Locks rendered image
@param in_self context
@param out_fmt format
@param out_rowbytes row-bytes
@param out_mask mask
@param out_maskrowbytes mask row-bytes
@param out_size size
@retval pointer to locked image
*/
WKC_PEER_API void* wkcGLLockImagePeer(void* in_self, int* out_fmt, int* out_rowbytes, void** out_mask, int* out_maskrowbytes, WKCSize* out_size);

/**
@brief Unlocks rendered image
@param in_self context
@param image previously locked image
*/
WKC_PEER_API void wkcGLUnlockImagePeer(void* in_self, void* image);

/**
@brief Obtain destination texture
@param in_self context
@return texture Id
*/
WKC_PEER_API wkcGLobject wkcGLDestTexturePeer(void* in_self);

/**
@brief Obtain sizeo of destination texture
@param in_self context
@param out_width width
@param out_height height
*/
WKC_PEER_API void wkcGLGetDestTextureSizePeer(void* in_self, unsigned int* out_width, unsigned int* out_height);

/**
@brief Notify start of painting
*/
WKC_PEER_API void wkcGLBeginPaintPeer(void);
/**
@brief Notify end of painting
*/
WKC_PEER_API void wkcGLEndPaintPeer(void);

// below peers are 1-1 matching for each identical OpenGL ES 2.0 APIs.
WKC_PEER_API void wkcGLActiveTexturePeer(void* in_context, wkcGLenum texture);
WKC_PEER_API void wkcGLAttachShaderPeer(void* in_context, wkcGLobject program, wkcGLobject shader);
WKC_PEER_API void wkcGLBindAttribLocationPeer(void* in_context, wkcGLobject, wkcGLuint index, const char* name);
WKC_PEER_API void wkcGLBindBufferPeer(void* in_context, wkcGLenum target, wkcGLobject);
WKC_PEER_API void wkcGLBindFramebufferPeer(void* in_context, wkcGLenum target, wkcGLobject);
WKC_PEER_API void wkcGLBindRenderbufferPeer(void* in_context, wkcGLenum target, wkcGLobject);
WKC_PEER_API void wkcGLBindTexturePeer(void* in_context, wkcGLenum target, wkcGLobject);
WKC_PEER_API void wkcGLBlendColorPeer(void* in_context, wkcGLclampf red, wkcGLclampf green, wkcGLclampf blue, wkcGLclampf alpha);
WKC_PEER_API void wkcGLBlendEquationPeer(void* in_context, wkcGLenum mode);
WKC_PEER_API void wkcGLBlendEquationSeparatePeer(void* in_context, wkcGLenum modeRGB, wkcGLenum modeAlpha);
WKC_PEER_API void wkcGLBlendFuncPeer(void* in_context, wkcGLenum sfactor, wkcGLenum dfactor);
WKC_PEER_API void wkcGLBlendFuncSeparatePeer(void* in_context, wkcGLenum srcRGB, wkcGLenum dstRGB, wkcGLenum srcAlpha, wkcGLenum dstAlpha);
WKC_PEER_API void wkcGLBufferDataPeer(void* in_context, wkcGLenum target, wkcGLsizeiptr size, const void* data, wkcGLenum usage);
WKC_PEER_API void wkcGLBufferSubDataPeer(void* in_context, wkcGLenum target, wkcGLintptr offset, wkcGLsizeiptr size, const void* data);
WKC_PEER_API wkcGLenum wkcGLCheckFramebufferStatusPeer(void* in_context, wkcGLenum target);
WKC_PEER_API void wkcGLClearPeer(void* in_context, wkcGLbitfield mask);
WKC_PEER_API void wkcGLClearColorPeer(void* in_context, wkcGLclampf red, wkcGLclampf green, wkcGLclampf blue, wkcGLclampf alpha);
WKC_PEER_API void wkcGLClearDepthPeer(void* in_context, wkcGLclampf depth);
WKC_PEER_API void wkcGLClearStencilPeer(void* in_context, wkcGLint s);
WKC_PEER_API void wkcGLColorMaskPeer(void* in_context, wkcGLboolean red, wkcGLboolean green, wkcGLboolean blue, wkcGLboolean alpha);
WKC_PEER_API void wkcGLCompileShaderPeer(void* in_context, wkcGLobject);
WKC_PEER_API void wkcGLCompressedTexImage2DPeer(void* in_context, wkcGLenum target, wkcGLint level, wkcGLenum internalformat, wkcGLsizei width, wkcGLsizei height, wkcGLint border, wkcGLsizei imageSize, const void* data);
WKC_PEER_API void wkcGLCompressedTexSubImage2DPeer(void* in_context, wkcGLenum target, wkcGLint level, wkcGLint xoffset, wkcGLint yoffset, wkcGLsizei width, wkcGLsizei height, wkcGLenum format, wkcGLsizei imageSize, const void* data);
WKC_PEER_API void wkcGLCopyTexImage2DPeer(void* in_context, wkcGLenum target, wkcGLint level, wkcGLenum internalformat, wkcGLint x, wkcGLint y, wkcGLsizei width, wkcGLsizei height, wkcGLint border);
WKC_PEER_API void wkcGLCopyTexSubImage2DPeer(void* in_context, wkcGLenum target, wkcGLint level, wkcGLint xoffset, wkcGLint yoffset, wkcGLint x, wkcGLint y, wkcGLsizei width, wkcGLsizei height);
WKC_PEER_API void wkcGLCullFacePeer(void* in_context, wkcGLenum mode);
WKC_PEER_API void wkcGLDepthFuncPeer(void* in_context, wkcGLenum func);
WKC_PEER_API void wkcGLDepthMaskPeer(void* in_context, wkcGLboolean flag);
WKC_PEER_API void wkcGLDepthRangePeer(void* in_context, wkcGLclampf zNear, wkcGLclampf zFar);
WKC_PEER_API void wkcGLDetachShaderPeer(void* in_context, wkcGLobject, wkcGLobject);
WKC_PEER_API void wkcGLDisablePeer(void* in_context, wkcGLenum cap);
WKC_PEER_API void wkcGLDisableVertexAttribArrayPeer(void* in_context, wkcGLuint index);
WKC_PEER_API void wkcGLDrawArraysPeer(void* in_context, wkcGLenum mode, wkcGLint first, wkcGLsizei count);
WKC_PEER_API void wkcGLDrawBuffersPeer(void* in_context, wkcGLsizei n, const wkcGLenum* bufs);
WKC_PEER_API void wkcGLDrawElementsPeer(void* in_context, wkcGLenum mode, wkcGLsizei count, wkcGLenum type, wkcGLintptr offset);
WKC_PEER_API void wkcGLEnablePeer(void* in_context, wkcGLenum cap);
WKC_PEER_API void wkcGLEnableVertexAttribArrayPeer(void* in_context, wkcGLuint index);
WKC_PEER_API void wkcGLFinishPeer(void* in_context);
WKC_PEER_API void wkcGLFlushPeer(void* in_context);
WKC_PEER_API void wkcGLFramebufferRenderbufferPeer(void* in_context, wkcGLenum target, wkcGLenum attachment, wkcGLenum renderbuffertarget, wkcGLobject);
WKC_PEER_API void wkcGLFramebufferTexture2DPeer(void* in_context, wkcGLenum target, wkcGLenum attachment, wkcGLenum textarget, wkcGLobject, wkcGLint level);
WKC_PEER_API void wkcGLFrontFacePeer(void* in_context, wkcGLenum mode);
WKC_PEER_API void wkcGLGenerateMipmapPeer(void* in_context, wkcGLenum target);
WKC_PEER_API void wkcGLGetActiveAttribPeer(void* in_context, wkcGLobject program, wkcGLuint index, wkcGLsizei bufsize, wkcGLsizei* length, wkcGLint* size, wkcGLenum* type, char* name);
WKC_PEER_API void wkcGLGetActiveUniformPeer(void* in_context, wkcGLobject program, wkcGLuint index, wkcGLsizei bufsize, wkcGLsizei* length, wkcGLint* size, wkcGLenum* type, char* name);
WKC_PEER_API void wkcGLGetAttachedShadersPeer(void* in_context, wkcGLobject program, wkcGLsizei maxCount, wkcGLsizei* count, wkcGLobject* shaders);
WKC_PEER_API wkcGLint wkcGLGetAttribLocationPeer(void* in_context, wkcGLobject, const char* name);
WKC_PEER_API void wkcGLGetBooleanvPeer(void* in_context, wkcGLenum pname, wkcGLboolean* value);
WKC_PEER_API void wkcGLGetBufferParameterivPeer(void* in_context, wkcGLenum target, wkcGLenum pname, wkcGLint* value);
WKC_PEER_API wkcGLenum wkcGLGetErrorPeer(void* in_context);
WKC_PEER_API void wkcGLGetFloatvPeer(void* in_context, wkcGLenum pname, wkcGLfloat* value);
WKC_PEER_API void wkcGLGetFramebufferAttachmentParameterivPeer(void* in_context, wkcGLenum target, wkcGLenum attachment, wkcGLenum pname, wkcGLint* value);
WKC_PEER_API void wkcGLGetIntegervPeer(void* in_context, wkcGLenum pname, wkcGLint* value);
WKC_PEER_API void wkcGLGetProgramivPeer(void* in_context, wkcGLobject program, wkcGLenum pname, wkcGLint* value);
WKC_PEER_API void wkcGLGetProgramInfoLogPeer(void* in_context, wkcGLobject, wkcGLsizei maxLength, wkcGLsizei* length, char* infoLog);
WKC_PEER_API void wkcGLGetRenderbufferParameterivPeer(void* in_context, wkcGLenum target, wkcGLenum pname, wkcGLint* value);
WKC_PEER_API void wkcGLGetShaderivPeer(void* in_context, wkcGLobject, wkcGLenum pname, wkcGLint* value);
WKC_PEER_API void wkcGLGetShaderInfoLogPeer(void* in_context, wkcGLobject, wkcGLsizei maxLength, wkcGLsizei* length, char* infoLog);
WKC_PEER_API void wkcGetShaderPrecisionFormatPeer(void* in_context, wkcGLenum shaderType, wkcGLenum precisionType, wkcGLint* range, wkcGLint* precision);
WKC_PEER_API void wkcGLGetShaderSourcePeer(void* in_context, wkcGLobject, wkcGLsizei maxLength, wkcGLsizei* length, char* infoLog);
WKC_PEER_API const wkcGLubyte* wkcGLGetStringPeer(void* in_context, wkcGLenum name);
WKC_PEER_API void wkcGLGetTexParameterfvPeer(void* in_context, wkcGLenum target, wkcGLenum pname, wkcGLfloat* value);
WKC_PEER_API void wkcGLGetTexParameterivPeer(void* in_context, wkcGLenum target, wkcGLenum pname, wkcGLint* value);
WKC_PEER_API void wkcGLGetUniformfvPeer(void* in_context, wkcGLobject program, wkcGLint location, wkcGLfloat* value);
WKC_PEER_API void wkcGLGetUniformivPeer(void* in_context, wkcGLobject program, wkcGLint location, wkcGLint* value);
WKC_PEER_API wkcGLint wkcGLGetUniformLocationPeer(void* in_context, wkcGLobject, const char* name);
WKC_PEER_API void wkcGLGetVertexAttribfvPeer(void* in_context, wkcGLuint index, wkcGLenum pname, wkcGLfloat* value);
WKC_PEER_API void wkcGLGetVertexAttribivPeer(void* in_context, wkcGLuint index, wkcGLenum pname, wkcGLint* value);
WKC_PEER_API wkcGLsizeiptr wkcGLGetVertexAttribOffsetPeer(void* in_context, wkcGLuint index, wkcGLenum pname);
WKC_PEER_API void wkcGLGetnUniformfvPeer(void* in_context, wkcGLuint program, wkcGLint location, wkcGLsizei bufSize, wkcGLfloat *params);
WKC_PEER_API void wkcGLGetnUniformivPeer(void* in_context, wkcGLuint program, wkcGLint location, wkcGLsizei bufSize, wkcGLint *params);
WKC_PEER_API void wkcGLHintPeer(void* in_context, wkcGLenum target, wkcGLenum mode);
WKC_PEER_API wkcGLboolean wkcGLIsBufferPeer(void* in_context, wkcGLobject);
WKC_PEER_API wkcGLboolean wkcGLIsEnabledPeer(void* in_context, wkcGLenum cap);
WKC_PEER_API wkcGLboolean wkcGLIsFramebufferPeer(void* in_context, wkcGLobject);
WKC_PEER_API wkcGLboolean wkcGLIsProgramPeer(void* in_context, wkcGLobject);
WKC_PEER_API wkcGLboolean wkcGLIsRenderbufferPeer(void* in_context, wkcGLobject);
WKC_PEER_API wkcGLboolean wkcGLIsShaderPeer(void* in_context, wkcGLobject);
WKC_PEER_API wkcGLboolean wkcGLIsTexturePeer(void* in_context, wkcGLobject);
WKC_PEER_API void wkcGLLineWidthPeer(void* in_context, wkcGLfloat);
WKC_PEER_API void wkcGLLinkProgramPeer(void* in_context, wkcGLobject);
WKC_PEER_API void wkcGLPixelStoreiPeer(void* in_context, wkcGLenum pname, wkcGLint param);
WKC_PEER_API void wkcGLPolygonOffsetPeer(void* in_context, wkcGLfloat factor, wkcGLfloat units);
WKC_PEER_API void wkcGLReadPixelsPeer(void* in_context, wkcGLint x, wkcGLint y, wkcGLsizei width, wkcGLsizei height, wkcGLenum format, wkcGLenum type, void* data);
WKC_PEER_API void wkcGLReleaseShaderCompilerPeer(void* in_context);
WKC_PEER_API void wkcGLRenderbufferStoragePeer(void* in_context, wkcGLenum target, wkcGLenum internalformat, wkcGLsizei width, wkcGLsizei height);
WKC_PEER_API void wkcGLSampleCoveragePeer(void* in_context, wkcGLclampf value, wkcGLboolean invert);
WKC_PEER_API void wkcGLScissorPeer(void* in_context, wkcGLint x, wkcGLint y, wkcGLsizei width, wkcGLsizei height);
WKC_PEER_API void wkcGLShaderSourcePeer(void* in_context, wkcGLobject, const char* string);
WKC_PEER_API void wkcGLStencilFuncPeer(void* in_context, wkcGLenum func, wkcGLint ref, wkcGLuint mask);
WKC_PEER_API void wkcGLStencilFuncSeparatePeer(void* in_context, wkcGLenum face, wkcGLenum func, wkcGLint ref, wkcGLuint mask);
WKC_PEER_API void wkcGLStencilMaskPeer(void* in_context, wkcGLuint mask);
WKC_PEER_API void wkcGLStencilMaskSeparatePeer(void* in_context, wkcGLenum face, wkcGLuint mask);
WKC_PEER_API void wkcGLStencilOpPeer(void* in_context, wkcGLenum fail, wkcGLenum zfail, wkcGLenum zpass);
WKC_PEER_API void wkcGLStencilOpSeparatePeer(void* in_context, wkcGLenum face, wkcGLenum fail, wkcGLenum zfail, wkcGLenum zpass);
WKC_PEER_API void wkcGLTexImage2DPeer(void* in_context, wkcGLenum target, wkcGLint level, wkcGLenum internalformat, wkcGLsizei width, wkcGLsizei height, wkcGLint border, wkcGLenum format, wkcGLenum type, const void* pixels);
WKC_PEER_API void wkcGLTexParameterfPeer(void* in_context, wkcGLenum target, wkcGLenum pname, wkcGLfloat param);
WKC_PEER_API void wkcGLTexParameteriPeer(void* in_context, wkcGLenum target, wkcGLenum pname, wkcGLint param);
WKC_PEER_API void wkcGLTexSubImage2DPeer(void* in_context, wkcGLenum target, wkcGLint level, wkcGLint xoffset, wkcGLint yoffset, wkcGLsizei width, wkcGLsizei height, wkcGLenum format, wkcGLenum type, const void* pixels);
WKC_PEER_API void wkcGLUniform1fPeer(void* in_context, wkcGLint location, wkcGLfloat x);
WKC_PEER_API void wkcGLUniform1fvPeer(void* in_context, wkcGLint location, wkcGLsizei size, wkcGLfloat* v);
WKC_PEER_API void wkcGLUniform1iPeer(void* in_context, wkcGLint location, wkcGLint x);
WKC_PEER_API void wkcGLUniform1ivPeer(void* in_context, wkcGLint location, wkcGLsizei size, wkcGLint* v);
WKC_PEER_API void wkcGLUniform2fPeer(void* in_context, wkcGLint location, wkcGLfloat x, wkcGLfloat y);
WKC_PEER_API void wkcGLUniform2fvPeer(void* in_context, wkcGLint location, wkcGLsizei size, wkcGLfloat* v);
WKC_PEER_API void wkcGLUniform2iPeer(void* in_context, wkcGLint location, wkcGLint x, wkcGLint y);
WKC_PEER_API void wkcGLUniform2ivPeer(void* in_context, wkcGLint location, wkcGLsizei size, wkcGLint* v);
WKC_PEER_API void wkcGLUniform3fPeer(void* in_context, wkcGLint location, wkcGLfloat x, wkcGLfloat y, wkcGLfloat z);
WKC_PEER_API void wkcGLUniform3fvPeer(void* in_context, wkcGLint location, wkcGLsizei size, wkcGLfloat* v);
WKC_PEER_API void wkcGLUniform3iPeer(void* in_context, wkcGLint location, wkcGLint x, wkcGLint y, wkcGLint z);
WKC_PEER_API void wkcGLUniform3ivPeer(void* in_context, wkcGLint location, wkcGLsizei size, wkcGLint* v);
WKC_PEER_API void wkcGLUniform4fPeer(void* in_context, wkcGLint location, wkcGLfloat x, wkcGLfloat y, wkcGLfloat z, wkcGLfloat w);
WKC_PEER_API void wkcGLUniform4fvPeer(void* in_context, wkcGLint location, wkcGLsizei size, wkcGLfloat* v);
WKC_PEER_API void wkcGLUniform4iPeer(void* in_context, wkcGLint location, wkcGLint x, wkcGLint y, wkcGLint z, wkcGLint w);
WKC_PEER_API void wkcGLUniform4ivPeer(void* in_context, wkcGLint location, wkcGLsizei size, wkcGLint* v);
WKC_PEER_API void wkcGLUniformMatrix2fvPeer(void* in_context, wkcGLint location, wkcGLsizei size, wkcGLboolean transpose, wkcGLfloat* value);
WKC_PEER_API void wkcGLUniformMatrix3fvPeer(void* in_context, wkcGLint location, wkcGLsizei size, wkcGLboolean transpose, wkcGLfloat* value);
WKC_PEER_API void wkcGLUniformMatrix4fvPeer(void* in_context, wkcGLint location, wkcGLsizei size, wkcGLboolean transpose, wkcGLfloat* value);
WKC_PEER_API void wkcGLUseProgramPeer(void* in_context, wkcGLobject);
WKC_PEER_API void wkcGLValidateProgramPeer(void* in_context, wkcGLobject);
WKC_PEER_API void wkcGLVertexAttrib1fPeer(void* in_context, wkcGLuint index, wkcGLfloat x);
WKC_PEER_API void wkcGLVertexAttrib1fvPeer(void* in_context, wkcGLuint index, wkcGLfloat* values);
WKC_PEER_API void wkcGLVertexAttrib2fPeer(void* in_context, wkcGLuint index, wkcGLfloat x, wkcGLfloat y);
WKC_PEER_API void wkcGLVertexAttrib2fvPeer(void* in_context, wkcGLuint index, wkcGLfloat* values);
WKC_PEER_API void wkcGLVertexAttrib3fPeer(void* in_context, wkcGLuint index, wkcGLfloat x, wkcGLfloat y, wkcGLfloat z);
WKC_PEER_API void wkcGLVertexAttrib3fvPeer(void* in_context, wkcGLuint index, wkcGLfloat* values);
WKC_PEER_API void wkcGLVertexAttrib4fPeer(void* in_context, wkcGLuint index, wkcGLfloat x, wkcGLfloat y, wkcGLfloat z, wkcGLfloat w);
WKC_PEER_API void wkcGLVertexAttrib4fvPeer(void* in_context, wkcGLuint index, wkcGLfloat* values);
WKC_PEER_API void wkcGLVertexAttribPointerPeer(void* in_context, wkcGLuint index, wkcGLint size, wkcGLenum type, wkcGLboolean normalized, wkcGLsizei stride, wkcGLintptr offset);
WKC_PEER_API void wkcGLViewportPeer(void* in_context, wkcGLint x, wkcGLint y, wkcGLsizei width, wkcGLsizei height);
WKC_PEER_API void wkcGLReshapePeer(void* in_context, int width, int height);
WKC_PEER_API void wkcGLMarkContextChangedPeer(void* in_context);
WKC_PEER_API void wkcGLMarkLayerCompositedPeer(void* in_context);
WKC_PEER_API bool wkcGLLayerCompositedPeer(void* in_context);
WKC_PEER_API wkcGLobject wkcGLCreateBufferPeer(void* in_context);
WKC_PEER_API wkcGLobject wkcGLCreateFramebufferPeer(void* in_context);
WKC_PEER_API wkcGLobject wkcGLCreateProgramPeer(void* in_context);
WKC_PEER_API wkcGLobject wkcGLCreateRenderbufferPeer(void* in_context);
WKC_PEER_API wkcGLobject wkcGLCreateShaderPeer(void* in_context, wkcGLenum);
WKC_PEER_API wkcGLobject wkcGLCreateTexturePeer(void* in_context);
WKC_PEER_API void wkcGLDeleteBufferPeer(void* in_context, wkcGLobject);
WKC_PEER_API void wkcGLDeleteFramebufferPeer(void* in_context, wkcGLobject);
WKC_PEER_API void wkcGLDeleteProgramPeer(void* in_context, wkcGLobject);
WKC_PEER_API void wkcGLDeleteRenderbufferPeer(void* in_context, wkcGLobject);
WKC_PEER_API void wkcGLDeleteShaderPeer(void* in_context, wkcGLobject);
WKC_PEER_API void wkcGLDeleteTexturePeer(void* in_context, wkcGLobject);

/*
 see: GraphicsContext3D::getExtensions().
*/
WKC_PEER_API bool wkcGLExtSupportsPeer(void* in_context, const char*);
WKC_PEER_API void wkcGLExtEnsureEnabledPeer(void* in_context, const char*);
WKC_PEER_API bool wkcGLExtIsEnabledPeer(void* in_context, const char*);
WKC_PEER_API int wkcGLExtGetGraphicsResetStatusARBPeer(void* in_context);
WKC_PEER_API void wkcGLExtBlitFramebufferPeer(void* in_context, long srcX0, long srcY0, long srcX1, long srcY1, long dstX0, long dstY0, long dstX1, long dstY1, unsigned long mask, unsigned long filter);
WKC_PEER_API void wkcGLExtRenderbufferStorageMultisamplePeer(void* in_context, unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height);
WKC_PEER_API wkcGLobject wkcGLExtCreateVertexArrayOESPeer(void* in_context);
WKC_PEER_API void wkcGLExtDeleteVertexArrayOESPeer(void* in_context, wkcGLobject);
WKC_PEER_API wkcGLboolean wkcGLExtIsVertexArrayOESPeer(void* in_context, wkcGLobject);
WKC_PEER_API void wkcGLExtBindVertexArrayOESPeer(void* in_context, wkcGLobject);
WKC_PEER_API void wkcGLReadnPixelsPeer(void* in_context, wkcGLint x, wkcGLint y, wkcGLsizei width, wkcGLsizei height, wkcGLenum format, wkcGLenum type, wkcGLsizei bufSize, void* data);

WKC_END_C_LINKAGE
/*@}*/

#endif // _WKC_GL_PEER_H_
