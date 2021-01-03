#include <Arduino.h>
#include "Logger.h"

//#define DEBUG

void Logger::AddEntry(uint32_t t, uint16_t v)
{
#ifdef DEBUG
    Serial.printf_P(PSTR("AddEntry(%u, %u)\n"), t, v);
#endif

    mEntries[mHead].set(t, v);
    mHead = (mHead + 1) % NUM_ENTRIES;
    if (mCount < NUM_ENTRIES) {
        ++mCount;
    }
}

const LogEntry* Logger::getEntry(uint16_t index) const
{
    if (index >= mCount)
        return nullptr;

    if (mCount >= NUM_ENTRIES)
        index += mHead;
    return &mEntries[index % NUM_ENTRIES];
}