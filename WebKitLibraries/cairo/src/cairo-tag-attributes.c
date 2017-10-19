/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2016 Adrian Johnson
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is Adrian Johnson.
 *
 * Contributor(s):
 *	Adrian Johnson <ajohnson@redneon.com>
 */

#include "cairoint.h"

#include "cairo-array-private.h"
#include "cairo-list-inline.h"
#include "cairo-tag-attributes-private.h"

#include <string.h>

typedef enum {
    ATTRIBUTE_BOOL,  /* Either true/false or 1/0 may be used. */
    ATTRIBUTE_INT,
    ATTRIBUTE_FLOAT, /* Decimal separator is in current locale. */
    ATTRIBUTE_STRING, /* Enclose in single quotes. String escapes:
                       *   \'  - single quote
                       *   \\  - backslash
                       */
} attribute_type_t;

typedef struct _attribute_spec {
    const char *name;
    attribute_type_t type;
    int array_size; /* 0 = scalar, -1 = variable size array */
} attribute_spec_t;

/*
 * name [required] Unique name of this destination (UTF-8)
 * x    [optional] x coordinate of destination on page. Default is x coord of
 *                 extents of operations enclosed by the dest begin/end tags.
 * y    [optional] y coordinate of destination on page. Default is y coord of
 *                 extents of operations enclosed by the dest begin/end tags.
 * internal [optional] If true, the name may be optimized out of the PDF where
 *                     possible. Default false.
 */
static attribute_spec_t _dest_attrib_spec[] = {
    { "name",     ATTRIBUTE_STRING },
    { "x",        ATTRIBUTE_FLOAT },
    { "y",        ATTRIBUTE_FLOAT },
    { "internal", ATTRIBUTE_BOOL },
    { NULL }
};

/*
 * rect [optional] One or more rectangles to define link region. Default
 *                 is the extents of the operations enclosed by the link begin/end tags.
 *                 Each rectangle is specified by four array elements: x, y, width, height.
 *                 ie the array size must be a multiple of four.
 *
 * Internal Links
 * --------------
 * either:
 *   dest - name of dest tag in the PDF file to link to (UTF8)
 * or
 *   page - Page number in the PDF file to link to
 *   pos  - [optional] Position of destination on page. Default is 0,0.
 *
 * URI Links
 * ---------
 * uri [required] Uniform resource identifier (ASCII).

 * File Links
 * ----------
 * file - [required] File name of PDF file to link to.
 *   either:
 *     dest - name of dest tag in the PDF file to link to (UTF8)
 *   or
 *     page - Page number in the PDF file to link to
 *     pos  - [optional] Position of destination on page. Default is 0,0.
 */
static attribute_spec_t _link_attrib_spec[] =
{
    { "rect", ATTRIBUTE_FLOAT, -1 },
    { "dest", ATTRIBUTE_STRING },
    { "uri",  ATTRIBUTE_STRING },
    { "file", ATTRIBUTE_STRING },
    { "page", ATTRIBUTE_INT },
    { "pos",  ATTRIBUTE_FLOAT, 2 },
    { NULL }
};

typedef union {
    cairo_bool_t b;
    int i;
    double f;
    char *s;
} attrib_val_t;

typedef struct _attribute {
    char *name;
    attribute_type_t type;
    int array_len; /* 0 = scalar */
    attrib_val_t scalar;
    cairo_array_t array; /* array of attrib_val_t */
    cairo_list_t link;
} attribute_t;

static const char *
skip_space (const char *p)
{
    while (_cairo_isspace (*p))
	p++;

    return p;
}

static const char *
parse_bool (const char *p, cairo_bool_t *b)
{
    if (*p == '1') {
	*b = TRUE;
	return p + 1;
    } else if (*p == '0') {
	*b = FALSE;
	return p + 1;
    } else if (strcmp (p, "true") == 0) {
	*b = TRUE;
	return p + 4;
    } else if (strcmp (p, "false") == 0) {
	*b = FALSE;
	return p + 5;
    }

    return NULL;
}

static const char *
parse_int (const char *p, int *i)
{
    int n;

    if (sscanf(p, "%d%n", i, &n) > 0)
	return p + n;

    return NULL;
}

static const char *
parse_float (const char *p, double *d)
{
    int n;

    if (sscanf(p, "%lf%n", d, &n) > 0)
	return p + n;

    return NULL;
}

static const char *
decode_string (const char *p, int *len, char *s)
{
    if (*p != '\'')
	return NULL;

    p++;
    if (! *p)
	return NULL;

    *len = 0;
    while (*p) {
	if (*p == '\\') {
	    p++;
	    if (*p) {
		if (s)
		    *s++ = *p;
		p++;
		(*len)++;
	    }
	} else if (*p == '\'') {
	    return p + 1;
	} else {
	    if (s)
		*s++ = *p;
	    p++;
	    (*len)++;
	}
    }

    return NULL;
}

