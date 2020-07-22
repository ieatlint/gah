/*
 *
 * gah -- toc.h -- v1.36 Beta
 * Performs all functions needed to connect to AIM using the TOC protocol and
 * maintain that connection.  It detects user and network input and processes
 * it on the fly.
 *
 * 
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

class toc {
	private:
		debuglog *log;
		logging *logger;
		struct buddylist bdlist[MAX_BUDDIES+1],*me;
		struct settings data;
		int sockfd,seq,buddies,totalbuds;
		char passwd[35],cuser[17];
		struct hostent *he;
		struct sockaddr_in dst;
		userint gui;

		/* Too Many Functions */
		void parse(char*);
		void parsenet(char*);
		void roastpass(char*);

		void upidle();
		void whois(char*);
		void whom();
		void setidle(char*);
		void setaway(char*);
		void setupbud(char*);
		int updatebud(char*);
		int findbuddy(char*);
		void normalize(char*,char*);
		void addslashes(char*,char*);

		void sndim(char*,char*,bool);
		void snddata(int,int,char*);
		void rcvdata();
	public:
		toc(debuglog*,char*);
		void setlogin(char*,char*);
		void signon();
		void loop();
};

toc::toc(debuglog *logtmp,char *configfile) {
	conf cfile(configfile,&data);
	logging loggertmp(data.log);
	logger=&loggertmp;
	cuser[0]='\0';
	me=&bdlist[MAX_BUDDIES];
	bdlist[MAX_BUDDIES].avail=true;
	strcpy(me->group,"n/a");
	gui.log=logtmp;
	log=logtmp;
	log->start("init");
	if((he=gethostbyname(data.host))==NULL) {
		cerr<<data.host<<" failed to resolve"<<endl;
		exit(1);
	}
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	dst.sin_family=AF_INET;
	dst.sin_port=htons(data.port);
	dst.sin_addr=*((struct in_addr *)he->h_addr);
	memset(&dst.sin_zero,'\0',8);
	seq=0;
	log->end();
}

void toc::setlogin(char *user,char *pass) {
	log->start("setlogin");
	char *buf=pass;
	if(user==NULL) {
		if(data.username[0]=='\0') {
			cout<<"Login: ";
			cin.getline(me->nuser,16);
		}
		else {
			strcpy(me->nuser,data.username);
		}
	}
	else {
		strncpy(me->nuser,user,16);
	}
	if(pass==NULL) {
		if(data.password[0]=='\0') {
			char password[17];
			buf=password;
			struct termios n,s;
			tcgetattr(0,&s);
			n=s;
			n.c_lflag &=(~ECHO);
			cout<<"Password: ";
			tcsetattr(0,TCSANOW,&n);
			cin.getline(password,16);
			tcsetattr(0,TCSANOW,&s);
			cout.put('\n');
			roastpass(password);
		}
		else {
			buf=data.password;
		}
	}
	if(buf[0]=='0' && buf[1]=='x') {
		strcpy(passwd,buf);
	}
	else {
		roastpass(buf);
	}
	log->fend("setlogin");
}

void toc::roastpass(char *pass) {
	log->start("roastpass");
	char roast[]="Tic/Toc";
	int pos=2;
	int x;
	strcpy(passwd,"0x");
	for(x=0;(x<150) && pass[x];x++)
		pos+=sprintf(&passwd[pos],"%02x",pass[x]^roast[x%7]);
	passwd[pos]='\0';
	log->end();
}

	

