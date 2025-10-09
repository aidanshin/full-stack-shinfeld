#include "SegmentInfo.hpp"
#include "Logger.hpp"
#include "Flags.hpp"

SegmentInfo::SegmentInfo(
    uint32_t seqNum,
    uint8_t flag,
    uint32_t start,
    uint32_t dataSize,
    bool tracking
) : sequence_number(seqNum),
    flag(flag),
    start(start),
    data_size(dataSize),
    tracking(tracking)
{
    INFO_SRC("SegmentInfo - New Segment Tracking [SEQ=%u FLAG=%s start=%u dataSize=%u tracking=%d]", sequence_number, flagsToStr(flag).c_str(), start, data_size, tracking);
}


uint32_t SegmentInfo::getSeqNum() const {return sequence_number;}
uint8_t SegmentInfo::getFlag() const {return flag;}
uint32_t SegmentInfo::getStart() const {return start;}
uint32_t SegmentInfo::getDataSize() const {return data_size;}
bool SegmentInfo::isTracking() const {return tracking;}

void SegmentInfo::setSeqNum(uint32_t val) {sequence_number = val;}
void SegmentInfo::setFlag(uint8_t val) {flag = val;}
void SegmentInfo::setStart(uint32_t val) {start = val;}
void SegmentInfo::setDataSize(uint32_t val) {data_size = val;}
void SegmentInfo::setTracking(bool val) {tracking = val;}
void SegmentInfo::setTimeSent(std::chrono::steady_clock::time_point val) {time_sent = val;}



std::chrono::steady_clock::time_point SegmentInfo::getTimeSent() const { 
    std::lock_guard<std::mutex> lock(mtx); 
    return time_sent; 
} 

void SegmentInfo::setTimeSent() { 
    std::lock_guard<std::mutex> lock(mtx); 
    time_sent = std::chrono::steady_clock::now(); 
    auto sent_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_sent.time_since_epoch()).count();
    TRACE_SRC("SegmentInfo[setTimeSent] - updated time now [SEQ=%u time_sent=%lld]", sequence_number, sent_ms);
} 
std::function<void()> SegmentInfo::LastTimeMessageSent(std::shared_ptr<SegmentInfo> self) { 
    return [self] {self->setTimeSent();}; 
}

long long SegmentInfo::printableTimeSent() {
    std::lock_guard<std::mutex> lock(mtx);
    auto sent_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_sent.time_since_epoch()).count();
    return static_cast<long long>(sent_ms);
}