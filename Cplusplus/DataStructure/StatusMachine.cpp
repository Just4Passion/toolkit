/*
    首先状态机，需要现有状态
    事件驱动状态发生变化
        状态集合：
            (当前状态，输入事件) -> 新的状态
    

*/



#include <iostream>
#include <map>
#include <tuple>

//有锁状态机
#include <mutex>
#include <stdexcept>

//多线程
#include <thread>

#include <vector>
#include <string>

//无锁状态机
#include <atomic>

//回调函数
#include <functional>

using namespace std;

//状态枚举
//空，运行，暂停，停止
enum class State{
    Idle,
    Running,
    Paused,
    Stopped
};

//事件枚举
enum class Event{
    Start,
    Pause,
    Resume,
    Stop
};

//
std::vector<string> strState = {"Idle", "Running", "Paused", "Stopped"};

//状态转换表
//状态转换规则：当前状态 + 事件 -> 新状态
//当前状态，收到事件后的，下一个状态
std::map<std::tuple<State, Event>, State> transitionRules = {
    {{State::Idle, Event::Start}, State::Running},
    {{State::Running, Event::Pause}, State::Paused},
    {{State::Paused, Event::Resume}, State::Running},
    {{State::Running, Event::Stop}, State::Stopped},
    {{State::Paused, Event::Stop}, State::Stopped}
};

//同步状态机？？
class SyncStateMachine{
private:
    State currentState;
    mutable std::mutex mtx;

public:
    SyncStateMachine(): currentState(State::Idle){}

    //当事件来的时候，看一下当前状态，事件，下一状态是否存在
    //存在，则说明可以跳转到下一个状态
    //使用map可能不适用于消费场景，因为key必须是唯一的
    //所以可以使用unorder_map
    bool handleEvent(Event event){
        //lock_gurad用于离开后自动解锁
        std::lock_guard<std::mutex> lock(mtx);
        
        //查找转换规则
        auto key = std::make_tuple(currentState, event);
        auto it = transitionRules.find(key);

        if (it != transitionRules.end())
        {
            //进入下一个状态
            cout << "Come in next status [" << 
            strState[static_cast<int>(currentState)] << 
                "] -> [" << 
            strState[static_cast<int>(it->second)]<<"]" << endl;
            currentState = it->second;
            return true;    //转换成功
        }
        return false;
    }

    //获取当前状态（线程安全）
    State getCurrentState() const {
        std::lock_guard<std::mutex> lock(mtx);
        return currentState;
    }

};


//现在有状态机了
//   (当前状态，发生的事件) -> 下一个状态

// 还需要不断发生的事件
// 线程1：
// 当事件发生时，被发送到某个center，集中处理
// 线程2：
// 不断从center中取出事件，进行处理
// 处理时，就需要使用状态机检查事件是否正确
// 不正确，则消费失败 —— 对于不正确的事件，不一定要消费失败，还可以超时
// 所以当不匹配的事件发生时，会有两种结果，一种是直接失败，另一种就是继续等待
// 

void worker(SyncStateMachine &sm)
{
    sm.handleEvent(Event::Start);
    sm.handleEvent(Event::Pause);
    sm.handleEvent(Event::Resume);
    sm.handleEvent(Event::Stop);
}


//无锁实现状态机
//使用std::atomic替代锁
class LockFreeStateMachine{
private:
    std::atomic<State> currentState;
public:
    LockFreeStateMachine() : currentState(State::Idle){}
    //处理事件
    bool handleEvent(Event event){
        State expected, desired;
        do{
            expected = currentState.load();
            auto key = std::make_tuple(expected, event);
            auto it = transitionRules.find(key);
            if (it == transitionRules.end()) return false;
            desired = it->second;
        }while(!currentState.compare_exchange_weak(expected, desired));
        return true;
    }

    State getCurrentState() const {
        return currentState.load();
    }
};

class SyncStateMachine_Callback{
private:
    State currentState;
    mutable std::mutex mtx;
    //状态转换回调函数 —— 在状态发生转换时调用
    using TransitionCallback = std::function<void(State, Event, State)>;
    TransitionCallback onStateChanged;
public:

    SyncStateMachine_Callback(): currentState(State::Idle){}
    
    void setCallback(TransitionCallback callback){
        onStateChanged = callback;
    }

    //事件处理函数
    bool handleEvent(Event event){
        std::lock_guard<std::mutex> lock(mtx);
        auto key = std::make_tuple(currentState, event);
        auto it = transitionRules.find(key);
        if (it != transitionRules.end()){
            State oldState = currentState;
            currentState = it->second;
            if (onStateChanged){
                onStateChanged(oldState, event, currentState);
            }
            return true;
        }
        return false;
    }

};

int main()
{
    cout << "Hello World" << endl;
    SyncStateMachine sm;

    //多线程测试
    std::thread t1(worker, std::ref(sm));
    std::thread t2(worker, std::ref(sm));
    t1.join();
    t2.join();

    std::cout << "Final State: " << static_cast<int>(sm.getCurrentState()) << std::endl;

    return 0;
}
