#include "task.h"

#define MAX_EVENTS (0xff)
#define BUFFER_LENGTH (0xff)
struct epoll_event ev, events[MAX_EVENTS];
int listen_sock, conn_sock, nfds, epollfd;

bool is_quit;

int setnonblocking(int fd) {
	int ret = 0;
	printf("%s:%d:%s\n", __FILE__, __LINE__, __func__);
	do {
		int flags = fcntl(fd, F_GETFL);    
		if (flags == -1) {
			ret = flags;
			printf("%s(%d)\n", strerror(errno), errno);
			break;
		}
		flags |= O_NONBLOCK; /* If the O_NONBLOCK flag is enabled, 
								then the system call fails with the error EAGAIN. */
		ret = fcntl(fd, F_SETFL, flags);
		if (ret == -1) {
			printf("%s(%d)\n", strerror(errno), errno);
			break;
		}
	} while (0);
	return ret;
}

int reads(Transport *t) {
	int ret = 0;
	int fd = t->get_fd();
	if (t == NULL) {
		printf("t = %p\n", t);
		return ret;
	}

	printf("fd = %d, %s\n", fd, __func__);
	do {
		char buffer[BUFFER_LENGTH + 1];
		memset(buffer, 0, sizeof buffer);
		ret = read(fd, buffer, BUFFER_LENGTH);
		if (ret < 0) {
			printf("%s(%d)\n", strerror(errno), errno);
			break;
		} else if (ret == 0) {
			t->set_alive(false);
			printf("client close\n", fd);
			break;
		} else if (ret > 0 && ret <= BUFFER_LENGTH) {
			printf("[%s]\n", buffer);
			t->set_data(buffer, ret);
			memset(buffer, 0, ret);
			if (ret != BUFFER_LENGTH) {
				break;
			}
		}
	} while (1);
	return ret;
}

int writes(Transport *t) {
	int ret = 0;
	int fd = t->get_fd();
	if (t == NULL) {
		printf("t = %p\n", t);
		return ret;
	}

	printf("fd = %d, %s\n", fd, __func__);
	do {
		char buffer[BUFFER_LENGTH + 1];
		memset(buffer, 0, sizeof buffer);
		sprintf(buffer, "send");
		size_t length = strlen(buffer);
		ret = write(fd, buffer, length);
		if (ret < 0) {
			printf("%s(%d)\n", strerror(errno), errno);
			break;
		}
		printf("[%s]\n", buffer);
	} while (0);
	return ret;
}

void handler(int signum) {
	printf("Received signal %d\n", signum);

	if (signum == SIGINT || signum == SIGTERM) {
		is_quit = true;
	}

	switch (signum) {
		case SIGINT :
		case SIGTERM:
			is_quit = true;
			break;
		case SIGUSR1:
		case SIGUSR2:
			is_quit = true;
			break;
		default:
			printf("");
	}
	return ;
}

void set_disposition(void) {
	int arr[] = {SIGINT, SIGUSR1, SIGUSR2, SIGTERM};
	int i = 0;
	int signum = 0;

	for (i = 0; i < sizeof arr / sizeof (int); i++) {
		signum = arr[i];
		if (signal(signum, handler) == SIG_ERR) {
			printf("set the disposition of the signal(signum = %d) to handler.\n", signum);
		}
	}
	return ;
}

