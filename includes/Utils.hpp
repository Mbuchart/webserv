#ifndef UTILS_HPP
# define UTILS_HPP

#include <iostream>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
class Utils
{
public:
	static std::string	trim(std::string& line, const std::string& whitespace = " \t");
	static bool			isValidMethod(const std::string& word);
	int long  autoindx[65535];
};

#endif
