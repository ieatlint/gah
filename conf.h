/* gah -- conf.h -- Parses configuration file */
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

class conf {
	private:
		FILE *cfile;
		settings *data;
		void parse();
	public:
		conf(char*,settings*);
};

conf::conf(char *configfile,settings *tmpdata) {
	data=tmpdata;
	if(configfile==NULL) {
		cfile=fopen("gah.conf","rt");
		if(cfile==NULL)
			cfile=fopen("/etc/gah.conf","rt");
	}
	else {
		cfile=fopen(configfile,"rt");
	}
	if(cfile!=NULL) parse();
}

void conf::parse() {
	char buf[120];
	for(int line=1;!feof(cfile);line++) {
		if(isalpha((buf[0]=fgetc(cfile)))) {
			ungetc(buf[0],cfile);
			if(fscanf(cfile,"username = %s",buf)>0) {
				strncpy(data->username,buf,16);
			}
			else if(fscanf(cfile,"password = %s",buf)>0) {
				strncpy(data->password,buf,34);
			}
			else if(fscanf(cfile,"host = %s",buf)>0) {
				strncpy(data->host,buf,63);
			}
			else if(fscanf(cfile,"ort = %s",buf)>0) {
				data->port=atoi(buf);
			}
			else if(fscanf(cfile,"logging = %s",buf)>0) {
				if(buf[0]=='t')
					data->log=true;
			}
			else {
				cerr<<"Invalid data on line "<<line<<":"<<endl;
				while((buf[0]=fgetc(cfile))!='\n' && !feof(cfile))
						cout<<buf[0];
				cout<<endl;
				exit(1);
			}
		}
		if(buf[0]=='\n') continue;
		while(fgetc(cfile)!='\n' && !feof(cfile));
	}
}
