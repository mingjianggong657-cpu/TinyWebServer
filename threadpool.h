#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<vector>
#include<queue>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<cassert>

class ThreadPool
{
	public:
             //构造函数：初始化并启动指定数量的工作线程
	     explicit ThreadPool(size_t threadCount = 8) : m_isClosed(false)
             {
		     assert(threadCount > 0);
		     for(size_t i =0;i < threadCount; ++i)
		     {
			     m_threads.emplace_back([this]
			     {
                               while(true)
			       {
			        std::function<void()> task;
                                //临界区开始
				{
				 std::unique_lock<std::mutex> lock(m_mutex);
                                 
				 //条件变量等待：有任务或线程池关闭时被唤醒
				 m_cond.wait(lock,[this]{
						 return m_isClosed || !m_tasks.empty();
						 });

				 //线程池已关闭且无剩余任务->退出线程
				 if(m_isClosed && m_tasks.empty()){
				    return;
				 }

				 //取出队首任务
				 task = std::move(m_tasks.front());
				 m_tasks.pop();
				}
                                //临界区结束，自动解锁

				//锁外执行任务，最大化并发性能
				task();
			       }

                               });
		     }


	     }

	     //析构函数：优雅关闭线程池，保证所有任务执行完毕
	     ~ThreadPool(){
		     //设置关闭标志
		     {
			     std::lock_guard<std::mutex> lock(m_mutex);
			     m_isClosed = true;
		     }

		     m_cond.notify_all(); //唤醒所有休眠线程
					  
                     //等待所有工作线程执行完毕再退出
		     for(std::thread& thread : m_threads){
                       if(thread.joinable()){
			       thread.join();
		       }
		     }			     
	     }

	     //添加任务：支持任意可调用对象（函数/Lambda/仿函数等）
	     template<typename F>
             void addTask(F& task){
               {
		       std::lock_guard<std::mutex> lock(m_mutex);
		       if(m_isClosed) return;
		       m_tasks.emplace(std::forward<F>(task));
	       }
               m_cond.notify_one(); //唤醒一个工作线程处理任务

	     }




	private:
		std::vector<std::thread> m_threads; //工作线程数组
                std::queue<std::function<void()>> m_tasks; //任务队列
                std::mutex m_mutex; //互斥锁							   
                std::condition_variable m_cond; //条件变量
                bool m_isClosed; //线程池关闭标志
};

#endif
