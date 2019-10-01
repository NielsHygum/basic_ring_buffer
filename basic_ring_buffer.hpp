//
// Created by nni on 12.10.18.
//

#pragma once

#include <cstring>
#include <cstddef>
#include <string>

template <size_t bufferSize>
class RingBuffer
{
protected:
    //size_t bufferSize;
    //char * data;
    char data [bufferSize];
    std::atomic<size_t> head = {0};
    std::atomic<size_t> tail = {0};

public:
    template<typename T>
    float rootMeanSquareOfSamples(const size_t number_of_samples) const;
    size_t copyOldestDataFromBuffer(void * destination, const size_t bytesToRead);
    void writeNewestDataToBuffer(const void * source, const size_t bytesToWrite);
    size_t currentFreeCapacity();
    size_t currentFilledCapacity();
    size_t sizeOfBuffer();
    size_t headPosition();
    void setHead(size_t newPosition);
    void zerorizeBuffer();
    void setTailHeadDiff(size_t delta);// delta is positive
    void saveBufferToFile(std::string filename);
    void resetHeadAndTail();
};

#include <iostream>
#include <fstream>
#include <cmath>

template <size_t bufferSize>
template <typename T>
float RingBuffer<bufferSize>::rootMeanSquareOfSamples(const size_t number_of_samples) const
{
    float squared_sum = 0;

    size_t sum_over_samples = std::min(number_of_samples, bufferSize/sizeof(T));

    size_t sample_head = head/sizeof(T);

    const T * buffer_start_ptr = reinterpret_cast<const T*>(data);
    const T * head_ptr = reinterpret_cast<const T*>(data+head);

    if(sample_head > sum_over_samples)
    {
        for(const T* sample_data_ptr = buffer_start_ptr + (sample_head-sum_over_samples); sample_data_ptr < head_ptr ; ++sample_data_ptr)
        {
            squared_sum += static_cast<float>((*sample_data_ptr)*(*sample_data_ptr));
        }

    } else {

        const T* last_sample_in_buffer_ptr = reinterpret_cast<const T*>(buffer_start_ptr) + bufferSize/sizeof(T)-1;

        for(const T* sample_data_ptr = buffer_start_ptr+(bufferSize/sizeof(T)-(sum_over_samples-sample_head)); sample_data_ptr < last_sample_in_buffer_ptr; ++sample_data_ptr)
        {
            squared_sum += static_cast<float>((*sample_data_ptr)*(*sample_data_ptr));
        }

        for(const T* sample_data_ptr = buffer_start_ptr; sample_data_ptr < head_ptr ; ++sample_data_ptr)
        {
            squared_sum += static_cast<float>((*sample_data_ptr)*(*sample_data_ptr));
        }

    }

    return sqrt(squared_sum);
}


template <size_t bufferSize>
size_t RingBuffer<bufferSize>::copyOldestDataFromBuffer(void * destination, const size_t bytesToRead)
{
    size_t bytes_copied = 0;
    if(bytesToRead < bufferSize)
    {
        size_t current_head = head;
        size_t data_available = currentFilledCapacity();
        if(bytesToRead < data_available)
        {
            if(current_head > tail)
            {
                bytes_copied = bytesToRead;
                memcpy(destination, data+tail, bytes_copied);
                tail += bytes_copied;
            } else {

                bytes_copied = std::min(bufferSize-tail, bytesToRead);
                memcpy(destination, data+tail, bytes_copied);

                if(bytes_copied <= bytesToRead)
                {
                    tail = 0;
                    size_t bytes_left = bytesToRead-bytes_copied;
                    memcpy(static_cast<char *>(destination)+bytes_copied, data, bytes_left);
                    bytes_copied += bytes_left;
                    tail = bytes_left;

                } else {
                    tail += bytes_copied;
                }

            }

        } else {

            if(current_head > tail)
            {
                bytes_copied = data_available;
                memcpy(destination, data+tail, bytes_copied);
                tail += bytes_copied;

            } else {

                bytes_copied = bufferSize-tail;
                memcpy(destination, data+tail, bytes_copied);
                tail = 0;
                memcpy(static_cast<char *>(destination)+bytes_copied, data, current_head);
                bytes_copied += current_head;
                tail = current_head;
            }

        }
        /*
        size_t bytesLeftInRing = bufferSize-tail;
        if(bytesLeftInRing > bytesToRead)
        {
            memcpy(destination, data+tail, bytesToRead);
            tail += bytesToRead;
        }
        else
        {
            memcpy(destination, data+tail,bytesLeftInRing);
            tail = 0;
            memcpy(static_cast<char *>(destination)+bytesLeftInRing, data, bytesToRead-bytesLeftInRing);
        }*/
    }
    else
    {
        std::cout << "** error ** trying to read more data than available in buffer! " <<  "Read size " << bytesToRead << "bytes. Buffer size " << bufferSize << " bytes.\n";
    }

    return bytes_copied;
}

template <size_t bufferSize>
void RingBuffer<bufferSize>::writeNewestDataToBuffer(const void * source, const size_t bytesToWrite)
{
    if(bytesToWrite < bufferSize)
    {
        size_t bytesLeftInRing = bufferSize-head;
        // allways positive!

        if(bytesLeftInRing > bytesToWrite)
        {
            memcpy(data+head, source, bytesToWrite);
            head += bytesToWrite;
        }
        else
        {
            memcpy(data+head, source, bytesLeftInRing);
            head = 0;
            memcpy(data, static_cast<const char*>(source)+bytesLeftInRing, bytesToWrite-bytesLeftInRing);
            head = bytesToWrite-bytesLeftInRing;
        }
    }
}

template <size_t bufferSize>
size_t RingBuffer<bufferSize>::currentFreeCapacity()
{
    if (head >= tail)
        return bufferSize - (head - tail);
    else 	return tail - head;
}

template <size_t bufferSize>
size_t RingBuffer<bufferSize>::currentFilledCapacity()
{
    return bufferSize - currentFreeCapacity();
}

template <size_t bufferSize>
size_t RingBuffer<bufferSize>::sizeOfBuffer()
{
    return bufferSize;
}

template <size_t bufferSize>
void RingBuffer<bufferSize>::saveBufferToFile(std::string filename)
{
    std::ofstream file(filename, std::fstream::trunc | std::fstream::binary);
    file.write(data, bufferSize);
    file.close();
    std::cout << "wrote buffer to file\n";
}

template <size_t bufferSize>
size_t RingBuffer<bufferSize>::headPosition()
{
    return head;
}

template <size_t bufferSize>
void RingBuffer<bufferSize>::setHead(size_t newPosition)
{
    if (newPosition < bufferSize)
        head = newPosition;
    else
        std::cout << "new head position exceeds buffer size\n";
}

template <size_t bufferSize>
void RingBuffer<bufferSize>::zerorizeBuffer()
{
    memset(data, 0, bufferSize);
}

template <size_t bufferSize>
void RingBuffer<bufferSize>::setTailHeadDiff(size_t delta)
{
    if (delta < bufferSize)
        head = (tail + delta) % bufferSize; // this isnt thread safe
    else
        std::cout << "new head position exceeds buffer size\n";
}

template <size_t bufferSize>
void RingBuffer<bufferSize>::resetHeadAndTail()
{
    head = 0;
    tail = 0;
}

