#ifndef __ASYNCONEWIRE_H
#define __ASYNCONEWIRE_H

#include <stdlib.h>
#include <stdint.h>


#include <Arduino.h>
#define ONEWIRE_SET_LOW() do {  \
    digitalWrite (pin, LOW);    \
    pinMode (pin, OUTPUT);      \
} while (0)
#define ONEWIRE_SET_HIGH() do { \
    digitalWrite (pin, HIGH);   \
    pinMode (pin, OUTPUT);      \
} while (0)
#define ONEWIRE_RELEASE() pinMode (pin, INPUT)
#define ONEWIRE_READ() digitalRead (pin)


typedef enum
{
    ONEWIRE_CMD_NONE = 0,
    ONEWIRE_CMD_RESET,
    ONEWIRE_CMD_WRITE0,
    ONEWIRE_CMD_WRITE1,
    ONEWIRE_CMD_PWRITE0,
    ONEWIRE_CMD_PWRITE1,
    ONEWIRE_CMD_READ,
    ONEWIRE_CMD_SEARCH_FIRST,
    ONEWIRE_CMD_SEARCH_NEXT,
    ONEWIRE_CMD_POWER,
    ONEWIRE_CMD_UNPOWER,
    ONEWIRE_CMD_WAIT,
    ONEWIRE_CMD_CALLBACK
} enumOneWireCmd;


typedef enum
{
    ONEWIRE_STATE_NONE = 0,
    ONEWIRE_STATE_BEGIN = 1,
    ONEWIRE_STATE_IN_PROGRESS = 2
} enumOneWireState;


typedef struct {
    size_t qsize;
    uint8_t *buf;
    uint8_t *bufend;
    uint8_t *bufrdptr;
    uint8_t *bufwrptr;
    enumOneWireState state;
    uint8_t presence;
    uint8_t data;
    int last_search_result;
    uint8_t last_search_rom[8];
} AsyncOneWireQueue;


class AsyncOneWire;
typedef void (*AsyncOneWireCallback)(AsyncOneWire *, void *);

class AsyncOneWire
{
    private:
        uint8_t pin;
        AsyncOneWireQueue *queue;
        enumOneWireCmd getNextCommand (void **data);
        int doReset (void);
        int doWrite (uint8_t bit);
        int doWriteAndPower (uint8_t bit);
        int doRead (void);
        int doSearch (uint8_t search_cmd);
        int doPower (void);
        int doUnpower (void);
        int doWait (void *data);
        int doCallback (void *data);
        virtual void onCommandFinished (enumOneWireCmd cmd);

    public:
        AsyncOneWire (uint8_t pin, size_t qsize);
        void begin (void);
        int beginTransaction (void);
        int reset (void);
        int writeBit (uint8_t bit);
        int writeBitAndPower (uint8_t bit);
        int readBit ();
        int write (uint8_t *data, size_t len);
        int write (uint8_t *data, size_t len, uint8_t power);
        int read (uint8_t *data, size_t len);
        int searchFirst (void);
        int searchNext (void);
        int power (void);
        int unpower (void);
        int wait (unsigned long us);
        int callback (AsyncOneWireCallback callback, void *param);
        int commit (void);
        int inProgress (void);

        uint8_t getData (void);
        int getLastSearchResult (uint8_t *rom);
        void handle (void);

        uint8_t crc (const uint8_t *addr, uint8_t len);
};

#endif // __ASYNCONEWIRE_H
