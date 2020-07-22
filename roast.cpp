#include <iostream>
#include <termios.h>
using namespace std;

int main(int argc,char *argv[]) {
	char password[17],*pass=NULL;
	if(argc>1) {
		pass=argv[1];
	}
	else {
		struct termios n,s;
		tcgetattr(0,&s);
		n=s;
		n.c_lflag &=(~ECHO);
		cout<<"Please enter the password: ";
		tcsetattr(0,TCSANOW,&n);
		cin.getline(password,16);
		tcsetattr(0,TCSANOW,&s);
		cout.put('\n');
		pass=password;
	}
	if(strlen(pass)>16) {
		cerr<<"The maximum password length is 16 charectors!"<<endl;
		return 1;
	}
	char roast[]="Tic/Toc";
	cout<<"Roasted: 0x";
	for(int x=0;pass[x];x++)
		printf("%02x",pass[x]^roast[x%7]);
	cout<<endl;
	return 0;
}
