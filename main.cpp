#include "server.hpp"
#include "RateLimiter.hpp"

int main(){
	// max 3 tokens, refill rate of 1 token per second
	RateLimiter limiter(1024, 3.0, 0.5);
	
	EpollServer server(8080, limiter);
	server.start();

	return 0;
}
