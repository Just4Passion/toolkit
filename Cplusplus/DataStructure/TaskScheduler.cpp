/*
    调度器
        管理任务队列
        按照一定策略执行任务
            FIFO
            优先级调度
            轮转调度
    调度器的工作就是选择一个任务，执行这个任务

    首先需要要给任务类，可派生出各种任务
    然后需要一个策略类，可以让调度器按照这个策略执行
    最后就是一个调度器类，执行调度
*/

/*
使用百度deepseek，设计出了任务类，策略类，调度器类

任务类：
    任务优先级
    任务执行接口

策略类：
    存在策略队列：符合算法和数据结构的绑定关系

调度器类：
    需要策略类进行初始化


关键设计点
    ‌智能指针管理‌：
        使用 unique_ptr 明确资源所有权
        避免内存泄漏和悬空指针
    ‌策略模式‌：
        将算法（调度策略）抽象为独立对象
        运行时动态切换算法
    ‌类型擦除‌：
        通过基类指针操作各种派生类对象
        统一处理不同类型的任务和策略

*/

#include <iostream>
#include <memory>
#include <queue>
#include <vector>
#include <vector>
#include <functional>

//任务基类
//任务的执行
//任务优先级
class Task{
public:
    virtual void execute() = 0;
    virtual ~Task() = default;
    virtual int getPriority() const { return 0; }
};

//具体任务
class PrintTask: public Task{
    std::string message_;
public:
    explicit PrintTask(std::string msg): message_(std::move(msg)) {}
    void execute() override{
        std::cout << "Executing: " << message_ << std::endl;
    }
};

//带优先级的任务
class PriorityTask: public Task{
    std::string message_;
    int priority_;
public:
    PriorityTask(std::string msg, int prio)
        : message_(std::move(msg)), priority_(prio) {}
    
    void execute() override{
        std::cout << "[P" << priority_ << "]" << message_ << std::endl;
    }

    int getPriority() const override { return priority_; }
};

//调度策略接口
//抽象基类，只有接口，没有数据结构
//调度类负责选择一个任务执行
//调度类负责添加一个任务到队列中
class SchedulingStrategy{
public:
    virtual void addTask(std::unique_ptr<Task> task) = 0;
    virtual std::unique_ptr<Task> getNextTask() = 0;
    virtual ~SchedulingStrategy() = default;
};

//具体策略类：先进先出（继承实现策略）
class FifoStrategy: public SchedulingStrategy{
    //先进先出队列
    //即普通队列
    std::queue<std::unique_ptr<Task>> queue_;
public:
    void addTask(std::unique_ptr<Task> task){
        queue_.push(std::move(task));
    }
    std::unique_ptr<Task> getNextTask() override{
        if (queue_.empty()) return nullptr;
        auto task = std::move(queue_.front());
        queue_.pop();
        return task;
    }
};

//优先级队列
//最小堆模式
//
class PriorityStrategy: public SchedulingStrategy{
    struct TaskComparator{
        bool operator() (const std::unique_ptr<Task>& a, 
                            const std::unique_ptr<Task>& b)
        {
            return a->getPriority() > b->getPriority(); //最小堆
        }
    };
    std::priority_queue<
        std::unique_ptr<Task>,
        std::vector<std::unique_ptr<Task>>,
        TaskComparator> pq_;

public:
    void addTask(std::unique_ptr<Task> task) override{
        pq_.push(std::move(task));
    }
    std::unique_ptr<Task> getNextTask() override{
        if (pq_.empty()) return nullptr;
        auto task = std::move(const_cast<std::unique_ptr<Task>&>(pq_.top()));
        pq_.pop();
        return task;
    }
};

//调度器
//需要一个策略进行初始化
class Scheduler{
    std::unique_ptr<SchedulingStrategy> strategy_;
public:
    explicit Scheduler(std::unique_ptr<SchedulingStrategy> strategy)
        : strategy_(std::move(strategy)) {}
    
    void submit(std::unique_ptr<Task> task){
        strategy_->addTask(std::move(task));
    }

    void run(){
        while(auto task = strategy_->getNextTask()){
            task->execute();
        }
    }
};

int main()
{
    //FIFO策略演示
    {
        auto strategy = std::make_unique<FifoStrategy>();
        Scheduler scheduler(std::move(strategy));
        
        scheduler.submit(std::make_unique<PrintTask>("First"));
        scheduler.submit(std::make_unique<PrintTask>("Second"));
        scheduler.submit(std::make_unique<PrintTask>("Third"));
        
        std::cout << "FIFO Schedule:" << std::endl;
        scheduler.run(); 
    }

    //优先级策略演示
    {
        auto strategy = std::make_unique<PriorityStrategy>();
        Scheduler scheduler(std::move(strategy));
        
        scheduler.submit(std::make_unique<PriorityTask>("Low", 3));
        scheduler.submit(std::make_unique<PriorityTask>("Urgent", 1));
        scheduler.submit(std::make_unique<PriorityTask>("Medium", 2));
        
        std::cout << "\nPriority Schedule:" << std::endl;
        scheduler.run();
    }
    return 0;
}
