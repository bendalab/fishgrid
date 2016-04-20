ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

EXTRA_DIST = \
    bootstrap.sh \
    moc4.mk \
    doxygen.mk \
    doc/Doxyfile \
    doc/fishgridcalibcomedi.doc \
    doc/fishgrid.doc \
    doc/fishgridrecorder.doc \
    doc/fishgridstepper.doc


DX_INSTALL_SUBDIR = /api
DX_DEPENDS = Makefile.am
DX_SETTINGS = PROJECT_NAME='Fish-Grid'
DX_SETTINGS += INPUT='$(DOXYGEN_INPUT) $(abs_builddir)'
DX_SETTINGS += IMAGE_PATH='$(DOXYGEN_IMAGEPATHES)'
DX_SETTINGS += PREDEFINED='$(DOXYGEN_PREDEF)'

include doxygen.mk

if DX_COND_doc

.PHONY: doclinks

doc: doxygen-doc
all-local: doxygen-runall
html-local: doxygen-run
ps-local: doxygen-ps
pdf-local: doxygen-pdf
clean-local: doxygen-clean
install-data-local: doxygen-install
uninstall-local: doxygen-uninstall

endif DX_COND_doc