void toc::loop() {
	char buf[BUFFER_SIZE];
	char *p=buf;
	int inbuf;
	fd_set a,b;
	timeval wait;
	FD_ZERO(&a);
	FD_SET(1,&a);
	FD_SET(sockfd,&a);
	wait.tv_usec=0;
	gui.start();
	while(1) {
		while(allisgood) {
			wait.tv_sec=60;
			b=a;
			select(sockfd+1,&b,NULL,NULL,&wait);
			if(FD_ISSET(STDIN,&b)) {
				inbuf=gui.getinput();
				if(isascii(inbuf)) {
					if(inbuf=='\n') {
						p[0]='\0';
						if(buf[0]=='\0') continue;//no blank lines
						gui.eol();
						parse(buf);
						p=buf;
					}
					else {
						p[0]=inbuf;
						gui.putch((char)inbuf);
						p++;
					}
				}
				else if(inbuf==KEY_BACKSPACE) {
					if(p!=buf) {
						p--;
						if(p[0]=='\t') {
							for(int i=0;i<(9-(strlen(buf)%9));i++) gui.bksp();
						}
						else gui.bksp();
						p[0]='\0';
					}
				}
				continue;
			}
			if(FD_ISSET(sockfd,&b)) {
				log->write("data on network");
				rcvdata();
				continue;
			}
			//if neither are set, then theres been no data for 60 seconds.  keepalives are sent every 50 seconds.  the connection must be dead. OR theres been an error on select run, either way were fucked.
			sprintf(buf,"\nA fatal network error has occured: %s",strerror(errno));
			log->write(buf);
			gui.print(buf);
			break;
		}
		//handle quit
		if(sig==0) break;
		if(sig==SIGWINCH) {
			sig=0;
			allisgood=true;
			gui.redraw();
			gui.setstatus(&bdlist[MAX_BUDDIES],true);
			if(cuser[0]!='\0') {
				int y=findbuddy(cuser);
				if(y==-1) gui.setnick(cuser);
				else gui.setstatus(&bdlist[y],false);
			}
			continue;
		}
		else {
			break;
		}
	}
	close(sockfd);
	gui.close();
}

void toc::upidle() {
	log->start("upidle");
	for(int i=0;i<MAX_BUDDIES+1;i++) {
		if(bdlist[i].idle>0) {
			bdlist[i].idle+=50;//increment 50 seconds.
		}
		if(!strncmp(&bdlist[i].nuser[1],&cuser[1],strlen(&cuser[1]))) {
			gui.setstatus(&bdlist[i],false);
		}
		if(i==MAX_BUDDIES) {
			gui.setstatus(&bdlist[i],true);
		}
	}
	log->end();
}

void toc::parse(char *buf) {
	log->start("parse");
	if(buf[0]=='/') {
		log->write("command parse");
		buf++;
		if(!strncmp(buf,"query",5)) {
			log->write("query");
			buf+=6;
			strcpy(cuser,buf);
			int i=0;
			i=findbuddy(cuser);
			if(i==-1) {
			       gui.setnick(cuser);
			       setupbud(cuser);
			}
			else gui.setstatus(&bdlist[i],false);
		}
		else if(!strncmp(buf,"msg",3) || !strncmp(buf,"auto",4)) {
			log->write("msg");
			bool autor;
			if(buf[0]=='a') {
				autor=true;
				buf+=5;
			}
			else {
				autor=false;
				buf+=4;
			}
			char user[17];
			int i;
			for(i=0;buf[0]!=' ' && i<16;i++,buf++) user[i]=buf[0];
			buf++;
			sndim(user,buf,autor);
		}
		else if(!strncmp(buf,"away",4)) {
			log->write("away");
			buf+=4;
			if(buf[0]==' ') buf++;
			setaway(buf);
		}
		else if(!strncmp(buf,"idle",4)) {
			log->write("idle");
			buf+=4;
			setidle(buf);
		}
		else if(!strncmp(buf,"whois",5) || !strncmp(buf,"wi",2)) {
			log->write("whois");
			if(buf[1]=='h')
				buf+=5;
			else
				buf+=2;
			if(buf[0]=='\0') {
				whois(cuser);
			}
			else {
				buf++;
				whois(buf);
			}
		}
		else if(!strncmp(buf,"whoami",6)) {
			log->write("whoami");
			whois(me->nuser);
		}
		else if(!strncmp(buf,"who",3)) {
			log->write("who");
			whom();
		}
		else if(!strncmp(buf,"quit",4) || !strncmp(buf,"exit",4)) {
			log->close("quit");
			close(sockfd);
			gui.close();
		}
		else if(!strncmp(buf,"quote",5)) {
			buf+=6;
			log->write("User is performing a raw command, pray for them:");
			log->write(buf);
			snddata(DATA,strlen(buf),buf);
		}
		else {
			log->write("errcmd");
			gui.print("Unrecognized command.");
		}
	}
	else if(cuser[0]=='\0') {
		log->write("talking to non-existant query");
		gui.print("Your not in a query with anyone!");
	}
	else {
		log->write("talking to query");
		sndim(cuser,buf,false);
	}
	log->fend("parse");
}
void toc::whois(char *who) {
	log->start("whois");
	int i=findbuddy(who);
	if(i==-1) {
		gui.print("That user is not on your buddy list!");
	}
	else {
		char buf[256];
		sprintf(buf,"%s is in group %s, is ",bdlist[i].fuser,bdlist[i].group);
		if(bdlist[i].online) {
			struct tm *ctime;
			time_t beep;
			time(&beep);
			beep-=bdlist[i].signon;
			int idle=beep/60;
			int hr=idle/60;
			idle%=60;
			ctime=localtime(&bdlist[i].signon);
			sprintf(&buf[strlen(buf)],"online, has a %i%% warning level and signed on at %02i:%02i:%02i on %02i/%02i/%02i (%i hours and %i min ago).  They are currently ",bdlist[i].warn,ctime->tm_hour,ctime->tm_min,ctime->tm_sec,ctime->tm_mon+1,ctime->tm_mday,ctime->tm_year-100,hr,idle);
			if(bdlist[i].idle>0) {
				idle=bdlist[i].idle/60;
				hr=idle/60;
				idle%=60;
				sprintf(&buf[strlen(buf)],"idled for %i hours and %i minutes",hr,idle);
			}
			else {
				strcat(buf,"not idled");
			}
			if(bdlist[i].avail) {
				strcat(buf," and are not set as away.");
			}
			else {
				strcat(buf," and are marked as away.");
			}
			log->write(buf);
			gui.print(buf);
		}
		else {
			log->write("offline");
			strcat(buf,"offline.");
			gui.print(buf);
		}
	}
	log->fend("whois");
}

