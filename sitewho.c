#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <maxminddb.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "structonline.h"
#include "sitewho.h"

int		debug = 0;
int		groups = 0, GROUPS = 0;

static struct ONLINE *user;
static struct GROUP **group;

long long	shmid;
struct shmid_ds	ipcbuf;
struct stat	filestat;

char           *header = 0, *footer = 0, *glpath = 0, *mpaths = 0, *husers = 0, *hgroups = 0, *ipckey = 0, *glgroup = 0, *nocase = 0, *mmdb_file = 0,
               *def_ipckey = "0x0000DEAD", *def_glgroup = "/etc/group", *def_nocase = "false", 
	       *def_husers = "pzs-ng", *def_hgroups = "pzs-ng", *def_mpaths = "/ftp-data/pzs-ng", *def_glpath = "/glftpd/",
	       *def_count_hidden = "true", *count_hidden = 0, *def_header = "/ftp-data/misc/who.head",
	       *def_footer = "/ftp-data/misc/who.foot", *def_mmdb_file = "/var/lib/GeoIP/GeoLite2-Country.mmdb";
int		maxusers = 20 , showall = 0, uploads = 0, downloads = 0, onlineusers = 0, browsers = 0, idlers = 0, chidden = 1,
		idle_barrier = -1, def_idle_barrier = 30, threshold = -1, def_threshold = 1024;
double		total_dn_speed = 0, total_up_speed = 0;
int		totusers = 0;

#if ( GLVERSION == 13232 )
	static int	glversion = 13232;
#elif ( GLVERSION == 20032 )
	static int	glversion = 20032;
#elif ( GLVERSION == 20132 )
	static int	glversion = 20132;
#elif ( GLVERSION == 20232 )
	static int	glversion = 20232;
#elif ( GLVERSION == 20164 )
	static int	glversion = 20164;
#else
	static int	glversion = 20264;
#endif

/* CORE CODE */
int 
main(int argc, char **argv)
{

#ifndef _WITH_SS5
	char		raw_output = 0;
	int		user_idx = 1;
#else
	char		raw_output = 2;
	int		user_idx = 2;
#endif
	int		gnum = 0;
	readconfig(argv[0]);
	if (!ipckey)
		ipckey = def_ipckey;
	if (!glgroup)
		glgroup = def_glgroup;
	if (!nocase)
		nocase = def_nocase;
	if (!husers)
		husers = def_husers;
	if (!hgroups)
		hgroups = def_hgroups;
	if (!mpaths)
		mpaths = def_mpaths;
	if (!glpath)
		glpath = def_glpath;
	if (!count_hidden)
		count_hidden = def_count_hidden;
	if (!header)
		header = def_header;
	if (!footer)
		footer = def_footer;
	if (!mmdb_file)
		mmdb_file = def_mmdb_file;
	if (idle_barrier < 0)
		idle_barrier = def_idle_barrier;
	if (threshold < 1)
		threshold = def_threshold;

	gnum = buffer_groups(glgroup, 0);

	if (argc > 1 && strlen(argv[1]) == 5) {
		if (!strcasecmp(argv[1], "--raw")) {
			user_idx = 2;
			raw_output = 1;
		} else if (!strcasecmp(argv[1], "--ss5")) {
			user_idx = 2;
			raw_output = 2;
		} else if (!strcasecmp(argv[1], "--nbw")) {
			user_idx = 2;
			raw_output = 3;
		}
	}
	if ((shmid = shmget((key_t) strtoll(ipckey, NULL, 16), 0, 0)) == -1) {
		if (argc == 1 || (raw_output)) {
			if (!raw_output && strlen(header))
				show(header);
			showtotals(raw_output);
			if (!raw_output && strlen(footer))
				show(footer);
		} else {
			if (!raw_output)
				printf("\002%s\002 is not online\n", argv[user_idx]);
			else
				printf("\"ERROR\" \"User %s not online.\"\n", argv[user_idx]);
		}
		exit(0);
	}
	if ((user = (struct ONLINE *)shmat(shmid, NULL, SHM_RDONLY)) == (struct ONLINE *)-1) {
		if (!raw_output)
			printf("Error!: (SHMAT) failed...");
		else
			printf("\"ERROR\" \"SHMAT Failed.\"\n");
		exit(1);
	}

	if (shmctl(shmid, IPC_STAT, &ipcbuf) == -1) {
		perror("shmctl");
		exit(EXIT_FAILURE);
	}

	if (argc == 1 && (!raw_output) && strlen(header))
		show(header);

	totusers = (ipcbuf.shm_segsz / sizeof(struct ONLINE));
	if (raw_output < 2)
		showusers((totusers > maxusers ? maxusers : totusers), argc - raw_output - 1, argv[user_idx], raw_output);
	else if (argc == 1)
		showusers((totusers > maxusers ? maxusers : totusers), argc - 1, argv[user_idx], raw_output);
	else if (raw_output == 3)
		showusers((totusers > maxusers ? maxusers : totusers), argc - 2, argv[user_idx], raw_output);
	else
		showusers((totusers > maxusers ? maxusers : totusers), 0, argv[user_idx], raw_output);


	if (argc == 1 || raw_output == 3) {
		showtotals(raw_output);
		if (!raw_output && strlen(footer))
			show(footer);
	} else {
		if (!onlineusers) {
			if (!raw_output)
				printf("\002%s\002 is not online\n", argv[user_idx]);
			else
				printf("\"ERROR\" \"User %s not online.\"\n", argv[user_idx]);
		}
#ifndef _WITH_ALTWHO
		else if (!raw_output) {
			printf("\n");
		}
#endif
	}
	buffer_groups(glgroup, gnum);
	if (footer != def_footer)
		free(footer);
	if (header != def_header)
		free(header);
	if (mpaths != def_mpaths)
		free(mpaths);
	if (husers != def_husers)
		free(husers);
	if (hgroups != def_hgroups)
		free(hgroups);
	if (glpath != def_glpath)
		free(glpath);
	if (ipckey != def_ipckey)
		free(ipckey);
	if (glgroup != def_glgroup)
		free(glgroup);
	if (nocase != def_nocase)
		free(nocase);
	if (count_hidden != def_count_hidden)
		free(count_hidden);
	if (mmdb_file != def_mmdb_file)
		free(mmdb_file);
	return 0;
}

