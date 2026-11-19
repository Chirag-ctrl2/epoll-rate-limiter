#pragma once
#include "hashmap.hpp"
#include "MinHeap.hpp"
#include <chrono>
#include <algorithm>

class RateLimiter{
	private:
		RateLimitHashMap map;
		double max_tokens;
		double refill_rate;
		MinHeap min_heap;
		int64_t ttl_duration_ms = 6000;
		int64_t getCurrentTimestamp() const{
			auto now = std::chrono::steady_clock::now();
			return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		}
		


	public:
		RateLimiter(size_t map_capacity, double max_tok, double refill_r)
			: map(map_capacity), max_tokens(max_tok), refill_rate(refill_r){}

		bool allowRequest(const std::string &client_ip){
			int64_t now = getCurrentTimestamp();
			
			int64_t ttl_expiration = now+ttl_duration_ms;	//6 sec ttl
			min_heap.push(client_ip, ttl_expiration);

			TokenBucket *bucket = map.get(client_ip);

			if(bucket==nullptr){
				TokenBucket new_bucket;
				new_bucket.tokens = max_tokens-1.0;
				new_bucket.last_refreshed = now;
				map.put(client_ip, new_bucket);
				return true;
			}
			int64_t elapsed_ms = now - bucket->last_refreshed;
			double elapsed_sec = static_cast<double>(elapsed_ms)/1000.0;

			double tokens_to_add = elapsed_sec*refill_rate;
			bucket->tokens = std::min(max_tokens, bucket->tokens+tokens_to_add);
			bucket->last_refreshed = now;

			if(bucket->tokens >= 1.0){
				bucket->tokens -= 1.0;
				return true;
			}

			return false;
		}

		void cleanupStaleUsers(){
			int64_t now = getCurrentTimestamp();
			while(!min_heap.empty()){
				HeapNode oldest = min_heap.top();
				TokenBucket *bucket = map.get(oldest.ip);

				//case a: user not exist anymore
				if(bucket == nullptr || (bucket->last_refreshed + ttl_duration_ms) > now){
					min_heap.pop();
				}
				//case b: truly expired
				else if(oldest.expire_time <= now){
					map.remove(oldest.ip);
					min_heap.pop();
				}
				else{
					break;
				}
			}
		}
		
};
