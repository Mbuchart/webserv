#include "../includes/Webserv.hpp"
#include "../includes/Config.hpp"

/***********************************************************************/
/*                                MAIN                                 */
/***********************************************************************/

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cout << "Usage: " << argv[0] << " config.conf" << std::endl;
		return (1);
	}

    try 
    {
		Config    			cf(argv[1]);
        std::vector<ServerMembers>	servers = cf.getConfig();

		Webserv webserv(servers);
        webserv.run();
	}
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
		return (1);
    }

	std::cout << " Server closed" << std::endl;
	return (0);
}
