SUBDIRS = data src docs

DISTCHECK_CONFIGURE_FLAGS =								\
	--enable-gtk-doc								\
	--with-udevrulesdir=$$dc_install_base/$(udevrulesdir)				\
	--with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir)

EXTRA_DIST = README.md