static const char *
parse_string (const char *p, char **s)
{
    const char *end;
    int len;

    end = decode_string (p, &len, NULL);
    if (!end)
	return NULL;

    *s = malloc (len + 1);
    decode_string (p, &len, *s);
    (*s)[len] = 0;

    return end;
}

static const char *
parse_scalar (const char *p, attribute_type_t type, attrib_val_t *scalar)
{
    switch (type) {
	case ATTRIBUTE_BOOL:
	    return parse_bool (p, &scalar->b);
	case ATTRIBUTE_INT:
	    return parse_int (p, &scalar->i);
	case ATTRIBUTE_FLOAT:
	    return parse_float (p, &scalar->f);
	case ATTRIBUTE_STRING:
	    return parse_string (p, &scalar->s);
    }

    return NULL;
}

static cairo_int_status_t
parse_array (const char *p, attribute_type_t type, cairo_array_t *array, const char **end)
{
    attrib_val_t val;
    cairo_int_status_t status;

    p = skip_space (p);
    if (! *p)
	return _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);

    if (*p++ != '[')
	return _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);

    while (TRUE) {
	p = skip_space (p);
	if (! *p)
	    return _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);

	if (*p == ']') {
	    *end = p + 1;
	    return CAIRO_INT_STATUS_SUCCESS;
	}

	p = parse_scalar (p, type, &val);
	if (!p)
	    return _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);

	status = _cairo_array_append (array, &val);
	if (unlikely (status))
	    return status;
    }

    return _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);
}

static cairo_int_status_t
parse_name (const char *p, const char **end, char **s)
{
    const char *p2;
    char *name;
    int len;

    if (! _cairo_isalpha (*p))
	return _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);

    p2 = p;
    while (_cairo_isalpha (*p2) || _cairo_isdigit (*p2))
	p2++;

    len = p2 - p;
    name = malloc (len + 1);
    if (unlikely (name == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    memcpy (name, p, len);
    name[len] = 0;
    *s = name;
    *end = p2;

    return CAIRO_INT_STATUS_SUCCESS;
}

static cairo_int_status_t
parse_attributes (const char *attributes, attribute_spec_t *attrib_def, cairo_list_t *list)
{
    attribute_spec_t *def;
    attribute_t *attrib;
    char *name = NULL;
    cairo_int_status_t status;
    const char *p = attributes;

    if (! p)
	return _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);

    while (*p) {
	p = skip_space (p);
	if (! *p)
	    break;

	status = parse_name (p, &p, &name);
	if (status)
	    return status;

	def = attrib_def;
	while (def->name) {
	    if (strcmp (name, def->name) == 0)
		break;
	    def++;
	}
	if (! def->name) {
	    status = _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);
	    goto fail1;
	}

	attrib = calloc (1, sizeof (attribute_t));
	if (unlikely (attrib == NULL)) {
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    goto fail1;
	}

	attrib->name = name;
	attrib->type = def->type;
	_cairo_array_init (&attrib->array, sizeof(attrib_val_t));

	p = skip_space (p);
	if (def->type == ATTRIBUTE_BOOL && *p != '=') {
	    attrib->scalar.b = TRUE;
	} else {
	    if (*p++ != '=') {
		status = _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);
		goto fail2;
	    }

	    if (def->array_size == 0) {
		p = parse_scalar (p, def->type, &attrib->scalar);
		if (!p) {
		    status = _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);
		    goto fail2;
		}

		attrib->array_len = 0;
	    } else {
		status = parse_array (p, def->type, &attrib->array, &p);
		if (unlikely (status))
		    goto fail2;

		attrib->array_len = _cairo_array_num_elements (&attrib->array);
		if (def->array_size > 0 && attrib->array_len != def->array_size) {
		    status = _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);
		    goto fail2;
		}
	    }
	}

	cairo_list_add_tail (&attrib->link, list);
    }

    return CAIRO_INT_STATUS_SUCCESS;

  fail2:
    _cairo_array_fini (&attrib->array);
    free (attrib);
  fail1:
    free (name);

    return status;
}

static void
free_attributes_list (cairo_list_t *list)
{
    attribute_t *attr, *next;

    cairo_list_foreach_entry_safe (attr, next, attribute_t, list, link)
    {
	cairo_list_del (&attr->link);
	free (attr->name);
	_cairo_array_fini (&attr->array);
	free (attr);
    }
}

static attribute_t *
find_attribute (cairo_list_t *list, const char *name)
{
    attribute_t *attr;

    cairo_list_foreach_entry (attr, attribute_t, list, link)
    {
	if (strcmp (attr->name, name) == 0)
	    return attr;
    }

    return NULL;
}

