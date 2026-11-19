#include <string>
#include <vector>
#include <iostream>
#include <cstdint>

//data structure within hashmap for each user
struct TokenBucket{
	double tokens;		 //tokens left
	int64_t last_refreshed;	 //timestamp of last refill
};


//Node for separate chaining collision resolution

struct HashNode{
	std::string key;	//Client API key or IP
	TokenBucket value;	//Token bucket state
	HashNode *next;		//Pointer to next node

	HashNode(const std::string &k, const TokenBucket &val)
		:key(k), value(val), next(nullptr){}
};



//Implementing the custom hash map

class RateLimitHashMap{
	private:
		std::vector<HashNode*> buckets;
		size_t capacity;
		size_t size;

		//fast non-cryptographic hash function (FNV-1a)
		size_t hash(const std::string &str) const{
			uint32_t hash_val = 2166136261u;
			for(const char &c:str){
				hash_val ^= static_cast<uint32_t>(c);
				hash_val *= 16777619u;
			}
			return hash_val%capacity;
		}


	public:
		RateLimitHashMap(size_t cap = 1024) : capacity(cap), size(0) {
			buckets.resize(capacity, nullptr);
		}

		~RateLimitHashMap(){
			//clean up mem to prevent leaks
			for(size_t i=0; i<capacity; i++){
				HashNode *current = buckets[i];
				while(current != nullptr){
					HashNode *prev = current;
					current = current->next;
					delete prev;
				}
			}
		}
		
		//insert or update a token bucket for a key
		void put(const std::string &key, const TokenBucket &value){
			size_t index = hash(key);
			HashNode *current = buckets[index];

			//check if key already exists
			while(current !=nullptr){
				if(current->key == key){
					current->value = value;
					return;
				}
				current = current->next;
			}

			//inserting new node at head of bucket's linked list
			HashNode *newNode = new HashNode(key, value);
			newNode->next = buckets[index];
			buckets[index] = newNode;
			size++;
		}

		//retrieve a pointer to the tocken bucket (returns nullptr if not found)
		TokenBucket *get(const std::string &key){
			size_t index = hash(key);
			HashNode *curr = buckets[index];
			while(curr != nullptr){
				if(curr->key == key){
					return &(curr->value);
				}
				curr = curr->next;
			}
			return nullptr;
		}

		void remove(const std::string &key){
			size_t idx = hash(key);
			HashNode *current = buckets[idx];
			HashNode *prev = nullptr;
			while(current != nullptr){
				if(current->key == key){
					if(prev == nullptr){
						buckets[idx] = current->next;
					}
					else{
						prev->next = current->next;
					}
					delete current;
					size--;
					return;
				}
				prev = current;
				current = current->next;
			}
		}
};


