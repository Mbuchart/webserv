#include "../includes/Config.hpp"

Config::Config(const std::string& file_name)
	: m_ctx(MAIN), m_curr_line(0)
{
	if (file_name.size() < 6 || file_name.substr(file_name.size() - 5) != ".conf")
		throw std::runtime_error("Invalid file extension : " + file_name);
	m_ifs.open(file_name.c_str(), std::ifstream::in);
	if (m_ifs.fail())
		throw std::runtime_error("Unable to open the file : " + file_name);
	parse();
}

Config::~Config(void)
{
	try
	{
		if (m_ifs.is_open())
		{
			m_ifs.close();
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Failed to close the file (" << e.what() << ")" << std::endl;
	}
}

std::vector<ServerMembers>&	Config::getConfig(void)
{
	return (m_cms);
}

void	Config::parse(void)
{
	std::string	line;

	while (std::getline(m_ifs, line))
	{
		m_curr_line++;
		line = Utils::trim(line);
		if (line == "")
			continue ;
		else if (line == "}")
			endScope();
		else if (m_ctx == MAIN)
			parseMainCtx(line);
		else if (m_ctx == SERVER)
			parseServerCtx(line);
		else if (m_ctx == LOCATION)
			parseLocationCtx(line);
	}

	std::stringstream tmp;
	tmp << m_curr_line + 1;
	std::string scl = tmp.str();

	if (m_ctx != MAIN)
		throw std::runtime_error("Expected  '}' on line : " + scl);
	if (m_cms.empty())
		throw std::runtime_error("File is empty");
}

void	Config::parseMainCtx(std::string& line)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	ServerMembers		new_server;

	if (line != "server {")
		throw std::runtime_error("Expected 'server {' on line : " + scl);
	m_cms.push_back(new_server);
	m_ctx = SERVER;
}

void	Config::parseServerCtx(std::string& line)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	int i = 0;
	while(line[i] != '\0'){
		i++;
	}
	i--;

	if (line[i] != ';' && line.substr(0, 8) != "location")
		throw std::runtime_error("Expected ';' at the end of the line : " + scl);
	if (line[i] == ';')
		line.erase(i);

	std::string			word;
	std::stringstream	ss(line);
	ServerMembers		&sm = m_cms.back();

	ss >> word;
	if (word == "listen")
		addListen(ss, sm);
	else if (word == "root")
		addRoot(ss, sm);
	else if (word == "server_name")
		addServerName(ss, sm);
	else if (word == "client_max_body_size")
		addMaxBodySize(ss, sm);
	else if (word == "error_page")
		addErrorPages(ss, sm);
	else if (word == "location")
		addLocation(ss, sm);
	else if (word == "index")
		addIndex(ss, sm);
	else if (word == "autoindex")
		addAutoIndex(ss, sm);
	else if (word == "cgi_param")
		addCgis(ss, sm);
	else
		throw std::runtime_error("Unexpected argument '" + word + "' on line : " + scl);
}

void	Config::parseLocationCtx(std::string& line)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	int i = 0;
	while(line[i] != '\0'){
		i++;
	}
	i--;

	if (line[i] != ';')
		throw std::runtime_error("Expected ';' at the end of the line : " + scl);
	if (line[i] == ';')
		line.erase(i);

	std::string			word;
	std::stringstream	ss(line);
	LocationMembers		&lm = m_cms.back().locations.back();

	ss >> word;
	if (word == "root")
		addRoot(ss, lm);
	else if (word == "client_max_body_size")
		addMaxBodySize(ss, lm);
	else if (word == "error_page")
		addErrorPages(ss, lm);
	else if (word == "index")
		addIndex(ss, lm);
	else if (word == "autoindex")
		addAutoIndex(ss, lm);
	else if (word == "cgi_param")
		addCgis(ss, lm);
	else if (word == "accept_methods")
		addAllowedMethods(ss, lm);
	else
		throw std::runtime_error("Unexpected argument '" + word + "' on line : " + scl);
}

void	Config::endScope(void)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	if (m_ctx == SERVER)
		m_ctx = MAIN;
	else if (m_ctx == LOCATION)
		m_ctx = SERVER;
	else
		throw std::runtime_error("Unexpected  '}' on line : " + scl);
}

