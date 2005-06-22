#ifndef THREADPOOL
#define THREADPOOL

#include <queue>
#include<semaphore.h>
#include <pthread.h>
#include <common/Mutex.h>

using namespace std;
 
#define MAX_THREADS 1000

class Semaphore {
  sem_t sem;

 public:

  void init(int i) {
    sem_init(&sem, 0, i);
  }

  void get() {
    sem_wait(&sem);
  }

  void put() {
    sem_post(&sem);
  }
};

template <class T>
class ThreadPool {

 private:
  queue<T *> q;
  Mutex q_lock;
  Semaphore q_sem;
  pthread_t thread[MAX_THREADS];
  int num_ops;
  int num_threads;
  void (*func)(T*);

  static void *foo(void *arg) {
    ThreadPool *t = (ThreadPool *)arg;
    t->do_ops(arg);
  }

  void * do_ops(void *nothing) {
    T* op;

    cout << "Thread ready for action\n";
    while(1) {
      q_sem.get();
      op = get_op();

      if(op == NULL) {
	cout << "Thread exiting\n";
	pthread_exit(0);
      }
      func(op);
    }
  }


  T* get_op() {
    T* op;

    q_lock.Lock();
    op = q.front();
    q.pop();
    num_ops--;
    q_lock.Unlock();

    return op;
  }

 public:
  ThreadPool(int howmany, void (*f)(T*)) {
    num_ops = 0;
    num_threads = 0;

    int status;

    func = f;

    num_threads = howmany;

    q_sem.init(0);

    for(int i = 0; i < howmany; i++) {
      status = pthread_create(&thread[i], NULL, (void*(*)(void *))&ThreadPool::foo, this);
    }
  }

  ~ThreadPool() {
    q_lock.Lock();
    for(int i = num_ops; i > 0; i--) 
      get_op();

    for(int i = 0; i < num_threads; i++) {
      put_op((T*)NULL);
    }
    q_lock.Unlock();

    for(int i = 0; i < num_threads; i++) {
      cout << "Waiting for thread " << i << " to die\n";
      pthread_join(thread[i], NULL);
    }

  }

  void put_op(T* op) {
    q_lock.Lock();
    q.push(op);
    num_ops++;
    q_sem.put();
    q_lock.Unlock();
  }

};
#endif