void toc::whom() {//bdlist print
	log->start("whom");
	char buf[120];
	for(int x=0;x<MAX_BUDDIES;x++) {
		if(bdlist[x].online==false) continue;
		sprintf(buf,"%s from group %s is ",bdlist[x].fuser,bdlist[x].group);
		if(bdlist[x].online) {//im aware this will always be true
			strcat(buf,"online ");
			if(bdlist[x].avail) {
				strcat(buf,"and available.");
			}
			else {
				strcat(buf,"but away.");
			}
			
		}
		gui.print(buf);
	}
	log->fend("whom");
}
void toc::setidle(char *buf) {
	// HH:mm (HH can be really high)
	log->start("idle");
	int hrs,min,sec;
	if(buf[0]=='\0') {
		if(me->idle==0) sec=1;
		else sec=0;
	}
	else {
		buf++;
		hrs=atoi(buf);
		for(;buf[0]!=':' && buf[0]!='\0';buf++);
		if(buf[0]=='\0') min=0;
		else {
			buf++;
			min=atoi(buf);
		}
		min+=hrs*60;
		sec+=min*60;
	}
	char buff[80];
	sprintf(buff,"toc_set_idle %i",sec);
	snddata(DATA,strlen(buff),buff);
	log->fend("setidle");
}

void toc::setaway(char *buf) {
	log->start("setaway");
	if(me->avail) me->avail=false;
	else me->avail=true;
	gui.setstatus(&bdlist[MAX_BUDDIES],true);
	char buff[256]="toc_set_away";
	if(buf[0]!='\0') {
		sprintf(&buff[12]," \"%s\"",buf);
	}
	snddata(DATA,strlen(buff),buff);
	log->fend("setaway");
}

void toc::sndim(char *who,char *msg,bool autor) {
	log->start("sndim");
	char buf[BUFFER_SIZE];
	char whom[17];
	char mesg[BUFFER_SIZE];
	normalize(whom,who);
	addslashes(mesg,msg);
	log->write(msg);
	log->write(mesg);
	sprintf(buf,"toc_send_im %s \"%s\"",whom,mesg);
	if(autor) {
		strcat(buf," auto");
	}
	log->fwrite("sndim",buf);
	int x=findbuddy(who);
	if(x>-1) who=bdlist[x].fuser;
	gui.rcvim(who,msg,autor,true);//report the same data to be displayed as outgoing
	logger->im(who,msg,autor,false);
	snddata(DATA,strlen(buf),buf);
	log->fend("sndim");
}

