#include <allegro_flare/allegro_flare.h>
#include <flare_network/network_service.hpp>




typedef struct ALLEGRO_EVENT_NETWORK_SOURCE ALLEGRO_EVENT_NETWORK_SOURCE;

#define ALLEGRO_EVENT_NETWORK_RECEIVE_MESSAGE ALLEGRO_GET_EVENT_TYPE('N', 'T', 'W', 'K')

struct ALLEGRO_EVENT_NETWORK_SOURCE {
	ALLEGRO_EVENT_SOURCE event_source;
	int field1;
	int field2;
	/* etc. */
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


class NetworkServiceScreen : public Screen, public NetworkService
{
private:
	ALLEGRO_EVENT_NETWORK_SOURCE *network_event_source;
	Screen *main_project_screen;
	bool connected;
public:
	NetworkServiceScreen(Screen *main_project_screen)
		: Screen(NULL)
		, NetworkService()
		, main_project_screen(main_project_screen)
		, connected(false)
		, network_event_source(NULL)
	{
		network_event_source = al_create_network_event_source();
		al_register_event_source(af::event_queue, &network_event_source->event_source);
	}
	void on_message_receive(std::string message) override
	{
		ALLEGRO_EVENT user_event;
		user_event.type = ALLEGRO_EVENT_NETWORK_RECEIVE_MESSAGE;
		std::string *msg = new std::string(message);
		user_event.user.data1 = (intptr_t)(msg);
		al_emit_user_event(&network_event_source->event_source, &user_event, my_network_event_dtor);
	}
	void key_down_func() override
	{
		switch (af::current_event->keyboard.keycode)
		{
		case ALLEGRO_KEY_SPACE:
			if (!connected)
			{
				connect("openvpa.com", "3000");
				connected = true;
			}
			else
			{
				// WARNING!
				// THERE IS CURRENTLY A BUG, AND AFTER DISCONNECT
				// A RECONNECTION CAN NOT BE CREATED
				if (connected) disconnect();
				connected = false;
			}
			break;
		}
	}
	void user_event_func() override
	{
		switch (af::current_event->type)
		{
		case ALLEGRO_EVENT_NETWORK_RECEIVE_MESSAGE:
			//std::cout << " USEREVENT!" << std::endl;
			//std::string message = *((std::string *)(af::current_event->user.data1));
			main_project_screen->receive_signal("NETWORK_EVENT", (void *)(af::current_event->user.data1));
			al_unref_user_event(&af::current_event->user);
			break;
		}
	}
	bool is_connected()
	{
		return connected;
	}
	void primary_timer_func() override
	{
	}
};



class Project : public Screen
{
private:
	std::string last_message;

public:
	NetworkServiceScreen *nwk;

	Project(Display *display)
		: Screen(display)
		, nwk(NULL)
	{
	}
	void receive_signal(std::string const &signal, void *data) override
	{
		if (signal == "NETWORK_EVENT")
		{
			last_message = *((std::string *)(data));
			std::cout << last_message << std::endl;
		}
	}
	void primary_timer_func() override
	{
		//al_clear_to_color(color::orange);
		std::string output_message = "Last Message: ";
		output_message += last_message.c_str();
		al_draw_text(af::fonts["DroidSans.ttf 30"], color::white, 200, 300, ALLEGRO_FLAGS_EMPTY, last_message.c_str());
		
		al_draw_text(af::fonts["DroidSans.ttf 30"], color::darkblue, 500, 100, ALLEGRO_ALIGN_CENTER, "Press SPACEBAR to toggle connect/disconnect from server.");
		al_draw_text(af::fonts["DroidSans.ttf 30"], color::darkblue, 500, 150, ALLEGRO_ALIGN_CENTER, (nwk->is_connected()) ? "status: CONNECTED" : "status: not connected");
	}
};


int main(int argc, char* argv[])
{
	af::initialize();
	Display *display = af::create_display();
	Project *proj = new Project(display);
	NetworkServiceScreen *nss = new NetworkServiceScreen(proj);
	proj->nwk = nss;
	af::run_loop();
}