int init(int argc, char **argv) {
	int ret = 0;
	do {
		is_quit = false;
		set_disposition();

		listen_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (listen_sock < 0) {
			printf("%s(%d)\n", strerror(errno), errno);
			break;
		}

		printf("listen_sock = %d\n", listen_sock);

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof (struct sockaddr_in));

		addr.sin_family = AF_INET;

#ifdef D
		addr.sin_port = htons(atoi("12340"));
		// ret = inet_pton(AF_INET, "127.0.0.1", (struct sockaddr *) &addr.sin_addr.s_addr);
		ret = inet_pton(AF_INET, "192.168.56.101", (struct sockaddr *) &addr.sin_addr.s_addr);
#else
		addr.sin_port = htons(atoi(argv[2]));
		ret = inet_pton(AF_INET, argv[1], (struct sockaddr *) &addr.sin_addr.s_addr);
#endif

		if (ret != 1) {
			printf("%s(%d)\n", strerror(errno), errno);
			break;
		}

		socklen_t addrlen = sizeof (struct sockaddr_in);
		ret = bind(listen_sock, (struct sockaddr *) &addr, addrlen);
		if (ret == -1) {
			printf("%s(%d)\n", strerror(errno), errno);
			break;
		}

		int backlog = 5; /* may be wrong */
		ret = listen(listen_sock, backlog);
		if (ret == -1) {
			printf("%s(%d)\n", strerror(errno), errno);
			break;
		}

		epollfd = epoll_create(MAX_EVENTS);
		if (epollfd == -1) {
			printf("%s(%d)\n", strerror(errno), errno);
			break;
		}
		printf("epollfd = %d\n", epollfd);

		ev.events = EPOLLIN | EPOLLET; /* epoll edge triggered */
		ev.data.fd = listen_sock; /* bind & listen's fd */
		ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev);
		if (ret == -1) {
			printf("%s(%d)\n", strerror(errno), errno);
			break;
		}
	} while (0);
	return ret;
}

int uninit(std::map<int, Transport*> *m) {
	int ret = 0;
	do {
		if (listen_sock > 0) {
			printf("close listen_sock\n");
			ret = close(listen_sock);
			if (ret == -1) {
				printf("%s(%d)\n", strerror(errno), errno);
			}
		}

		if (epollfd > 0) {
			printf("close epollfd\n");
			ret = close(epollfd);
			if (ret == -1) {
				printf("%s(%d)\n", strerror(errno), errno);
			}
		}

		;;;;;;;;;;;;;;;;;;
		std::map<int, Transport*>::iterator i = m->begin();
		while (i != m->end()) {
			delete (*i).second;
			m->erase(i++);
		}
		;;;;;;;;;;;;;;;;;;

	} while (0);
	return ret;
}

int task(int argc, char **argv) {
	int ret = 0;
	std::queue<Transport*> *r = new std::queue<Transport*>();
	std::queue<Transport*> *w = new std::queue<Transport*>();
	std::map<int, Transport*> *m = new std::map<int, Transport*>();

	do {
		ret = init(argc, argv);
		if (ret == -1) {
			break;
		}

		while (true) {
			ret = task_r(r, m);
			ret = task_x(r, w, m);
			ret = task_w(w);
			if (is_quit) {
				break;
			}
		}
	} while (0);
	ret = uninit(m);
	return ret;
}

