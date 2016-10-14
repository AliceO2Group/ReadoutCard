/// \file BufferManager.h
/// \brief Definition of the BufferManager class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_RORC_SRC_BUFFERMANAGER_H_
#define ALICEO2_RORC_SRC_BUFFERMANAGER_H_

#include <cassert>

template<int FIFO_CAPACITY>
class BufferManager
{
  public:

    void setBufferCapacity(int capacity)
    {
      mBuffer.capacity = capacity;
    }

    int getBufferHead() const
    {
      return getHead(mBuffer);
    }

    int getFifoHead() const
    {
      return getHead(mFifo);
    }

    void advanceHead()
    {
      assert(mBuffer.size < mBuffer.getCapacity());
      assert(mFifo.size < mFifo.getCapacity());

      mBuffer.size++;
      mFifo.size++;
    }

    void advanceTail()
    {
      assert(mBuffer.size != 0);
      assert(mFifo.size != 0);

      incrementTail(mFifo);
      incrementTail(mBuffer);
    }

    int getBufferSize() const
    {
      return mBuffer.size;
    }

    int getFifoSize() const
    {
      return mFifo.size;
    }

    int getBufferCapacity() const
    {
      return mBuffer.getCapacity();
    }

    int getFifoCapacity() const
    {
      return mFifo.getCapacity();
    }

    int getBufferFree() const
    {
      return getBufferCapacity() - getBufferSize();
    }

    int getFifoFree() const
    {
      return getFifoCapacity() - getFifoSize();
    }

    int getFree() const
    {
      return std::min(getBufferFree(), getFifoFree());
    }

    int getBufferTail() const
    {
      return mBuffer.tail;
    }

    int getFifoTail() const
    {
      return mFifo.tail;
    }

    bool isBufferEmpty() const
    {
      return getBufferSize() == 0;
    }

    bool isFifoEmpty() const
    {
      return getFifoSize() == 0;
    }

    /// If buffer tail is more than the FIFO size behind the head, we know for sure the page has moved out of the FIFO,
    /// generally meaning its transfer has completed
    bool isBufferTailOutOfFifo() const
    {
      return getBufferSize() > getFifoCapacity();
    }

  private:
    // Some FIFO helper functions

    template<typename T>
    int getHead(const T& fifo) const
    {
      return addWrapped(fifo.tail, fifo.size, fifo.getCapacity());
    }

    template<typename T>
    T addWrapped(const T& x, const T& add, const T& max) const
    {
      return (x + add) % max;
    }

    template<typename T>
    T wrappedDistance(const T& head, const T& tail, const T& size) const
    {
      return (head >= tail) ? (head - tail) : (head + size - tail);
    }

    template<typename T>
    int getFreeCount(const T& fifo) const
    {
      return fifo.getCapacity() - fifo.size;
    }

    template<typename T>
    void incrementTail(T& fifo)
    {
      fifo.tail = addWrapped(fifo.tail, 1, fifo.getCapacity());
      fifo.size--;
    }

    struct Buffer
    {
        /// Index of page that's the tail of the circular buffer, i.e. the oldest page that has not yet been acknowledged
        int tail = 0;

        /// Current amount of pages in buffer
        int size = 0;

        /// Max amount of pages in buffer
        int capacity = 0;

        int getCapacity() const
        {
          return capacity;
        }
    } mBuffer;

    struct Fifo
    {
        /// Index of page that's the tail of the FIFO, i.e. the page that was least recently pushed
        /// This is the page that must be checked for arrival
        int tail = 0;

        /// Current amount of pages in FIFO
        int size = 0;

        /// Max amount of pages in FIFO
        int getCapacity() const
        {
          return FIFO_CAPACITY;
        }
        ;
    } mFifo;
};

#endif // ALICEO2_RORC_SRC_BUFFERMANAGER_H_
