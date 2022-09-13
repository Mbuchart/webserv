#ifndef REQUEST_HPP
# define REQUEST_HPP

#include <iostream>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <vector>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include "Utils.hpp"

#define DATA_BUFFER 800000

struct RequestMembers
{

	//	Struct for postdata
	struct post_file {
		std::string	filename;
		std::string	envname;
		std::string	data;
	};

	RequestMembers(void)
		: port(0), content_length(0)
	{
	}

	//	Request header
	std::string							method;
	std::string							location;
	std::string							protocol;

	std::string							host;
	int									port;

	size_t								content_length;
	std::vector<std::string>			cookies;

	//	Request body
	post_file							post_file;
	std::vector<std::string>			small_datas;
};

class Request
{

public:
	//	Public member functions

	Request(void);

	~Request(void);

	void					manage_request(int fd);
	bool					is_all_received(void);
	const RequestMembers&	getRequest(void);

private:
			//	Private member functions
	
	std::string	read_client(int fd);
	void		parse(std::string &buffer);

	//	Main parsing
	void		parseHeader(std::string& line);
	void		parseBody(std::string& line);
	void		parseBoundary(std::string& line);

	//	Header parsing
	void		parseMethod(std::stringstream& ss, std::string& word);
	void		parseHost(std::stringstream& ss);
	void		parseContentLength(std::stringstream& ss);
	void		parseContentType(std::stringstream& ss);
	void		parseCookie(std::stringstream& ss);

	//	Body parsing
	void		parseContentDisposition(std::stringstream& ss);
	void		parseFile(void);
	void		parsePost(std::string& line);


	//	Context for parsing
	enum Context
	{
		HEADER,
		BODY,
		BOUNDARY,
		CONTENT
	};

			//	Private variables
	RequestMembers					m_rm;

	Context							ctx;
	std::string						boundary;
	bool							has_read;
	size_t							content_received;
	std::string						whole_buff;
};

std::ostream&	operator<<(std::ostream &ostr, RequestMembers& rm);

#endif
