//
// Created by john on 4/18/19.
//

#ifndef MY_TINYMUDUO_NET_CALLBACKS_HPP
#define MY_TINYMUDUO_NET_CALLBACKS_HPP

#include "base/Timestamp.hpp"

#include <functional>
#include <memory>

namespace muduo{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;

    template<typename T>
    inline T* get_pointer(const std::shared_ptr<T>&ptr){
        return ptr.get();
    }

    template<typename T>
    inline T* get_pointer(const std::unique_ptr<T>& ptr){
        return ptr.get();
    }
    namespace net{
            class Buffer;
            class TcpConnection;
            typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
            typedef std::function<void()>TimerCallback;
            typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
            typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
            typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
            typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

            typedef std::function<void (const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;

            void defaultConnectionCallback(const TcpConnectionPtr& conn);
            void defaultMessageCallback(const TcpConnectionPtr&conn, Buffer* buffer, Timestamp receiveTime);
        }
}

#endif //MY_TINYMUDUO_NET_CALLBACKS_H