int
check_path(char *filename)
{
	if (stat(filename, &filestat) != 0)
		return 0;
	return 1;
}

unsigned long 
filesize(char *filename)
{
	char           *file;

	file = malloc(strlen(glpath) + strlen(filename) + 2);
	sprintf(file, "%s/%s", glpath, filename);
	if (stat(file, &filestat) != 0) {
		if (!strcmp(filename, "")) {
			filestat.st_size = 1;
		} else {
			sprintf(file, "/%s", filename);
			if (stat(file, &filestat) != 0) {
				if (!strcmp(filename, "")) {
					filestat.st_size = 1;
				} else {
					if (debug) {
						fprintf(stderr, "Could not stat file '%s', is glrootpath set correctly in sitewho.conf? (error: %s)\n", file, strerror(errno));
					}
					filestat.st_size = 1;
				}
			}
		}
	}
	free(file);
	return filestat.st_size;
}

char           *
get_g_name(unsigned int gid)
{
	int		n;

	for (n = 0; n < groups; n++) {
		if (group[n]->id == gid)
			return group[n]->name;
	}

	return "NoGroup";
}

int 
strplen(char *strin)
{
	int		n = 0;

	while (isprint(strin[n]) && strin[n])
		n++;

	return n;
}

short int
matchpath(char *instr, char *path)
{
	int pos = 0, c = 0;

	if ( (int)strlen(instr) < 2 || (int)strlen(path) < 2)
		return 0;

	do {
		switch (*instr) {
		case 0:
		case ' ':
			if ((int)strlen(path) == pos - 1 && *(path + pos - 2) != '/' && *(instr - 1) == '/')
				c = 1;
			if (!strncmp(instr - pos, path, pos - c)) {
				if (*(instr - 1) == '/')
					return 1;
				if ((int)strlen(path) >= pos) {
					if (*(path + pos) == '/' || *(path + pos) == '\0')
						return 1;
				} else
					return 1;
			}
			c = 0;
			pos = 0;
			break;
		default:
			++pos;
			break;
		}
	} while (*instr++);

	return 0;
}

