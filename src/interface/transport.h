/*
 * This is free software; see the source for copying conditions.  There is NO
 * warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdint.h>

#include <queue>
#include <string>
#include <map>

#include <arpa/inet.h>

/* output BYTES bytes per output line */
#define WIDTH (8)

/* 1024 * 1024 bytes = 1MB */
#define SIZE (1024*1024)

class Transport {
private:
	int identification; /* auth token */
	time_t created; /* the first communication time */
	time_t updated; /* the lastest communication time */
	bool alive; /* true: live; false: die */
	int fd; /* file descriptor */
	void *rx; /* transport data `Read'  */
	void *wx; /* transport data `Write' */
	int rp; /* transport data `Read' pointer position */
	int wp; /* transport data `Write' pointer position */
	int rs; /* transport data `Read' size  */
	int ws; /* transport data `Write' size */
	double speed; /* bytes per second */

	struct sockaddr_in peer_addr;
	socklen_t peer_addrlen;

public:
	Transport(int fd, time_t created, struct sockaddr_in peer_addr, socklen_t peer_addrlen, int size = SIZE) {
		memset(this, 0, sizeof *this);

		this->created = created;
		this->updated = this->created;

		this->fd = fd;
		this->peer_addr = peer_addr;
		this->peer_addrlen = peer_addrlen;

		if (size <= 0) {
			size = SIZE;
		}
		this->rs = size;
		this->ws = size;
		this->rx = malloc(this->rs);
		this->wx = malloc(this->ws);
		memset(this->rx, 0, this->rs);
		memset(this->wx, 0, this->ws);
	}

	int set_identification(int identification) {
		return this->identification = identification;
	}

	int get_identification(void) {
		return this->identification;
	}

	int set_fd(int fd) {
		return this->fd = fd;
	}

	int get_fd(void) {
		return this->fd;
	}

	int set_rp(int rp) {
		return this->rp = rp;
	}

	int get_rp(void) {
		return this->rp;
	}

	int set_wp(int wp) {
		return this->wp = wp;
	}

	int get_wp(void) {
		return this->wp;
	}

	void *set_rx(void *rx, int rs) {
		while (rs >= this->rs - this->rp) {
			printf("realloc %p %d\n", this->rx, this->rs);
			this->rx = realloc(this->rx, this->rs * 2);
			if (this->rx == NULL) {
				return this->rx;
			} else {
				this->rs *= 2;
				memset(this->rx + this->rp, 0, this->rs - this->rp);
			}
		}
		memcpy(this->rx + this->rp, rx, rs);
		this->rp += rs;
		return this->rx + this->rp;
	}

	void *clear_rx(int size = SIZE) {
		if (size <= 0) {
			size = SIZE;
		}
		memset(this->rx, 0, sizeof this->rp);
		this->rx = realloc(this->rx, size);
		this->rp = 0;
	}

	void *get_rx(void) {
		return this->rx;
	}

	void *set_wx(void *wx, int ws) {
		while (ws >= this->ws - this->wp) {
			printf("realloc %p %d\n", this->wx, this->ws);
			this->wx = realloc(this->wx, this->ws * 2);
			if (this->wx == NULL) {
				return this->wx;
			} else {
				this->ws *= 2;
				memset(this->wx + this->wp, 0, this->ws - this->wp);
			}
		}
		memcpy(this->wx + this->wp, wx, ws);
		this->wp += ws;
		return this->wx + this->wp;
	}

	void *clear_wx(int size = SIZE) {
		if (size <= 0) {
			size = SIZE;
		}
		memset(this->wx, 0, sizeof this->wp);
		this->wx = realloc(this->wx, size);
		this->wp = 0;
	}

	void *get_wx(void) {
		return this->wx;
	}

	int set_rs(int rs) {
		return this->rs = rs;
	}

	int get_rs(void) {
		return this->rs;
	}

	int set_ws(int ws) {
		return this->ws = ws;
	}

	int get_ws(void) {
		return this->ws;
	}

	bool set_alive(bool alive) {
		return this->alive = alive;
	}

	bool get_alive(void) {
		return this->alive;
	}

	void pr(int width = WIDTH, bool b0 = false) {
		int i = 0;
		if (width <= 0 || width > 1024) {
			width = WIDTH;
		}

		printf("--- begin (hexadecimal 2-byte units) -- %s --\n", __func__);
		while (i < this->rp) {
			if (i % width == 0) {
				printf("%p ", this->rx + i);
			}
			printf(" 0x%02x", *(unsigned char*)(this->rx + i));
			if (b0) {
				printf(" %c", *(unsigned char*)(this->rx + i));
			}

			i++;
			if (i % width == 0) {
				puts("");
			}
		}
		puts("\n--- end ---");
		return ;
	}

	void pw(int width = WIDTH, bool b0 = false) {
		int i = 0;
		if (width <= 0 || width > 1024) {
			width = WIDTH;
		}

		printf("--- begin (hexadecimal 2-byte units) -- %s --\n", __func__);
		while (i < this->wp) {
			if (i % width == 0) {
				printf("%p ", this->wx + i);
			}
			printf(" 0x%02x", *(unsigned char*)(this->wx + i));
			if (b0) {
				printf(" %c", *(unsigned char*)(this->rx + i));
			}

			i++;
			if (i % width == 0) {
				puts("");
			}
		}
		puts("\n--- end ---");
		return ;
	}

	~Transport() {
		free(this->rx);
		free(this->wx);
		printf( "fd = %d, %s\n"
				"rx = %p, %f%% (%d / %d)\n"
				"wx = %p, %f%% (%d / %d)\n", 
				this->fd, __func__, 
				this->rx, 100. * this->rp / this->rs, this->rp, this->rs, 
				this->wx, 100. * this->wp / this->ws, this->wp, this->ws);
		memset(this, 0, sizeof *this);
	}
};

#endif
