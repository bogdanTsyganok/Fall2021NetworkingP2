/*
* Author:		Jarrid Steeper 0883583, Bogdan Tsyganok 0886354
* Class:		INFO6016 Network Programming
* Teacher:		Lukas Gustafson
* Project:		Project02
* Due Date:		Nov 12
* Filename:		cSecurity.h
* Purpose:		Security class definition for generating a salt and hash for user created password that will be stored in the authentification database.
*/

#include <string>
#include "cSecurity.h"
#include "sha256.h"
#include <time.h>

cSecurity::cSecurity()
{
	this->mSalt = "";
}

cSecurity::~cSecurity()
{
}

std::string cSecurity::GetSalt()
{
	return this->mSalt;
}

std::string cSecurity::GenerateSalt()
{
	char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"`~,<.>/?;:'\"[{]}-_=+\\|/*";
	std::string newSalt;
	newSalt.reserve(64);
	srand(time(NULL));
	for (int i = 0; i < 64; i++)
	{
		newSalt += alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	this->mSalt = newSalt;
	return this->mSalt;
}

std::string cSecurity::GenerateHash(std::string saltedPassword)
{
	/*if (this->mSalt == "")
	{
		GenerateSalt();
	}
	std::string temp = mSalt + plainTextPassword;*/
	SHA256 sha256;
	return sha256(saltedPassword);
}

void cSecurity::Reset()
{
	this->mSalt = "";
}