short int
strcomp(char *instr, char *searchstr)
{
	int	pos = 0, ncase = 0;
	int	k = strlen(searchstr);

	if (!strncasecmp(nocase, "true", 4))
		ncase = 1;

	if ( (int)strlen(instr) == 0 || k == 0)
		return 0;

	do {
		switch (*instr) {
		case 0:
		case ' ':
			if (k == pos) {
				if (ncase == 0 && !strncmp(instr - pos, searchstr, pos)) {
					return 1;
				} else if (ncase == 1 && !strncasecmp(instr - pos, searchstr, pos)) {
					return 1;
				}
			}
			pos = 0;
			break;
		default:
			pos++;
			break;
		}
	} while (*instr++);
	return 0;
}

char *
host2ip (char *userhost)
{
	// Get GeoIP Country Code

	struct hostent *he;
	struct in_addr **addr_list;
	char *hostname, *rhost, **token, ibuf[46];
	char *ipaddr = "0.0.0.0";
	int k;

	token = (char **)malloc(sizeof(void*)*64);
	//rhost = strdup(user[x].host);
	rhost = strdup(userhost);
	//if (debug) { printf("DEBUG: original hostname: '%s'\n", hostname); }

	int j = 0; while( (*(token+j) = strsep(&rhost,"@")) != NULL )
		j++;

	//if (debug) { for (k=j-1; k>=0; k--) { printf("DEBUG: token+n: %s\n",*(token+k) ); } }
	if (debug) { printf("DEBUG: token+1 %s\n", *(token+1)); }
	hostname = *(token+1);

	free(rhost);
	free(token);

	if (strlen(hostname) > 0) {
		if ((he = gethostbyname(hostname))) {
			addr_list = (struct in_addr **) he->h_addr_list;
			for (k = 0; he->h_addr_list[k] != NULL; k++) {
				strcpy(ibuf, inet_ntoa(*addr_list[k]));
				if (debug) { printf("DEBUG: ibuf: %s\n", ibuf); }
			}
			if (ibuf[1]) {
				ipaddr = malloc(sizeof(ibuf)+1);
				strcpy(ipaddr, ibuf);
			}
		}
	}
	//printf("DEBUG: return ipaddr: %s\n", ipaddr);
	return ipaddr;
}

char *
get_mmdb(char *ipaddr)
{
	char *country_code;
	char *ip_address = ipaddr;
	country_code = "\"\"";

	MMDB_s mmdb;
	MMDB_open(mmdb_file, MMDB_MODE_MMAP, &mmdb);

	int gai_error, mmdb_error;
	MMDB_lookup_result_s result = MMDB_lookup_string(&mmdb, ip_address, &gai_error, &mmdb_error);

	MMDB_entry_data_list_s *entry_data_list = NULL;
	if (result.found_entry) {
		MMDB_get_entry_data_list(&result.entry, &entry_data_list);
	}

	//if (NULL != entry_data_list) { MMDB_dump_entry_data_list(stdout, entry_data_list, 2); }

	FILE *memstream;
	char *membuf, *line[1024], match[128];
	size_t memlen;
	int i;
	memstream = open_memstream(&membuf, &memlen);

	if (entry_data_list != NULL && memstream != NULL) {
		MMDB_dump_entry_data_list(memstream, entry_data_list, 2);
		fflush(memstream);
		//int i = 0; while( (*(r+i) = strsep(&membuf, "\n")) != NULL )

		i = 0 ; while ((line[i] = strsep(&membuf,"\n")) != NULL )
			i++;
		//for (int i = 0; i < sizeof(line)/sizeof(line[0]); i++)
		i = 0; while (line[i] != NULL) {
			if ((strstr(line[i], "iso_code")) != NULL) {
				i++;
				sprintf(match, "%.14s", line[i]);
				country_code = strstr(match, "\"");
				if (debug) { printf("DEBUG: line: %i %s\n", i, line[i]); }
				break;
			} else {
				i++;
			}
		}

	}
	fclose(memstream);
	free(membuf);

	MMDB_free_entry_data_list(entry_data_list);
	MMDB_close(&mmdb);

	//printf("DEBUG: return country_code: %s", country_code);
	return country_code;
}

