prefix=/jail/glftpd
CC=gcc
RM=rm -f
MAKE=make
INSTALL=install

CFLAGS=-g -O2 -W -Wall -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -Winline -DGLVERSION=20264 -D_WITH_ALTWHO
MAXMINDDB = $(shell ld -o /dev/null -lmaxminddb >/dev/null 2>&1; echo $$?)

all: sitewho

sitewho: sitewho.c
	$(CC) $(CFLAGS) -o sitewho sitewho.c -lmaxminddb
ifeq (1,${MAXMINDDB})
	@echo
	@echo "Make sure you have libmaxminddb installed, see README.md for details"
	@echo
endif

#install: all
#	$(INSTALL) -m755 sitewho $(prefix)/bin
#	@(printf "\nInstalling sitewho head/foot/conf files...\n")
#	@([ -f "$(prefix)/ftp-data/misc/who.foot" ] && echo "$(prefix)/ftp-data/misc/who.foot already exists. NOT overwriting it." || $(INSTALL) -m644 who.foot $(prefix)/ftp-data/misc)
#	@([ -f "$(prefix)/ftp-data/misc/who.head" ] && echo "$(prefix)/ftp-data/misc/who.head already exists. NOT overwriting it." || $(INSTALL) -m644 who.head $(prefix)/ftp-data/misc)
#	@([ -f "$(prefix)/bin/sitewho.conf" ] && echo "$(prefix)/bin/sitewho.conf already exists. NOT overwriting it." || $(INSTALL) -m644 sitewho.conf $(prefix)/bin)
#	@(echo)

install: all geoipconf

distclean: clean
	$(RM) sitewho.conf

clean:
	$(RM) sitewho

strip:
	strip sitewhno

geoipconf:
ifeq ($(wildcard /etc/GeoIP.conf),)
	@echo "# The following AccountID and LicenseKey are required placeholders." > /etc/GeoIP.conf
	@echo "# For geoipupdate versions earlier than 2.5.0, use UserId here instead of AccountID." >> /etc/GeoIP.conf
	@echo "#AccountID 0" >> /etc/GeoIP.conf
	@echo "UserId 0" >> /etc/GeoIP.conf
	@echo "LicenseKey 000000000000" >> /etc/GeoIP.conf
	@echo "# Include one or more of the following edition IDs:" >> /etc/GeoIP.conf
	@echo "# * GeoLite2-City - GeoLite 2 City" >> /etc/GeoIP.conf
	@echo "# * GeoLite2-Country - GeoLite2 Country" >> /etc/GeoIP.conf
	@echo "# For geoipupdate versions earlier than 2.5.0, use ProductIds here instead of EditionIDs." >> /etc/GeoIP.conf
	@echo "#EditionIDs GeoLite2-City GeoLite2-Country" >> /etc/GeoIP.conf
	@echo "ProductIds GeoLite2-Country" >> /etc/GeoIP.conf
else
	@echo
	@echo "Skipping creation of /etc/GeoIP.conf because it already exists"
	@echo
endif
