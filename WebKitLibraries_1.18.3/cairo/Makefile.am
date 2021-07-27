include $(top_srcdir)/build/Makefile.am.common

EXTRA_DIST += \
	KNOWN_ISSUES \
	README.win32 \
	Makefile.win32 \
	build/Makefile.win32.common \
	build/Makefile.win32.inform \
	build/Makefile.win32.features \
	build/Makefile.win32.features-h \
	$(NULL)
#MAINTAINERCLEANFILES += \
#	$(srcdir)/build/Makefile.win32.features \
#	$(srcdir)/build/Makefile.win32.features-h \
#	$(NULL)

ACLOCAL_AMFLAGS = -I build ${ACLOCAL_FLAGS}

DIST_SUBDIRS = src doc util boilerplate test perf
SUBDIRS = src doc util
# libpng is required for our test programs
if CAIRO_HAS_PNG_FUNCTIONS
SUBDIRS += boilerplate test perf
endif

doc:
	cd doc && $(MAKE) $(AM_MAKEFLAGS) $@
test retest recheck: all
	cd test && $(MAKE) $(AM_MAKEFLAGS) $@
perf: all
	cd perf && $(MAKE) $(AM_MAKEFLAGS) $@
check-valgrind: all
	cd test && $(MAKE) $(AM_MAKEFLAGS) check-valgrind
	cd perf && $(MAKE) $(AM_MAKEFLAGS) check-valgrind
.PHONY: doc test retest recheck perf check-valgrind


EXTRA_DIST += \
	AUTHORS \
	BIBLIOGRAPHY \
	BUGS \
	CODING_STYLE \
	COPYING \
	COPYING-LGPL-2.1 \
	COPYING-MPL-1.1 \
	HACKING \
	INSTALL \
	NEWS \
	PORTING_GUIDE \
	README \
	RELEASING \
	autogen.sh \
	$(NULL)

# Meson build system files
EXTRA_DIST += \
	meson.build \
	meson_options.txt \
	version.py \
	boilerplate/make-cairo-boilerplate-constructors.py \
	boilerplate/meson.build \
	src/meson.build \
	test/make-cairo-test-constructors.py \
	test/meson.build \
	test/pdiff/meson.build \
	util/cairo-fdr/meson.build \
	util/cairo-gobject/meson.build \
	util/cairo-missing/meson.build \
	util/cairo-script/meson.build \
	util/cairo-sphinx/meson.build \
	util/cairo-trace/meson.build \
	util/meson.build \
	meson-cc-tests/atomic-ops-cxx11.c \
	meson-cc-tests/ipc_rmid_deferred_release.c \
	meson-cc-tests/ft_has_color.c \
	meson-cc-tests/mkdir-variant-1.c \
	meson-cc-tests/pthread.c \
	meson-cc-tests/atomic-ops-gcc-legacy.c \
	meson-cc-tests/mkdir-variant-2.c \
	$(NULL)

DISTCLEANFILES += config.cache

MAINTAINERCLEANFILES += \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/autoscan.log \
	$(srcdir)/build/compile \
	$(srcdir)/build/config.guess \
	$(srcdir)/build/config.sub \
	$(srcdir)/build/depcomp \
	$(srcdir)/build/install-sh \
	$(srcdir)/build/ltmain.sh \
	$(srcdir)/build/missing \
	$(srcdir)/build/mkinstalldirs \
	$(srcdir)/config.h.in \
	$(srcdir)/configure.scan \
	$(NULL)

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-gtk-doc \
	--enable-test-surfaces \
	--enable-full-testing \
	$(NULL)

include $(srcdir)/build/Makefile.am.changelog
include $(srcdir)/build/Makefile.am.releasing
include $(srcdir)/build/Makefile.am.analysis