void toc::signon() {//not in curses mode yet fyi
	log->start("signon");
	/* Connect */
	cout<<"Connecting";
	if(connect(sockfd,(struct sockaddr *)&dst,sizeof(struct sockaddr))==-1) {
		cerr<<"Unable to connect!"<<endl;
		exit(1);
	}
	cout.put('1');//these will be replaced with .'s for releases
	
	/* FLAPON String */
	if(send(sockfd,"FLAPON\r\n\r\n",10,0)==-1) {
		cerr<<"Failed to send FLAPON string"<<endl;
		close(sockfd);
		exit(1);
	}
	cout.put('2');

	/* FLAPON Packet-R */
	rcvdata();
	cout.put('3');

	/* FLAPON Packet-S */
	struct sodata so;
	strcpy(so.username,me->nuser);
	so.len=htons(strlen(me->nuser));
	snddata(SIGNON,8+strlen(me->nuser),(char *)&so);
	cout.put('4');
	
	/* toc_signon */
	char buf[120];
	sprintf(buf,"toc_signon login.oscar.aol.com 5190 %s %s english \"assmonkez\"",me->nuser,passwd);
	snddata(DATA,strlen(buf),buf);
	cout.put('5');

	/* SIGN_ON */
	rcvdata();
	cout.put('6');

	/* toc_add_buddy */
	sprintf(buf,"toc_add_buddy %s",me->nuser);
	snddata(DATA,strlen(buf),buf);
	cout.put('7');

	/* toc_init_done */
	strcpy(buf,"toc_init_done");
	snddata(DATA,strlen(buf),buf);
	cout.put('8');

	cout<<endl<<"Connected!"<<endl;
	log->fend("signon");

}

void toc::snddata(int type,int len,char *buf) {//len never includes null terminator here
	log->start("snddata");
	struct flaphdr sflap;
	sflap.ast='*';
	sflap.type=type;
	sflap.seq=htons(seq++);//i think this is right...
	len++;
	if(type==SIGNON && buf[0]==0) {
		len--;//this is easier then adding multiple if statements..
	}
	sflap.len=htons(len);
	char buff[len+6];
	memcpy(buff,&sflap,6);
	memcpy(&buff[6],buf,len);
	if(send(sockfd,&buff,len+6,0)==-1) {
		gui.print("A fatal network error has occured while sending data:");
		gui.print(strerror(errno));
		close(sockfd);
		gui.close();
	}
	log->fend("snddata");
}

void toc::rcvdata() {
	log->start("rcvdata");
	char buf[4097];
	struct flaphdr sflap;
	if(recv(sockfd,&sflap,6,0)==-1 || recv(sockfd,&buf,ntohs(sflap.len),0)==-1) {
		cout<<endl;
		printf("A fatal network error has occured:\n%s\n",strerror(errno));
		close(sockfd);
		if(seq>0) {
			gui.print(buf);
			gui.close();
		}
		else {
			cerr<<buf<<endl;
			exit(1);
		}
	
	}
	buf[ntohs(sflap.len)]='\0';
	if(sflap.type==DATA) {
		log->write("data msg recv");
		parsenet(buf);
	}
	else if(sflap.type==KEEPALIVE) {
		log->write("keepalive recv");
		upidle();
	}
	else if(sflap.type==SIGNON) {
		log->write("flapon recv");
		seq=ntohs(sflap.seq);
	}
	else {
		sprintf(buf,"A malformed packet has been received.",strerror(errno));
		gui.print(buf);
		close(sockfd);
		gui.close();
	}
	log->fend("rcvdata");
}

void toc::setupbud(char *user) {
	//takes formated or normalized
	char username[17];
	normalize(username,user);
	char buf[32];
	sprintf(buf,"toc_add_buddy %s",username);
	snddata(DATA,strlen(buf),buf);
}

