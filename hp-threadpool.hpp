#ifndef HIGH_PERFORMANCE_THREAD_POOL_HPP_20181005
#define HIGH_PERFORMANCE_THREAD_POOL_HPP_20181005

#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <type_traits>



/// @brief high-performance-thread-pool(HPThreadPool)
/// We have specific application (high-frequency trading system) in mind for designing the HPThreadPool

namespace HPThreadPool{

	/// @brief class SpinMutex; see https://en.wikipedia.org/wiki/Spinlock and https://en.cppreference.com/w/cpp/atomic/atomic_flag
	/// (pro) SpinMutex is faster than std::mutex for saving wakeup time from thread
	/// (con) SpinMutex keeps cpu busy and needs more energy
	class SpinLockMutex {
		public:
			SpinLockMutex(){}
			inline bool try_lock(){
				return false == locked_.test_and_set(std::memory_order_acquire);
			}
			inline void lock() {
				while (locked_.test_and_set(std::memory_order_acquire));
			}
			inline void unlock() {
				locked_.clear(std::memory_order_release);
			}
		protected:
			std::atomic_flag locked_;
		private:
			SpinLockMutex(SpinLockMutex&&) = delete;
			SpinLockMutex(SpinLockMutex const&) = delete;
			SpinLockMutex& operator=(SpinLockMutex&&) = delete;
			SpinLockMutex& operator=(SpinLockMutex const&) = delete;
	};
		
	class WorkerBase{
		public:
			WorkerBase(): 
		///		mutex_(), 
				stop_(true), task_done_(true), t_() {	};

			inline void join(){ if(t_.joinable()) t_.join(); }
			inline void stop(){		stop_.store(true, std::memory_order_relaxed); }
			inline bool is_stopped(){ return stop_.load(std::memory_order_relaxed); }
			inline bool is_task_done(){	return task_done_.load(std::memory_order_relaxed); }
		protected:	
		///	SpinLockMutex mutex_;
			std::atomic<bool> stop_;
			std::atomic<bool> task_done_;
			std::thread t_;
		private:
			WorkerBase(WorkerBase&&) = delete;
			WorkerBase(WorkerBase const&) = delete;
			WorkerBase& operator=(WorkerBase&&) = delete;
			WorkerBase& operator=(WorkerBase const&) = delete;
	};

	/// @brief template Task should support method Task(); internally, we will call (task()); 
	template<typename Task>
	class Worker : public WorkerBase {
		public:
			Worker(): WorkerBase(), task_() {};
			Worker(const Task & task): WorkerBase(), task_(task) {};

			void start(){
				if(is_stopped()){
					stop_.store(false);
					t_ = std::thread(std::bind(&Worker::_run, this));
				}
			}

			/// @brief please make sure task_ is a valid task
			void restart_task(){
				task_done_.store(false, std::memory_order_relaxed);
			}

			/// @brief: return true if task is posted successfully
			bool post(const Task & task){
				if(task_done_.load(std::memory_order_acquire)){ 
		/// I do not think we need a mutex here and in protected method _run, and in my tests, all is running ok.
		/// Please do not hesitate to contact me if you have a second mind.
		///			mutex_.lock();
						task_ = task;
		///			mutex_.unlock();
					task_done_.store(false);
					return true;
				}
				return false;		
			}
		protected:
			/// @implementation in mind: we always keep the thread in busy state, when a new task is posted, the thread can execute the task immediately
			///                          our main application is for high-frequency trading system and time is always in top priority consideration
			void _run(){
				while(true){
					if(!task_done_.load(std::memory_order_acquire)){ 
		///				mutex_.lock();
						   _do_task(std::integral_constant<bool, std::is_pointer<Task>::value >());
		///				mutex_.unlock();
						task_done_.store(true);					
					}
					if(stop_.load(std::memory_order_relaxed)) break;
				}
			}

		private:
			Worker(Worker&&) = delete;
			Worker(Worker const&) = delete;
			Worker& operator=(Worker&&) = delete;
			Worker& operator=(Worker const&) = delete;
		protected:
			Task task_;
		private:
			inline void _do_task(const std::integral_constant<bool, true> &){
				(*task_)();
			}
			inline void _do_task(const std::integral_constant<bool, false> &){
				task_();
			}

	};

	template<typename Task>
	class ThreadPool{
		public:
			ThreadPool(int thread_size)
			: thread_size_(thread_size), workers_() {
				for(int i = 0; i < thread_size; ++i){
					workers_.emplace_back(new Worker<Task>());
				}
			}
			ThreadPool(int thread_size, const std::vector<Task> & tasks)
			: thread_size_(thread_size), workers_() {
				for(int i = 0; i < thread_size; ++i){
					workers_.emplace_back(new Worker<Task>(tasks[i]));
				}
			}

			~ThreadPool(){
				for(int i = 0; i < thread_size_; ++i){
					delete workers_[i];
				}
				
			}
		public:
			/// @brief standard threadpool method post
			///        return true if task is posted successfully
			bool post(const Task & task){
				bool posted = false;
				for(Worker<Task>* & worker : workers_){
					if(worker->is_task_done()){
						worker->post(task);
						posted = true;
						break;
					}
				}
				return posted;
			}

			/// @note:Developer should be carefull when using the following two methods since there is some assumption when calling them; see details below.

			/// @implementation in mind: when developer call post_task, 
			///                          developer should make sure the corresponding worker is in idle
			void post_task(int ind, const Task & task){
				workers_[ind]->post(task);
			}

			/// @implementation in mind: when developer call post_tasks, 
			///                          developer should make sure the all workers are in idle and number of tasks equals thread_size
			void post_tasks(const std::vector<Task> & tasks){
				for(int i = 0; i < thread_size_; ++i){
					workers_[i]->post(tasks[i]);
				}
			}
			/// @implementation in mind: when developer call restart_tasks, 
			///                          developer should make sure the all workers are in idle and already assigned task (either by post or constructor)
			void restart_tasks(){
				for(int i = 0; i < thread_size_; ++i){
					workers_[i]->restart_task();
				}
			}
		public:
			Worker<Task>* get_worker(int ind){
				return workers_[ind];
			}
			void start(){
				for(Worker<Task>* & worker : workers_){
					worker->start();
				}
			}
			void stop(){
				stop_only(); joinall();
			}
			void restart(){
				stop(); start();
			}
			void stop_only(){
				for(Worker<Task>* & worker : workers_){
					worker->stop();
				}
			}
			void joinall(){
				for(Worker<Task>* & worker : workers_){
					worker->join();
				}
			}
			bool is_task_done(){
				bool is_done = true;
				for(Worker<Task>* & worker : workers_){
					is_done &= worker->is_task_done();
				}
				return is_done;
			}
		protected:
			const int thread_size_;
			std::vector< Worker<Task>* > workers_;
	};
}

#endif