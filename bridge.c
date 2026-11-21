#include <stdio.h>
#include <stdlib.h>
#include "_ressources/mongoose.h"

#define HOST "192.168.1.190:4242"
#define WEBBAPP_URL "http://192.168.1.190:8000"

/* Bridge manages JSON writing of combos and reading of rooms and devices on data.json.
 * This function sends the data to the webpage script on GET /data_json.
 */

static void send_data_json(struct mg_connection *c){

	struct mg_str f_content = mg_file_read(&mg_fs_posix, "data.json");

	if (f_content.buf == NULL) {
		mg_http_reply(c, 500, "Content-Type: application/json\r\n",
	                "{\"error\":\"data.json introuvable\"}");
		return;
	}

	mg_http_reply(c, 200,
	            "Content-Type: application/json\r\n"
	            "Cache-Control: no-store\r\n",
	            "%.*s", f_content.len, f_content.buf);
	free(f_content.buf);
}

/* Http event handler.
 * Handle GET and POST.
 * On POST /cmd, get the command as a JSON with paths "room", "device" and "action".
 * Parse that JSON and sends it as a clean PUB to the broker.
 */
static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {

	/* Retrieving the broker connection from void* pointer fn_data */
	struct mg_connection *broker_conn = (struct mg_connection*) c->fn_data;

	if (ev == MG_EV_HTTP_MSG) {  /* Http request received */
		struct mg_http_message *hm = (struct mg_http_message *) ev_data; /* Parse the request */

		/* GET */
		if (mg_match(hm->method, mg_str("GET"), NULL)) {
			if (mg_match(hm->uri, mg_str("/data_json"), NULL)) {
				send_data_json(c);
			} else {
				struct mg_http_serve_opts opts = {.root_dir = "./html", .fs = &mg_fs_posix};
				mg_http_serve_dir(c, hm, &opts);
			}
		}

		/* POST */
		else if (mg_match(hm->method, mg_str("POST"), NULL)){

		  	if (mg_match(hm->uri, mg_str("/cmd/#"), NULL)) {
				char uri[128], room[32], device[32], action[32];

				/* Copy ended by \0 */
				snprintf(uri, sizeof(uri), "%.*s", (int) hm->uri.len, hm->uri.buf);

	  			/* Parse cmd */
				sscanf(uri, "/cmd/%31[^/]/%31[^/]/%31[^/]", room, device, action);

				/* Send to broker */
				mg_printf(broker_conn, "PUB %s/%s/%s\n", room, device, action);

			  	mg_http_reply(c, 200, "", "{%m:%d}\n", MG_ESC("status"), 1);
		   	}

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

int main(void){

	/* Initiate a mongoose manager.
	 * We  create a tcp connection to the broker,
	 * then initiate the bridge webserver with http listen.
	 * The broker connection is passed to the http event handler,
	 * Thus, the event handler can send data from POST requests to the broker.
	 */

	struct mg_mgr mgr;
 	mg_log_set(MG_LL_ERROR);
	mg_mgr_init(&mgr);

	/* Broker connection */
	struct mg_connection *broker_conn;
	broker_conn = mg_connect(&mgr, HOST, NULL, NULL);
	if (broker_conn==NULL){
		perror("Error on connecting to broker.");
	}

	/* Http webserver startup */
	if (mg_http_listen(&mgr, WEBBAPP_URL, ev_handler, broker_conn)==NULL){
		perror("Error on listening.");
	}

	printf("Running on %s\n", WEBBAPP_URL);

	/* Mainloop */
	for(;;){

		mg_mgr_poll(&mgr, 100);
	}

	return EXIT_SUCCESS;
}
