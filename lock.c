//��Ⲣ���Ͷ��̣߳�ģ�����Ʊ
#include<stdio.h>
#include<pthread.h>

#define THREAD_COUNT 10 //����ʮ������
pthread_mutex_t mutex;//������
pthread_spinlock_t spinlock;//������

int inc(int *value,int add){
    int old;
    //���ָ��
    _asm_volatile(
        "lock;xadd1 %2,%1;" //%2,%1�ڶ����������ϵ�һ������
        : "=a" (old);
        : "m" (*value),"a"(add);//value + 1 -> value
        : "cc","memory"
    );
    return old;
}

void *thread_callback(void *arg){
    int *pcount = (int *)arg;//����ǿת
    int i = 0;
    while(i ++ < 100000){
#if 0     
        //�����������ݹ������ɻ���      (*pcount) ++����ԭ�Ӳ���
        (*pcount) ++;
#elif 0 
        //������ �����߳��л����ͷ���Դ   �ʺ��������ݱȽ϶࣬�����ˣ��̰߳�ȫ�ĺ����rbtree
        pthread_mutex_lock(&mutex);
        (*pcount) ++;//��Ҫ�ӻ����� �����߳̽����� ��һ��ԭ�Ӳ���
        pthread_mutex_unlock(&mutex);
#elif 0  
    //��������ʹ��ͬ������mutex
    pthread_spin_lock(&spinlock);//while(1)  �������ݺ��٣�����С��mutex�����л�
     (*pcount) ++;
    pthread_spin_unlock(&spinlock);
    
#else //ԭ�Ӳ��� ����cpuָ��ʵ��
    inc(pcount,1);
#endif
        usleep(1);
    }
}
int main(){
    pthread_t threadid[THREAD_COUNT] = {0};

    pthread_mutex_init(&mutex,NULL);//���̴߳���֮ǰ��ʼ��������,NUllΪĬ��
    pthread_spin_init(&spinlock,PTHREAD_PROCESS_SHARED);//PTHREAD_PROCESS_SHARED�������߳�֮�乲��
    int i =0;
    int count = 0;
    for(i =0 ;i < THREAD_COUNT; i ++) {//����ʮ������ ÿ������д�뵽count
        pthread_create(&threadid[i],NULL, thread_callback,&count);//thread_calback�ǻص�������count��Ϊ���� ����ַvoid *arg
    }
    for(i = 0;i < 100; i++){
        printf("count : %d\n",count);
        sleep(1);
    }
    return 0;
}