int toc::updatebud(char *user) {
	//note that this is a TEMPORARY add, it will not be saved past this connection!
	//takes a normalized username
	log->start("updatebud");
	log->write(user);
	if(totalbuds>=MAX_BUDDIES) {
		gui.print("error: updatebud() failed: no more spaces left!");
		log->fend("updatebud");
		return -1;
	}
	else {
		strncpy(bdlist[++totalbuds].nuser,user,16);
		strncpy(bdlist[totalbuds].fuser,user,16);//until a formatted username is set, we need to do this
	}
	return totalbuds;
}


int toc::findbuddy(char *user) {// this now takes normalized usernames
	log->start("findbuddy");
	log->write(user);
	for(int i=0;i<MAX_BUDDIES+1;i++) {
		if(!strcmp(bdlist[i].nuser,user)) {
			log->end();
			return i;
		}
	}
	log->end();
	return -1;
}

void toc::normalize(char *dst,char *src) {
	log->start("normalize");
	for(;src[0]!='\0';src++,dst++) {
		if(src[0]==' ') {
			dst--;
			continue;
		}
		dst[0]=tolower(src[0]);//this should do it
	}
	dst[0]='\0';
	log->end();
}

void toc::addslashes(char *dst,char *src) {
	log->start("addslashes");
	for(;src[0]!='\0';src++,dst++) {
		if(src[0]=='$' || src[0]=='{' || src[0]=='}' || src[0]=='[' || src[0]==']' || src[0]=='(' || src[0]==')' || src[0]=='"' || src[0]=='\'' || src[0]=='\\') {
			dst[0]='\\';
			dst++;
		}
		dst[0]=src[0];
	}
	dst[0]='\0';
	log->end();
}
		

