/* log.h  Version 1.0 Beta -- Logs stuff for debug mode
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

class debuglog {
	private:
		bool debug;
		char *buf;
		FILE *log;
		int pid;
	public:
		debuglog(bool);
		void start(char*);
		void end();
		void write(char*);
		void fatal(char*);
		void focus(char*);
		void fwrite(char*,char*);
		void fend(char*);
		void close(char*);
};
debuglog::debuglog(bool debugtmp) {
	debug=debugtmp;
	if(debug) {
		pid=getpid();
		log=fopen("debug","a");
		fprintf(log,"%i->gah version %s\n",pid,VERSION);
		fflush(log);
	}
}
void debuglog::start(char *what) {
	buf=what;
	if(debug) {
		fprintf(log,"%i->Entering %s Function.\n",pid,what);
		fflush(log);
	}
}
void debuglog::end()  {
	if(debug) {
		fprintf(log,"%i->Exited %s Successfully.\n",pid,buf);
		fflush(log);
	}
	buf=NULL;
}
void debuglog::write(char *what) {
	if(debug) {
		fprintf(log,"%i->[%s] %s\n",pid,buf,what);
		fflush(log);
	}
}
void debuglog::fatal(char *what) {
	if(debug) {
		fprintf(log,"%i->Fatal Error Occured\n%i->[%s] %s\n",pid,pid,buf,what);
		fflush(log);
	}
}
void debuglog::focus(char *what) {
	buf=what;
}
void debuglog::fwrite(char *what,char *msg) {
	buf=what;
	write(msg);
}
void debuglog::fend(char *what) {
	buf=what;
	end();
}	
void debuglog::close(char *what) {
	if(debug) fprintf(log,"%i->[%s] %s\n%i->gah is exiting, log closed.\n",pid,buf,what,pid);
}



class logging {
	private:
		FILE *log;
		bool on;
	public:
		logging(bool);
		void im(char*,char*,bool,bool);
		void status(buddylist*);
};
logging::logging(bool ontmp) {
	on=ontmp;
	if(on) {
		log=fopen("log","at");
	}
}

void logging::im(char *who,char *msg, bool autor, bool recv) {
	if(on) {
		fprintf(log,"%s%c%s%c %s\n",(recv ? "<-":"->"),(autor ? '|':'<'),who,(autor ?'|':'>'),msg);
		fflush(log);
	}
}

void logging::status(buddylist *y) {
	if(on) {
		fprintf(log,"%s is %s, idle for %imin, %i%%warning, signed on at %i and is %s\n",y->fuser,(y->online ? "online":"offline"),y->idle/60,y->warn,y->signon,(y->avail ? "available":"away"));
		fflush(log);
	}
}
