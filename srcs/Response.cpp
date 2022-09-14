#include "../includes/Response.hpp"

/***********************************************************************/
/*                             CONSTR DESTR                            */
/***********************************************************************/

//constructor
Response::Response(ServerMembers s)
	: server(s)
{
	error_responses[200] = "OK";
	error_responses[201] = "Created";
	error_responses[204] = "No Content";
	error_responses[403] = "Forbidden";
	error_responses[404] = "Not Found";
	error_responses[405] = "Method Not Allowed";
	error_responses[413] = "Payload Too Large";
	error_responses[501] = "Not Implemented";
	error_responses[502] = "Bad Gateway";
}

//destructor
Response::~Response()
{
}

/***********************************************************************/
/*                              DISPATCH                               */
/***********************************************************************/

// dispatch
void Response::manage_response(int socket, RequestMembers r)
{
	request = r;
	curr_sock = socket;

	// parse and get response
	// and stock response in http_response
	http_response = get_response();

	// write/send response
	write_response();
}

// dispatch 1
std::string	Response::get_response(void)
{
	int			error_code;
	std::string	path;
	std::string	file;

	//	There is still data from previous write
	if (http_response != "")
		return (http_response);

	// get location from .conf
	get_current_loc();

	// Check if correct method
	error_code = check_method();

	// only code 200 is ok
	if (error_code != 200)
		return (http_error(error_code));

	// Add index
	if (request.method == "GET" && curr_loc.uri == request.location && curr_loc.autoindex == false)
		request.location += curr_loc.index;

	// Check if correct path
	path = get_path(curr_loc.root + request.location);
	// checks if the path is real
	error_code = check_path_access(path);
	if (error_code != 200)
		return (http_error(error_code));

	// Manage DELETE
	if (request.method == "DELETE")
	{
		// removes file , return 0 if successful
		if (remove(path.c_str()))
			return (http_error(404));
	}

	// Manage GET
	else if (request.method == "GET")
	{
		// is it a file? yes so open and get its content
		if (is_file(path))
			file = retrieve_file(path);
		else // no so autoindex
			file = get_autoindex(path, request.location);
	}

	// Manage POST
	else if (request.method == "POST")
		file = manage_post_request(path);

	// Exec cgis
	for (std::map<std::string, std::string>::iterator it = curr_loc.cgis.begin();
			it != curr_loc.cgis.end(); ++it)
	{
		size_t	idx = path.rfind('.');
		// take off the path until . to get py for python or php for php
		if (idx != std::string::npos && path.substr(idx) == it->first && is_file(path))
			file = exec_cgi(path, it->second);
	}

	// Check body size
	if (file.size() > curr_loc.max_body_size)
		return (http_error(413));

	// Write response
	return (make_response(file, error_code, path));
}

// make response
std::string	Response::make_response(std::string file, int error_code, std::string path)
{
	std::string	response;

	std::stringstream tmp;
	tmp << error_code;
	std::string sec = tmp.str();

	std::stringstream tmp1;
	tmp1 << file.size();
	std::string sfs = tmp1.str();

	//	Headers
   	response += request.protocol;
	response += " ";
	response += sec;
	response += " ";
	response += error_responses[error_code];

	response += "\nDate: " + get_date();
	response += "\nContent-Length: " + sfs;
	response += "\nContent-Type: " + get_content_type(path);
	if (cookie != "")
		response += "\nSet-Cookie: " + cookie;
	response += "\r\n\r\n";

	//	Body
	response += file;
	response += "\r\n";
	return (response);
}

// write (send) response
void	Response::write_response(void)
{
	int			ret;
	std::string	&buffer = http_response;

	ret = write(curr_sock, buffer.c_str(), buffer.size());
	if (ret == -1)
		throw std::runtime_error("Write failed.");

	// Client disconnected
	if (ret == 0)
	{
		//close_connection(socket);
		return ;
	}
	buffer = buffer.substr(ret);
}

/***********************************************************************/
/*                               ERRORS                                */
/***********************************************************************/

