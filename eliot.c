/* SINGLE FILE ELIOT */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "_ressources/cJSON.h"
#include "_ressources/mongoose.h"

#define WEBBAPP_URL "http://192.168.1.190:8000"
#define PORT_URL   "tcp://192.168.1.190:4242"
#define MAX_LINE   512          // taille maximale d'une ligne acceptée
#define MAX_SUBS 16
#define MAX_BROKER_CLIENTS 64


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

struct eliot_state {
	struct mg_mgr mgr;
	struct client broker_clients[MAX_BROKER_CLIENTS];
	int nb_broker_clients;
	cJSON *json_data;

};

struct eliot_state eliot;

/* UTILS */
static int read_json(){

	struct mg_str fcontent = mg_file_read(&mg_fs_posix, "data.json");
	if((eliot.json_data = cJSON_Parse(fcontent.buf))==NULL){
		perror("Error on JSON parsing\n");
	}

	mg_free(fcontent.buf);

	return 0;
}

/* WEBAPP FUNCTIONS */
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

/* BROKER FUNCTIONS */
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


/* EVENT HANDLERS */

/* Webapp event handler. */
static void webapp_ev_handler(struct mg_connection *c, int ev, void *ev_data) {

	/* Retrieving the broker connection from void* pointer fn_data */
	struct mg_connection *broker_conn = (struct mg_connection*) c->fn_data;

	if (ev == MG_EV_HTTP_MSG) {  /* Http request received */
		struct mg_http_message *hm = (struct mg_http_message *) ev_data; /* Parse the request */

	/* GET */

		if (mg_match(hm->method, mg_str("GET"), NULL)) {
			if (mg_match(hm->uri, mg_str("/get_json_dat"), NULL)) {

				cJSON *ha_json = cJSON_GetObjectItemCaseSensitive(eliot.json_data, "home_assist");
				char *json_str = cJSON_Print(ha_json);

				mg_http_reply(c, 200,
				            "Content-Type: application/json\r\n"
				            "Cache-Control: no-store\r\n",
				            "%s", json_str);

			} else {
				struct mg_http_serve_opts opts = {.root_dir = "./html", .fs = &mg_fs_posix};
				mg_http_serve_dir(c, hm, &opts);
			}
		}

	/* POST */

		else if (mg_match(hm->method, mg_str("POST"), NULL)){

		/* HOME CMD */
		  	if (mg_match(hm->uri, mg_str("/cmd/#"), NULL)) {
				char uri[128], room[32], device[32], action[32];

				/* Copy ended by \0 */
				snprintf(uri, sizeof(uri), "%.*s", (int) hm->uri.len, hm->uri.buf);

	  			/* Parse cmd */
				sscanf(uri, "/cmd/%31[^/]/%31[^/]/%31[^/]", room, device, action);

			  	mg_http_reply(c, 200, "", "{%m:%d}\n", MG_ESC("status"), 1);
		   	}

		/* CREATE COMBO */
			if (mg_match(hm->uri, mg_str("/makecombo/"), NULL)) {
				struct mg_str data = mg_file_read(&mg_fs_posix, "data.json");
				int len = 0;
				int off = mg_json_get(data, "$.combos", &len);

				int i = off + len - 1; // position du ']' fermant
				while (i > off + 1 && isspace((unsigned char) data.buf[i - 1])){
					i--;
				}
				int vide = (i == off + 1);  // tableau vide : pas de virgule

				char *out = mg_mprintf("%.*s%s%.*s%.*s",
						i, data.buf,
						vide ? "" : ",\n\t\t",
						(int) hm->body.len, hm->body.buf,
						(int) data.len - i, data.buf + i);
				mg_file_write(&mg_fs_posix, "data.json", out, strlen(out));
				mg_http_reply(c, 200, "", "ok\n");

				free(out);
				free((void *) data.buf);
			}
		}
	}
}

/* Event handler for broker TCP conections. */
static void broker_ev_handler(struct mg_connection *c, int ev, void *ev_data) {


	/* Accept new client. */
    if (ev==MG_EV_ACCEPT){

	    printf("[conn %lu] connected (%d clients)\n", c->id, eliot.nb_broker_clients);
		if (eliot.nb_broker_clients==MAX_BROKER_CLIENTS){
			c->is_closing = 1;
			printf("Refused a connection. Max client nb attained.\n");
		} else {
			/* Create client pointer, add it to mg_connection fn_data */
			struct client *client = calloc(1, sizeof(*client));
			client->id = c->id;
			c->fn_data = (struct client*) client;

			eliot.broker_clients[eliot.nb_broker_clients] = *client;
		    eliot.nb_broker_clients++;
		}
    }

    /* Receive from client.*/
    else if (ev==MG_EV_READ){

  		char *nl;

    	/* while newline located in buffer, handle buffer */
	    while ((nl = memchr(c->recv.buf, '\n', c->recv.len)) != NULL) {

			size_t used = (size_t) (nl - (char *) c->recv.buf) + 1;
			char *line = (char *) c->recv.buf;

			/* Parse PUB/SUB command and topic */
			char cmd[3];
			struct topic topic = {0};
			if (parse_request(line, cmd, &topic)==0){

				/* If PUB, update all subscribed clients
				 * Iterate mg_mgr connections for clients, test subs, and
				 * post accordingly
				 */
				if (strcmp(cmd, "PUB")==0) {


					printf("PUBACK : %s", line); // Debug print Line
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

					printf("SUBACK : %s", line);
					struct client *cl = (struct client *) c->fn_data;
					if (cl->nsubs<MAX_SUBS){
						cl->subs[cl->nsubs] = topic;
						cl->nsubs++;
					}
					// TODO check for double subs to the same topic!
				}

				else if (strcmp(cmd, "LST")==0) {

					printf("LSTACK : %s", line);
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


	    if (c->recv.len > MAX_LINE) {
	        printf("[conn %lu] line too long\n", c->id);
	        c->is_closing = 1;
	    }

    }

    else if (ev==MG_EV_CLOSE){
   			eliot.nb_broker_clients--;
			free(c->fn_data);
			printf("[conn %lu] closed (%d clients)\n", c->id, eliot.nb_broker_clients);
    }

}



int main(void){

	/* Broker setup */
    mg_log_set(MG_LL_ERROR);
    mg_mgr_init(&eliot.mgr);


    mg_listen(&eliot.mgr, PORT_URL, broker_ev_handler, &eliot);
    printf("Broker listening.\n");
    mg_http_listen(&eliot.mgr, WEBBAPP_URL, webapp_ev_handler, &eliot);
	printf("Webapp running on %s\n", WEBBAPP_URL);

	read_json();
    mainloop:
    for (;;) {
    	mg_mgr_poll(&eliot.mgr, 100);
    }

    cleanup:
    cJSON_Delete(eliot.json_data);
}
