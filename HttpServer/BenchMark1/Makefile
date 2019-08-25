CFLAGS?=	-Wall -ggdb -W -O
CC?=		gcc
LIBS?=
LDFLAGS?=
PREFIX?=	/usr/local/webbench
VERSION=1.5
TMPDIR=/tmp/webbench-$(VERSION)

all:   BenchMark tags

tags:  *.c
	-ctags *.c

install: webbench
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s webbench $(DESTDIR)$(PREFIX)/bin
	ln -sf $(DESTDIR)$(PREFIX)/bin/webbench $(DESTDIR)/usr/local/bin/webbench

	install -d $(DESTDIR)/usr/local/man/man1
	install -d $(DESTDIR)$(PREFIX)/man/man1
	install -m 644 webbench.1 $(DESTDIR)$(PREFIX)/man/man1
	ln -sf $(DESTDIR)$(PREFIX)/man/man1/webbench.1 $(DESTDIR)/usr/local/man/man1/webbench.1

	install -d $(DESTDIR)$(PREFIX)/share/doc/webbench
	install -m 644 debian/copyright $(DESTDIR)$(PREFIX)/share/doc/webbench
	install -m 644 debian/changelog $(DESTDIR)$(PREFIX)/share/doc/webbench

BenchMark: BenchMark.o Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o BenchMark BenchMark.o $(LIBS) -lpthread 

clean:
	-rm -f *.o BenchMark *~ core *.core tags
	
tar:   clean
	-debian/rules clean
	rm -rf $(TMPDIR)
	install -d $(TMPDIR)
	cp -p Makefile BenchMark.c socket.c BenchMark.1 $(TMPDIR)
	install -d $(TMPDIR)/debian
	-cp -p debian/* $(TMPDIR)/debian
	ln -sf debian/copyright $(TMPDIR)/COPYRIGHT
	ln -sf debian/changelog $(TMPDIR)/ChangeLog
	-cd $(TMPDIR) && cd .. && tar cozf BenchMark-$(VERSION).tar.gz BenchMark-$(VERSION)

BenchMark.o:	BenchMark.c socket.c Makefile

.PHONY: clean install all tar
