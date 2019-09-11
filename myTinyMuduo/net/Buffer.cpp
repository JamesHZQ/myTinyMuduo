//
// Created by john on 4/21/19.
//

#include "net/Buffer.hpp"
#include "net/SocketsOps.hpp"

#include <errno.h>
#include <sys/uio.h>

using namespace muduo;
using namespace muduo::net;

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

//ssize_t Buffer::readFd(int fd,int* savedErrno){
//
//}


ssize_t Buffer::readFd(int fd, int *savedErrno) {
    char extrabuff[65536];
    struct iovec vec[2];
    const size_t  writable = writableBytes();
    vec[0].iov_base = begin()+writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuff;
    vec[1].iov_len = sizeof(extrabuff);
    //当Buffer对象有足够空间的时候，直接读到Buffer里
    const int iovcnt = (writable < sizeof(extrabuff)) ? 2:1;
    const ssize_t n = sockets::readv(fd,vec,iovcnt);

    if(n < 0){
        *savedErrno = errno;
    }else if(static_cast<size_t >(n) <= writable){
        //读到的字节数小于原Buffer的可写区，则数据全部读到Buffer，
        // Buffer可读区并入那个字节的可写区
        writerIndex_ += n;
    }else{
        //读到的字节数大于Buffer的可写空间，则Buffer满
        //写指针移到buffer_末尾，然后在将extrabuff里数据，添加到Buffer中（需要扩充buffer_）
        //这样可以减少read()的调用次数(减少到1次)（大部分时候都可以一次读完了）
        writerIndex_ = buffer_.size();
        append(extrabuff, n - writable);
    }
    return n;
}
