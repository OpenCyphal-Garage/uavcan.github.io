/**
 * This header implements a virtual CAN driver for multi-threaded libuavcan nodes.
 * It is supposed to be connected to the secondary node (aka sub-node) in place of a real CAN driver.
 * Once connected, the virtual driver will be redirecting outgoing CAN frames to the main node, and
 * it will be copying RX frames received by the main node to the secondary node (i.e. every RX frame will
 * go into the main node AND the secondary node).
 *
 * @file uavcan_virtual_driver.hpp
 * @author Pavel Kirienko <pavel.kirienko@zubax.com>
 *
 * The source code contained in this file is distributed under the terms of CC0 (public domain dedication).
 */

#pragma once

#include <iostream>             // For std::cout
#include <thread>               // For std::mutex
#include <condition_variable>   // For std::condition_variable
#include <uavcan/uavcan.hpp>    // Main libuavcan header

namespace uavcan_virtual_driver
{
/**
 * Generic queue based on the linked list class defined in libuavcan.
 * It does not use heap memory, but uses the libuavcan's deterministic pool allocator.
 *
 * This class is used to implement synchronized RX queue of the secondary thread.
 */
template <typename T>
class Queue
{
    struct Item : public uavcan::LinkedListNode<Item>
    {
        T payload;

        template <typename... Args>
        Item(Args... args) : payload(args...) { }
    };

    uavcan::LimitedPoolAllocator allocator_;
    uavcan::LinkedListRoot<Item> list_;

public:
    /**
     * @param arg_allocator             Used to allocate memory for stored items.
     *
     * @param block_allocation_quota    Maximum number of memory blocks this queue can take from the allocator.
     *                                  Defines the depth of the queue.
     */
    Queue(uavcan::IPoolAllocator& arg_allocator,
          std::size_t block_allocation_quota) :
        allocator_(arg_allocator, block_allocation_quota)
    {
        uavcan::IsDynamicallyAllocatable<Item>::check();
    }

    bool isEmpty() const { return list_.isEmpty(); }

    /**
     * Creates one item in-place at the end of the list.
     * Returns true if the item has been appended successfully, false if there's not enough memory.
     * Complexity is O(N) where N is queue length.
     */
    template <typename... Args>
    bool tryEmplace(Args... args)
    {
        // Allocating memory
        void* const ptr = allocator_.allocate(sizeof(Item));
        if (ptr == nullptr)
        {
            return false;
        }

        // Constructing the new item
        Item* const item = new (ptr) Item(args...);
        assert(item != nullptr);

        // Inserting the new item at the end of the list
        Item* p = list_.get();
        if (p == nullptr)
        {
            list_.insert(item);
        }
        else
        {
            while (p->getNextListNode() != nullptr)
            {
                p = p->getNextListNode();
            }
            assert(p->getNextListNode() == nullptr);
            p->setNextListNode(item);
            assert(p->getNextListNode()->getNextListNode() == nullptr);
        }

        return true;
    }

    /**
     * Accesses the first element.
     * Nullptr will be returned if the queue is empty.
     * Complexity is O(1).
     */
    T*       peek()       { return isEmpty() ? nullptr : &list_.get()->payload; }
    const T* peek() const { return isEmpty() ? nullptr : &list_.get()->payload; }

