#pragma once
#include "RateLimiter.hpp"
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>


class EpollServer {
	private:
		int server_fd;
		int epoll_fd;
		int port;
		RateLimiter &limiter;
		const int MAX_EVENTS = 64;

		//helper to set a socket to non blocking mode
		bool setNonBlocking(int sock){
			int flags = fcntl(sock, F_GETFL, 0);
			if(flags == -1) return false;
			return fcntl(sock, F_SETFL, flags | O_NONBLOCK) == 0;
		}

	public:
		EpollServer(int p, RateLimiter &l) : port(p), limiter(l), server_fd(-1), epoll_fd(-1) {}

		~EpollServer(){
			if(server_fd != -1) close(server_fd);
			if(epoll_fd != -1) close(epoll_fd);
		}

		bool start(){
			//creating tcp socket
			server_fd = socket(AF_INET, SOCK_STREAM, 0);
			if(server_fd == -1){
				std::cerr << "Error : failed to create socket" << std::endl;
				return false;
			}

			//allow immediate reuse of port
			int opt = 1;
			setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			
			//bind socket to port
			sockaddr_in address{};
			address.sin_family = AF_INET;
			address.sin_addr.s_addr = INADDR_ANY;
			address.sin_port = htons(port);

			if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
				std::cerr << "Error: failed to bind port" << std::endl;
				return false;
			}

			//listen for connections
			if(listen(server_fd, SOMAXCONN) <0){
				std::cerr << "Error : failed to listen on socket" << std::endl;
				return false;
			}

			setNonBlocking(server_fd);

			//creating epoll instance
			epoll_fd = epoll_create1(0);
			if(epoll_fd == -1){
				std::cerr << "Error : failed to create epoll instance " << std::endl;
				return false;
			}

			//add server socket to epoll watchlist
			epoll_event ev{};
			ev.events = EPOLLIN | EPOLLET;
			ev.data.fd = server_fd;
			if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1){
				std::cerr << "Error : failed to add server socket to epoll watchlist" << std::endl;
				return false;
			}

			std::cout << "[Systems Daemon] Rate Limiter gateway listening on port " << port << "..." << std::endl;

			//the event loop
			std::vector<epoll_event> events(MAX_EVENTS);
			while(true){
				//periodically cleaning up stale heap mem
				limiter.cleanupStaleUsers();

				int nfds = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, 1000);
				if(nfds < 0){
					break;
				}

				for(int i=0; i<nfds; i++){
					if(events[i].data.fd == server_fd){
						//new incoming connections
						while(true){
							sockaddr_in client_addr{};
							socklen_t client_len = sizeof(client_addr);
							int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

							if(client_fd < 0){
								if(errno == EWOULDBLOCK || errno == EAGAIN){
									break;
								}
								break;
							}
							//extract client IP addr string
							char ip_str[INET_ADDRSTRLEN];
							inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
							std::string client_ip(ip_str);

							setNonBlocking(client_fd);

							epoll_event client_ev{};
							client_ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
							client_ev.data.fd = client_fd;
							epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev);
						}
					}
					else{
						//data arrived from an existing client socket
						int client_fd = events[i].data.fd;
						char buffer[1024];
						memset(buffer, 0, sizeof(buffer));

						ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer)-1);
						if(bytes_read <= 0){
							//client disconnected or error
							epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
							close(client_fd);
						}
						else{
							//extract ip for this client
							sockaddr_in peer_addr{};
							socklen_t peer_len = sizeof(peer_addr);
							getpeername(client_fd, (struct sockaddr *)&peer_addr, &peer_len);
							char ip_str[INET_ADDRSTRLEN];
							inet_ntop(AF_INET, &(peer_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
							std::string client_ip(ip_str);

							//run through our rate limiter engine
							bool allowed = limiter.allowRequest(client_ip);

							std::string response;
							if(allowed){
								response = "HTTP/1.1 200 OK\r\nContent-Length: 13\rnConnection: close\r\n\r\nRequest";
							}
							else{
								response = "HTTP/1.1 429 too many requests";
							}
							write(client_fd, response.c_str(), response.size());
							close(client_fd);	//close connection after response
						}
					}
				}
			}
			return true;
		}
};
