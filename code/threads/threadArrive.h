#ifndef THREAD_ARRIVE_H
#define THREAD_ARRIVE_H

#include "thread.h"
#include "callback.h"

/**
 * @brief 用來模擬一個 thread 在 x 個單位時間 + 1 tick 後抵達並 fork 的 class
 * @note 該 class 的物件只能透過 makeThreadArrive 建立
 */
class ThreadArrive: public CallBackObj {
    Thread* const _t;              ///< 模擬哪個 thread
    VoidFunctionPtr const _func;   ///< Fork 後要執行的函數
    void* const _arg;              ///< 傳給 _func 的參數

    /// @brief private constructor called by makeThreadArrive
    ThreadArrive(Thread* t, int x, VoidFunctionPtr func, void* arg);

public:
    /**
     * @brief t 將會在 x 個 TimerTicks + 1 tick 後抵達並 fork
     * @param t    - 還沒 Fork 的 Thread 物件
     * @param x    - 幾個 TimerTicks
     * @param func - Fork 後要執行的函數
     * @param arg  - 傳給 func 的參數
     */
    static void makeThreadArrive(Thread* t, int x, VoidFunctionPtr func, void* arg);

    /// @brief Fork the thread, when the interrupt happens
    void CallBack() override;
};

#endif // THREAD_ARRIVE_H