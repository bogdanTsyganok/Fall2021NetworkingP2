#include "DBMaster.h"
#include "cSecurity.h"
#include "cUser.h"

DBMaster::DBMaster(void)
	: m_IsConnected(false)
	, m_IsConnecting(false)
	, m_Connection(0)
	, m_Driver(0)
	, m_ResultSet(0)
{
}

bool DBMaster::IsConnected(void)
{
	return m_IsConnected;
}

CreateAccountWebResult DBMaster::CreateAccount(const string& email, const string& password, string& date, int& userId)
{
	cSecurity* hasher = new cSecurity();
	sql::Statement* stmt = m_Connection->createStatement();
	cUser newUser;
	//sql::PreparedStatement* retrieve_stmt = m_Connection->prepareStatement("SELECT email, salt, hashed_password, userId FROM web_auth VALUES (?, ?, ?, ?)");
	//Check if email is already in use 
	
	try
	{
		std::string selectQuery = "SELECT id, email FROM web_auth WHERE email = '" + email + "'";
		m_ResultSet = stmt->executeQuery(selectQuery);

	}
	catch (SQLException e)
	{
		std::cout << e.what() << std::endl;
		printf("Failed to retrieve web_auth data!\n");
		return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
	}

	if (m_ResultSet->rowsCount() > 0)
	{
		delete hasher;
		delete stmt;
		return ACCOUNT_ALREADY_EXISTS;
	}
	//while (m_ResultSet->next())
	//{
	//	cUser* tempUser = new cUser();
	//	//tempUser->email
	//	std::cout << "\t... MySQL replies: ";
	//	/* Access column data by alias or column name */
	//	std::cout << m_ResultSet->getString("email") << std::endl;
	//	std::cout << "\t... MySQL says it again: ";
	//	/* Access column data by numeric offset, 1 is the first column */
	//	std::cout << m_ResultSet->getString(2) << std::endl;
	//}


	//Make new account
	newUser.salt = hasher->GenerateSalt();
	std::string temp = newUser.salt + password;
	newUser.hashed_password = hasher->GenerateHash(temp);
	try
	{
		std::string insertUser = "INSERT INTO user_table(last_login, creation_date) VALUES (CURTIME(), CURDATE())";
		stmt->execute(insertUser);
		std::string selectLastId = "SELECT LAST_INSERT_ID() AS LastId";
		m_ResultSet = stmt->executeQuery(selectLastId);
	}
	catch (SQLException e)
	{
		std::cout << e.what() << std::endl;
		printf("Failed to retrieve web_auth data!\n");
		return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
	}
	m_ResultSet->next();
	newUser.userId = m_ResultSet->getInt("LastId");

	try
	{
		std::string selectDate = "SELECT creation_date FROM user_table WHERE id = " + std::to_string(newUser.userId);
		m_ResultSet = stmt->executeQuery(selectDate);
	}
	catch (SQLException e)
	{
		std::cout << e.what() << std::endl;
		printf("Failed to retrieve web_auth data!\n");
		return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
	}

	try
	{
		m_ResultSet->next();
		date = m_ResultSet->getString("creation_date");
	}
	catch (SQLException e)
	{
		std::cout << e.what() << std::endl;
	}
	//sql::PreparedStatement* preparedStatement = m_Connection->prepareStatement("SELECT * FROM `web_auth`;");
	sql::PreparedStatement* insert_stmt = m_Connection->prepareStatement("INSERT INTO web_auth(email, salt, hashed_password, userId) VALUES (?, ?, ?, ?)");
	insert_stmt->setString(1, email);
	insert_stmt->setString(2, newUser.salt);
	insert_stmt->setString(3, newUser.hashed_password);
	insert_stmt->setInt(4, newUser.userId);
	//preparedStatement->setString(1, "web_auth");
	userId = newUser.userId;

	try
	{
		insert_stmt->execute();
		//m_ResultSet = stmt->executeQuery("SELECT * FROM `web_auth`;");
	}
	catch (SQLException e)
	{
		std::cout << e.what() << std::endl;
		printf("Failed to retrieve web_auth data!\n");
		return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
	}


	delete hasher;
	delete insert_stmt;
	delete stmt;
	printf("Successfully retrieved web_auth data!\n");
	return CreateAccountWebResult::SUCCESS;
}

CreateAccountWebResult DBMaster::AuthenticateAccount(const string& email, const string& password, string& date, int& userId)
{
	cSecurity* hasher = new cSecurity();
	sql::Statement* stmt = m_Connection->createStatement();
	cUser user;
	CreateAccountWebResult result;
	try
	{
		std::string selectQuery = "SELECT id, email, salt, hashed_password, userId FROM web_auth WHERE email = '" + email + "'";
		m_ResultSet = stmt->executeQuery(selectQuery);
		//m_ResultSet = stmt->executeQuery("SELECT * FROM `web_auth`;");
	}
	catch (SQLException e)
	{
		delete hasher;
		delete stmt;
		return INTERNAL_SERVER_ERROR;
	}

	if (!m_ResultSet->next())
	{
		delete hasher;
		delete stmt;
		return INVALID_CREDENTIALS;
	}

	user.salt = m_ResultSet->getString("salt");
	user.hashed_password = m_ResultSet->getString("hashed_password");
	user.userId = m_ResultSet->getInt("userId");
	user.id = m_ResultSet->getInt("id");
	userId = user.id;
	if (user.hashed_password == hasher->GenerateHash(user.salt + password))
	{
		try
		{
			std::string updateStatement = "UPDATE user_table SET last_login = CURTIME() WHERE id = " + std::to_string(userId);
			stmt->executeUpdate(updateStatement);
			updateStatement = "SELECT creation_date FROM user_table WHERE id = " + std::to_string(userId);
			m_ResultSet =  stmt->executeQuery(updateStatement);
			m_ResultSet->next();
			date = m_ResultSet->getString("creation_date");
		}
		catch (SQLException e)
		{
			std::cout << stderr << e.what() << std::endl;
			delete hasher;
			delete stmt;
			return INTERNAL_SERVER_ERROR;
		}
		delete hasher;
		delete stmt;
		result = SUCCESS;
	}
	else
	{
		delete hasher;
		delete stmt;
		result = INVALID_CREDENTIALS;
	}

	/*newUser.salt = hasher->GenerateSalt();
	std::string temp = newUser.salt + password;
	newUser.hashed_password = hasher->GenerateHash(temp);*/
	return result;
}

void DBMaster::Connect(const string& hostname, const string& username, const string& password)
{
	if (m_IsConnecting)
		return;

	m_IsConnecting = true;
	try
	{
		m_Driver = mysql::get_driver_instance();
		m_Connection = m_Driver->connect(hostname, username, password);
		m_Connection->setSchema("auth_schema");
	}
	catch (SQLException e)
	{
		printf("Failed to connect to database with error: %s\n", e.what());
		m_IsConnecting = false;
		return;
	}
	m_IsConnected = true;
	m_IsConnecting = false;

	printf("Successfully connected to database!\n");
}