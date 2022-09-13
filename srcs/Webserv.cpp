#include "../includes/Webserv.hpp"

// quit via signal
bool	quit;

/***********************************************************************/
/*                               SIGNAL                                */
/***********************************************************************/

// catching CTRL C to quit via destructors
void	signalHandler( int signum )
{
	(void)signum;

	quit = true;
}

/***********************************************************************/
/*                        CONSTR DESTR WEBSERV                         */
/***********************************************************************/

// constructor
Webserv::Webserv(std::vector<ServerMembers> &sm)
{
	quit = false;

	std::signal(SIGINT, signalHandler);	

	//	Create servers
	for (size_t i = 0; i < sm.size(); ++i)
		servers.push_back(sm[i]);

	//	Init current_sockets with server sockets.
	FD_ZERO(&current_sockets);
	for (size_t i = 0; i < servers.size(); ++i)
		FD_SET(servers[i].sock, &current_sockets);
}

// destructor
Webserv::~Webserv(void)
{
	for (size_t i = 0; i < servers.size(); i++)
		close(servers[i].sock);
}

/***********************************************************************/
/*                             RUN SERVER                              */
/***********************************************************************/

// run
void Webserv::run()
{
	fd_set						ready_r_sock;
	fd_set						ready_w_sock;

	while (1)
	{
		if (quit == true)
			break ;

		//	Copy because select is destructive.
		ready_r_sock = current_sockets;
		ready_w_sock = current_sockets;

		if (select(FD_SETSIZE, &ready_r_sock, &ready_w_sock, NULL, NULL) < 0 && quit == false)
			throw std::runtime_error("Select failed.");

		//	Check if client asks for connection
		for (size_t i = 0; i < servers.size(); ++i)
		{
			if (quit == true)
				break ;
			if (FD_ISSET(servers[i].sock, &ready_r_sock))
				servers[i].accept_client(current_sockets);

			for (std::vector<Client>::iterator cli = servers[i].clients.begin(); 
				cli != servers[i].clients.end(); ++cli)
			{
				if (quit == true)
					break ;
				if (cli->request.is_all_received() == false)
				{
					if (FD_ISSET(cli->sock, &ready_r_sock))
						cli->get_request();
				}

				if (cli->request.is_all_received() == true)
				{
					if (FD_ISSET(cli->sock, &ready_w_sock))
						servers[i].send_response(*cli);

					if (cli->response.is_sent())
					{
						FD_CLR(cli->sock, &current_sockets);
						close(cli->sock);
						servers[i].clients.erase(cli);
						break ;
					}
				}
			}
		}
	}

}

/***********************************************************************/
/*                            CONSTR SERVER                            */
/***********************************************************************/

//	Server constructor (socket)
Webserv::Server::Server(ServerMembers &sm)
	: members(sm) 
{
	struct sockaddr_in	saddr;

	//	Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0); 
	if (sock < 0)
		throw std::runtime_error("Socket failed.");

	if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("Fcntl failed.");

	//	Allow the port to be reusable when restarting
	int	reuse = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
		throw std::runtime_error("Setsockopt failed.");

	saddr.sin_family = AF_INET;        
	saddr.sin_port = htons(sm.port);
	saddr.sin_addr.s_addr = inet_addr(sm.host.c_str());

	//	Bind socket to address and listen to it
	if (bind(sock, (struct sockaddr*) &saddr, sizeof(saddr)) < 0)
		throw std::runtime_error("Bind failed.");
	if (listen(sock, 4092) < 0)
		throw std::runtime_error("Listen failed.");
}

/***********************************************************************/
/*                          REQUEST RESPONSE                           */
/***********************************************************************/

// get request before sending response
void	Webserv::Client::get_request(void)
{
	request.manage_request(sock);
}

// send response
void	Webserv::Server::send_response(Client &client)
{
	RequestMembers rm = client.request.getRequest();
	client.response.manage_response(client.sock, rm);
}

/***********************************************************************/
/*                               CLIENT                                */
/***********************************************************************/

//	Client
Webserv::Client::Client(ServerMembers &sm)
	: response(sm)
{
}

// accept client
void	Webserv::Server::accept_client(fd_set &current_sockets)
{
	Client	new_client(members);

	new_client.sock = accept(sock, 0, 0);
	if (new_client.sock == -1)
		throw std::runtime_error("Accept failed.");

	if (fcntl(new_client.sock, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("Fcntl failed.");

	clients.push_back(new_client);
	FD_SET(new_client.sock, &current_sockets);
}
