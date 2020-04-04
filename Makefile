STUDENT_ID=XXXXXXX

CFILELIST = quash.c
HFILELIST = quash.h

DOXYGENCONF = quash_doc

RAWC = $(patsubst %.c,%,$(CFILELIST))
RAWH = $(patsubst %.h,%,$(HFILELIST))


OBJFILES = $(patsubst %.c,%.o,$(CFILELIST))
EXECNAME = $(patsubst %,./%,$(PROGNAME))


all: quash

test: quash
	./quash

quash: quash.c
	gcc -g $^ -o $@

clean:
	rm -f *.o quash
	rm -rf $(STUDENT_ID)-quash_project

doc: $(CFILELIST) $(HFILELIST) $(DOXYGENCONF) README.md
	doxygen $(DOXYGENCONF)

zip: clean
#	Perform renaming copies across the Makefile and all .c and .h
#	files
	cp Makefile Makefile.txt
	$(foreach file, $(RAWH), cp $(file).h $(file)-h.txt;)
	$(foreach file, $(RAWC), cp $(file).c $(file)-c.txt;)

#	Make a temporary directory
	mkdir -p $(STUDENT_ID)-project1-quash

#	Move all the renamed files into the temporary directory
	mv Makefile.txt $(STUDENT_ID)-project1-quash
	mv $(HFILELIST:.h=-h.txt) $(STUDENT_ID)-project1-quash
	mv $(CFILELIST:.c=-c.txt) $(STUDENT_ID)-project1-quash

#	Compress and remove temporary directory
	zip -r $(STUDENT_ID)-project1-quash.zip $(STUDENT_ID)-project1-quash
	-rm -rf $(STUDENT_ID)-project1-quash

.PHONY: all test doc zip clean