// get error page
std::string	Response::http_error(int error_code)
{
	std::string response;
	std::string	path;
	std::string	file;

	//	Get error page
	path = get_path(curr_loc.error_pages[error_code]);
	if (check_path_access(path) == 200)
		file = retrieve_file(path);
	else
		file = "";
	if (file.size() > curr_loc.max_body_size)
		file = "";

	std::stringstream tmp;
	tmp << error_code;
	std::string sec = tmp.str();
	std::stringstream tmp1;
	tmp1 << file.size();
	std::string sfs = tmp1.str();

	//	Headers
   	response += request.protocol;
	response += " ";
	response += sec;
	response += " ";
	response += error_responses[error_code];

	response += "\nDate: " + get_date();
	response += "\nContent-Length: " + sfs;
	response += "\nContent-Type: " + get_content_type(path);
	response += "\r\n\r\n";

	//	Body
	response += file;
	response += "/r/n";
	return (response);
}

/***********************************************************************/
/*                                POST                                 */
/***********************************************************************/

std::string	Response::manage_post_request(std::string &path)
{
	//	Add potential cookies
	if (request.location == "/login.php" || request.location == "/other.php")
	{
		for (size_t i = 0; i < request.small_datas.size(); ++i)
		{
			std::string data = request.small_datas[i];

			// if login with username -> add to cookie
			if (data.substr(0, 9) == "username=")
				cookie = data + "; Expires=Wed, 27 Jun 2025 07:28:00 GMT; Path=/";
			else if (data == "logout=logout") // if logout erase cookie username
			{
				cookie = "username=xx; Expires=Wed, 27 Jun 2022 07:28:00 GMT; Path=/";
				for (std::vector<std::string>::iterator it = request.cookies.begin();
						it != request.cookies.end(); ++it)
				{
					if (it->substr(0, 9) == "username=")
					{
						request.cookies.erase(it);
						break ;
					}
				}
			}
		}
	}

	// if post file open and get content
	if (is_file(path))
		return (retrieve_file(path));
	else
		return ("");
}

/***********************************************************************/
/*                                CGI                                  */
/***********************************************************************/

std::string	Response::exec_cgi(std::string file_path, std::string exec_path)
{
	std::vector<const char *>	env;
	std::vector<const char *>	exec;

	int							pid;
	int							fd[2];
	char 						buf[65535];

	//	Create execution command and environnment
	// exec_path in .conf for example usr/bin/python
	exec.push_back(exec_path.c_str());
	// file_path the path to the file for example .py
	exec.push_back(file_path.c_str());
	exec.push_back(0);

	for (size_t i = 0; i < request.small_datas.size(); ++i)
		env.push_back(request.small_datas[i].c_str());

	for (size_t i = 0; i < request.cookies.size(); ++i)
		env.push_back(request.cookies[i].c_str());
	env.push_back(0);


	//	Pipe and fork
	if (pipe(fd) == -1)
		throw std::runtime_error("Pipe failed.");
	pid = fork();
	if (pid == -1)
		throw std::runtime_error("Fork failed.");
	else if (pid == 0)
	{
		//	Execute command
		if (dup2(fd[1], 1) == -1)
			throw std::runtime_error("Dup2 failed.");
		execve(exec[0], (char **)&exec[0], (char **)&env[0]);

		//	Print bad gateway file
		std::string path = get_path(curr_loc.error_pages[502]);
		std::cout << retrieve_file(path);
		exit(0);
	}
	else
	{
		//	Read the output and return
		wait(0);
		bzero(buf, 65535);
		if (read(fd[0], buf, 65535) == -1)
			throw std::runtime_error("Read failed.");
		close(fd[0]);
		close(fd[1]);

		return (buf);
	}
}

/***********************************************************************/
/*                             AUTOINDEX                               */
/***********************************************************************/

// autoindex (or when url finish with a '/')
std::string Response::get_autoindex(std::string fullpath, std::string path)
{
	DIR *dir = opendir(fullpath.c_str());

	// create a listing of dir from the path
	std::string Autoindex_Page =
	"<!DOCTYPE html>\n\
    <html>\n\
    <head>\n\
    <title>" + path + "</title>\n\
    </head>\n\
    <body>\n\
    <h1>INDEX</h1>\n\
    <p>\n";

	if (dir == NULL)
		throw std::runtime_error("Opendir failed : " + fullpath);
		
	int i = 0;
	while(path[i] != '\0'){
		i++;
	}
	i--;

	if (path[i] != '/')
		path += "/";

	// add every dir from full_path
	for (struct dirent *dir_entry = readdir(dir); dir_entry; dir_entry = readdir(dir))
		Autoindex_Page += dir_to_html(std::string(dir_entry->d_name), path);

	Autoindex_Page += "\
    </p>\n\
    </body>\n\
    </html>\n";
	closedir(dir);
	return Autoindex_Page;
}

