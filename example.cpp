#include <iostream>
#include <thread>
#include <chrono>
#include <functional>

#include "high_resolution_timer.h"
#include "hp-threadpool.hpp"


double time_comsuming_work(const int n){
	int nn = 1000;
	double c = 1;
	for (int i = 0; i < nn; ++i){
		if(i > 10000){
			c += i * n;
		}else{
			c -= i;
		}
		if (c > 100){
			c = 1;
		}else if(c < -100){
			c = -1;
		}
	}
	return c;
}

class RunningTask{
public:
	RunningTask(): beg_(0), end_(0), a_(NULL), sum(0) {}

	RunningTask(int beg, int end, int * a):
	beg_(beg), end_(end), a_(a), sum(0){};	
	
	void compute(int n){
		sum += time_comsuming_work(n);
	}

	void operator()(){
		for(int n = beg_; n < end_; ++n){
			sum += a_[n] * time_comsuming_work(n);
		}
	}
public:
	int beg_;
	int end_;
	int * a_;
	double sum;
};


void benchmark_test(){
	const int size = 300;
		int a[size];

		for (int i = 0; i < size; ++i) {
			a[i] = i % 2 + i % 3;
		}

		HighResolutionTimer timer;

		timer.start();
			RunningTask task1( 0, size/3, a), task2(size/3, (2*size)/3, a), task3((2*size)/3, size, a);
			std::vector<RunningTask*> tasks; 
				tasks.push_back(&task1);
				tasks.push_back(&task2);
				tasks.push_back(&task3);
			HPThreadPool::ThreadPool<RunningTask*> tp(3, tasks); 
			tp.start();

		double t0 = timer.microseconds_elapsed();
		
		double sum = 0, sum2=0;

		for(int k = 0; k < 30; ++k){
			timer.start();
			tp.restart_tasks();

//			tp.post_task(0, &task1);
//			tp.post_task(1, &task2);
//			tp.post_task(2, &task3);
			while( ! tp.is_task_done() );
			
			sum = task1.sum + task2.sum + task3.sum; 
			double t1 = timer.microseconds_elapsed();

			timer.start();
			for (int n = 0; n < size; ++n){
				sum2 += a[n] * time_comsuming_work(n);
			}
			double t2 = timer.microseconds_elapsed();

			std::cout << "\tt1=" << t1 << " t2=" << t2 << std::endl;
			std::this_thread::sleep_for( std::chrono::duration<double, std::milli>(100) );
		}
		
		
		std::cout << "\tConstruction Time Of ThreadPool=" << t0 
				<< "\n\tComputation sum1=" << sum <<" sum2="<< sum2 << std::endl;
		tp.stop();
}



void std_testing(){
	
	HPThreadPool::ThreadPool< std::function<void()> > tp(3); 
	tp.start();
	RunningTask task;

	for(int i = 0; i < 20; ++i){
		if( ! tp.post( std::bind(&RunningTask::compute, &task, i + 10 )) ){
			std::cout<<"\tall worker are busy, sleeping..."<<std::endl;
			std::this_thread::sleep_for( std::chrono::duration<double, std::milli>(100) );
		}
	}

	std::cout<<"\tComputation sum="<<task.sum<<std::endl;
	tp.stop();
}


int main()
{
	std::cout<<"standard Testing..."<<std::endl;
	std_testing();

	std::cout<<"Bencharmk Testing..."<<std::endl;
	benchmark_test();
	return 0;

}
