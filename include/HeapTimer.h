#ifndef HEAPTIMER_H
#define HEAPTIMER_H

#include<chrono>
#include<vector>
#include<unordered_map>
#include<algorithm>

class HeapTimer {
	public:
		using TimePoint = std::chrono::steady_clock::time_point;

		struct TimerNode {
			int fd;  //文件描述符，用于找到对应的连接
			TimePoint expireTime; //绝对过期时间点

			TimerNode(int fd = -1,TimePoint expire = TimePoint{})
				: fd(fd),expireTime(expire) {}
		};

		HeapTimer() = default;
		~HeapTimer() = default;

		//添加或刷新定时器
		//如果fd已存在，先删除旧节点，再插入新节点
		void addTimer(int fd,int timeoutMs) {
			if(fd < 0) return;

			//计算过期时间点：当前时间 + 超时阈值
			TimePoint expire = std::chrono::steady_clock::now()
				+ std::chrono::milliseconds(timeoutMs);

			//如果该fd已经在定时器中，先删除旧节点
			auto it = m_index.find(fd);
			if(it != m_index.end()) {
				size_t idx = it->second;

				//将待删除节点与堆尾交换，然后删除堆尾
				swapNode(idx,m_heap.size()-1);
				m_heap.pop_back();
				m_index.erase(fd);

				//如果交换上来的元素还需要调整，就进行上滤和下滤
				if(idx < m_heap.size()) {
					heapifyUp(idx);
					heapifyDown(idx);
				}
			}

			//插入新节点到堆尾
			m_heap.emplace_back(fd,expire);
			size_t newIdx = m_heap.size()-1;
			m_index[fd] = newIdx;

			//从新节点位置向上调整，维持小根堆性质
			heapifyUp(newIdx);
		}

		//刷新定时器：连接有活动时调用，重新计时
		void adjustTimer(int fd,int timeoutMs) {
			addTimer(fd,timeoutMs);
		}

		//删除定时器：连接关闭时调用
		void removeTimer(int fd) {
			auto it = m_index.find(fd);
			if(it == m_index.end()) return; //不存在则直接返回

			size_t idx = it->second;

			//将待删除节点与堆尾交换，然后删除堆尾
			swapNode(idx,m_heap.size()-1);
			m_heap.pop_back();
			m_index.erase(fd);

			//如果交换上来的元素还需要调整，就进行上滤和下滤
			if(idx < m_heap.size())
			{
				heapifyUp(idx);
				heapifyDown(idx);
			}
		}

		//获取距离下一个超时还有多少毫秒
		//返回-1表示当前没有任何定时器
		int getNextTimeout() const {
			if(m_heap.empty()) return -1;

			auto now = std::chrono::steady_clock::now();
			auto expire = m_heap.front().expireTime;

			//如果已经超时，返回0，让epoll_wait立即返回
			if(expire <= now) return 0;

			return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(expire - now).count());
		}

		// 检查并返回所有已超时的 fd
		// 调用后会从堆中移除这些超时节点
		std::vector<int> tick() {
			std::vector<int> expiredFds;
			auto now = std::chrono::steady_clock::now();

			// 只要堆顶过期，就不断弹出并记录
			while (!m_heap.empty() && m_heap.front().expireTime <= now) {
				int fd = m_heap.front().fd;
				expiredFds.push_back(fd);
				removeTimer(fd);   // 删除堆顶节点并调整堆
			}

			return expiredFds;
		}


	private:
		std::vector<TimerNode> m_heap; //底层存储数组
		std::unordered_map<int,size_t> m_index; //fd -> 堆数组索引

		//交换节点并更新哈希表映射
		void swapNode(size_t i,size_t j) {
			std::swap(m_heap[i],m_heap[j]);
			m_index[m_heap[i].fd] = i;
			m_index[m_heap[j].fd] = j;
		}

		//上滤操作：节点到期时间比父节点早，向上浮动
		void heapifyUp(size_t index) {
			while(index > 0) {
				size_t parent = (index-1)/2;
				if(m_heap[index].expireTime < m_heap[parent].expireTime) {
					swapNode(index,parent);
					index = parent;
				} else {
					break;
				}

			}

		}

		//下滤操作：节点到期时间比子节点晚，向下沉降
		void heapifyDown(size_t index) {
			size_t size = m_heap.size();
			while(true) {
				size_t left = 2 * index + 1;
				size_t right = 2 * index + 2;
				size_t smallest = index;

				if(left < size && m_heap[left].expireTime < m_heap[smallest].expireTime) {
					smallest = left;
				}
				if(right < size && m_heap[right].expireTime < m_heap[smallest].expireTime) {
					smallest = right;
				}

				if(smallest == index) break;

				swapNode(index,smallest);
				index = smallest;
			}
		}





};


#endif
