#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/queue.h>
#include <ev.h>

#define PORT_NO 3034
#define BUFFER_SIZE 8192

struct client_t {
	ev_io io;
	int socketd;
	TAILQ_ENTRY (client_t) entries;
};

struct server_t {
	ev_io io;
	int socketd;
};

TAILQ_HEAD (, client_t) client_tailq_head;

int total_clients = 0;

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

int main() {
	struct ev_loop *loop = ev_default_loop(0);

	int sd;
	struct sockaddr_in addr;
	struct ev_io w_accept;

	TAILQ_INIT (&client_tailq_head);

	if((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		return -1;
	}

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT_NO);
	addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
		perror("bind error");
	}

	if(listen(sd, 2) < 0) {
		perror("listen error");
		return -1;
	}

	ev_io_init(&w_accept, accept_cb, sd, EV_READ);
	ev_io_start(loop, &w_accept);

	ev_loop(loop, 0);

	return 0;
}

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_sd;
	struct client_t* client = NULL;

	if(EV_ERROR & revents) {
		perror("got invalid event");
		return;
	}

	client_sd = accept(watcher->fd, (struct sockaddr*)&client_addr, &client_len);

	if(client_sd < 0) {
		perror("accept error");
		return;
	}

	client = (struct client_t*) calloc (1, sizeof (*client));

	if (client == NULL) {
		perror ("malloc failed");
		return;
	}
	client->socketd = client_sd;

	total_clients++;
	printf("Successfully connected with client\n");
	printf("%d clients connected.\n", total_clients);

	ev_io_init(&client->io, read_cb, client_sd, EV_READ);

	TAILQ_INSERT_TAIL(&client_tailq_head, client, entries);
	ev_io_start(loop, &client->io);
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
	struct client_t* client;
	struct client_t* this_client = (struct client_t*) watcher;

	uint8_t buffer[BUFFER_SIZE];
	ssize_t read;

	if(EV_ERROR & revents) {
		perror("got invalid event");
		return;
	}

	read = recv (this_client->socketd, (char*) buffer, BUFFER_SIZE, 0);
	if(read < 0) {
		perror("read error");
		return;
	}

	if(read == 0) {
		ev_io_stop(loop, &this_client->io);

		TAILQ_REMOVE(&client_tailq_head, this_client, entries);

		close(this_client->socketd);
		free(this_client);
		total_clients--;
		return;
	}

	TAILQ_FOREACH(client, &client_tailq_head, entries) {
		if (client != this_client) {
			send(client->socketd, (void*) buffer, read, 0);
		}
	}
}
