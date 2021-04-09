# Time-stamp: <03/10/26 16:04:25 ptr>
# $Id: top.mak 2697 2006-10-26 19:44:09Z dums $

.SUFFIXES: .o .cc

!ifndef RULESBASE
RULESBASE = $(SRCROOT)/Makefiles
!endif

!ifndef ALL_TAGS
ALL_TAGS = all-shared all-static
!endif

#!if [echo $(ALL_TAGS)]
#!endif
#!if [echo $(RULESBASE)]
#!endif

all:	dirs $(ALL_TAGS)

all-dynamic: all-shared
all-shared: release-shared dbg-shared stldbg-shared
all-static: release-static dbg-static stldbg-static

install-shared: install-release-shared install-dbg-shared install-stldbg-shared
install-static: install-release-static install-dbg-static install-stldbg-static

# include file, generated by configure, if available
!if EXIST( $(RULESBASE)/config.mak )
!include $(RULESBASE)/config.mak
!endif
# define what make clone we use
USE_MAKE = nmake
# identify OS and build date
!include $(RULESBASE)/$(USE_MAKE)/sysid.mak
# OS-specific definitions, like ar, ln, install, etc.
!include $(RULESBASE)/$(USE_MAKE)/sys.mak
# rules to make dirs for targets
!include $(RULESBASE)/$(USE_MAKE)/targetdirs.mak
# extern libraries
!include $(RULESBASE)/$(USE_MAKE)/extern.mak

# derive common targets (*.o, *.d),
# build rules (including output catalogs)
!include $(RULESBASE)/$(USE_MAKE)/targets.mak
# dependency
#include ${RULESBASE}/depend-$(COMPILER_NAME).mak

# general clean
!include $(RULESBASE)/$(USE_MAKE)/clean.mak

# if target is library, rules for library
!ifdef LIBNAME
!include $(RULESBASE)/$(USE_MAKE)/lib/top.mak
!endif

# if target is program, rules for executable
!ifdef PRGNAME
!include $(RULESBASE)/$(USE_MAKE)/app/top.mak
!endif
