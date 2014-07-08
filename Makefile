
CFLAGS= -O -Wall

OBJS= stegcommon.o

# can change to -ll for lex
LIBS= -lfl

.DUMMY: all install clean test package doc

all: stegparty doc

doc: stegparty.txt base27.txt	

install:
	@echo "Install it yourself, lazybones! ;-)"

clean:
	@echo "What am I, a maid???"

%.l : %.rulez make_steg_lex
	perl make_steg_lex <$*.rulez >$@

% : %.rulez %.o ${OBJS}
	gcc -o$@ $@.o ${OBJS} ${LIBS}

%.txt : %.1
	nroff -man $< | sed -e 's/.[^ -~]//g' >$@

#
# TESTING STUFF
#

test: test/1.test test/2.test test/3.test test/4.test test/5.test

test/%.encode : test/%.txt stegparty test/secret
	./stegparty -v -c test/secret -i test/$*.txt -o $@

test/%.decode : test/%.encode
	./stegparty -v -d -i test/$*.encode -o $@

test/%.debug : test/%.txt stegparty test/secret
	./stegparty -vv -c test/secret -i test/$*.txt -o /dev/null 2>test/$*.debug

test/%.rebug : test/%.encode
	./stegparty -vv -d -i test/$*.encode -o /dev/null 2>test/$*.rebug

test/%.dezip : test/%.decode
	-gzip -cd test/$*.decode > $@

test/%.test : test/%.dezip stegparty
	gzip -cd test/secret | diff test/$*.dezip -
	@echo
	@echo "***Test $* passed"
	@echo

package:
	cvs export -D now steg/stegparty
	cd steg/stegparty ; gmake doc
	cd steg ; tar cf - stegparty | gzip -c9 > ../stegparty.tar.gz
	rm -fr steg
