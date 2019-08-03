# Makefile for TinyMUX 2.12
#
# Search for the text 'CONFIGURATION SECTION' and make any changes needed
# there.
#

SHELL=/bin/sh
srcdir = .

BIN = ../game/bin
CC = gcc
CXX = g++ -std=c++11
CXXCPP = g++ -E -std=c++11
LIBS = -lm -lcrypt  

.SUFFIXES: .cpp

# After we finish beta testing, we'll take away the debugging -g flag.
# If you don't use GCC, don't use -g. Add -pg for profiling (gprof netmux
# gmon.out)
#
#OPTIM = -O
#OPTIM = -g -pg -O
OPTIM = -g -O

# By default, REALITY_LVLS is not enabled.  If you wish to use REALITY_LVLS,
# run configure with --enable-realitylvls.  Please read the docs/REALITY.
#
# NOTE: To ensure a clean build, please 'make clean' first.
#
REALITY_LVLS = 
REALITY_SRC = 
REALITY_OBJ = 

# By default, WOD_REALMS is not enabled.  If you wish to use WOD_REALMS,
# run configure with --enable-wodrealms.  Please read the docs/REALMS.
#
# NOTE: To ensure a clean build, please 'make clean' first.
#
WOD_REALMS = 

# Disk-based caching is the default.  If you wish to use memory-based, run
# configure with --enable-memorybased.  Disk-based usually uses less memory,
# but more disk space.  Memory-based usually uses more memory, less
# disk-space, and some gain in performance for a very narrow set of
# operations.
#
# NOTE: To ensure a clean build, please 'make clean' first.
#
MEMORY_BASED = 

# Stub Slave. By default, this is off. This can be turned on by running
# configure with --enable-stubslave. The Stub Slave is how to load modules in
# a separate process including support for asyncronous SQL.
#
STUB_SLAVE = 
STUBLIBS   =  # -ldbi

# Firan MUX. By default, this is off. This can be turned on by running
# configure with --enable-firanmux. Firan MUX modifications are necessary
# in order to run Firan's database.  Typically, --enable-memorybased is also
# necessary.
#
FIRANMUX    = 

# SQL support.  This optionally provides an inline sql() function which links
# to a MySQL database, providing softcode compatibility with the SQL support
# in TinyMUSH 3.x and PennMUSH.
#
INLINESQL   = 
SQL_LIBS    =  
SQL_INCLUDE = 

# SSL encryption support.  This provides a 'port_ssl' configuration directive
# which adds SSL-enabled ports to the listening MUX ports.  It also enables the
# 'ssl_certificate_file' and 'ssl_certificate_key' and 'ssl_certificate_password'
# configuration options.
#
SSL               = 

# Deprecated Features. By default, this is off. This can be turned on by
# running configure with --enable-deprecated. Features which are infrequently
# used become deprecated and are eventually removed from the source.
#
DEPRECATED = 

# Potentially expensive internal self-checking of the server. This does not
# increase softcode validation or validation of the operating environment.
# It's useful primarily during development while running test cases.
#
SELFCHECK = 

# Base source and object files for building netmux
#
NETMUX_BASE_SRC = _build.cpp alarm.cpp alloc.cpp attrcache.cpp boolexp.cpp \
    bsd.cpp command.cpp comsys.cpp conf.cpp cque.cpp create.cpp db.cpp \
    db_rw.cpp eval.cpp file_c.cpp flags.cpp funceval.cpp funceval2.cpp \
    functions.cpp funmath.cpp game.cpp help.cpp htab.cpp local.cpp log.cpp \
    look.cpp mail.cpp match.cpp mathutil.cpp mguests.cpp modules.cpp move.cpp \
    muxcli.cpp netcommon.cpp object.cpp predicates.cpp player.cpp player_c.cpp \
    plusemail.cpp powers.cpp quota.cpp rob.cpp pcre.cpp set.cpp sha1.cpp \
    speech.cpp stringutil.cpp strtod.cpp svdrand.cpp svdhash.cpp timer.cpp \
    timeabsolute.cpp timedelta.cpp timeparser.cpp timeutil.cpp timezone.cpp \
    unparse.cpp utf8tables.cpp vattr.cpp walkdb.cpp wild.cpp wiz.cpp
NETMUX_BASE_OBJ = _build.o alarm.o alloc.o attrcache.o boolexp.o bsd.o \
    command.o comsys.o conf.o cque.o create.o db.o db_rw.o eval.o file_c.o \
    flags.o funceval.o funceval2.o functions.o funmath.o game.o help.o \
    htab.o local.o log.o look.o mail.o match.o mathutil.o mguests.o modules.o \
    move.o muxcli.o netcommon.o object.o predicates.o player.o player_c.o \
    plusemail.o powers.o quota.o rob.o pcre.o set.o sha1.o speech.o \
    stringutil.o strtod.o svdrand.o svdhash.o timer.o timeabsolute.o \
    timedelta.o timeparser.o timeutil.o timezone.o unparse.o utf8tables.o \
    vattr.o walkdb.o wild.o wiz.o