void 
showusers(int n, int mode, char *ucomp, char raw)
{
	char		status    [30];
	char		online    [30];
	char           *filename = 0;
	char		realfile  [512];
	char		bar       [20];
	struct timeval	tstop;
	double		mb_xfered = 0;
	double		speed  , pct = 0;
	double		my_filesize = 0;
	int		mask;
	int		noshow;
	char		maskchar;
	int		i         , x, m;
	unsigned	hours;
	unsigned char	minutes;
	unsigned	seconds;

	gettimeofday(&tstop, (struct timezone *)0);

	if (!strncasecmp(count_hidden, "false", 5))
		chidden = 0;

	for (x = 0; x < n; x++) {
		if (!user[x].procid)
			continue;

		maskchar = ' ';
		mask = noshow = 0;
              
		if (strcomp(husers, user[x].username) || strcomp(hgroups, get_g_name(user[x].groupid))) {
			if (showall)
				maskchar = '*';
			else
				noshow++;
		}
		if (noshow == 0 && strlen(mpaths)) {
			if (maskchar == ' ' && matchpath(mpaths, user[x].currentdir)) {
				if (showall)
					maskchar = '*';
				else
					mask++;
			}
		}
		if (strplen(user[x].status) > 5)
			filename = malloc(strplen(user[x].status) - 5 + 1);
		else
			filename = malloc(1);

		if ((!strncasecmp(user[x].status, "STOR ", 5) ||
		     !strncasecmp(user[x].status, "APPE ", 5)) &&
		    user[x].bytes_xfer) {

			speed = user[x].bytes_xfer / 1024. /
				((tstop.tv_sec - user[x].tstart.tv_sec) * 1. + (tstop.tv_usec - user[x].tstart.tv_usec) / 1000000.);

			if (debug) {
				printf("DEBUG SPEED INFO: username   = %s\nDEBUG SPEED INFO: time spent = %.2f seconds\nDEBUG SPEED INFO: transfered = %.0f KB\nDEBUG SPEED INFO: speed      = %.2f KB/s\n", user[x].username, ((tstop.tv_sec - user[x].tstart.tv_sec) * 1. + (tstop.tv_usec - user[x].tstart.tv_usec) / 1000000.), (double)(user[x].bytes_xfer / 1024.), speed);
				}
			if ((!noshow && !mask && !(maskchar == '*')) || chidden) {
				total_up_speed += speed;
				uploads++;
			}
			if (!mask) {
				pct = -1;
				m = strplen(user[x].status) - 5;
				if (m < 15 || raw)
					sprintf(filename, "%.*s", m, user[x].status + 5);
				else
					sprintf(filename, "%.15s", user[x].status + m - 10);

				strcpy(bar, "?->");
				if (!raw)
					if (speed > threshold) {
						speed = (speed / 1024.0);
						sprintf(status, "Up: %7.2fMB/s", speed);
					} else {
						sprintf(status, "Up: %7.0fKB/s", speed);
					}
				else if (raw == 1)
					sprintf(status, "\"UP\" \"%.0f\"", speed);
				else
					sprintf(status, "upld|%.0f", speed);

				mb_xfered = user[x].bytes_xfer * 1.0 / 1024 / 1024;
			}
		} else if ((!strncasecmp(user[x].status, "RETR ", 5) && user[x].bytes_xfer)) {
			mb_xfered = 0;

			sprintf(realfile, "%s", user[x].currentdir);

			/*
			 * Dirty way to get around the fact that the buffered
			 * reading will change user[x].currentdir to not
			 * include filename once it's done reading the entire
			 * file "to memory". This means user[x].currentdir in
			 * fact will be _currentdir_ and this cannot tell us
			 * a true filesize since it's calculated from
			 * filesize(/site/incoming/path) - w/o filename :(
			 */
			my_filesize = filesize(realfile);
			if (my_filesize < user[x].bytes_xfer) {
				my_filesize = user[x].bytes_xfer;
			}
			pct = (user[x].bytes_xfer * 1. / my_filesize) * 100;
			i = 15 * user[x].bytes_xfer * 1. / my_filesize;
			i = (i > 15 ? 15 : i);

			bar[i] = 0;

			for (m = 0; m < i; m++)
				bar[m] = 'x';
			speed = user[x].bytes_xfer / 1024. /
				((tstop.tv_sec - user[x].tstart.tv_sec) * 1. +
				(tstop.tv_usec - user[x].tstart.tv_usec) / 1000000.);

			if (debug) {
				printf("DEBUG SPEED INFO: username   = %s\nDEBUG SPEED INFO: time spent = %.2f seconds\nDEBUG SPEED INFO: transfered = %.0f KB\nDEBUG SPEED INFO: speed      = %.2f KB/s\n", user[x].username, ((tstop.tv_sec - user[x].tstart.tv_sec) * 1. + (tstop.tv_usec - user[x].tstart.tv_usec) / 1000000.), (double)(user[x].bytes_xfer / 1024.), speed);
				}

			if ((!noshow && !mask && !(maskchar == '*')) || chidden) {
				total_dn_speed += speed;
				downloads++;
			}
			m = strplen(user[x].status) - 5;
			if (!mask) {
				if (m < 15 || raw)
					sprintf(filename, "%.*s", m, user[x].status + 5);
				else
					sprintf(filename, "%.15s", user[x].status + m - 10);

				if (!raw)
					if (speed > threshold) {
						speed = (speed / 1024.0);
						sprintf(status, "Dn: %7.2fMB/s", speed);
					} else {
						sprintf(status, "Dn: %7.0fKB/s", speed);
					}
				else if (raw == 1)
					sprintf(status, "\"DN\" \"%.1f\"", speed);
				else
					sprintf(status, "dnld|%.1f", speed);
			}
		} else {
			pct = *bar = *filename = hours = minutes = mb_xfered = 0;
			seconds = tstop.tv_sec - user[x].tstart.tv_sec;
			while (seconds >= 3600) {
				hours++;
				seconds -= 3600;
			}
			while (seconds >= 60) {
				minutes++;
				seconds -= 60;
			}
			if ((!noshow && !mask && !(maskchar == '*')) || chidden) {
				if ((int)seconds > idle_barrier)
					idlers++;
				else
					browsers++;
			}
			if (!raw)
				sprintf(status, "Idle: %02d:%02d:%02d", hours, minutes, seconds);
			else if (raw == 1)
				sprintf(status, "\"ID\" \"%d\"", (hours * 60 * 60) + (minutes * 60) + seconds);
			else
				sprintf(status, "idle|%02d:%02d:%02d", hours, minutes, seconds);
		}

		hours = minutes = 0;
		seconds = tstop.tv_sec - user[x].login_time;
		while (seconds >= 3600) {
			hours++;
			seconds -= 3600;
		}
		while (seconds >= 60) {
			minutes++;
			seconds -= 60;
		}
		sprintf(online, "%02d:%02d:%02d", hours, minutes, seconds);

		if (mode == 0 && raw != 3 ) {
			if (!raw && (showall || (!noshow && !mask && !(maskchar == '*')))) {
				if (mb_xfered)
					printf("|%1c%-16.16s/%-10.10s | %-15s | XFER: %13.1fMB |\n", maskchar, user[x].username, get_g_name(user[x].groupid), status, mb_xfered);
				else
					printf("|%1c%-16.16s/%-10.10s | %-15s | %3.0f%%: %-15.15s |\n", maskchar, user[x].username, get_g_name(user[x].groupid), status, pct, bar);

				printf("| %-27.27s | since %8.8s  | file: %-15.15s |\n", user[x].tagline, online, filename);
				printf("+-----------------------------------------------------------------------+\n");
			} else if (raw == 1 && (showall || (!noshow && !mask && !(maskchar == '*')))) {
				/*
				 * Maskeduser / Username / GroupName / Status
				 * / TagLine / Online / Filename / Part
				 * up/down-loaded / Current dir / PID
				 */
				//printf("\"USER\" \"%1c\" \"%s\" \"%s\" %s \"%s\" \"%s\" \"%s\" \"%.1f%s\" \"%s\" \"%d\"\n", maskchar, user[x].username, get_g_name(user[x].groupid), status, user[x].tagline, online, filename, (pct >= 0 ?pct : mb_xfered), (pct >= 0 ? "%" : "MB"), user[x].currentdir, user[x].procid);
				printf("\"USER\" \"%1c\" \"%s\" \"%s\" %s \"%s\" \"%s\" \"%s\" \"%.1f%s\" \"%s\" \"%d\" \"%s\" \"%s\" %s\n", maskchar, user[x].username, get_g_name(user[x].groupid), status, user[x].tagline, online, filename, (pct >= 0 ? pct : mb_xfered), (pct >= 0 ? "%" : "MB"), user[x].currentdir, user[x].procid, user[x].host, host2ip(user[x].host), get_mmdb(host2ip(user[x].host)));
			} else if (showall || (!noshow && !mask && !(maskchar == '*'))) {
				printf("%s|%s|%s|%s|%s\n", user[x].username, get_g_name(user[x].groupid), user[x].tagline, status, filename);
			}
			if ((!noshow && !mask && !(maskchar == '*')) || chidden) {
				onlineusers++;
			}
		} else if (raw == 3) {
			if ((!noshow && !mask && !(maskchar == '*')) || chidden) {
				onlineusers++;
			}
		} else if (user[x].username && !strcmp(ucomp, user[x].username)) {
#ifdef _WITH_ALTWHO
			if (!raw && (showall || (!noshow && !mask && !(maskchar == '*')))) {
				if (mb_xfered)
					printf("%s : %1c%s/%s has xfered %.1fMB of %s and has been online for %8.8s.\n", status, maskchar, user[x].username, get_g_name(user[x].groupid), mb_xfered, filename, online);
				else if (strcmp(filename, ""))
					printf("%s : %1c%s/%s has xfered %.0f%% of %s and has been online for %8.8s.\n", status, maskchar, user[x].username, get_g_name(user[x].groupid), pct, filename, online);
				else
					printf("%s : %1c%s/%s has been online for %8.8s.\n", status, maskchar, user[x].username, get_g_name(user[x].groupid), online);
			} else if (raw == 1 && (showall || (!noshow && !mask && !(maskchar == '*')))) {
				printf("\"USER\" \"%1c\" \"%s\" \"%s\" %s \"%s\" \"%s\" \"%s\" \"%.1f%s\" \"%s\" \"%d\"\n", maskchar, user[x].username, get_g_name(user[x].groupid), status, user[x].tagline, online, filename, (pct >= 0 ? pct : mb_xfered), (pct >= 0 ? "%" : "MB"), user[x].currentdir, user[x].procid);
			} else if (showall || (!noshow && !mask && !(maskchar == '*'))) {
				printf("%s|%s|%s|%s|%s\n", user[x].username, get_g_name(user[x].groupid), user[x].tagline, status, filename);
			}
#else
			if (!onlineusers) {
				if (!raw && (showall || (!noshow && !mask && !(maskchar == '*'))))
					printf("\002%s\002 - %s", user[x].username, status);
				else if (raw == 1 && (showall || (!noshow && !mask && !(maskchar == '*'))))
					printf("\"USER\" \"%s\" %s", user[x].username, status);
				else if (showall || (!noshow && !mask && !(maskchar == '*')))
					printf("\002%s\002 - %s", user[x].username, status);
			} else {
				if (!raw && (showall || (!noshow && !mask && !(maskchar == '*'))))
					printf(" - %s", status);
				else if (raw == 1 && (showall || (!noshow && !mask && !(maskchar == '*'))))
					printf("\"USER\" \"\" %s", status);
				else if (showall || (!noshow && !mask && !(maskchar == '*')))
					printf(" - %s", status);
			}
#endif
			if (!noshow && !mask && !(maskchar == '*'))
				onlineusers++;
			else if (chidden)
				onlineusers++;

		}
		free(filename);
	}
}

