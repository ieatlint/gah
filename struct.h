/* Contains all data structures and definition tags */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define PORT		5190
#define HOSTNAME	"toc.oscar.aol.com"

#define STDIN		1
#define STDOUT		0
#define BUFFER_SIZE	2048 //No need to make this higher, 2048 is tocs max
#define MAX_BUDDIES	100 //max num of buddies in your buddy list

#define SIGNON		1
#define DATA		2
#define KEEPALIVE	5


struct buddylist {
	char nuser[17];//normalized username
	char fuser[32];//formatted username
	char group[32];
	bool online;
	unsigned int warn;
	time_t signon;
	unsigned int idle;
	char away[47];
	bool avail;
	buddylist();
};
buddylist::buddylist() {
	online=false;
	fuser[0]='\0';
	away[0]='\0';
}

struct logdata {
	char user[17];
	int what;
	FILE *log;
};


struct flaphdr {
	unsigned char ast;
	unsigned char type;
	unsigned short seq;
	unsigned short len;
};
struct sodata {
	unsigned long ver;
	unsigned short tlv;
	unsigned short len;
	char username[80];
	sodata();
};

sodata::sodata() {
	ver=htonl(1);
	tlv=htons(1);
}

struct settings {
	char username[17];
	char password[35];
	char host[64];
	int port;
	bool log;
	settings();
};
settings::settings() {
	username[0]='\0';
	password[0]='\0';
	strcpy(host,HOSTNAME);
	port=PORT;
	log=false;
}
