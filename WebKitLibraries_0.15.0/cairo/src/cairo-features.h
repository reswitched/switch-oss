#ifndef CAIRO_FEATURES_H
#define CAIRO_FEATURES_H 1

#define CAIRO_HAS_IMAGE_SURFACE 1
#define CAIRO_HAS_PNG_FUNCTIONS 1

#ifdef USE_CAIROGL
#define CAIRO_HAS_GLESV2_SURFACE 1
#define CAIRO_HAS_WGL_FUNCTIONS 1
#endif

#include "wkccairorename.h"

#endif /* CAIRO_FEATURES_H */
