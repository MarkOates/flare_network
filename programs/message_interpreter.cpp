#include <allegro5/allegro.h> // for compatibility with union/Makefile


#include <flare_network/NetworkService.hpp>
#include <string>
#include <iostream>


#include <lib/fnv.h>


bool shutdown_program = false;


class SpecialNetworkService : public NetworkService
{
public:
   SpecialNetworkService()
      : NetworkService()
   {}
	virtual void on_message_receive(std::string message) override
   {
      std::cout << "Boooond>" << message << std::endl;
   }
};






//// custom allegro events: 


#define ALLEGRO_EVENT_NETWORK_RECEIVE_MESSAGE ALLEGRO_GET_EVENT_TYPE('N', 'T', 'W', 'K')

struct ALLEGRO_EVENT_NETWORK_SOURCE {
	ALLEGRO_EVENT_SOURCE event_source;
	int field1;
	int field2;
	// etc...
};

ALLEGRO_EVENT_NETWORK_SOURCE *al_create_network_event_source(void)
{
	ALLEGRO_EVENT_NETWORK_SOURCE *thing = (ALLEGRO_EVENT_NETWORK_SOURCE *)malloc(sizeof(ALLEGRO_EVENT_NETWORK_SOURCE)); 
	if (thing) {
		al_init_user_event_source(&thing->event_source);
		thing->field1 = 0;
		thing->field2 = 0;
	}

	return thing;
}

static void my_network_event_dtor(ALLEGRO_USER_EVENT *user_event)
{
	std::cout << "dtor" << std::endl;
	delete (std::string *)(user_event->data1);
}






int main(int argc, char* argv[])
{
   al_init();

   ALLEGRO_EVENT_QUEUE *event_queue = al_create_event_queue();
   //ALLEGRO_EVENT_QUEUE event_queue;


	ALLEGRO_EVENT_NETWORK_SOURCE *network_event_source;
   network_event_source = al_create_network_event_source();
   al_register_event_source(event_queue, &network_event_source->event_source);


   try
   {
      std::string ip_or_url = "localhost";
      std::string port_num = "54321";

      if (argc != 3)
      {
         std::cout << "Usage: chat_client <host> <port>\n";
         std::cout << "no values provided, defaulting to:" << std::endl;
      }
      else
      {
         ip_or_url = argv[1];
         port_num = argv[2];
      }

      std::cout << "host: " << ip_or_url << std::endl;
      std::cout << "port: " << port_num << std::endl;

      // create the network service object

      SpecialNetworkService *network_service = new SpecialNetworkService();
      network_service->connect(ip_or_url, port_num);

      // for this example, we'll use the cin to get input from the user


      while (!shutdown_program)
      {
         ALLEGRO_EVENT current_event;
         al_wait_for_event(event_queue, &current_event);

         switch (current_event.type)
         {
         case ALLEGRO_EVENT_NETWORK_RECEIVE_MESSAGE:
            std::cout << "message received" << std::endl;
            break;
         default:
            break;
         }
      }


      /*

      char line[SpecialNetworkService::max_message_length + 1];
      bool abort = false;
      while (!abort)
      {
         std::cin.getline(line, SpecialNetworkService::max_message_length + 1);
         if (line[0] == 'q') abort = true;
         else network_service->send_message(line);
      }

      */

      // after it's over, disconnect from the service

      network_service->disconnect();
   }
   catch (std::exception& e)
   {
      std::cerr << "Exception: " << e.what() << "\n";
   }

   al_uninstall_system();

   return 0;
}

