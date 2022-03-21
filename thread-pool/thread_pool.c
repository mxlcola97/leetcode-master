#include<stdio.h>
#include<pthread.h>
#include<string.h>
#include<stdlib.h>

#define LIST_INSERT(item,list) do {//�궨������������� ͷ�巨
item->prev = NULL;
item->next = list;
if ((list) != NULL) (list)->prev = item;
(list) = item;
} while (0)

#define LIST_REMOVE(item,list) do{//˫ѭ������ɾ����㣬ע���ֹ����
	if (item->prev != NULL) item->prev->next = item->next;
if (item->next != NULL) item->next->prev = item->prev;
if (list == item) list = item->next;
item->prev = item->next = NULL;
} while (0)

//�̳߳عǼ�  ������У�ִ�ж��У��������
	struct nTask { //�������
		void (*task_func)(struct nTask *task);//��������ִ�� ȡ����
		void *user_date;//ִ����������Ҫ�Ĳ���

		//����ʽ
		struct nTask* prev;
		struct nTask* next;
	};

struct nWorker { //ִ�ж���
	pthread_t threadid;//�߳�id
	int terminate;//���������ʶ���ж������Ƿ�ִ�����
	struct nManager *manager;

	struct nWorker* prev;
	struct nWorker* next;
};

typedef struct nManager {
	struct nTask* tasks;//��������׽�� ����һ��
	struct nWorker* workers;
	//����������������  ��֤����ִ��
	pthread_mutex_t mutex;
	pthread_cond_t cond;//��������  ���ã���Ӫҵ��û�пͻ�ʱ��������Ա��Ϣ
} ThreadPool;

//����Ľӿ� API
//������callback ���������� callback�ǹ��������̣����������Ǵ������е�һ��ҵ��
static void *nThreadPoolCallBack(void *arg) {
	//�ж���������Ƿ�����������������ִ�У�û������ �ȴ�������
	struct nWorker *worker = (struct nWorker*) arg;//ǿ������ת��
	while (1) { //ӪҵԱһֱ�ڵȴ�����
		/*ӪҵԱ����
		    1���ж����������û������,û�о�һֱ�ȴ�
		    2����������ó���
		    3��ִ������
		*/
		pthread_mutex_lock(&worker->manager->mutex);
		while (worker->manager->tasks == NULL) { //Ӫҵ������û������
			if (worker->terminate)break;

			pthread_cond_wait(&worker->manager->cond, &worker->manager->mutex); //�����ȴ����������ڹ���״̬
		}
		if (worker->terminate) {
			pthread_mutex_unlock(&worker->manager->mutex);//ע���������Ȼ���������
			break;
		}
		//�������ó�һ���� tasks��������е�ͷָ�룬task�Ƕ���ͷ���
		struct nTask* task = worker->manager->tasks;
		LIST_REMOVE(task, worker->manager->tasks); //�Ƴ����е�����
		pthread_mutex_unlock(&worker->manager->mutex);

		task->task_func(task);//��ʼִ������
	}
	free(worker);
}

int nThreadPoolCreate(ThreadPool *pool, int numworkers) { //��ʼ���̳߳� Ա������
	//param ����У��
	if (pool == NULL) return -1;
	if (numworkers < 1) numworkers = 1; //Ĭ��Ӫҵ��������Ա��
	memset(pool, 0, sizeof(ThreadPool));

	pthread_cond_t blank_cond = PTHREAD_COND_INITALIZER;//��ʼ����������
	memcpy(&pool->cond, &blank_cond, sizeof(pthread_cond_t));

	pthread_mutex_init(&pool->mutex, NULL);

	int i = 0;
	for (int i = 0; i < numworkers; i ++) { //��������
		struct nWorker *worker = (struct nWorker*)malloc(sizeof(struct nWorker));
		if (worker == NULL) {
			perror("malloc error");
			return -2;
		}
		memset(worker, 0, sizeof(struct nWorker)); //ʹ�ö���Ҫע��memset

		worker->manager = pool;

		int ret = pthread_create(&worker->threadid, NULL, nThreadPoolCallBack, worker); //ÿ����Ա�Ĺ��� nTnThreadPoolCallBack(�ص���������һ��)����������һ��������ͬ
		if (ret) { //�����ɹ�����0��ʧ�ܷ��ط���
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

int nThreadPoolDestroy(ThreadPool *pool, int nworker) { //�����̳߳�
	struct nWorker *worker = NULL;
	for (worker = pool->workers; worker != NULL; worker = worker->next) {
		worker->terminate;//�������
	}
	pthread_mutex_lock(&pool->mutex);//�������ȴ���ʱ����һ����
	pthread_cond_broadcast(&pool->cond);//�����㲥 �������еȴ������߳�
	pthread_mutex_unlock(&pool->mutex);

	pool->tasks == NULL;
	pool->workers == NULL;
	return 0;

}

int nThreadPoolPushTask(ThreadPool *pool, struct nTask* task) { //���̳߳��������
	//����������������һ������
	//֪ͨ��Ա��������
	pthread_mutex_lock(&pool->mutex);
	LIST_INSERT(task, pool->tasks);
	pthread_cond_signal(&pool->cond);//֪ͨ ����һ���ȴ��߳�  pthread_cond_broadcast(&pool->cond);//�����㲥 �������еȴ������߳�
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

