#include <string>
#include "cSecurity.h"

std::string cSecurity::GenerateSalt()
{
	char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"`~,<.>/?;:'\"[{]}-_=+\\|/*";
	std::string output;
	output.reserve(64);
	for (int i = 0; i < 64; i++)
	{
		output += alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	return output;
}