void	Config::addListen(std::stringstream& ss, ServerMembers& cm)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	std::string		word;
	size_t			found;

	if (!(ss >> word))
		throw std::runtime_error("Expected listen argument on line : " + scl);
	found = word.find(":");
	if (found != std::string::npos)
	{
		cm.host = word.substr(0, found);
		word = word.substr(found + 1, word.size());
	}
	try
	{
		int i;

		std::istringstream ( word ) >> i;

		cm.port = i;
	}
	catch (std::exception& e)
	{
		throw std::runtime_error("Unexpected argument '" + word + "' on line : " + scl);
	}
	if (ss >> word)
		throw std::runtime_error("Unexpected argument '" + word + "' on line : " + scl);
}

void	Config::addServerName(std::stringstream& ss, ServerMembers& sm)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	std::string		word;

	while (ss >> word)
		sm.server_name.push_back(word);
	if (sm.server_name.empty())
		throw std::runtime_error("Expected server names argument on line : " + scl);
}

void	Config::addRoot(std::stringstream& ss, ConfigMembers& cm)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	std::string		word;

	if (!(ss >> word))
		throw std::runtime_error("Expected root argument on line : " + scl);
	cm.root = word;
	if (ss >> word)
		throw std::runtime_error("Unexpected argument '" + word + "' on line : " + scl);
}

void	Config::addIndex(std::stringstream& ss, ConfigMembers& cm)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	std::string		word;

	if (!(ss >> word))
		throw std::runtime_error("Expected index argument on line : " + scl);
	cm.index = word;
	if (ss >> word)
		throw std::runtime_error("Unexpected argument '" + word + "' on line : " + scl);
}

void	Config::addMaxBodySize(std::stringstream& ss, ConfigMembers& cm)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	std::string		word;

	if (!(ss >> word))
		throw std::runtime_error("Expected bodysize argument on line : " + scl);
	try
	{
		int i;

		std::istringstream ( word ) >> i;

		cm.max_body_size = i;
	}
	catch (std::exception& e)
	{
		throw std::runtime_error("Unexpected argument '" + word + "' on line : " + scl);
	}
	if (ss >> word)
		throw std::runtime_error("Unexpected argument '" + word + "' on line : " + scl);
}

void	Config::addErrorPages(std::stringstream& ss, ConfigMembers& cm)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	std::string	word;
	size_t		error;

	if (!(ss >> word))
		throw std::runtime_error("Expected error pages argument on line : " + scl);
	try
	{
		int i;

		std::istringstream ( word ) >> i;

		error = i;
	}
	catch (std::exception& e)
	{
		throw std::runtime_error("Unexpected argument '" + word + "' on line : " + scl);
	}
	if (!(ss >> word))
		throw std::runtime_error("Expected file name on line : " + scl);
	cm.error_pages[error] = word;
	if (ss >> word)
		throw std::runtime_error("Unexpected argument '" + word + "' on line : " + scl);
}

void	Config::addAutoIndex(std::stringstream& ss, ConfigMembers& cm)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	std::string		word;

	if (!(ss >> word))
		throw std::runtime_error("Expected autoindex argument on line : " + scl);
	if (word == "on")
		cm.autoindex = true;
	else if (word == "off")
		cm.autoindex = false;
	else
		throw std::runtime_error("UUUUnexpected argument '" + word + "' on line : " + scl);
	if (ss >> word)
		throw std::runtime_error("Unexpected argument '" + word + "' on line : " + scl);
}

void	Config::addCgis(std::stringstream& ss, ConfigMembers& cm)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	std::string	ext;
	std::string	path;

	if (!(ss >> ext) || !(ss >> path))
		throw std::runtime_error("Expected cgi argument on line : " + scl);
	cm.cgis[ext] = path;
	if (ss >> ext)
		throw std::runtime_error("Unexpected argument '" + ext + "' on line : " + scl);
}

void	Config::addLocation(std::stringstream& ss, ServerMembers& sm)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	std::string		word;
	LocationMembers	lm(sm);

	if (!(ss >> word))
		throw std::runtime_error("Expected '{' on line : " + scl);
	if (word != "{")
	{
		lm.uri = word;
		ss >> word;
		if (word != "{")
			throw std::runtime_error("Expected '{' on line : " + scl);
	}
	sm.locations.push_back(lm);
	m_ctx = LOCATION;
	if (ss >> word)
		throw std::runtime_error("Unexpected argument '" + word + "' on line : " + scl);
}

