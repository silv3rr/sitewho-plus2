# sitewho+2
### /saīthuːplʌstu/

Why +2? Because this modfied version of pzs-ng's sitewho adds 2 things:

- User ident@host and IP address
- GeoIP2 County Code

> Looking for unmodified sitewho instead? Get that here: [sitewho](https://github.com/pzs-ng/pzs-ng/tree/master/sitewho) ([pzs-ng.eu](https://pzs-ng.eu))

## Requirements

Make sure you have libmaxminddb installed or you will get errors about missing header and "record_info_for_database failed".

Debian: `apt install libmaxminddb-dev geoipupdate`

RedHat: `yum install libmaxminddb-devel geoipupdate geoipupdate-cron`

Or download from:
- https://github.com/maxmind/libmaxminddb/releases
- https://github.com/maxmind/geoipupdate/releases

## Installation

Now run make: `make && make install`

"make install" will not copy/create any sitewho files to your glftpd dir, so you can keep it seperate from the normal version. Just run sitewho+2 from current dir.

geoipupdate downloads and updates the required `/var/lib/GeoIP/GeoLite2-Country.mmdb` file.
It needs /etc/GeoIP.conf which should have been created by "make install". If not, try: `make geoipconf`.

If geoipupdate runs OK you can add a weekly cron to keep mmdb updated:

`ln -s `which geoipupdate` /etc/cron.weekly/geoipupdate`

On RedHat installing geoipupdate-cron takes care of this

## Usage

Only raw mode is changed, not normal output and the other modes. The new IP and CC fields should show up at the end of each line:

`./sitewho --raw` 

