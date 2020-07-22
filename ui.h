/*
 *
 * gah -- ui.h -- v1.36 Beta
 * Performs all functions needed to create the user interface, display input
 * on it, and recieve input from it.
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


#define W_Bl	1 //White on Blue
#define G_B	2 //Green on Black
#define R_Bl	3
#define G_Bl	4
#define Y_B	5 //yellow on black
#define R_B	6 //red on black
#define Y_Bl	8
class userint {
	private:
		WINDOW *output,*status,*input;
		struct tm* ctime;
		time_t epoc;
		struct winsize size;
		char inbuf[2049];
	public:
		debuglog *log;
		void start();
		void redraw();
		void resetstatus(bool);
		void setnick(char*);
		void setstatus(struct buddylist*,bool);
		void print(char*);
		void close();
		void rcvim(char*,char*,bool,bool);
		int getinput();
		void putch(char);
		void bksp();
		void eol();
};
void userint::start() {
	log->start("start");
	initscr();
	if(!has_colors()) {
		cerr<<"You suck!"<<endl;
		endwin();
		exit(1);
	}
	if((output=newwin((LINES-3),0,0,0))==NULL) {
		endwin();
		cerr<<"Unable to create output window."<<endl;
		exit(1);
	}
	if((status=newwin(2,0,(LINES-3),0))==NULL) {
		endwin();
		cerr<<"Unable to create status window."<<endl;
		exit(1);
	}
	if((input=newwin(1,0,(LINES-1),0))==NULL) {
		endwin();
		cerr<<"Unable to create input window."<<endl;
		exit(1);
	}
	start_color();
	init_pair(W_Bl,COLOR_WHITE,COLOR_BLUE);
	init_pair(G_B,COLOR_GREEN,COLOR_BLACK);
	init_pair(R_Bl,COLOR_RED,COLOR_BLUE);
	init_pair(G_Bl,COLOR_GREEN,COLOR_BLUE);
	init_pair(Y_B,COLOR_YELLOW,COLOR_BLACK);
	init_pair(R_B,COLOR_RED,COLOR_BLACK);
	init_pair(R_Bl,COLOR_RED,COLOR_BLUE);
	init_pair(Y_Bl,COLOR_YELLOW,COLOR_BLUE);
	cbreak();
	noecho();
	keypad(input,true);
	wtimeout(input,0);//dont wait for input
	scrollok(output,true);
	scrollok(input,true);
	resetstatus(1);
	resetstatus(0);
	log->fwrite("start","NOTICE: Entered GUI Mode");
	log->end();
}

void userint::redraw() {
	log->start("redraw");
	ioctl(STDOUT,TIOCGWINSZ,&size);
	if(LINES>size.ws_row) {
		for(int i=0;i<(LINES-size.ws_row);i++) waddch(output,'\n');
		wrefresh(output);
	}
	resize_term(size.ws_row,size.ws_col);
	wrefresh(curscr);
	mvwin(status,(LINES-3),0);
	mvwin(input,(LINES-1),0);
	wresize(output,(LINES-3),(COLS-1));
	wresize(status,2,(COLS-1));
	wresize(input,1,(COLS-1));
	wrefresh(curscr);
	wrefresh(output);
	wrefresh(input);
	log->fend("redraw");
}

void userint::close() {
	log->start("close");
	endwin();
	log->close("gui.close");
	cout<<"gah has exited."<<endl;
	exit(0);
}

void userint::print(char *msg) {
	log->start("print");
	log->write(msg);
	waddch(output,'\n');
	waddstr(output,msg);
	wrefresh(output);
	log->end();
}


void userint::resetstatus(bool query) {
	log->start("resetstatus");
	char buf[]="[]                 []        []      [  :  ] [  :  ] []   ";
	mvwaddstr(status,query,0,buf);
	mvwaddch(status,query,(COLS-1),' ');//makes sure the entire line is filled with blue
	mvwchgat(status,query,0,-1,A_BOLD,W_Bl,NULL);
	wrefresh(status);
	log->end();
}

void userint::setnick(char *nick) {
	resetstatus(false);
	wattron(status,COLOR_PAIR(W_Bl));
	mvwaddnstr(status,0,1,nick,16);
	wattron(status,A_BOLD);
	waddch(status,']');
	wattroff(status,COLOR_PAIR(W_Bl) | A_BOLD);
	wrefresh(status);
}

void userint::setstatus(struct buddylist *bud,bool query) {
	log->start("setstatus");
	resetstatus(query);//clear out the old data
	//nick
	wattron(status,COLOR_PAIR(W_Bl));
	if(strlen(bud->fuser)>16) {
		mvwaddstr(status,query,1,bud->nuser);
	}
	else {
		mvwaddstr(status,query,1,bud->fuser);
	}
	wattron(status,A_BOLD);
	waddch(status,']');
	wattroff(status,COLOR_PAIR(W_Bl));
	//online
	wattron(status,COLOR_PAIR(bud->online+3));//3==R_Bl | 4==G_Bl
	if(bud->online) {
		mvwaddstr(status,query,20,"online");
	}
	else {
		mvwaddstr(status,query,20,"offline");
	}
	wattroff(status,COLOR_PAIR(bud->online+3));
	wattron(status,COLOR_PAIR(W_Bl));
	waddch(status,']');
	//available
	wattron(status,COLOR_PAIR(bud->avail+3));
	if(bud->avail) {
		mvwaddstr(status,query,30,"avail");
	}
	else {
		mvwaddstr(status,query,30,"away");
	}
	wattroff(status,COLOR_PAIR(bud->avail+3));
	wattron(status,COLOR_PAIR(W_Bl));
	waddch(status,']');
	wattroff(status,A_BOLD);
	//idle (idle is in second!)
	int idle=bud->idle/60;//min now
	mvwprintw(status,query,38,"%02i",(int)(idle/60));//show hours
	idle-=((int)(idle/60))*60;//remove the time just shown (ie if 1hr shown, rm 60m)
	mvwprintw(status,query,41,"%02i",idle);//show remaining minutes
	//online time
	time(&epoc);
	epoc-=bud->signon;
	ctime=gmtime(&epoc);
	idle=(ctime->tm_yday*24)+ctime->tm_hour;
	mvwprintw(status,query,46,"%02i",idle);
	mvwprintw(status,query,49,"%02i",ctime->tm_min);
	wattroff(status,COLOR_PAIR(W_Bl));
	//warning level
	if(bud->warn<20) idle=G_Bl;
	else if(bud->warn<80) idle=Y_Bl;
	else idle=R_Bl;
	wattron(status,COLOR_PAIR(idle) | A_BOLD);
	mvwprintw(status,query,54,"%i",bud->warn);
	waddch(status,'%');
	wattroff(status,COLOR_PAIR(idle));
	wattron(status,COLOR_PAIR(W_Bl));
	waddch(status,']');
	wattroff(status,COLOR_PAIR(W_Bl) | A_BOLD);
	wrefresh(status);
	log->end();
}

void userint::rcvim(char *who,char *msg,bool autor, bool out) {
	log->start("rcvim");
	log->write(who);
	log->write(msg);
	char buf[strlen(msg)+30],*p=buf;
	time(&epoc);
	ctime=localtime(&epoc);
	if(out) {
		wattron(output,COLOR_PAIR(Y_B));
	}
	else if(autor) {
		wattron(output,COLOR_PAIR(R_B));
	}
	else {
		wattron(output,COLOR_PAIR(G_B));
	}
	sprintf(buf,"\n[%02i:%02i:%02i] <%s> ",ctime->tm_hour,ctime->tm_min,ctime->tm_sec,who);
	p=&buf[strlen(buf)];
	for(bool skip=false;msg[0]!='\0';msg++,p++) {
		if(msg[0]=='<') skip=true;
		if(!skip) p[0]=msg[0];
		else p--;
		if(msg[0]=='>') skip=false;
	}
	p[0]='\0';
	waddstr(output,buf);
	if(out) {//should just change this to an catchall attroff...
		wattroff(output,COLOR_PAIR(Y_B));
	}
	else if(autor) {
		wattroff(output,COLOR_PAIR(R_B));
	}
	else {
		wattroff(output,COLOR_PAIR(G_B));
	}
	wrefresh(output);
	log->end();
}

int userint::getinput() {
	return wgetch(input);
}

void userint::putch(char buf) {
	waddch(input,buf);
	wrefresh(input);
}

void userint::bksp() {
	int x;
	getyx(input,x,x);
	wmove(input,0,x-1);
	waddch(input,' ');
	wmove(input,0,x-1);
	wrefresh(input);
}

void userint::eol() {
	werase(input);
	wrefresh(input);
}
