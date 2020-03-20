STUDENT_ID=XXXXXXX

CFILELIST = quash.c
HFILELIST = quash.h quash_test.h

RAWC = $(patsubst %.c,%,$(CFILELIST))
RAWH = $(patsubst %.c,%,$(HFILELIST))


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

zip: clean
#	create temp dir
	mkdir $(STUDENT_ID)-quash
#	get all the c files to be .txt for archiving
	$(foreach file, $(RAWC), cp $(file).c $(file)-c.txt;)
	$(foreach file, $(RAWH), cp $(file).h $(file)-h.txt;)
#	move the necessary files into the temp dir
	cp quash.c Makefile $(STUDENT_ID)-quash_project/
	mv *-c.txt $(STUDENT_ID)-quash_project/
	zip -r $(STUDENT_ID)-quash_project.zip $(STUDENT_ID)-quash_project
	rm -rf $(STUDENT_ID)-quash_project

.PHONY: clean zip
