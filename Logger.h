#include <stdint.h>

class LogEntry {
    friend class Logger;

public:

    uint32_t t() const { return (mEntry >> TIME_SHIFT)& TIME_MASK; }
    uint16_t v() const { return (mEntry >> VAL_SHIFT) & VAL_MASK; }

private:
    enum {
        TIME_SHIFT = 0,
        TIME_SIZE = 20,
        TIME_MASK = (1 << TIME_SIZE) - 1,

        VAL_SHIFT = TIME_SIZE,
        VAL_SIZE = 10,
        VAL_MASK = (1 << VAL_SIZE) - 1
    };

    void set(uint32_t t, uint16_t v) {
        mEntry = ((t & TIME_MASK) << TIME_SHIFT) | ((v & VAL_MASK) << VAL_SHIFT);   
    }

    uint32_t    mEntry;
};

class Logger {
public:
    enum { NUM_ENTRIES = 1024 };

    Logger() = default;
    uint16_t        size() const             { return mCount; };

    void            AddEntry(uint32_t t, uint16_t val);
    const LogEntry* getEntry(uint16_t index) const;

private:
    uint16_t mHead = 0;
    uint16_t mCount = 0;

    LogEntry mEntries[NUM_ENTRIES];
};