# Base sources and object files for building libmux.so
#
LIBMUX_BASE_SRC = libmux.cpp
LIBMUX_BASE_OBJ = libmux.o
MUX_LIBS = -lmux
DL_LIB = -ldl

# Version number routines
#
VER_SRC = version.cpp
VER_OBJ = version.o
VER_FLG = -DMUX_BUILD_DATE="\"`date`\"" \
          -DMUX_BUILD_NUM="\"`sh ./buildnum.sh`\""

# ===================== CONFIGURATION SECTION ====================
#
# Select the correct C compiler.  Whatever you choose, it must be able
# to grok ANSI C (function prototypes)
#
#-----CC or GCC (must be able to grok function prototypes)
#
DEFS =
#
#-----CC on a NeXT system, really weird derivative of GCC
#
#DEFS = -DNEXT -DNEED_STRDUP
#
#-----GCC if the libraries were built for a pcc-derived cc compiler
#     (most systems)
#
#DEFS = -fpcc-struct-return -Wall
#
#-----GCC with GCC-compatible libraries if you want verbose error messages
#
#DEFS = -Wall
#
#-----HP-UX C compiler
#
#DEFS = -w +Obb800 -Aa -D_INCLUDE_POSIX_SOURCE -D_INCLUDE_HPUX_SOURCE -D_INCLUDE_XOPEN_SOURCE
#
#-----MIPS C compiler (also DEC 3xxx, Prime EXL7xxx)
#
#DEFS = -signed
#

# ================== END OF CONFIGURATION SECTION =================

# Auxiliary source files: only used by offline utilities.
#
AUX_SRC = unsplit.cpp
ALLCXXFLAGS = $(CXXFLAGS) $(OPTIM) $(DEFS) $(MEMORY_BASED) $(WOD_REALMS) \
    $(REALITY_LVLS) $(STUB_SLAVE) $(FIRANMUX) $(DEPRECATED) $(SELFCHECK) \
    $(INLINESQL) $(SQL_INCLUDE) $(SSL)

# Compiliation source files.
#
ALLSRC     = $(NETMUX_BASE_SRC) $(LIBMUX_BASE_SRC) $(REALITY_SRC) $(VER_SRC) $(AUX_SRC)
NETMUX_SRC = $(NETMUX_BASE_SRC) $(REALITY_SRC)
NETMUX_OBJ = $(NETMUX_BASE_OBJ) $(REALITY_OBJ)

SUBDIRS = modules tools
.PHONY: subdirs $(SUBDIRS)

.cpp.o:
	$(CXX) $(ALLCXXFLAGS) -c $<

all: libmux.so netmux slave  links subdirs

links: netmux slave 
	cd ../game/bin ; rm -f dbconvert ; ln -s ../../src/netmux dbconvert
	cd ../game/bin ; rm -f netmux ; ln -s ../../src/netmux netmux
	cd ../game/bin ; rm -f slave ; ln -s ../../src/slave slave
	( if [ ! -z "" -a -f "" ]; then cd ../game/bin ; rm -f ; ln -s ../../src/  ; fi )
	( if [ ! -z "libmux.so" -a -f "libmux.so" ]; then cd ../game/bin ; rm -f libmux.so; ln -s ../../src/libmux.so libmux.so ; fi )

stubslave: stubslave.o
	$(CXX) $(ALLCXXFLAGS) -o stubslave stubslave.o -L. $(LIBS) $(MUX_LIBS) $(STUBLIBS)

slave: slave.o
	$(CXX) $(ALLCXXFLAGS) -o slave slave.o $(LIBS)

unsplit: unsplit.o
	$(CXX) $(ALLCXXFLAGS) -o unsplit unsplit.o

netmux: $(NETMUX_OBJ) $(VER_SRC)
	$(CXX) $(ALLCXXFLAGS) $(VER_FLG) -c $(VER_SRC)
	( if [ -f netmux ]; then mv -f netmux netmux~ ; fi )
	$(CXX) $(ALLCXXFLAGS) -o netmux $(NETMUX_OBJ) $(VER_OBJ) -L. $(LIBS) $(SQL_LIBS) $(MUX_LIBS)

libmux.so: $(LIBMUX_BASE_SRC)
	( if [ -f libmux.so ]; then mv -f libmux.so libmux.so~ ; fi )
	$(CXX) $(ALLCXXFLAGS) -shared -fPIC -o libmux.so $(LIBMUX_BASE_SRC) $(LIBS) $(DL_LIB) $(SQL_LIBS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

modules: libmux.so

depend: $(ALLSRC) unsplit
	for i in $(ALLSRC) slave.cpp stubslave.cpp ; do $(CXXCPP) $(ALLCXXFLAGS) -M $$i; done | ./unsplit > .depend~
	mv .depend~ .depend

realclean:
	-rm -f *.o a.out core gmon.out dbconvert config.status config.log netmux netmux~ libmux.so libmux.so~ slave  autoconf.h Makefile

clean:
	-rm -f *.o a.out core gmon.out warn.log config.status config.log netmux~ libmux.so~

include .depend