    /**
     * Removes the first element.
     * If the queue is empty, nothing will be done and assertion failure will be triggered.
     * Complexity is O(1).
     */
    void pop()
    {
        Item* const item = list_.get();
        assert(item != nullptr);
        if (item != nullptr)
        {
            list_.remove(item);
            item->~Item();
            allocator_.deallocate(item);
        }
    }
};

/**
 * This class implements one virtual interface.
 *
 * Objects of this class are owned by the secondary thread.
 * This class does not use heap memory, instead it uses a block allocator provided by reference to the constructor.
 */
class Iface final : public uavcan::ICanIface,
                    uavcan::Noncopyable
{
    struct RxItem
    {
        const uavcan::CanRxFrame frame;
        const uavcan::CanIOFlags flags;

        RxItem(const uavcan::CanRxFrame& arg_frame,
               uavcan::CanIOFlags arg_flags) :
            frame(arg_frame),
            flags(arg_flags)
        { }
    };

    std::mutex& mutex_;
    uavcan::CanTxQueue prioritized_tx_queue_;
    Queue<RxItem> rx_queue_;

    /**
     * Implements uavcan::ICanDriver. Will be invoked by the sub-node.
     */
    std::int16_t send(const uavcan::CanFrame& frame,
                      uavcan::MonotonicTime tx_deadline,
                      uavcan::CanIOFlags flags) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        prioritized_tx_queue_.push(frame, tx_deadline, uavcan::CanTxQueue::Volatile, flags);
        return 1;
    }

    /**
     * Implements uavcan::ICanDriver. Will be invoked by the sub-node.
     */
    std::int16_t receive(uavcan::CanFrame& out_frame,
                         uavcan::MonotonicTime& out_ts_monotonic,
                         uavcan::UtcTime& out_ts_utc,
                         uavcan::CanIOFlags& out_flags) override
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (rx_queue_.isEmpty())
        {
            return 0;
        }

        const auto item = *rx_queue_.peek();
        rx_queue_.pop();

        out_frame = item.frame;
        out_ts_monotonic = item.frame.ts_mono;
        out_ts_utc = item.frame.ts_utc;
        out_flags = item.flags;

        return 1;
    }

    /**
     * Stubs for uavcan::ICanDriver. Will be invoked by the sub-node.
     * These methods are meaningless for a virtual interface.
     */
    std::int16_t configureFilters(const uavcan::CanFilterConfig*, std::uint16_t) override { return -uavcan::ErrDriver; }
    std::uint16_t getNumFilters() const override { return 0; }
    std::uint64_t getErrorCount() const override { return 0; }

public:
    /**
     * This class should be instantiated by the secondary thread.
     *
     * @param allocator         Needed to provide storage for the prioritized TX queue and the RX queue.
     *
     * @param clock             Needed for the TX queue so it can discard stale frames.
     *
     * @param arg_mutex         Needed because the main and the secondary threads communicate through this class,
     *                          therefore it must be thread-safe.
     *
     * @param quota_per_queue   Defines how many frames the queues, both RX and TX, can accommodate.
     */
    Iface(uavcan::IPoolAllocator& allocator,
          uavcan::ISystemClock& clock,
          std::mutex& arg_mutex,
          unsigned quota_per_queue) :
        mutex_(arg_mutex),
        prioritized_tx_queue_(allocator, clock, quota_per_queue),
        rx_queue_(allocator, quota_per_queue)
    { }

    /**
     * This method adds one frame to the RX queue of the secondary thread.
     * It is invoked by the main thread when the node receives a frame from the bus.
     *
     * Note that RX queue overwrites oldest items when overflowed.
     * No additional locking is required - this method is thread-safe. Call from the main thread only.
     *
     * @param frame     The frame to be received by the sub-node.
     *
     * @param flags     Flags associated with the frame. See @ref uavcan::CanIOFlags for available flags.
     */
    void addRxFrame(const uavcan::CanRxFrame& frame,
                    uavcan::CanIOFlags flags)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!rx_queue_.tryEmplace(frame, flags) && !rx_queue_.isEmpty())
        {
            rx_queue_.pop();
            (void)rx_queue_.tryEmplace(frame, flags);
        }
    }

    /**
     * This method flushes frames from the sub-node's TX queue into the main node's TX queue.
     *
     * No additional locking is required - this method is thread-safe. Call from the main thread only.
     *
     * @param main_node         Reference to the main node, which will receive the frames.
     *
     * @param iface_index       Index of the interface in which the frames will be sent.
     */
    void flushTxQueueTo(uavcan::INode& main_node,
                        std::uint8_t iface_index)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const std::uint8_t iface_mask = static_cast<std::uint8_t>(1U << iface_index);

        while (auto e = prioritized_tx_queue_.peek())
        {
#if !NDEBUG && UAVCAN_TOSTRING
            std::cout << "uavcan_virtual_driver::Iface: TX injection [iface_index=" << int(iface_index) << "]: "
                      << e->toString() << std::endl;
#endif
            const int res = main_node.injectTxFrame(e->frame, e->deadline, iface_mask,
                                                    uavcan::CanTxQueue::Qos(e->qos), e->flags);
            prioritized_tx_queue_.remove(e);
            if (res <= 0)
            {
                break;
            }
        }
    }

    /**
     * This method reports whether there's data for the sub-node to read from the RX thread.
     *
     * No additional locking is required - this method is thread-safe. Call from the secondary thread only.
     */
    bool hasDataInRxQueue() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return !rx_queue_.isEmpty();
    }
};

