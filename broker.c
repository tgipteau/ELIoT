#include <stdbool.h>
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

static int is_subscribed(struct client cl, struct topic t){
	/* Determine if a certain client cl is subscribed to the topic t.
	 * Handles wildcard '*' meaning 'any topic'
	 */
	for (int i=0 ; i<cl.nsubs ; i++){
		struct topic sub = cl.subs[i];
		if (strcmp(sub.room, t.room)==0 || strcmp(sub.room, "*")==0){
			/* Room is subbed */
			if (strcmp(sub.device, t.device)==0 || strcmp(sub.device, "*")==0){
				/* Device is subbed */
				return true;
			}
		};
	}
	return false;
}

static int parse_request(char *request, char *method, struct topic *t){
	/** Parse a PUB or SUB request and store data in topic struct
	 * and char* method(PUB or SUB)
	 */

	char topic_line[128];
	sscanf(request, "%s %s\n", method, topic_line);
	if(strcmp(method, "PUB")==0){
		/* Expecting full topic struct with action and options */
		sscanf(topic_line, "%[^/]/%[^/]/%[^/]/%s", t->room, t->device, t->action, t->options);
		return 0;
	}
	else if (strcmp(method, "SUB")==0) {
		/* Expecting semi-topic (room and device only) */
		sscanf(topic_line, "%[^/]/%s", t->room, t->device);
		return 0;
	}
	else if (strcmp(method, "LST")==0) {
		return 0;
	}

	return -1; /*Error */

}

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
			if (nclients==MAX_CLIENTS){
				printf("Refused connection. Max client nb attained.\n");
				// TODO
			} else {
				/* Create client pointer, add it to mg_connection fn_data */
				struct client *client = calloc(1, sizeof(*client));
				client->id = c->id;
				c->fn_data = (struct client*) client;

				clients[nclients] = *client;
			    nclients++;
			}

		break;

     	case(MG_EV_READ):
    		/* Receive from client.*/

		    for (;;) {

			/* We need to split on '\n' characters, c->recv being a stream.
			* Measure len used in received stream, then clear the used part.
			*/
			char *nl = memchr(c->recv.buf, '\n', c->recv.len);
			if (nl == NULL) break;

			size_t used = (size_t) (nl - (char *) c->recv.buf) + 1;
			char *line = (char *) c->recv.buf;

			/* Parse PUB/SUB command and topic */
			char cmd[3];
			struct topic topic = {0};
			if (parse_request(line, cmd, &topic)==0){


				if (strcmp(cmd, "PUB")==0) {
					/* If PUB, update all subscribed clients
					 * Iterate mg_mgr connections for clients, test subs, and
					 * post accordingly
					 */
					for (struct mg_connection *con = c->mgr->conns; con != NULL; con = con->next) {
					    struct client *cl = (struct client *) con->fn_data;
						/* No PUB to NULL clients and no self-PUB */
					    if (cl == NULL || c->id == con->id) continue;

						if (is_subscribed(*cl, topic)){
							mg_printf(con, "%s", line);
						}
					}
				}

				else if (strcmp(cmd, "SUB")==0) {
					/* If SUB, update client subscribe list.
					 * Corresponding client struct is pointed at by c->fn_data
					 * (see accept protocol)
					 */
					struct client *cl = (struct client *) c->fn_data;
					if (cl->nsubs<MAX_SUBS){
						cl->subs[cl->nsubs] = topic;
						cl->nsubs++;
					}
					// TODO check for double subs to the same topic!
				}

				else if (strcmp(cmd, "LST")==0) {
					/* Send back informations about subs */
					struct client *cl = (struct client *) c->fn_data;
					for (int i = 0 ; i<cl->nsubs ; i++){
						mg_printf(c, "Sub : %s/%s\n", cl->subs[i].room, cl->subs[i].device);
					}
				}

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
			free(c->fn_data);
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
