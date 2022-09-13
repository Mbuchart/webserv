#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include "Config.hpp"
#include "Response.hpp"
#include "Request.hpp"

class Response; 
class Webserv
{
	//	Client class
	class	Client {

	public:
		Client(ServerMembers &sm);

		void	get_request(void);

		int				sock;
		Response	response;
		Request	request;
	};

	//	Server class
	class	Server {

	public:
		Server(ServerMembers &sm);
		//~Server();

		void	send_response(Client &client);
		void	accept_client(fd_set &current_sockets);

		int					sock;
		std::vector<Client>	clients;
		ServerMembers		members;
	};

public:

			//	Public functions

	Webserv(std::vector<ServerMembers> &sm);
	~Webserv(void);

	void	run();

private:

			//	Private variables

	std::vector<Server>		servers;
	fd_set					current_sockets;
};

#endif