/* COMPARE USERFLAGS ON CHECKFLAGS */
short 
compareflags(char *flags, char *checkflags)
{
	unsigned int	n1 = 0, n2 = 0;
	char           *userflags;

	userflags = (flags != NULL ? flags : "3");

	for (n1 = 0; n1 < strlen(userflags); n1++) {
		for (n2 = 0; n2 < strlen(checkflags); n2++) {
			if (*(userflags + n1) == *(checkflags + n2))
				return 1;
		}
	}
	return 0;
}

/* READ CONFIGURATION FILE */
void 
readconfig(char *arg)
{
	char           *buf, *tmp;
	FILE           *cfgfile;
	int		n         , b_w = 0,	/* Beginning of second word */
			e_c = 0,/* Position of equal char */
			e_w = 0,/* End of first word */
			l_b = 0;/* Beginning of the line */

	n = strlen(arg);
	while (arg[n] != '/' && n > 0)
		n--;

	if (n == 0) {
		tmp = malloc(18);
		sprintf(tmp, "/bin/sitewho.conf");
	} else {
		tmp = malloc(n + 14);
		sprintf(tmp, "%.*s/sitewho.conf", n, arg);
	}

	if (stat(tmp, &filestat) == -1 || filestat.st_size <= 0) {
		printf("Config file does not exist (%s)\n", tmp);
		exit(0);
	}
	cfgfile = fopen(tmp, "r");
	free(tmp);
	buf = malloc(filestat.st_size);
	*buf = 0;
	if (!fread(buf, 1, filestat.st_size, cfgfile)) {
		printf("Failed to fread() %s: %s\n", tmp, strerror(errno));
	}
	fclose(cfgfile);

	for (n = 0; n < filestat.st_size; n++) {
		switch (*(buf + n)) {
		case '\n':
			if (b_w > l_b && e_w > l_b && b_w > e_c) {
				tmp = malloc(n - b_w + 1);
				memcpy(tmp, buf + b_w, n - b_w);
				*(tmp + n - b_w) = 0;
				if (!memcmp(buf + l_b, "headerfile", 10))
					header = tmp;
				else if (!memcmp(buf + l_b, "footerfile", 10))
					footer = tmp;
				else if (!memcmp(buf + l_b, "maskeddirectories", 17))
					mpaths = tmp;
				else if (!memcmp(buf + l_b, "hiddenusers", 11))
					husers = tmp;
				else if (!memcmp(buf + l_b, "hiddengroups", 12))
					hgroups = tmp;
				else if (!memcmp(buf + l_b, "glrootpath", 10))
					glpath = tmp;
				else if (!memcmp(buf + l_b, "ipc_key", 7))
					ipckey = tmp;
				else if (!memcmp(buf + l_b, "grp_path", 8))
					glgroup = tmp;
				else if (!memcmp(buf + l_b, "case_insensitive", 16))
					nocase = tmp;
				else if (!memcmp(buf + l_b, "count_hidden", 12))
					count_hidden = tmp;
				else if (!memcmp(buf + l_b, "mmdb_file", 8))
					mmdb_file = tmp;
				else {
					if (!memcmp(buf + l_b, "seeallflags", 11))
						showall = compareflags(getenv("FLAGS"), tmp);
					else if (!memcmp(buf + l_b, "maxusers", 8))
						maxusers = atoi(tmp);
					else if (!memcmp(buf + l_b, "idle_barrier", 12))
						idle_barrier = atoi(tmp);
					else if (!memcmp(buf + l_b, "speed_threshold", 15))
						threshold = atoi(tmp);
					else if (!memcmp(buf + l_b, "debug", 5))
						debug = atoi(tmp);
					free(tmp);
				}
			}
			l_b = n + 1;
			b_w = 0;
			break;
		case '=':
			if (!e_c && !b_w) {
				e_c = n;	/* Only one '=' char per line
						 * counts */
				b_w = n + 1;
			}
			break;
		case '\t':
		case ' ':
			if (l_b == ' ' || l_b == '\t')
				l_b++;	/* Remove spaces from beginning of
					 * the line */
			else if (e_w <= l_b)
				e_w = n;	/* End of first word */
			else if (e_c)
				b_w = n + 1;	/* Beginning of second word */
			break;
		default:
			e_c = 0;
		}
	}
	free(buf);

	if (debug) {
		printf("DEBUG: glversion=\"%d\"\nDEBUG: header=\"%s\"\nDEBUG: footer=\"%s\"\nDEBUG: mpaths=\"%s\"\nDEBUG: husers=\"%s\"\nDEBUG: hgroups=\"%s\"\nDEBUG: glpath=\"%s\"\nDEBUG: ipckey=\"%s\"\nDEBUG: glgroup=\"%s\"\nDEBUG: nocase=\"%s\"\nDEBUG: count_hidden=\"%s\"\nDEBUG: showall=\"%d\"\nDEBUG: maxusers=\"%d\"\nDEBUG: idle_barrier=\"%d\"\nDEBUG: threshold=\"%d\"\nDEBUG: mmdb_file=\"%s\"\n", glversion, header, footer, mpaths, husers, hgroups, glpath, ipckey, glgroup, nocase, count_hidden, showall, maxusers, idle_barrier, threshold, mmdb_file);
	}

//	if (filesize("") == 1)
//		*glpath = 0;
}

