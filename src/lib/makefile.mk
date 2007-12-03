EXTERNAL_WARNINGS_NOT_ERRORS := TRUE

PRJ=..$/..$/..$/..$/..$/..

PRJNAME=libwps
TARGET=wpslib
ENABLE_EXCEPTIONS=TRUE
LIBTARGET=NO

.INCLUDE :  settings.mk

.IF "$(GUI)"=="WNT"
CFLAGS+=-GR
.ENDIF
.IF "$(COM)"=="GCC"
CFLAGSCXX+=-frtti
.ENDIF

.IF "$(SYSTEM_LIBWPD)" == "YES"
INCPRE+=$(LIBWPD_CFLAGS) -I..
.ELSE
INCPRE+=$(SOLARVER)$/$(UPD)$/$(INPATH)$/inc$/libwpd
.ENDIF

SLOFILES= \
	$(SLO)$/libwps_internal.obj \
	$(SLO)$/WPS4.obj \
	$(SLO)$/WPS8.obj \
	$(SLO)$/WPSContentListener.obj \
	$(SLO)$/WPSDocument.obj \
	$(SLO)$/WPSHeader.obj \
	$(SLO)$/WPSPageSpan.obj \
	$(SLO)$/WPSParser.obj

LIB1ARCHIV=$(LB)$/libwpslib.a
LIB1TARGET=$(SLB)$/$(TARGET).lib
LIB1OBJFILES= $(SLOFILES)

.INCLUDE :  target.mk
