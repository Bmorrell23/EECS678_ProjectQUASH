STUDENT_ID=2866535

SRCDIR = ./
CFILELIST = quash.c

RAWC = $(patsubst %.c,%,$(addprefix $(SRCDIR), $(CFILELIST)))

all: quash

test: all
	./quash
	cat out.txt

quash: signals.c
	gcc -g $^ -o $@

quash_tester: signal_tester.c
	gcc -g $^ -o $@

clean:
	rm -f *.o quash
	rm -rf $(STUDENT_ID)-quash_project

zip: clean
#	create temp dir
	mkdir $(STUDENT_ID)-quash
#	get all the c files to be .txt for archiving
	$(foreach file, $(RAWC), cp $(file).c $(file)-c.txt;)
#	move the necessary files into the temp dir
	cp signals.c Makefile $(STUDENT_ID)-signals-lab/
	mv *-c.txt $(STUDENT_ID)-signals-lab/
	zip -r $(STUDENT_ID)-signals-lab.zip $(STUDENT_ID)-signals-lab
	rm -rf $(STUDENT_ID)-signals-lab

.PHONY: clean zip