void toc::parsenet(char *buf) {//i need to break this up into functions...
	log->start("parsenet");
	if(!strncmp(buf,"IM_IN",5)) {
		//IM_IN:buddyname:F:the F is for the auto response T/F
		log->write("im recv");
		int i;
		char username[17];
		buf+=6;
		for(i=0;buf[0]!=':';i++,buf++) username[i]=buf[0];
		buf+=3;
		username[i]='\0';
		gui.rcvim(username,buf,false,false);
		logger->im(username,buf,false,true);
	}
	else if(!strncmp(buf,"UP",2)) {
		log->write("buddy update recv");
		//UPDATE_BUDDY:buddyname:T:xxx:yyy:zzz:aaa
		//xxx == evil amt (percentage)
		//yyy == signon time (epoc)
		//zzz == idle time (minutes) -- We store this in seconds!
		struct buddylist y;
		buf+=13;
		int i;
		for(i=0;buf[0]!=':';i++,buf++) y.fuser[i]=buf[0];
		buf++;
		y.fuser[i]='\0';
		normalize(y.nuser,y.fuser);
		i=findbuddy(y.nuser);
		if(i==-1) {
			i=updatebud(y.nuser);
			strcpy(bdlist[i].fuser,y.fuser);
		}
		y.online=bdlist[i].online;
		if(buf[0]=='T') {
			bdlist[i].online=true;
		}
		else {
			bdlist[i].online=false;
		}
		buf+=2;
		y.warn=bdlist[i].warn;
		bdlist[i].warn=atoi(buf);
		for(;buf[0]!=':';buf++);
		buf++;
		y.signon=bdlist[i].signon;
		bdlist[i].signon=atoi(buf);
		for(;buf[0]!=':';buf++);
		buf++;
		y.idle=bdlist[i].idle;
		bdlist[i].idle=atoi(buf)*60;
		for(;buf[0]!=':';buf++);
		buf+=2;//just changed this.. prev. val. buf+=3
		if(i==MAX_BUDDIES) buf++;
		y.avail=bdlist[i].avail;
		if(buf[0]=='U') {
			bdlist[i].avail=false;
		}
		else {
			bdlist[i].avail=true;
		}

		if(y.online!=bdlist[i].online) {
			char buffer[80];
			sprintf(buffer,"%s has just signed ",bdlist[i].fuser);
			if(bdlist[i].online) {
				strcat(buffer,"on.");
				buddies++;
			}
			else {
				strcat(buffer,"off.");
				buddies--;
			}
			gui.print(buffer);
		}
		if(!strcmp(bdlist[i].nuser,cuser)) {
			gui.setstatus(&bdlist[i],false);
		}
		if(i==MAX_BUDDIES) {
			gui.setstatus(&bdlist[i],true);
		}
		logger->status(&bdlist[i]);
	}
	else if(!strncmp(buf,"SIGN_ON",7)) {
		log->write("signon recv");
		me->online=true;
		me->avail=true;
	}
	else if(!strncmp(buf,"NICK",4)) {
		log->write("nick recv");
		buf+=5;
		strcpy(me->fuser,buf);
		me->fuser[strlen(buf)]='\0';
	}
	else if(!strncmp(buf,"CONFIG",6)) {
		log->write("config recv");
		buf+=7;
		char addbud[512]="toc_add_buddy";
		char cgroup[48]="nogroup";//cover my ass
		int y=0;
		for(totalbuds=0;buf[0]!='\0';totalbuds++,buf++) {
			if(buf[0]=='g') {
				log->write("new group");
				buf+=2;
				for(y=0;buf[0]!='\n';buf++,y++) {
					cgroup[y]=buf[0];
				}
				cgroup[y+1]='\0';
				totalbuds--;
				log->write(cgroup);
			}
			else if(buf[0]=='b') {
				log->write("new person");
				buf+=2;
				for(y=0;buf[0]!='\n';buf++,y++) {
					bdlist[totalbuds].fuser[y]=buf[0];
				}
				bdlist[totalbuds].fuser[y+1]='\0';
				strcpy(bdlist[totalbuds].group,cgroup);
				normalize(bdlist[totalbuds].nuser,bdlist[totalbuds].fuser);
				log->write(bdlist[totalbuds].nuser);
				log->focus("parsenet");
				strcat(addbud," ");
				strcat(addbud,bdlist[totalbuds].nuser);
			}
			else {
				log->write("noop");
				log->write(buf);
			}
			for(;buf[0]!='\n';buf++);
		}
		snddata(DATA,strlen(addbud),addbud);//send out the addbud so we recv updates
	}
	else if(!strncmp(buf,"ERROR",5)) {
		log->write("error");
		buf+=6;
		int error=atoi(buf);
		char buff[200];
		buf+=4;
		switch(error) {//fatal errors here are written to gui and then exited, erasing the gui... fix this
			case 901://user not avail
				sprintf(buff,"%s is not available.",buf);
				gui.print(buff);
				break;
			case 902://warning not available
				break;
			case 903:
				gui.print("A message has been dropped, you are exceeding the server speed limit!");
				break;

			case 911:
				gui.print("Error validating input");
				break;
			case 912:
				gui.print("Invalid account");
				break;
			case 913:
				gui.print("Server-side error encountered while processing request");
				break;
			case 914:
			case 981:
				gui.print("The AOL AIM service is unavailable.");
				break;

			case 950:
				break;

			case 960:
				sprintf(buff,"You are sending messages too fast to %s!",buf);
				gui.print(buff);
				break;
			case 961:
				sprintf(buff,"You missed a message from %s because it was too big.",buf);
				gui.print(buff);
				break;
			case 962:
				sprintf(buff,"You missed a message from %s because it was sent too fast.",buf);
				gui.print(buff);
				break;

			case 980:
				gui.print("Incorrect username or password.");
				break;

			case 982:
				gui.print("Your warning level is too high to sign on.");
				break;
			case 983:
				gui.print("You have been reconnecting too frequently.");
				break;
			case 989:
				sprintf(buff,"An ungodly error has occured: %s",buf);
				gui.print(buff);
				break;
		}//end switch
	}//end if error
	else if(!strncmp(buf,"EVILED",6)) {
		log->write("warned");
		buf+=7;
		int old=me->warn;
		me->warn=atoi(buf);
		gui.setstatus(&bdlist[MAX_BUDDIES],true);
		if(old<me->warn) {
			for(;buf[0]!=':';buf++);
			buf++;
			char buff[80]="You were just warned by ";
			if(buf[0]=='\0')
				strcat(buff,"anonymous");
			else
				strcat(buff,buf);
			sprintf(&buff[strlen(buff)],".  Your warning level is now %i%%.",me->warn);
			gui.print(buff);
		}
	}
	else {
		log->write("unsupported");
		gui.print("Received a TOC command not supported by gah:");
		gui.print(buf);
	}
	log->fend("parsenet");
}//end parsenet()
