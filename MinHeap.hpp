#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

struct HeapNode {
	std::string ip;
	int64_t expire_time; //epoch timestamp (in ms) when this ip expires
};

class MinHeap {
	private:
		std::vector<HeapNode> heap;

		//move a node up to maintain minheap prop
		void siftup(size_t idx){
			while(idx>0){
				size_t parent = (idx-1)/2;
				if(heap[idx].expire_time<heap[parent].expire_time){
					std::swap(heap[idx], heap[parent]);
					idx = parent;
				}
				else{
					break;
				}
			}
		}

		//moving node down
		void siftdown(size_t idx){
			size_t size = heap.size();
			while(2*idx+1 < size){
				size_t left = 2*idx+1;
				size_t right = 2*idx+2;
				size_t smallest = idx;

				if(left < size && heap[left].expire_time < heap[smallest].expire_time){
					smallest = left;
				}
				if(right < size && heap[right].expire_time < heap[smallest].expire_time){
					smallest = right;
				}

				if(smallest != idx){
					std::swap(heap[idx], heap[smallest]);
					idx = smallest;
				}
				else{
					break;
				}
			}
		}
	
	public:
		//insert a new ip with expiration time (OlogN)
		void push(const std::string &ip, int64_t expire_time){
			heap.push_back({ip, expire_time});
			siftup(heap.size()-1);
		}

		//view element expiring the earliest
		HeapNode top() const{
			return heap.front();
		}

		//remove the top element - O(logN)
		void pop(){
			if(heap.empty()) return;
			heap[0] = heap.back();
			heap.pop_back();
			if(!heap.empty()){
				siftdown(0);
			}
		}

		bool empty() const{
			return heap.empty();
		}

		size_t size() const {
			return heap.size();
		}
};
