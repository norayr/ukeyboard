linguas = ca fi it sk uk_UA

all: po

update-po: ukeyboard.pot
	@for lang in $(linguas); do \
		msgmerge $$lang.po ukeyboard.pot -o $$lang.po.new; \
		mv $$lang.po.new $$lang.po; \
	done

po:
	@for lang in $(linguas); do \
		msgfmt -o $$lang.gmo $$lang.po; \
	done

install: po
	@for lang in $(linguas); do \
		install -d $(DESTDIR)/usr/share/locale/$$lang/LC_MESSAGES; \
		install -m 644 $$lang.gmo $(DESTDIR)/usr/share/locale/$$lang/LC_MESSAGES/ukeyboard.mo; \
	done

ukeyboard.pot:
	INTLTOOL_EXTRACT=intltool-extract srcdir=. intltool-update --gettext-package ukeyboard --pot

clean:
	rm -f *.gmo ukeyboard.pot