int task_r(std::queue<Transport*> *r, std::map<int, Transport*> *m) {
	int ret = 0;
	do {
		struct sockaddr_in peer_addr;
		socklen_t peer_addrlen = sizeof (struct sockaddr_in);
		memset(&peer_addr, 0, sizeof (struct sockaddr_in));

		nfds = epoll_wait(epollfd, events, MAX_EVENTS, 1000);
		if (nfds == -1) {
			printf("%s(%d)\n", strerror(errno), errno);
			break;
		}

		for (int n = 0; n < nfds; n++) {
			if (events[n].data.fd == listen_sock) {
				int acceptfd = accept(listen_sock, (struct sockaddr *) &peer_addr, &peer_addrlen);
				if (acceptfd == -1) {
					printf("%s(%d)\n", strerror(errno), errno);
					break;
				}

				printf("accept: acceptfd = %d\n", acceptfd);
				//set non blocking
				ret = setnonblocking(acceptfd);
				if (ret == -1) {
					break;
				}

				ev.events = EPOLLET | EPOLLIN | EPOLLRDHUP; /* edge triggered */
				// ev.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP; /* edge triggered */
				// ev.events = EPOLLIN | EPOLLOUT; //epoll level triggered (default)
				ev.data.fd = acceptfd;
				ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, acceptfd, &ev);
				if (ret == -1) {
					printf("%s(%d)\n", strerror(errno), errno);
					break;
				}

				;;;;;;;;;;;;;;;;;;;;
				printf("some one connect to me\n");
				;;;;;;;;;;;;;;;;;;;;

				Transport *t = new Transport(acceptfd, 32);
				(*m)[acceptfd] = t;

			} else {
				printf("events[%d].events = 0x%03x\n", n, events[n].events);
				if (events[n].events & EPOLLERR) {
					puts("Error condition happened on the associated file descriptor.  "
						"epoll_wait(2) will always wait for this event; it is not necessary to set it in events.");
					ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &events[n]);
					if (ret == -1) {
						printf("%s(%d)\n", strerror(errno), errno);
						break;
					}
					ret = close(events[n].data.fd);
					if (ret == -1) {
						printf("%s(%d)\n", strerror(errno), errno);
						break; //???
					}

					;;;;;;;;;;;
					delete (*m)[events[n].data.fd];
					m->erase(events[n].data.fd);
					;;;;;;;;;;;

					continue;
				}

				if (events[n].events & EPOLLHUP) {
					puts("Hang up happened on the associated file descriptor.");
					ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &events[n]);
					if (ret == -1) {
						printf("%s(%d)\n", strerror(errno), errno);
						break; //???
					}
					ret = close(events[n].data.fd);
					if (ret == -1) {
						printf("%s(%d)\n", strerror(errno), errno);
						break; //???
					}

					;;;;;;;;;;;
					delete (*m)[events[n].data.fd];
					m->erase(events[n].data.fd);
					;;;;;;;;;;;

					continue;
				}

				if (events[n].events & EPOLLRDHUP) {
					puts("EPOLLRDHUP");
					ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &events[n]);
					if (ret == -1) {
						printf("%s(%d)\n", strerror(errno), errno);
						break; //???
					}
					ret = close(events[n].data.fd);
					if (ret == -1) {
						printf("%s(%d)\n", strerror(errno), errno);
						break; //???
					}

					;;;;;;;;;;;
					delete (*m)[events[n].data.fd];
					m->erase(events[n].data.fd);
					;;;;;;;;;;;

					continue;
				}

				if (events[n].events & EPOLLIN) {
					puts("The associated file is available for read(2) operations.");
					if (m == NULL) {
						printf("m = %p\n", m);
						return ret;
					}
					Transport *t = (*m)[events[n].data.fd];
					ret = reads(t);
					if (ret < 0) {
						break;
					} else if (ret == 0) {
						;;;;;;;;;;;
					}

					/* Now, we need push to queue. */
					r->push(t);
				}

				if (events[n].events & EPOLLOUT) {
					puts("The associated file is available for write(2) operations.");
					if (m == NULL) {
						printf("m = %p\n", m);
						return ret;
					}
					Transport *t = (*m)[events[n].data.fd];
					ret = writes(t);
					if (ret < 0) {
						break;
					}
				}
			}

			;;;;;;;;;;;;;;;;;;;;
			printf("some data send to me\n");
			;;;;;;;;;;;;;;;;;;;;

		}

	} while (0);
	return ret;
}

int task_w(std::queue<Transport*> *w) {
	int ret = 0;
	if (w == NULL) {
		printf("w = %p\n", w);
		return ret;
	}

	;;;;;;;;;;;;;;;;;;;;
	/**
	 *  Returns true if the %queue is empty.
	 */
	while (!w->empty()) {
		printf("I should send data to some one.\n");
		Transport *t = w->front();
		printf("[OUT]: data = %p\n", t->get_data());
		t->print();

		w->pop();
		printf("I am writing data.\n");
	}
	;;;;;;;;;;;;;;;;;;;;

	return ret;
}

int task_x(std::queue<Transport*> *r, std::queue<Transport*> *w, std::map<int, Transport*> *m) {
	int ret = 0;
	if (r == NULL) {
		printf("r = %p, w = %p, m = %p\n", r, w, m);
		return ret;
	}

	;;;;;;;;;;;;;;;;;;;;
	/**
	 *  Returns true if the %queue is empty.
	 */
	while (!r->empty()) {
		printf("I should handle data, size = %d.\n", r->size());
		Transport *t = r->front();
		printf("[IN]: data = %p\n", t->get_data());
		t->print();

		r->pop();
		printf("I am handling data.\n");
	}
	;;;;;;;;;;;;;;;;;;;;

	return ret;
}
