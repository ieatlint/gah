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
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <termios.h>
#include <curses.h>
#include <signal.h>
#include <sys/ioctl.h>
using namespace std;

#define VERSION "1.38B"
#define PORT	5190

bool allisgood=true;
int sig=0;
void handler(int buf);
void help();
#include "struct.h"
#include "log.h"
#include "ui.h"
#include "conf.h"
#include "toc.h"


int main(int argc, char* argv[]) {
	cout<<"gah version "<<VERSION<<" by Jeffrey Malone <ieatlint@earthlink.net>"<<endl;
	bool debug=false;
	char *user=NULL,*pass=NULL,*configfile=NULL;
	int port=PORT;
	if(argc>1) {
		argv++;
		for(;argv[0];argv++) {
			if(argv[0][0]=='-') {
				switch((int)argv[0][1]) {
					case 'd'://debug
						cout<<"In debug mode.  Note that ALL DATA is logged!"<<endl;
						debug=true;
						break;
					case 'v'://version
						exit(0);
						break;
					case 'u':
						if(argv[0][2]!='\0') {
							argv[0]+=2;
						}
						else {
							argv++;
							if(!argv[0]) {
								cerr<<"-u requires an argument!"<<endl;
								help();
								exit(1);
							}
						}
						user=argv[0];
						break;
					case 'p':
						if(argv[0][2]!='\0') {
							argv[0]+=2;
						}
						else {
							argv++;
							if(!argv[0]) {
								cerr<<"-p requires an argument!"<<endl;
								help();
								exit(1);
							}
						}
						pass=argv[0];
						break;
					case 'h':
						help();
						exit(0);
						break;
					case 'P':
						if(argv[0][2]!='\0') {
							argv[0]+=2;
						}
						else {
							argv++;
							if(!argv[0]) {
								cerr<<"-P requires an argument!"<<endl;
								help();
								exit(1);
							}
						}
						port=atoi(argv[0]);
						if(port>65535 || port<1) {
							cerr<<"Invalid port.  Must be between 1 and 65535"<<endl;
							exit(1);
						}
						break;
					case 'c':
						if(argv[0][2]!='\0') {
							argv[0]+=2;
						}
						else {
							argv++;
							if(!argv[0]) {
								cerr<<"-c requires an argument"<<endl;
								help();
								exit(1);
							}
						}
						configfile=argv[0];
						break;
					default:
						if(!strncmp(argv[0],"--help",6)) {
							help();
							exit(0);
						}
						cerr<<"Unkown command '"<<argv[0]<<"'"<<endl<<endl;
						help();
						exit(1);
				}
			}
			else {
				cerr<<"Unknown command '"<<argv[0]<<"'"<<endl<<endl;
				help();
				exit(1);
			}
		}
	}
	signal(SIGINT,handler);
	signal(SIGWINCH,handler);
	debuglog log(debug);
	toc conn(&log,configfile);
	conn.setlogin(user,pass);
	conn.signon();
	conn.loop();
	return 0;
}

void handler(int buf) {
	sig=buf;
	allisgood=false;
}

void help() {

cout<<"gah is a console AIM client"<<endl<<endl;

cout<<"Usage:"<<endl;
cout<<"gah [options]"<<endl<<endl;

cout<<"Options:"<<endl;
cout<<" -d == Debug mode."<<endl;
cout<<" -v == Exit after version is printed."<<endl;
cout<<" -u == Specify username"<<endl;
cout<<" -l == Alias to above"<<endl;
cout<<" -p == specify password"<<endl;
cout<<" -P == Specify port to connect to"<<endl;
cout<<" -h == Print this message"<<endl<<endl;

cout<<"Example:"<<endl;
cout<<"gah -umyusername -p password"<<endl;
}
