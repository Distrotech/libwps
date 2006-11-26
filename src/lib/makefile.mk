PRJ=..$/..$/..$/..$/..$/..

PRJNAME=libwps
TARGET=wpslib
ENABLE_EXCEPTIONS=TRUE
LIBTARGET=NO

.INCLUDE :  svpre.mk
.INCLUDE :  settings.mk
.INCLUDE :  sv.mk

.IF "$(GUI)"=="WNT"
CFLAGS+=-GR
.ENDIF
.IF "$(COM)"=="GCC"
CFLAGSCXX+=-frtti
.ENDIF

SLOFILES= \
	$(SLO)$/libwps_internal.obj \
	$(SLO)$/WPS4.obj \
	$(SLO)$/WPS8.obj \
	$(SLO)$/WPSDocument.obj \
	$(SLO)$/WPSOLEStream.obj \
	$(SLO)$/WPSStreamImplementation.obj \
	$(SLO)$/WPSContentListener.obj \
	$(SLO)$/WPSHeader.obj \
	$(SLO)$/WPSListener.obj \
	$(SLO)$/WPSPageSpan.obj \
	$(SLO)$/WPSParser.obj \
	$(SLO)$/WPSStylesListener.obj

LIB1ARCHIV=$(LB)$/libwpslib.a
LIB1TARGET=$(SLB)$/$(TARGET).lib
LIB1OBJFILES= $(SLOFILES)

.INCLUDE :  target.mk
