#include "threadArrive.h"
#include <cmath>
#include "main.h"

ThreadArrive::ThreadArrive(Thread* t, int x, VoidFunctionPtr func, void* arg)
    : _t(t), _func(func), _arg(arg)
{
    ASSERT(_t->getStatus() == JUST_CREATED);

    // 在 x 個 TimerTicks + 1 tick 後抵達
    kernel->interrupt->Schedule(this, x * TimerTicks + 1, IntType::TimerInt);
}

void ThreadArrive::makeThreadArrive(Thread *t, int x, VoidFunctionPtr func, void *arg)
{
    new ThreadArrive(t, x, func, arg);
}

void ThreadArrive::CallBack()
{
    ASSERT(kernel->currentThread != nullptr);
    ASSERT(_t->getStatus() == JUST_CREATED);

    Thread* const CurrentThread = kernel->currentThread;
    const int CurrentTime = kernel->stats->totalTicks;

    // create the thread
    cout << "ThreadArrive::CallBack() thread " << _t->getName() << " arrived at " << CurrentTime << endl;
    _t->Fork(_func, _arg);

    // if _t can preempt the CurrentThread
    if (kernel->scheduler->getSchedulerType() == SRTF && _t->getBurstTime() < CurrentThread->getBurstTime()) {
        // yield
        kernel->interrupt->YieldOnReturn();
    }

    delete this;
}