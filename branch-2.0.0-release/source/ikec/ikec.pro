TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release

LIBS	+= ../ikea/.obj/config.o ../libip/utils.list.o ../libiked/libiked.so

INCLUDEPATH	+= ./.. ../ikea/ ../libip/ ../iked/ ../libiked/

SOURCES	+= main.cpp \
	ikec.cpp

FORMS	= root.ui \
	banner.ui

IMAGES	= png/ikec.png

######################################################################
# Automatically generated by qmake (1.07a) Thu Mar 8 23:08:30 2007
######################################################################


# Input
