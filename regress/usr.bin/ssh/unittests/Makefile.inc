#	$OpenBSD: Makefile.inc,v 1.3 2015/01/23 21:21:23 miod Exp $

.include <bsd.own.mk>
.include <bsd.obj.mk>

REGRESS_FAIL_EARLY= yes

# XXX detect from ssh binary? 
OPENSSL?=	yes
.if (${OPENSSL:L} == "yes")
CFLAGS+=	-DWITH_OPENSSL -DWITH_SSH1
.endif

# enable warnings
WARNINGS=Yes

DEBUG=-g
CFLAGS+=	-fstack-protector-all
CDIAGFLAGS=	-Wall
CDIAGFLAGS+=	-Wextra
CDIAGFLAGS+=	-Werror
CDIAGFLAGS+=	-Wchar-subscripts
CDIAGFLAGS+=	-Wcomment
CDIAGFLAGS+=	-Wformat
CDIAGFLAGS+=	-Wformat-security
CDIAGFLAGS+=	-Wimplicit
CDIAGFLAGS+=	-Winline
CDIAGFLAGS+=	-Wmissing-declarations
CDIAGFLAGS+=	-Wmissing-prototypes
CDIAGFLAGS+=	-Wparentheses
CDIAGFLAGS+=	-Wpointer-arith
CDIAGFLAGS+=	-Wreturn-type
CDIAGFLAGS+=	-Wshadow
CDIAGFLAGS+=	-Wsign-compare
CDIAGFLAGS+=	-Wstrict-aliasing
CDIAGFLAGS+=	-Wstrict-prototypes
CDIAGFLAGS+=	-Wswitch
CDIAGFLAGS+=	-Wtrigraphs
CDIAGFLAGS+=	-Wuninitialized
CDIAGFLAGS+=	-Wunused
.if ${COMPILER_VERSION} == "gcc4"
CDIAGFLAGS+=	-Wpointer-sign
CDIAGFLAGS+=	-Wold-style-definition
.endif

SSHREL=../../../../../usr.bin/ssh

CFLAGS+=-I${.CURDIR}/../test_helper -I${.CURDIR}/${SSHREL}

.if exists(${.CURDIR}/../test_helper/${__objdir})
LDADD+=-L${.CURDIR}/../test_helper/${__objdir} -ltest_helper
DPADD+=${.CURDIR}/../test_helper/${__objdir}/libtest_helper.a
.else
LDADD+=-L${.CURDIR}/../test_helper -ltest_helper
DPADD+=${.CURDIR}/../test_helper/libtest_helper.a
.endif

.if exists(${.CURDIR}/${SSHREL}/lib/${__objdir})
LDADD+=-L${.CURDIR}/${SSHREL}/lib/${__objdir} -lssh
DPADD+=${.CURDIR}/${SSHREL}/lib/${__objdir}/libssh.a
.else
LDADD+=-L${.CURDIR}/${SSHREL}/lib -lssh
DPADD+=${.CURDIR}/${SSHREL}/lib/libssh.a
.endif

LDADD+= -lutil
DPADD+= ${LIBUTIL}

.if (${OPENSSL:L} == "yes")
LDADD+= -lcrypto
DPADD+= ${LIBCRYPTO}
.endif
