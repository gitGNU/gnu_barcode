
CFLAGS = -Wall
LDFLAGS = -L. -l$(TARGET)

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
LIBDIR = $(PREFIX)/lib
INCDIR = $(PREFIX)/include
MAN1DIR = $(PREFIX)/man/man1
MAN3DIR = $(PREFIX)/man/man3
INFODIR = $(PREFIX)/man/info

TARGET = barcode
LIBRARY = lib$(TARGET).a
MAN1 = $(TARGET).1
MAN3 = $(TARGET).3
INFO = doc/$(TARGET).info
HEADER = $(TARGET).h

OBJECTS = library.o ean.o code128.o code39.o ps.o

all: .depend $(TARGET) $(LIBRARY) $(MAN1) $(MAN3) $(INFO) sample

$(TARGET): $(LIBRARY) main.o cmdline.o
	$(CC) $(CFLAGS)  main.o cmdline.o $(LDFLAGS) -o $(TARGET)

sample: sample.o $(LIBRARY)
	$(CC) $(CFLAGS) sample.o $(LDFLAGS) -o $@ 


$(LIBRARY): $(OBJECTS)
	$(AR) r $(LIBRARY) $(OBJECTS)


$(MAN1) $(MAN3): doc/doc.$(TARGET)
	gawk -f doc/manpager $^


$(INFO):
	$(MAKE) -C doc

.PHONY: all install uninstall terse clean depend tar printv distrib

install:
	$(INSTALL) -d $(BINDIR) $(INCDIR) $(LIBDIR) $(MAN1DIR) \
		$(MAN3DIR) $(INFODIR)
	$(INSTALL) -c $(TARGET) $(BINDIR)
	$(INSTALL) -c $(HEADER) $(INCDIR)
	$(INSTALL) -c $(LIBRARY) $(LIBDIR)
	$(INSTALL) -c $(MAN1) $(MAN1DIR)
	$(INSTALL) -c $(MAN3) $(MAN3DIR)
	$(INSTALL) -c $(INFO) $(INFODIR)

uninstall:
	$(RM) -f $(BINDIR)/$(TARGET)
	$(RM) -f $(INCDIR)/$(HEADER)
	$(RM) -f $(LIBDIR)/$(LIBRARY)
	$(RM) -f $(MAN1DIR)/$(MAN1)
	$(RM) -f $(MAN3DIR)/$(MAN3)
	$(RM) -f $(INDODIR)/$(INFO)

terse:
	$(RM) -f *.o *~ $(TARGET) $(LIBRARY) $(MAN1) $(MAN3) core sample
	$(MAKE) -C doc terse
	$(RM) -f .depend

clean: terse
	# remove the documents, too
	$(MAKE) -C doc clean

.depend: $(wildcard *.[c])
	$(CC) $(CFLAGS) -MM $^ > $@

depend: .depend

tar:
	n=`basename \`/bin/pwd\``; cd ..; tar cvf - $$n | gzip > $$n.tar.gz

#print the version, as I usually forget about it
printv:
	@grep VERSION $(HEADER)
	@grep set.version doc/doc.$(TARGET)

distrib: $(INFO) terse tar printv

ifeq (.depend,$(wildcard .depend))
include .depend
endif
