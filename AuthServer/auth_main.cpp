#include "DBMaster.h"
#include "sha256.h"
#include "cSecurity.h"
DBMaster database;


int main(int argc, char** argv)
{
	
	database.Connect("127.0.0.1", "root", "password");
	database.CreateAccount("l_gustafson@fanshaweonline.ca", "password");
	system("Pause");
	return 0;
}
