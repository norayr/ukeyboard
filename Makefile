vkb_compiler:	vkb_compiler.c
	gcc -W -Wall -o vkb_compiler vkb_compiler.c

deb:
	./make-debs

clean:
	rm -f vkb_compiler
	rm -rf build

distclean:
	rm -f vkb_compiler
	rm -rf build
	rm -rf debian
	rm -f *-stamp
