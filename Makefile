cs_CZ.vkb:	cs_CZ.def vkb_compiler
	./vkb_compiler cs_CZ.def cs_CZ.vkb
vkb_compiler:	vkb_compiler.c
	gcc -W -Wall -o vkb_compiler vkb_compiler.c


install-kbd:
	install -m 755 -d /usr/share/keyboards/
	install -m 644 cs_CZ.vkb /usr/share/keyboards/
