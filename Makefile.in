#prefix = /jail/glftpd
#GEOIPCFG = /etc/GeoIP.conf
prefix = .
DBG_LEVEL = 0
GEOIPCFG = GeoIP.conf
CC = gcc
RM = rm -f
MAKE = make
INSTALL=install

CFLAGS = -g -O2 -W -Wall -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -Winline -DGLVERSION=20264 -D_WITH_ALTWHO
LIBS = -lmaxminddb
MAXMINDDB = $(shell ld -o /dev/null -lmaxminddb >/dev/null 2>&1; echo $$?)

.PHONY: all debug geoip

sitewho:
	$(CC) $(CFLAGS) -o $@ $@.c $(LIBS)

debug:
	$(CC) $(CFLAGS) -o sitewho sitewho.c $(LIBS) -DDEBUG=$(DBG_LEVEL)

geoip:
	$(CC) $(CFLAGS) -o sitewho sitewho.c $(LIBS) -D_WITH_GEOIP

install: all
#	$(INSTALL) -m755 sitewho $(prefix)/bin
#	@(printf "\nInstalling sitewho head/foot/conf files...\n")
#	@([ -f "$(prefix)/ftp-data/misc/who.foot" ] && echo "$(prefix)/ftp-data/misc/who.foot already exists. NOT overwriting it." || $(INSTALL) -m644 who.foot $(prefix)/ftp-data/misc)
#	@([ -f "$(prefix)/ftp-data/misc/who.head" ] && echo "$(prefix)/ftp-data/misc/who.head already exists. NOT overwriting it." || $(INSTALL) -m644 who.head $(prefix)/ftp-data/misc)
#	@([ -f "$(prefix)/bin/sitewho.conf" ] && echo "$(prefix)/bin/sitewho.conf already exists. NOT overwriting it." || $(INSTALL) -m644 sitewho.conf $(prefix)/bin)
#	@(echo)
	@(printf "\nCreating sitewho.conf file...\n")
	@([ -f "$(prefix)/sitewho.conf" ] && echo "$(prefix)/bin/sitewho.conf already exists. NOT overwriting it." || $(INSTALL) -m644 sitewho.conf.in $(prefix)/sitewho.conf)
	@(printf "\nCreating GeoIP2 config...\n")
	@(if [ -f $(GEOIPCFG) ] && [ -w ./$(GEOIPCFG) ]; then \
		echo "AccountID 0" >> $(GEOIPCFG); \
		echo "LicenseKey 000000000000" >> $(GEOIPCFG); \
		echo "EditionIDs GeoLite2-Country" >> $(GEOIPCFG); \
		echo "#DatabaseDirectory /var/lib/GeoIP" >> $(GEOIPCFG); \
		printf "\nDont forget to Signup (free) and add your AccountId and LicenseKey to $(GEOIPCFG)\n"; \
		printf "https://www.maxmind.com/en/geolite2/signup\n"; \
	else \
		printf "$(GEOIPCFG) already exists or is not writable, skipping.\n"; \
	fi)
ifeq (1,${MAXMINDDB})
	@(printf "\"Make sure you have libmaxminddb installed, see README.md for details\n")
endif
	@(echo )

install: all

distclean: clean
	$(RM) sitewho.conf

clean:
	$(RM) sitewho

strip:
	strip sitewho