/***********************************************************************/
/*                               METHODS                               */
/***********************************************************************/

// check methods
int	Response::check_method(void)
{
	if (!is_method_implemented())
		return (501);
	if (!is_method_allowed())
		return (405);
	return (200);
}

// not supported methods
bool	Response::is_method_allowed()
{
	for (size_t i = 0; i < curr_loc.allowedMethods.size(); ++i)
	{
		if (curr_loc.allowedMethods[i] == request.method)
			return (true);
	}
	return (false);
}

// have to manage GET POST and DELETE
bool	Response::is_method_implemented(void)
{
	if (request.method == "GET" || request.method == "POST" || request.method == "DELETE")
		return (true);
	return (false);
}

/***********************************************************************/
/*                                UTILS                                */
/***********************************************************************/

// get current loc
void	Response::get_current_loc(void)
{
	//	Find correct location in server
	for (size_t i = 0; i < server.locations.size(); ++i)
	{
		if (request.location.find(server.locations[i].uri) == 0)
		{
			if (curr_loc.uri < server.locations[i].uri)
				curr_loc = server.locations[i];
		}
	}
}

// is sent
bool	Response::is_sent(void)
{
	if (http_response == "")
		return (true);
	return (false);
}

// get date
std::string Response::get_date(void)
{
	time_t		rawtime;
	struct tm	*timeinfo;
	char		buff[100];

	time(&rawtime);
 	timeinfo = localtime(&rawtime);
	// function to get the time however you want
	strftime(buff, 100, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
	return (std::string(buff));
}

// is file
bool	Response::is_file(std::string path)
{
	struct stat	s;

	// can we access it literally
	if (access(path.c_str(), F_OK) < 0)
		return (false);
	if (stat(path.c_str(), &s) < 0)
		throw std::runtime_error("Stat failed.");
	return (S_ISREG(s.st_mode));
}

// get path
std::string	Response::get_path(std::string path)
{
	char		cwd[256];

	if (getcwd(cwd, 256) == NULL)
		throw std::runtime_error("Getcwd failed.");
	return (cwd + path);
}

// get files
std::string	Response::retrieve_file(std::string path)
{
	std::ostringstream	sstr;
	// open the file with its path
	std::ifstream		ifs(path.c_str(), std::ifstream::in);

	sstr << ifs.rdbuf();

	// return its content
	return (sstr.str());
}

// dir path to html path
std::string Response::dir_to_html(std::string dir_entry, std::string path)
{
	std::stringstream ss;
	path = path.substr(path.find(curr_loc.root) + curr_loc.root.size());
	if (dir_entry != ".." && dir_entry != ".")
		ss << "\t\t<p><a href=\"http://" + request.host + ":" << request.port << path + dir_entry + "\">" + dir_entry + "/" + "</a></p>\n";
	return ss.str();
}

// check paths
int	Response::check_path_access(std::string path)
{
	//	Check if we can access to path
	if (access(path.c_str(), F_OK) < 0)
		return (404);

	//	Check if path is a file or if folder is allowed
	if (!is_file(path) && curr_loc.autoindex == false && request.method == "GET")
		return (404);

	//	Check if read access
	if (access(path.c_str(), R_OK) < 0)
		return (403);

	return (200);
}

// content_type
std::string	Response::get_content_type(std::string file)
{
	std::string	extension;
	size_t		idx;
	
	idx = file.rfind('.');
	if (idx != std::string::npos)
		extension = file.substr(idx + 1);

	if (extension == "html" || extension == "php")
		return ("text/html");
	else if (extension == "xml")
		return ("text/xml");
	else if (extension == "js")
		return ("text/javascript");
	else if (extension == "css")
		return ("text/css");
	else if (extension == "cpp" || extension == "hpp")
		return ("text/plain");

	else if (extension == "pdf")
		return ("application/pdf");
	else if (extension == "doc")
		return ("application/msword");

	else if (extension == "jpeg" || extension == "jpg" || extension == "pjp"
			|| extension == "jfif" || extension == "pjpeg")
		return ("image/jpeg");
	else if (extension == "png")
		return ("image/png");
	else if (extension == "gif")
		return ("image/gif");
	else if (extension == "ico")
		return ("image/x-icon");

	else if (extension == "mp4")
		return ("video/mp4");
	else if (extension == "webm")
		return ("video/webm");
	else if (extension == "mpeg")
		return ("video/mpeg");

	else if (extension == "mp3")
		return ("audio/mpeg");

	else
		return ("text/html");
}
