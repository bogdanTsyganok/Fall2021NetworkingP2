/*
* Author:		Jarrid Steeper 0883583, Bogdan Tsyganok 0886354
* Class:		INFO6016 Network Programming
* Teacher:		Lukas Gustafson
* Project:		Project02
* Due Date:		Nov 12
* Filename:		cSecurity.h
* Purpose:		Security class declaration for generating a salt and hash for user created password that will be stored in the authentification database.
*/

#pragma once

class cSecurity
{
private:
	std::string mSalt;
public:
	//Constructors and destructors
	cSecurity();
	~cSecurity();

	//Getters and setters
	std::string GetSalt();

	//Methods

	/// <summary>
	/// Generates a new salt for use in password hashing
	/// </summary>
	std::string GenerateSalt();

	/// <summary>
	/// Hashes user given plain text password with sha256 and stored salt, if no salt is stored it will generate new salt
	/// </summary>
	/// <param name="plainTextPassword"></param>
	/// <returns></returns>
	std::string GenerateHash(std::string plainTextPassword);

	/// <summary>
	/// Resets the stored salt
	/// </summary>
	void Reset();
};