void	Config::addAllowedMethods(std::stringstream& ss, LocationMembers& lm)
{
	std::stringstream tmp;
	tmp << m_curr_line;
	std::string scl = tmp.str();

	std::string	word;
	lm.allowedMethods.clear();

	while (ss >> word)
	{
		if (!Utils::isValidMethod(word))
			throw std::runtime_error("Unexpected argument '" + word + "' on line : " + scl);
		for (size_t i = 0; i < lm.allowedMethods.size(); ++i)
		{
			if (lm.allowedMethods[i] == word)
				throw std::runtime_error("Method '" + word + "' declared multiple times on line : " + scl);
		}
		lm.allowedMethods.push_back(word);
	}
	if (lm.allowedMethods.empty())
		throw std::runtime_error("Expected methods argument on line : " + scl);
}

std::ostream&	operator<<(std::ostream &ostr, Config& pc)
{
	std::vector<ServerMembers>::iterator	ite = pc.getConfig().end();
	std::vector<ServerMembers>::iterator	it = pc.getConfig().begin();

	for (int i = 1; it != ite; ++it, ++i)
	{
		ostr << "\nServer " << i << " : \n\n";
		ostr << "Listen : on host '" << it->host << "', port '" << it->port << "'" << std::endl;
		ostr << "Server names : ";
		for (std::vector<std::string>::iterator namesIt = it->server_name.begin(); namesIt != it->server_name.end(); ++namesIt)
			ostr << "'" << *namesIt << "' ";
		ostr << "\n";
		ostr << "Root : " << it->root << std::endl;
		ostr << "Index : " << it->index << std::endl;
		ostr << "Autoindex : ";
		if (it->autoindex)
			ostr << "true" << std::endl;
		else
			ostr << "false" << std::endl;
		ostr << "Cgis params : ";
		for (std::map<std::string, std::string>::iterator namesIt = it->cgis.begin(); namesIt != it->cgis.end(); ++namesIt)
			ostr << "'" << namesIt->first << "' : '" << namesIt->second << "'. ";
		ostr << "\n";
		ostr << "Max body size : " << it->max_body_size << std::endl;
		for (std::map<int, std::string>::iterator errIt = it->error_pages.begin(); errIt != it->error_pages.end(); ++errIt)
			ostr << "Error '" << errIt->first << "' = page '" << errIt->second << "'\n";
		int j = 1;
		for (std::vector<LocationMembers>::iterator locIt = it->locations.begin(); locIt != it->locations.end(); ++locIt, ++j)
		{
			ostr << "\nLocation " << j << " : \n\n";
			ostr << "\tRoot : " << locIt->root << std::endl;
			ostr << "\tIndex : " << locIt->index << std::endl;
			ostr << "\tAutoindex : ";
			if (locIt->autoindex)
				ostr << "true" << std::endl;
			else
				ostr << "false" << std::endl;
			ostr << "\tCgis params : ";
			for (std::map<std::string, std::string>::iterator namesIt = locIt->cgis.begin(); namesIt != locIt->cgis.end(); ++namesIt)
				ostr << "'" << namesIt->first << "' : '" << namesIt->second << "'. ";
			ostr << "\n";
			ostr << "\tMax body size : " << locIt->max_body_size << std::endl;
			for (std::map<int, std::string>::iterator errIt = locIt->error_pages.begin(); errIt != locIt->error_pages.end(); ++errIt)
				ostr << "\tError '" << errIt->first << "' = page '" << errIt->second << "'\n";
		}
		ostr << "\n";
	}
	return (ostr);
}

ConfigMembers::ConfigMembers(void)
	: root(""), max_body_size(1000000000), autoindex(false)	
{
	std::string	path_to_error_files = "/files/error_files/";

	this->error_pages[403] = path_to_error_files + "403.html";
	this->error_pages[404] = path_to_error_files + "404.html";
	this->error_pages[405] = path_to_error_files + "405.html";
	this->error_pages[413] = path_to_error_files + "413.html";
	this->error_pages[501] = path_to_error_files + "501.html";
	this->error_pages[502] = path_to_error_files + "502.html";
}

ServerMembers::ServerMembers(const ServerMembers& cpy)
{
	*this = cpy;
}

LocationMembers::LocationMembers(const LocationMembers& cpy)
{
	*this = cpy;
}

LocationMembers::LocationMembers(const ServerMembers& cpy)
{
	this->root = cpy.root;
	this->max_body_size = cpy.max_body_size;
	this->error_pages = cpy.error_pages;
	this->autoindex = cpy.autoindex;
	this->index = cpy.index;
	this->cgis = cpy.cgis;

	this->allowedMethods.push_back("GET");
	this->allowedMethods.push_back("POST");
	this->allowedMethods.push_back("DELETE");
}
