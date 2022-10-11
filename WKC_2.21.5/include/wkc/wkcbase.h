/*
 *  wkcbase.h
 *
 *  Copyright(c) 2009-2014 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKCBASE_H_
#define _WKCBASE_H_

#include <wkc/wkcconfig.h>

#ifdef __cplusplus
# define WKC_BEGIN_C_LINKAGE extern "C" {
# define WKC_END_C_LINKAGE }
#else
# define WKC_BEGIN_C_LINKAGE
# define WKC_END_C_LINKAGE

#endif /* __cplusplus */

/**
   @file
   @brief base definitions for WKC
*/

WKC_BEGIN_C_LINKAGE

#define WKC_PI      (3.141592653589793f)
#define WKC_INT_MAX (2147483647)

typedef struct WKCPoint_ {
    int fX;
    int fY;
} WKCPoint;

typedef struct WKCSize_ {
    int fWidth;
    int fHeight;
} WKCSize;

typedef struct WKCRect_ {
    int fX;
    int fY;
    int fWidth;
    int fHeight;
} WKCRect;

typedef struct WKCFloatPoint_ {
    float fX;
    float fY;
} WKCFloatPoint;

typedef struct WKCFloatSize_ {
    float fWidth;
    float fHeight;
} WKCFloatSize;

typedef struct WKCFloatRect_ {
    float fX;
    float fY;
    float fWidth;
    float fHeight;
} WKCFloatRect;


#define WKCRect_SetRect(__dr,__sr) \
  do { (__dr)->fX=(__sr)->fX; (__dr)->fY=(__sr)->fY; (__dr)->fWidth=(__sr)->fWidth; (__dr)->fHeight=(__sr)->fHeight; } while(0);
#define WKCRect_MakeEmpty(__r) \
  do { (__r)->fX = (__r)->fY = (__r)->fWidth = (__r)->fHeight = 0; } while (0);
#define WKCRect_SetXYWH(__r,__x,__y,__w,__h) \
  do { (__r)->fX = (__x); (__r)->fY = (__y); (__r)->fWidth=(__w); (__r)->fHeight=(__h); } while (0);
#define WKCRect_SetSize(__r,__w,__h) \
  do { (__r)->fWidth=(__w); (__r)->fHeight=(__h); } while (0);
#define WKCRect_Move(__r,__diff) \
  do { (__r)->fX+=(__diff)->fWidth; (__r)->fY+=(__diff)->fHeight; } while (0);

#define WKCFloatRect_SetRect(__dr,__sr) \
  do { (__dr)->fX=(__sr)->fX; (__dr)->fY=(__sr)->fY; (__dr)->fWidth=(__sr)->fWidth; (__dr)->fHeight=(__sr)->fHeight; } while(0);
#define WKCFloatRect_MakeEmpty(__r) \
  do { (__r)->fX = (__r)->fY = (__r)->fWidth = (__r)->fHeight = 0; } while (0);
#define WKCFloatRect_SetXYWH(__r,__x,__y,__w,__h) \
  do { (__r)->fX = (__x); (__r)->fY = (__y); (__r)->fWidth=(__w); (__r)->fHeight=(__h); } while (0);
#define WKCFloatRect_SetSize(__r,__w,__h) \
  do { (__r)->fWidth=(__w); (__r)->fHeight=(__h); } while (0);
#define WKCFloatRect_Move(__r,__diff) \
  do { (__r)->fX+=(__diff)->fWidth; (__r)->fY+=(__diff)->fHeight; } while (0);

WKC_PEER_API int WKCRect_Intersects(const WKCRect* in_rect1, const WKCRect* in_rect2);
WKC_PEER_API void WKCRect_Intersect(const WKCRect* in_src1, const WKCRect* in_src2, WKCRect* out_dest);
WKC_PEER_API int WKCRect_IsEmpty(const WKCRect* in_rect);
WKC_PEER_API int WKCRect_Contains(const WKCRect* in_rect, int in_x, int in_y);
WKC_PEER_API bool WKCRect_ContainRect(const WKCRect* in_rect, const WKCRect* in_child);
WKC_PEER_API void WKCRect_UnionRect(const WKCRect* in_rect1, const WKCRect* in_rect2, WKCRect* out_rect);

WKC_PEER_API int WKCFloatRect_Intersects(const WKCFloatRect* in_rect1, const WKCFloatRect* in_rect2);
WKC_PEER_API void WKCFloatRect_Intersect(const WKCFloatRect* in_src1, const WKCFloatRect* in_src2, WKCFloatRect* out_dest);
WKC_PEER_API int WKCFloatRect_IsEmpty(const WKCFloatRect* in_rect);
WKC_PEER_API int WKCFloatRect_Contains(const WKCFloatRect* in_rect, float in_x, float in_y);
WKC_PEER_API bool WKCFloatRect_ContainRect(const WKCFloatRect* in_rect, const WKCFloatRect* in_child);
WKC_PEER_API void WKCFloatRect_UnionRect(const WKCFloatRect* in_rect1, const WKCFloatRect* in_rect2, WKCFloatRect* out_rect);

#define WKCSize_Set(__s,__w,__h)     do { (__s)->fWidth=(__w); (__s)->fHeight=(__h); } while(0);
#define WKCSize_SetSize(__ds,__ss)   do { (__ds)->fWidth=(__ss)->fWidth; (__ds)->fHeight=(__ss)->fHeight; } while(0);
#define WKCPoint_Set(__p,__x,__y)    do { (__p)->fX=(__x); (__p)->fY=(__y); } while(0);
#define WKCPoint_SetPoint(__dp,__sp) do { (__dp)->fX=(__sp)->fX; (__dp)->fY=(__sp)->fY; } while(0);
#define WKCPoint_Move(__p,__dx,__dy) do { (__p)->fX+=(__dx); (__p)->fY+=(__dy); } while (0);

#define WKCFloatSize_Set(__s,__w,__h)     do { (__s)->fWidth=(__w); (__s)->fHeight=(__h); } while(0);
#define WKCFloatSize_SetSize(__ds,__ss)   do { (__ds)->fWidth=(__ss)->fWidth; (__ds)->fHeight=(__ss)->fHeight; } while(0);
#define WKCFloatPoint_Set(__p,__x,__y)    do { (__p)->fX=(__x); (__p)->fY=(__y); } while(0);
#define WKCFloatPoint_SetPoint(__dp,__sp) do { (__dp)->fX=(__sp)->fX; (__dp)->fY=(__sp)->fY; } while(0);
#define WKCFloatPoint_Move(__p,__dx,__dy) do { (__p)->fX+=(__dx); (__p)->fY+=(__dy); } while (0);

#define WKC_MIN(__a,__b) ((__a)>(__b) ? (__b) : (__a))
#define WKC_MAX(__a,__b) ((__a)>(__b) ? (__a) : (__b))
#define WKC_ABS(__a) (((__a)>=0) ? (__a) : -(__a))

#define WKC_CLIP256(__a) (((__a)<0)?0 :(((__a)>255)?255:(__a)))
#define WKC_CLIPU256(__a) ((__a)>255?255:(__a))

// for debugging
#define TIME_START ((void)0);
#define TIME_END1 ((void)0);
#define TIME_END2 ((void)0);
#define TIME_END ((void)0);

WKC_END_C_LINKAGE

#endif /* _WKCBASE_H_ */
