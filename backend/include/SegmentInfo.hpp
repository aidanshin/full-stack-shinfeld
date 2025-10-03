#ifndef SEGMENTINFO_HPP
#define SEGMENTINFO_HPP

#include <cstdint>
#include <mutex>
#include <functional>

class SegmentInfo {
    private:
        uint32_t sequence_number = 0;
        uint8_t flag = 0;
        uint32_t start = 0;
        uint32_t data_size = 0;
        bool tracking = false;
        std::chrono::steady_clock::time_point time_sent{};
        mutable std::mutex mtx;
    public:
    
        SegmentInfo() = default;

        SegmentInfo(
            uint32_t seqNum,
            uint8_t flag,
            uint32_t start,
            uint32_t dataSize,
            bool tracking
        );


        uint32_t getSeqNum() const;
        uint8_t getFlag() const;
        uint32_t getStart() const;
        uint32_t getDataSize() const;
        bool isTracking() const;
        std::chrono::steady_clock::time_point getTimeSent() const;

        void setSeqNum(uint32_t val);
        void setFlag(uint8_t flag);
        void setStart(uint32_t val);
        void setDataSize(uint32_t val);
        void setTracking(bool val);
        void setTimeSent(std::chrono::steady_clock::time_point val);

        void setTimeSent();
        std::function<void()> LastTimeMessageSent(std::shared_ptr<SegmentInfo> self);

        long long printableTimeSent();
        

};

#endif 