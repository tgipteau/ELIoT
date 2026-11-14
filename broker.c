#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "_ressources/mongoose.h"

#define PORT_URL   "tcp://192.168.1.190:4242"
#define MAX_LINE   512          // taille maximale d'une ligne acceptée
#define MAX_SUBS 16
#define MAX_CLIENTS 64


/* Define topic struct and client struct, storing its subscribed topics. */
struct topic {
	char room[32];
	char device[32];
	char action[32];
	char options[32];
};

struct client {
	unsigned long id;
	struct topic subs[MAX_SUBS];
	int nsubs;
};

static int nclients = 0;
struct client clients[MAX_CLIENTS];

/* Event handler for broker TCP conections.
 * On accept, store client with an ID. Send back that ID to synchronize client and broker.
 * On read, check for SUB requests, and update client accordingly.
 */
static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    (void) ev_data;

    switch (ev) {
    	case(MG_EV_ACCEPT):
	   		/* Store new client.
			* Send back unique ID.
			* */

		    printf("[conn %lu] connecté (%d clients)\n", c->id, nclients);
		   	struct client client = {0};
			client.id = c->id;
			if (nclients==MAX_CLIENTS){
				printf("Refused connection. Max client nb attained.\n");
				// DISCONNECT !!!
			} else {
				clients[nclients] = client;
			    nclients++;
			}

		break;

     	case(MG_EV_READ):
    		/* Receive from client.
      		* When received, parse the incoming line
	        * SUB topic/topic/...
	        * PUB topic/topic/...
	        */

		    for (;;) {

			/* We need to split on '\n' characters, c->recv being a stream.
			* Measure len used in received stream, then clear the used part.
			*/
			char *nl = memchr(c->recv.buf, '\n', c->recv.len);
			if (nl == NULL) break;

			size_t used = (size_t) (nl - (char *) c->recv.buf) + 1;
			char *line = (char *) c->recv.buf;

			/* Parse PUB/SUB command and topic */
			char cmd[3], topic_line[128];
			struct topic topic = {0};

			sscanf("%s %s\n", cmd, topic_line);
			if(strcmp(cmd, "PUB")==0){
				/* Expecting full topic struct with action and options */

			}


			/* Clear recv buffer from used line*/
			mg_iobuf_del(&c->recv, 0, used);
			}

		    if (c->recv.len > MAX_LINE) {                    // ligne trop longue sans '\n'
		        printf("[conn %lu] ligne trop longue, fermeture\n", c->id);
		        c->is_closing = 1;
		    }

		break;

	    case(MG_EV_CLOSE):
			nclients--;
			printf("[conn %lu] fermé (%d clients)\n", c->id, nclients);

		break;

    }

}


/* Initiate a mg manager, have it listen on PORT.
 * Run main poll loop.
 */
int main(void) {


    struct mg_mgr mgr;
    mg_log_set(MG_LL_ERROR);
    mg_mgr_init(&mgr);

    mg_listen(&mgr, PORT_URL, ev_handler, NULL);
    printf("Broker listening.\n");

    mainloop:
    for (;;) {
    	mg_mgr_poll(&mgr, 100);
    }
}