/**
 * This interface defines one method that will be called by the main node thread periodically in order to
 * transfer contents of TX queue of the sub-node into the TX queue of the main node.
 */
class ITxQueueInjector
{
public:
    virtual ~ITxQueueInjector() { }

    /**
     * Flush contents of TX queues into the main node's prioritized TX queue.
     * @param main_node         Reference to the main node.
     */
    virtual void injectTxFramesInto(uavcan::INode& main_node) = 0;
};

/**
 * This is the main, user-facing part of the virtual driver.
 * This class will be instantiated by the application and passed into the sub-node as its CAN interface.
 *
 * Objects of this class are owned by the secondary thread.
 * This class does not use heap memory, instead it uses a block allocator provided by reference to the constructor.
 */
class Driver final : public uavcan::ICanDriver,
                     public uavcan::IRxFrameListener,
                     public ITxQueueInjector,
                     uavcan::Noncopyable
{
    /**
     * Basic synchronization object. Can be replaced with whatever is appropriate for the target platform.
     */
    class Event
    {
        std::mutex m_;
        std::condition_variable cv_;

    public:
        /**
         * Note that in this implementation this method may return spuriously, which is OK.
         */
        void waitFor(uavcan::MonotonicDuration duration)
        {
            std::unique_lock<std::mutex> lk(m_);
            (void)cv_.wait_for(lk, std::chrono::microseconds(duration.toUSec()));
        }

        void signal() { cv_.notify_all(); }
    };

    Event event_;                                   ///< Used to unblock the sub-node's select() call when IO happens.
    std::mutex mutex_;                              ///< Shared across all ifaces
    uavcan::LazyConstructor<Iface> ifaces_[uavcan::MaxCanIfaces];
    const unsigned num_ifaces_;
    uavcan::ISystemClock& clock_;

    /**
     * Implements uavcan::ICanDriver. Will be invoked by the sub-node.
     */
    uavcan::ICanIface* getIface(std::uint8_t iface_index) override
    {
        return (iface_index < num_ifaces_) ? ifaces_[iface_index].operator Iface*() : nullptr;
    }

    /**
     * Implements uavcan::ICanDriver. Will be invoked by the sub-node.
     */
    std::uint8_t getNumIfaces() const override { return num_ifaces_; }

    /**
     * Implements uavcan::ICanDriver. Will be invoked by the sub-node.
     */
    std::int16_t select(uavcan::CanSelectMasks& inout_masks,
                        const uavcan::CanFrame* (&)[uavcan::MaxCanIfaces],
                        uavcan::MonotonicTime blocking_deadline) override
    {
        bool need_block = (inout_masks.write == 0);    // Write queue is infinite
        for (unsigned i = 0; need_block && (i < num_ifaces_); i++)
        {
            const bool need_read = inout_masks.read & (1U << i);
            if (need_read && ifaces_[i]->hasDataInRxQueue())
            {
                need_block = false;
            }
        }

        if (need_block)
        {
            event_.waitFor(blocking_deadline - clock_.getMonotonic());
        }

        inout_masks = uavcan::CanSelectMasks();
        for (unsigned i = 0; i < num_ifaces_; i++)
        {
            const std::uint8_t iface_mask = 1U << i;
            inout_masks.write |= iface_mask;           // Always ready to write
            if (ifaces_[i]->hasDataInRxQueue())
            {
                inout_masks.read |= iface_mask;
            }
        }

        return num_ifaces_;       // We're always ready to write, hence > 0.
    }

    /**
     * Implements uavcan::IRxFrameListener. Will be invoked by the main node.
     */
    void handleRxFrame(const uavcan::CanRxFrame& frame,
                       uavcan::CanIOFlags flags) override
    {
#if !NDEBUG && UAVCAN_TOSTRING
            std::cout << "uavcan_virtual_driver::Driver: RX [flags=" << flags << "]: "
                      << frame.toString(uavcan::CanFrame::StrAligned) << std::endl;
#endif
        if (frame.iface_index < num_ifaces_)
        {
            ifaces_[frame.iface_index]->addRxFrame(frame, flags);
            event_.signal();
        }
    }

    /**
     * Implements ITxQueueInjector. Will be invoked by the main thread.
     */
    void injectTxFramesInto(uavcan::INode& main_node) override
    {
        for (unsigned i = 0; i < num_ifaces_; i++)
        {
            ifaces_[i]->flushTxQueueTo(main_node, i);
        }
        event_.signal();
    }

public:
    /**
     * This class should be instantiated by the secondary thread.
     *
     * @param arg_num_ifaces    The number of interfaces defines how many virtual interfaces will be instantiated
     *                          by this class. IT DOESN'T HAVE TO MATCH THE NUMBER OF PHYSICAL INTERFACES THE MAIN
     *                          NODE IS USING - it can be less than that (but not greater), in which case the
     *                          sub-node will be using only interfaces with lower indices, which may be fine
     *                          (depending on the application's requirements). For example, if the main node has
     *                          three interfaces with indices 0, 1, 2, and the virtual driver implements only two,
     *                          the sub-node will only have access to interfaces 0 and 1.
     *
     * @param clock             Needed by the virtual iface class, and for select() timing.
     *
     * @param shared_allocator  This allocator will be used to keep inter-thread queues.
     *
     * @param block_allocation_quota_per_virtual_iface  Maximum number of blocks that can be allocated per virtual
     *                                                  iface. Ifaces use dynamic memory to keep RX/TX queues, so
     *                                                  higher quota enables deeper queues. Note that ifaces DO NOT
     *                                                  allocate memory unless they need it, i.e. the memory will not
     *                                                  be taken unless there is data to enqueue; once a queue is
     *                                                  flushed, the memory will be immediately freed.
     *                                                  One block fits exactly one CAN frame, i.e. a quota of 64
     *                                                  blocks allows the interface to keep up to 64 RX+TX CAN frames.
     */
    Driver(unsigned arg_num_ifaces,
           uavcan::ISystemClock& clock,
           uavcan::IPoolAllocator& shared_allocator,
           unsigned block_allocation_quota_per_virtual_iface) :
        num_ifaces_(arg_num_ifaces),
        clock_(clock)
    {
        assert(num_ifaces_ > 0 && num_ifaces_ <= uavcan::MaxCanIfaces);

        const unsigned quota_per_queue = block_allocation_quota_per_virtual_iface; // 2x overcommit

        for (unsigned i = 0; i < num_ifaces_; i++)
        {
            ifaces_[i].template
                construct<uavcan::IPoolAllocator&, uavcan::ISystemClock&, std::mutex&, unsigned>
                    (shared_allocator, clock_, mutex_, quota_per_queue);
        }
    }
};

}
