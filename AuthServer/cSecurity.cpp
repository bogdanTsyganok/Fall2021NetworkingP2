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
	for (int i = 0; i < 64; i++)
	{
		newSalt += alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	this->mSalt = newSalt;
	return this->mSalt;
}

std::string cSecurity::GenerateHash(std::string plainTextPassword)
{
	if (this->mSalt == "")
	{
		GenerateSalt();
	}
	SHA256 sha256;
	std::string temp = mSalt + plainTextPassword;
	return sha256(temp);
}

void cSecurity::Reset()
{
	this->mSalt = "";
}
