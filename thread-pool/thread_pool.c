#include<stdio.h>
#include<pthread.h>
#include<string.h>
#include<stdlib.h>

#define LIST_INSERT(item,list) do {//宏定义链表操作插入 头插法
item->prev = NULL;
item->next = list;
if ((list) != NULL) (list)->prev = item;
(list) = item;
} while (0)

#define LIST_REMOVE(item,list) do{//双循环链表删除结点，注意防止断链
	if (item->prev != NULL) item->prev->next = item->next;
if (item->next != NULL) item->next->prev = item->prev;
if (list == item) list = item->next;
item->prev = item->next = NULL;
} while (0)

//线程池骨架  任务队列，执行队列，管理组件
	struct nTask { //任务队列
		void (*task_func)(struct nTask *task);//具体任务执行 取款，存款
		void *user_date;//执行任务所需要的参数

		//链表方式
		struct nTask* prev;
		struct nTask* next;
	};

struct nWorker { //执行队列
	pthread_t threadid;//线程id
	int terminate;//引入任务标识，判断任务是否执行完成
	struct nManager *manager;

	struct nWorker* prev;
	struct nWorker* next;
};

typedef struct nManager {
	struct nTask* tasks;//任务队列首结点 、第一个
	struct nWorker* workers;
	//互斥量，条件变量  保证有序执行
	pthread_mutex_t mutex;
	pthread_cond_t cond;//条件变量  作用：当营业厅没有客户时，工作人员休息
} ThreadPool;

//对外的接口 API
//！！！callback 不等于任务 callback是工作的流程，但是任务是处理其中的一种业务
static void *nThreadPoolCallBack(void *arg) {
	//判断任务队列是否有任务，如果有任务就执行，没有任务 等待任务到来
	struct nWorker *worker = (struct nWorker*) arg;//强制类型转换
	while (1) { //营业员一直在等待任务
		/*营业员工作
		    1、判断任务队列有没有任务,没有就一直等待
		    2、有任务就拿出来
		    3、执行任务
		*/
		pthread_mutex_lock(&worker->manager->mutex);
		while (worker->manager->tasks == NULL) { //营业厅里面没有任务
			if (worker->terminate)break;

			pthread_cond_wait(&worker->manager->cond, &worker->manager->mutex); //条件等待函数，处于挂起状态
		}
		if (worker->terminate) {
			pthread_mutex_unlock(&worker->manager->mutex);//注意解锁，不然会造成死锁
			break;
		}
		//有任务拿出一个来 tasks是任务队列的头指针，task是队列头结点
		struct nTask* task = worker->manager->tasks;
		LIST_REMOVE(task, worker->manager->tasks); //移除队列的任务
		pthread_mutex_unlock(&worker->manager->mutex);

		task->task_func(task);//开始执行任务
	}
	free(worker);
}

int nThreadPoolCreate(ThreadPool *pool, int numworkers) { //初始化线程池 员工数量
	//param 参数校验
	if (pool == NULL) return -1;
	if (numworkers < 1) numworkers = 1; //默认营业厅里面有员工
	memset(pool, 0, sizeof(ThreadPool));

	pthread_cond_t blank_cond = PTHREAD_COND_INITALIZER;//初始化条件变量
	memcpy(&pool->cond, &blank_cond, sizeof(pthread_cond_t));

	pthread_mutex_init(&pool->mutex, NULL);

	int i = 0;
	for (int i = 0; i < numworkers; i ++) { //创建任务
		struct nWorker *worker = (struct nWorker*)malloc(sizeof(struct nWorker));
		if (worker == NULL) {
			perror("malloc error");
			return -2;
		}
		memset(worker, 0, sizeof(struct nWorker)); //使用堆需要注意memset

		worker->manager = pool;

		int ret = pthread_create(&worker->threadid, NULL, nThreadPoolCallBack, worker); //每个柜员的工作 nTnThreadPoolCallBack(回调函数都是一样)，但是任务不一样，对象不同
		if (ret) { //创建成功返回0，失败返回非零
			perror("pthread_create");
			free(worker);
			return -3;
		}
		LIST_INSERT(worker, pool->workers);
	}

	//function
	//return sucess
	return 0;
}

int nThreadPoolDestroy(ThreadPool *pool, int nworker) { //销毁线程池
	struct nWorker *worker = NULL;
	for (worker = pool->workers; worker != NULL; worker = worker->next) {
		worker->terminate;//间接销毁
	}
	pthread_mutex_lock(&pool->mutex);//和条件等待的时候是一把锁
	pthread_cond_broadcast(&pool->cond);//条件广播 唤醒所有等待条件线程
	pthread_mutex_unlock(&pool->mutex);

	pool->tasks == NULL;
	pool->workers == NULL;
	return 0;

}

int nThreadPoolPushTask(ThreadPool *pool, struct nTask* task) { //往线程池添加任务
	//往任务队列里面添加一个任务
	//通知柜员有人来了
	pthread_mutex_lock(&pool->mutex);
	LIST_INSERT(task, pool->tasks);
	pthread_cond_signal(&pool->cond);//通知 唤醒一个等待线程  pthread_cond_broadcast(&pool->cond);//条件广播 唤醒所有等待条件线程
	pthread_mutex_unlock(&pool->mutex);
}
#if 1

#define THREADPOOL_INIT_COUNT 20
#define TASK_INIT_SIZE 1000
void task_entry(struct nTask* task) {
	//struct nTask *task = (struct nTask*)task;
	int idx = *(int *)task->user_date;
	printf("idx: %d\n", idx);
	free(task->user_date);
	free(task);

}

int main(void) {
	ThreadPool pool;
	nThreadPoolCreate(&pool, THREADPOOL_INIT_COUNT);
	printf("nThreadPoolCreate --- finish!\n");
	int i;
	for (int i = 0; i < TASK_INIT_SIZE; i ++) {
		struct nTask* task = (struct nTask*)malloc(sizeof(struct nTask));
		if (task == NULL) {
			perror("malloc");
			exit(1);
		}
		memset(task, 0, sizeof(struct nTask));
		task->task_func = task_entry;
		task->user_date = malloc(sizeof(int));
		*(int *)task->user_date = i;

		nThreadPoolPushTask(&pool, task);

	}
	getchar();
	return 0;
}
#endif

