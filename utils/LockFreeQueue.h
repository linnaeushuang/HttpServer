#ifndef __LOCK_FREE_QUEUE_HEAD
#define __LOCK_FREE_QUEUE_HEAD

#include <atomic>
#include <cstddef>


template<typename T>
class LockFreeQueue{
private:
	struct Node{
		T data;
		std::atomic<size_t> tail;
		std::atomic<size_t> head;
	};
	size_t capacityMask_;
	Node *queue_;
	size_t capacity_;
	std::atomic<size_t> tail_;
	std::atomic<size_t> head_;
public:
	LockFreeQueue(size_t capacity=64){
		capacityMask_ = capacity -1;
		for(size_t i=1;i<=sizeof(void*)*4;i<<=1){
			capacityMask_ |= capacityMask_>>i;
		}
		capacity_ = capacityMask_  + 1;
		queue_ = (Node*)new char[sizeof(Node)*capacity_];
		for(size_t i = 0;i<capacity_;i++){
			queue_[i].tail.store(i,std::memory_order_relaxed);
			queue_[i].head.store(-1,std::memory_order_relaxed);
		}
		tail_.store(0,std::memory_order_relaxed);
		head_.store(0,std::memory_order_relaxed);
		
	}
	~LockFreeQueue(){
		for(size_t i = head_;i!=tail_;i++){
			(&queue_[i&capacityMask_].data)->~T();
			delete [] (char*) queue_;
		}
	}

	size_t capacity() const {return capacity_;}

	size_t size() const{
		size_t head = head_.load(std::memory_order_relaxed);
		return tail_.load(std::memory_order_relaxed) - head;
	}
	bool push(const T &data){
		Node* node;
		size_t tail = tail_.load(std::memory_order_relaxed);
		for(;;){
			node = &queue_[tail&capacityMask_];
			if(node->tail.load(std::memory_order_relaxed) != tail)
				return false;
			if((tail_.compare_exchange_weak(tail,tail+1,std::memory_order_relaxed)))
				break;
		}

		new (&node->data)T(data);
		node->head.store(tail,std::memory_order_relaxed);
		return true;

	}

	bool push(T &&data) noexcept{
		Node* node;
		size_t tail = tail_.load(std::memory_order_relaxed);
		for(;;){
			node = &queue_[tail&capacityMask_];
			if(node->tail.load(std::memory_order_relaxed) != tail)
				return false;
			if((tail_.compare_exchange_weak(tail,tail+1,std::memory_order_relaxed)))
				break;
		}

		node->data = std::move(data);
		node->head.store(tail,std::memory_order_relaxed);
		return true;

	}

	bool pop(T& result){
		Node* node;
		size_t head = head_.load(std::memory_order_relaxed);
		for(;;){
			node = &queue_[head&capacityMask_];
			if(node->head.load(std::memory_order_relaxed) != head)
				return false;
			if(head_.compare_exchange_weak(head,head+1,std::memory_order_relaxed))
				break;
		}
		result = std::move(node->data);
		node->tail.store(head+capacity_,std::memory_order_relaxed);
		return true;
	}
};

#endif