/* PRINT FILE */
void 
show(char *filename)
{
	int		fd        , n;
	char		buf       [128];
	char           *fname = 0;

	fname = malloc(strlen(glpath) + strlen(filename) + 2);
	sprintf(fname, "%s/%s", glpath, filename);
	if (!check_path(fname))
		sprintf(fname, "/%s", filename);

	if ((fd = open(fname, O_RDONLY)) != -1) {
		while ((n = read(fd, buf, sizeof(buf))) > 0)
			printf("%.*s", n, buf);
		close(fd);
	} else {
		sprintf(fname, "/%s", filename);
		if ((fd = open(fname, O_RDONLY)) != -1) {
			while ((n = read(fd, buf, sizeof(buf))) > 0)
				printf("%.*s", n, buf);
			close(fd);
		}
	}
	free(fname);
}

/* SHOW TOTALS */
void 
showtotals(char raw)
{
	if (!raw) {
		if ((total_up_speed > threshold) || (total_dn_speed > threshold)) {
			total_up_speed = (total_up_speed / 1024);
			total_dn_speed = (total_dn_speed / 1024);
			printf("| Up: %2i / %7.2fMB/s | Dn: %2i / %7.2fMB/s | Total: %2i / %7.2fMB/s |\n", uploads, total_up_speed, downloads, total_dn_speed, uploads + downloads, total_up_speed + total_dn_speed);
		} else {
			printf("| Up: %2i / %7.0fKB/s | Dn: %2i / %7.0fKB/s | Total: %2i / %7.0fKB/s |\n", uploads, total_up_speed, downloads, total_dn_speed, uploads + downloads, total_up_speed + total_dn_speed);
		}
		printf("| Currently %2i of %2i users are online...                                |\n", onlineusers, maxusers);
	} else if (raw == 1) {
		/*
		 * UpUsers / UpSpeed / DnUsers / DnSpeed / TotalUsers /
		 * TotalSpeed
		 */
		printf("\"STATS\" \"%i\" \"%.1f\" \"%i\" \"%.1f\" \"%i\" \"%.1f\"\n", uploads, total_up_speed, downloads, total_dn_speed, uploads + downloads, total_up_speed + total_dn_speed);
		/* } else { */
		/*
		 * upld | UpUsers | UpSpeed | dnld | DnUsers |DnSpeed | total
		 * | TotalUsers | TotalSpeed | online | OnlineUsers |
		 * MaxUsers
		 */
		/*
		 * printf("upld|%3i|%7.1f|dnld|%3i|%7.1f|total%3i|%7.1f|online
		 * |%2i|%2i\n", uploads, total_up_speed, downloads,
		 * total_dn_speed, uploads + downloads, total_up_speed +
		 * total_dn_speed, onlineusers, maxusers);
		 */
	} else if (raw == 3) {
		printf("%i %.1f %i %.1f %i %.1f %i %i %i %i\n", uploads, total_up_speed, downloads, total_dn_speed, uploads + downloads, total_up_speed + total_dn_speed, browsers, idlers, onlineusers, maxusers);
	}
}

