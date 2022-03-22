//理解并发和多线程，模拟买火车票
#include<stdio.h>
#include<pthread.h>

#define THREAD_COUNT 10 //创建十个进程
pthread_mutex_t mutex;//互斥量
pthread_spinlock_t spinlock;//自旋锁

int inc(int *value,int add){
    int old;
    //汇编指令
    _asm_volatile(
        "lock;xadd1 %2,%1;" //%2,%1第二个参数加上第一个参数
        : "=a" (old);
        : "m" (*value),"a"(add);//value + 1 -> value
        : "cc","memory"
    );
    return old;
}

void *thread_callback(void *arg){
    int *pcount = (int *)arg;//类型强转
    int i = 0;
    while(i ++ < 100000){
#if 0     
        //不加锁，数据共享会造成混乱      (*pcount) ++；是原子操作
        (*pcount) ++;
#elif 0 
        //互斥锁 引起线程切换，释放资源   适合锁的内容比较多，比如查核，线程安全的红黑树rbtree
        pthread_mutex_lock(&mutex);
        (*pcount) ++;//需要加互斥锁 其他线程进不来 是一个原子操作
        pthread_mutex_unlock(&mutex);
#elif 0  
    //自旋锁的使用同互斥量mutex
    pthread_spin_lock(&spinlock);//while(1)  锁的内容很少，代价小于mutex进程切换
     (*pcount) ++;
    pthread_spin_unlock(&spinlock);
    
#else //原子操作 单条cpu指令实现
    inc(pcount,1);
#endif
        usleep(1);
    }
}
int main(){
    pthread_t threadid[THREAD_COUNT] = {0};

    pthread_mutex_init(&mutex,NULL);//在线程创建之前初始化互斥量,NUll为默认
    pthread_spin_init(&spinlock,PTHREAD_PROCESS_SHARED);//PTHREAD_PROCESS_SHARED自旋锁线程之间共享
    int i =0;
    int count = 0;
    for(i =0 ;i < THREAD_COUNT; i ++) {//创建十个进程 每个进程写入到count
        pthread_create(&threadid[i],NULL, thread_callback,&count);//thread_calback是回调函数，count作为参数 传地址void *arg
    }
    for(i = 0;i < 100; i++){
        printf("count : %d\n",count);
        sleep(1);
    }
    return 0;
}
