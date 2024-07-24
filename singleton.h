//
// Created by zhangshiping on 24-5-28.
//

#ifndef GAMETOOLS_SINGLETON_H
#define GAMETOOLS_SINGLETON_H

#include <atomic>
#include <mutex>

namespace GameTools{

    ///懒汉模式创建单例
    template<typename ClassType>
    class Singleton{
    private:
        Singleton()=delete;///禁用默认构造函数 ,GameTools::Singleton<MyClass> instance;
        Singleton(const Singleton&)=delete;///禁用拷贝构造函数,GameTools::Singleton<MyClass> instance2=instance1;
        Singleton(Singleton&&)=delete;///禁用移动构造函数,GameTools::Singleton<MyClass> instance2=std::move(instance1);
        Singleton<ClassType>& operator=(const Singleton&)=delete;///禁用拷贝赋值操作符,GameTools::Singleton<MyClass> instance2;instance2=instance1;

        static std::mutex lock_;
        static std::atomic<ClassType*> instance_;
    public:
        template<typename ...Args>
        static ClassType* Instance(Args&& ...args){///参数包展开,&&表示右值引用
            ClassType* ins=instance_.load(std::memory_order_acquire);
            ///std::memory_order_acquire 确保多线程环境中，此读操作此之前的所有内存写操作都已经完成，读正确
            if(ins== nullptr){
                std::lock_guard<std::mutex> lock(lock_);
                ins=instance_.load(std::memory_order_relaxed);///load是因为第一次load没加锁
                ///std::memory_order_relaxed 不保证任何内存顺序，只保证原子操作的顺序，因为此时已经加锁，所以不需要保证内存顺序
                if(ins== nullptr){
                    ins=new ClassType(std::forward<Args>(args)...);///forward是完美转发,保持参数的左值或右值特性
                    instance_.store(ins,std::memory_order_release);
                    ///std::memory_order_release 确保多线程环境中，此写操作此之后的所有内存读操作
                }
            }
            return ins;
        }
        /** 双重检查锁定模式（Double-Checked Locking）是一个常见的用于确保线程安全和高效的单例模式实现方式。
         * 第一次检查是在无锁的情况下进行的，以减少锁的开销，而第二次检查是在锁定的情况下进行的，以确保只有一个实例被创建。
         * 这种方法结合了性能和正确性，在多线程环境中非常有效。
         **/

        static void destroy(){
            ClassType* ins=instance_.load(std::memory_order_acquire);
            if(ins!= nullptr){
                std::lock_guard<std::mutex> lock(lock_);
                ins=instance_.load(std::memory_order_acquire);
                if(ins!= nullptr){
                    delete ins;
                    instance_.store(nullptr,std::memory_order_release);
                }
            }
        }
    };

    template<typename T>
    std::mutex Singleton<T>::lock_;

    template<typename T>
    std::atomic<T*> Singleton<T>::instance_{nullptr};
    /**在 Singleton 类的声明之外定义 lock_ 和 instance_ 是因为它们是静态成员变量。
     * 静态成员变量在类的所有实例之间共享，并且需要在类定义外进行定义和初始化。这是 C++ 的语法要求。具体来说：

     静态成员变量
     静态成员变量在类的所有实例之间共享，它们不是类的某个特定实例的一部分，而是属于类本身。
     这意味着即使没有创建类的实例，也可以访问静态成员变量。

     在类外定义静态成员变量的原因
     存储分配：静态成员变量在程序的全局数据区域分配内存。因此，必须在类外定义它们以便为它们分配存储空间。
     初始化：静态成员变量只能在类定义外进行初始化。这是因为静态成员变量的初始化必须在编译时完成，
     而类定义只是声明它们，并没有分配存储空间或提供初始化值。
     *
     */
}


#endif //GAMETOOLS_SINGLETON_H
