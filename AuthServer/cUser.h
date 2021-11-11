#pragma once
#include <string>

struct cUser
{
	unsigned int id;
	std::string email;
	std::string salt;
	std::string hashed_password;
	unsigned int userId;
};