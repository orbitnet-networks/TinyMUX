CXX = g++ -std=c++11

ALLSRC     = funcs.cpp sample.cpp sum.cpp sqlproxy.cpp sqlslave.cpp

all: funcs.so sample.so sum.so sqlproxy.so sqlslave.so

funcs.so: funcs.cpp funcs.h ../autoconf.h ../config.h ../libmux.h ../modules.h
	( if [ -f funcs.so ]; then mv -f funcs.so funcs.so~ ; fi )
	$(CXX) -shared -fPIC -o funcs.so funcs.cpp -L.. -lmux
	( if [ -f funcs.so ]; then cd ../../game/bin ; rm -f funcs.so ; ln -s ../../src/modules/funcs.so funcs.so ; fi )

sample.so: sample.cpp sample.h ../autoconf.h ../config.h ../libmux.h ../modules.h
	( if [ -f sample.so ]; then mv -f sample.so sample.so~ ; fi )
	$(CXX) -shared -fPIC -o sample.so sample.cpp -L.. -lmux
	( if [ -f sample.so ]; then cd ../../game/bin ; rm -f sample.so ; ln -s ../../src/modules/sample.so sample.so ; fi )

sum.so: sum.cpp sum.h ../autoconf.h ../config.h ../libmux.h ../modules.h
	( if [ -f sum.so ]; then mv -f sum.so sum.so~ ; fi )
	$(CXX) -shared -fPIC -o sum.so sum.cpp -L.. -lmux
	( if [ -f sum.so ]; then cd ../../game/bin ; rm -f sum.so ; ln -s ../../src/modules/sum.so sum.so ; fi )

sqlproxy.so: sqlproxy.cpp sql.h ../autoconf.h ../config.h ../libmux.h ../modules.h
	( if [ -f sqlproxy.so ]; then mv -f sqlproxy.so sqlproxy.so~ ; fi )
	$(CXX) -shared -fPIC -o sqlproxy.so sqlproxy.cpp -L.. -lmux
	( if [ -f sqlproxy.so ]; then cd ../../game/bin ; rm -f sqlproxy.so ; ln -s ../../src/modules/sqlproxy.so sqlproxy.so ; fi )

sqlslave.so: sqlslave.cpp sql.h ../autoconf.h ../config.h ../libmux.h ../modules.h
	( if [ -f sqlslave.so ]; then mv -f sqlslave.so sqlslave.so~ ; fi )
	$(CXX) -shared -fPIC  -o sqlslave.so sqlslave.cpp -L..  -lmux 
	( if [ -f sqlslave.so ]; then cd ../../game/bin ; rm -f sqlslave.so ; ln -s ../../src/modules/sqlslave.so sqlslave.so ; fi )

depend: $(ALLSRC) ../unsplit
	for i in $(ALLSRC) ; do $(CXX) $(CXXFLAGS) -shared -fPIC  -M $$i; done | ../unsplit > .depend~
	mv .depend~ .depend

clean:
	-rm -f *.o a.out core gmon.out warn.log config.status config.log funcs.so~ sample.so~ sum.so~ sqlproxy.so~ sqlslave.so~

realclean:
	-rm -f *.o a.out core gmon.out warn.log config.status config.log funcs.so funcs.so~ sample.so sample.so~ sum.so sum.so~ sqlproxy.so sqlproxy.so~ sqlslave.so sqlslave.so~ autoconf.h Makefile

include .depend
