#include "../includes/Webserv.hpp"
#include "../includes/Config.hpp"

/***********************************************************************/
/*                                MAIN                                 */
/***********************************************************************/

int main(int argc, char **argv)
{
	// can only be used with a .conf file
	if (argc != 2)
	{
		std::cout << "Usage: " << argv[0] << " config.conf" << std::endl;
		return (1);
	}

    try 
    {
		// parsing of the .conf
		Config    			cf(argv[1]);
		// then put all the data in the different servers
        std::vector<ServerMembers>	servers = cf.getConfig();

		// Webserv constructor copies all the server in its class
		Webserv webserv(servers);
		// run the server
        webserv.run();
	}
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
		return (1);
    }

	// goodbye
	std::cout << " Server closed" << std::endl;
	return (0);
}
