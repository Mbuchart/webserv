#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <cstring>
#include <csignal>
#include "Utils.hpp"

struct ConfigMembers
{
	ConfigMembers(void);

	std::string							root;
	size_t								max_body_size;
	std::map<int, std::string>			error_pages;
	bool								autoindex;
	std::string							index;
	std::map<std::string, std::string>	cgis;
};

struct	LocationMembers;
struct ServerMembers : public ConfigMembers
{
	ServerMembers(void)
		: ConfigMembers(), host("0.0.0.0"), port(80)
	{
	}

	ServerMembers(const ServerMembers& cpy);

	std::string							host;
	int									port;
	std::vector<LocationMembers>		locations;
	std::vector<std::string>			server_name;
};

struct LocationMembers : public ConfigMembers
{
	LocationMembers(void)
	{
	}

	LocationMembers(const ServerMembers& cpy);

	LocationMembers(const LocationMembers& cpy);

	std::string					uri;
	std::vector<std::string>	allowedMethods;
};

class Config
{

public:
	//	Public member functions
	Config(const std::string& file_name);
	~Config( void );

	std::vector<ServerMembers>&	getConfig(void);

private:
	//	Private member functions

	//	Main parse functions
	void	parse(void);
	void	parseMainCtx(std::string& line);
	void	parseServerCtx(std::string& line);
	void	parseLocationCtx(std::string& line);

	//	Secondary parse functions
	void	endScope(void);

	//	Shared functions for server and location
	void	addRoot(std::stringstream& ss, ConfigMembers& cm);
	void	addMaxBodySize(std::stringstream& ss, ConfigMembers& cm);
	void	addErrorPages(std::stringstream& ss, ConfigMembers& cm);
	void	addIndex(std::stringstream& ss, ConfigMembers& cm);
	void	addAutoIndex(std::stringstream& ss, ConfigMembers& cm);
	void	addCgis(std::stringstream& ss, ConfigMembers& cm);

	//	Server functions
	void	addListen(std::stringstream& ss, ServerMembers& cm);
	void	addLocation(std::stringstream& ss, ServerMembers& sm);
	void	addServerName(std::stringstream& ss, ServerMembers& sm);

	//	Location functions
	void	addAllowedMethods(std::stringstream& ss, LocationMembers& lm);

	//	Private classes/structs
	enum Context
	{
		MAIN,
		SERVER,
		LOCATION
	};

	//	Private members
	std::ifstream	m_ifs;
	Context			m_ctx;
	size_t			m_curr_line;

	std::vector<ServerMembers>	m_cms;
};

std::ostream&	operator<<(std::ostream &ostr, Config& pc);

#endif