cairo_int_status_t
_cairo_tag_parse_link_attributes (const char *attributes, cairo_link_attrs_t *link_attrs)
{
    cairo_list_t list;
    cairo_int_status_t status;
    attribute_t *attr;
    attrib_val_t val;

    cairo_list_init (&list);
    status = parse_attributes (attributes, _link_attrib_spec, &list);
    if (unlikely (status))
	return status;

    memset (link_attrs, 0, sizeof (cairo_link_attrs_t));
    _cairo_array_init (&link_attrs->rects, sizeof (cairo_rectangle_t));
    if (find_attribute (&list, "uri")) {
	link_attrs->link_type = TAG_LINK_URI;
    } else if (find_attribute (&list, "file")) {
	link_attrs->link_type = TAG_LINK_FILE;
    } else if (find_attribute (&list, "dest")) {
	link_attrs->link_type = TAG_LINK_DEST;
    } else if (find_attribute (&list, "page")) {
	link_attrs->link_type = TAG_LINK_DEST;
    } else {
	link_attrs->link_type = TAG_LINK_EMPTY;
	goto cleanup;
    }

    cairo_list_foreach_entry (attr, attribute_t, &list, link)
    {
	if (strcmp (attr->name, "uri") == 0) {
	    if (link_attrs->link_type != TAG_LINK_URI) {
		status = _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);
		goto cleanup;
	    }

	    link_attrs->uri = strdup (attr->scalar.s);
	} else if (strcmp (attr->name, "file") == 0) {
	    if (link_attrs->link_type != TAG_LINK_FILE) {
		status = _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);
		goto cleanup;
	    }

	    link_attrs->file = strdup (attr->scalar.s);
	} else if (strcmp (attr->name, "dest") == 0) {
	    if (! (link_attrs->link_type == TAG_LINK_DEST ||
		   link_attrs->link_type != TAG_LINK_FILE)) {
		status = _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);
		goto cleanup;
	    }

	    link_attrs->dest = strdup (attr->scalar.s);
	} else if (strcmp (attr->name, "page") == 0) {
	    if (! (link_attrs->link_type == TAG_LINK_DEST ||
		   link_attrs->link_type != TAG_LINK_FILE)) {
		status = _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);
		goto cleanup;
	    }

	    link_attrs->page = attr->scalar.i;

	} else if (strcmp (attr->name, "pos") == 0) {
	    if (! (link_attrs->link_type == TAG_LINK_DEST ||
		   link_attrs->link_type != TAG_LINK_FILE)) {
		status = _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);
		goto cleanup;
	    }

	    _cairo_array_copy_element (&attr->array, 0, &val);
	    link_attrs->pos.x = val.f;
	    _cairo_array_copy_element (&attr->array, 1, &val);
	    link_attrs->pos.y = val.f;
	} else if (strcmp (attr->name, "rect") == 0) {
	    cairo_rectangle_t rect;
	    int i;
	    int num_elem = _cairo_array_num_elements (&attr->array);
	    if (num_elem == 0 || num_elem % 4 != 0) {
		status = _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);
		goto cleanup;
	    }

	    for (i = 0; i < num_elem; i += 4) {
		_cairo_array_copy_element (&attr->array, i, &val);
		rect.x = val.f;
		_cairo_array_copy_element (&attr->array, i+1, &val);
		rect.y = val.f;
		_cairo_array_copy_element (&attr->array, i+2, &val);
		rect.width = val.f;
		_cairo_array_copy_element (&attr->array, i+3, &val);
		rect.height = val.f;
		status = _cairo_array_append (&link_attrs->rects, &rect);
		if (unlikely (status))
		    goto cleanup;
	    }
	}
    }

  cleanup:
    free_attributes_list (&list);
    if (unlikely (status)) {
	free (link_attrs->dest);
	free (link_attrs->uri);
	free (link_attrs->file);
	_cairo_array_fini (&link_attrs->rects);
    }

    return status;
}

cairo_int_status_t
_cairo_tag_parse_dest_attributes (const char *attributes, cairo_dest_attrs_t *dest_attrs)
{
    cairo_list_t list;
    cairo_int_status_t status;
    attribute_t *attr;

    memset (dest_attrs, 0, sizeof (cairo_dest_attrs_t));
    cairo_list_init (&list);
    status = parse_attributes (attributes, _dest_attrib_spec, &list);
    if (unlikely (status))
	goto cleanup;

    cairo_list_foreach_entry (attr, attribute_t, &list, link)
    {
	if (strcmp (attr->name, "name") == 0) {
	    dest_attrs->name = strdup (attr->scalar.s);
	} else if (strcmp (attr->name, "x") == 0) {
	    dest_attrs->x = attr->scalar.f;
	    dest_attrs->x_valid = TRUE;
	} else if (strcmp (attr->name, "y") == 0) {
	    dest_attrs->y = attr->scalar.f;
	    dest_attrs->y_valid = TRUE;
	} else if (strcmp (attr->name, "internal") == 0) {
	    dest_attrs->internal = attr->scalar.b;
	}
    }

    if (! dest_attrs->name)
	status = _cairo_error (CAIRO_INT_STATUS_TAG_ERROR);

  cleanup:
    free_attributes_list (&list);

    return status;
}
