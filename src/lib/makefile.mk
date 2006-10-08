PRJ=..$/..$/..$/..$/..$/..

PRJNAME=libwpd
TARGET=wpdlib
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
	$(SLO)$/libwpd_internal.obj \
	$(SLO)$/libwpd_math.obj \
	$(SLO)$/WPXPageSpan.obj \
	$(SLO)$/WPXContentListener.obj \
	$(SLO)$/WPXHeader.obj \
	$(SLO)$/WPXListener.obj \
	$(SLO)$/WPXParser.obj \
	$(SLO)$/WPXStylesListener.obj \
	$(SLO)$/WPDocument.obj

LIB1ARCHIV=$(LB)$/libwpdlib.a
LIB1TARGET=$(SLB)$/$(TARGET).lib
LIB1OBJFILES= $(SLOFILES)

.INCLUDE :  target.mk