/* Buffer groups file */
int 
buffer_groups(char *groupfile, int setfree)
{
	char           *f_buf, *g_name, *f_name;
	long		f, n = 0, m, f_size, g_id, g_n_size, l_start = 0;

	if (setfree) {
		for (n = 0; n < setfree; n++) {
			free(group[n]->name);
			free(group[n]);
		}
		free(group);
		return 0;
        }

	f_name = malloc(strlen(glpath) + strlen(groupfile) + 2);
	sprintf(f_name, "%s/%s", glpath, groupfile);
	if (!check_path(f_name))
		sprintf(f_name, "/%s", groupfile);
	f = open(f_name, O_NONBLOCK);
	fstat(f, &filestat);
	f_size = filestat.st_size;
	f_buf = malloc(f_size);
	if (read(f, f_buf, f_size) == -1) {
		printf("Failed to read() %s: %s\n", f_name, strerror(errno));
	}

	for (n = 0; n < f_size; n++)
		if (f_buf[n] == '\n')
			GROUPS++;
	group = malloc(GROUPS * sizeof(struct GROUP *));

	for (n = 0; n < f_size; n++) {
		if (f_buf[n] == '\n' || n == f_size) {
			f_buf[n] = 0;
			m = l_start;
			while (f_buf[m] != ':' && m < n)
				m++;
			if (m != l_start) {
				f_buf[m] = 0;
				g_name = f_buf + l_start;
				g_n_size = m - l_start;
				m = n;
				while (f_buf[m] != ':' && m > l_start)
					m--;
				f_buf[m] = 0;
				while (f_buf[m] != ':' && m > l_start)
					m--;
				if (m != n) {
					g_id = atoi(f_buf + m + 1);
					group[groups] = malloc(sizeof(struct GROUP));
					group[groups]->name = malloc(g_n_size + 1);
					strcpy(group[groups]->name, g_name);
					group[groups]->id = g_id;
					groups++;
				}
			}
			l_start = n + 1;
		}
	}

	close(f);
	free(f_buf);
	free(f_name);
	return groups;
}

