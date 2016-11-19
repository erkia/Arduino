#include "AsyncOneWire.h"


static AsyncOneWireQueue queueData;


static void readCb (AsyncOneWire *obj, void *data)
{
    *((uint8_t *)data) = obj->getData ();
}


AsyncOneWire::AsyncOneWire (uint8_t pin, size_t qsize)
{
    this->pin = pin;
    queue = &queueData;
    queue->qsize = qsize;
    queue->buf = NULL;
}


void AsyncOneWire::begin (void)
{
    if (queue->buf == NULL) {
        queue->buf = (uint8_t *)malloc (queue->qsize);
        if (queue->buf != NULL) {
            queue->bufend = queue->buf + queue->qsize - 1;
        }
    }
}


int AsyncOneWire::beginTransaction (void)
{
    if (queue->state != ONEWIRE_STATE_NONE) {
        return -1;
    }

    queue->presence = 0;
    queue->data = 0;
    queue->bufrdptr = queue->bufwrptr = queue->buf;
    queue->last_search_result = -1;
    queue->state = ONEWIRE_STATE_BEGIN;

    return 0;
}


int AsyncOneWire::reset (void)
{
    if (queue->state != ONEWIRE_STATE_BEGIN || queue->bufwrptr >= queue->bufend) {
        return -1;
    }

    *(queue->bufwrptr++) = ONEWIRE_CMD_RESET;
    
    return 0;
}


int AsyncOneWire::writeBit (uint8_t bit)
{
    if (queue->state != ONEWIRE_STATE_BEGIN || queue->bufwrptr >= queue->bufend) {
        return -1;
    }

    if (bit) {
        *(queue->bufwrptr++) = ONEWIRE_CMD_WRITE1;
    } else {
        *(queue->bufwrptr++) = ONEWIRE_CMD_WRITE0;
    }
    
    return 0;
}


int AsyncOneWire::writeBitAndPower (uint8_t bit)
{
    if (queue->state != ONEWIRE_STATE_BEGIN || queue->bufwrptr >= queue->bufend) {
        return -1;
    }

    if (bit) {
        *(queue->bufwrptr++) = ONEWIRE_CMD_PWRITE1;
    } else {
        *(queue->bufwrptr++) = ONEWIRE_CMD_PWRITE0;
    }
    
    return 0;
}


int AsyncOneWire::readBit (void)
{
    if (queue->state != ONEWIRE_STATE_BEGIN || queue->bufwrptr >= queue->bufend) {
        return -1;
    }

    *(queue->bufwrptr++) = ONEWIRE_CMD_READ;
    
    return 0;
}


int AsyncOneWire::write (uint8_t *data, size_t len)
{
    size_t i, j;

    for (i = 0; i < len; i++) {
        for (j = 0; j < 8; j++) {
            if (writeBit ((data[i] >> j) & 0x01) == -1) {
                return -1;
            }
        }
    }
    
    return 0;
}


int AsyncOneWire::write (uint8_t *data, size_t len, uint8_t power)
{
    size_t i, j;

    for (i = 0; i < len; i++) {
        for (j = 0; j < 8; j++) {
            if ((i == (len - 1)) && (j == 7)) {
                if (writeBitAndPower ((data[i] >> j) & 0x01) == -1) {
                    return -1;
                }
            } else {
                if (writeBit ((data[i] >> j) & 0x01) == -1) {
                    return -1;
                }
            }
        }
    }
    
    return 0;
}


int AsyncOneWire::read (uint8_t *data, size_t len)
{
    size_t i, j;

    for (i = 0; i < len; i++) {
        for (j = 0; j < 8; j++) {
            if (readBit () == -1) {
                return -1;
            }
        }
        if (callback (readCb, &data[i]) == -1) {
            return -1;
        }
    }
    
    return 0;
}


int AsyncOneWire::searchFirst (void)
{
    if (queue->state != ONEWIRE_STATE_BEGIN || queue->bufwrptr >= queue->bufend) {
        return -1;
    }

    *(queue->bufwrptr++) = ONEWIRE_CMD_SEARCH_FIRST;
    
    return 0;
}


int AsyncOneWire::searchNext (void)
{
    if (queue->state != ONEWIRE_STATE_BEGIN || queue->bufwrptr >= queue->bufend) {
        return -1;
    }

    *(queue->bufwrptr++) = ONEWIRE_CMD_SEARCH_NEXT;

    return 0;
}


int AsyncOneWire::power (void)
{
    if (queue->state != ONEWIRE_STATE_BEGIN || queue->bufwrptr >= queue->bufend) {
        return -1;
    }

    *(queue->bufwrptr++) = ONEWIRE_CMD_POWER;

    return 0;
}


int AsyncOneWire::unpower (void)
{
    if (queue->state != ONEWIRE_STATE_BEGIN || queue->bufwrptr >= queue->bufend) {
        return -1;
    }

    *(queue->bufwrptr++) = ONEWIRE_CMD_UNPOWER;

    return 0;
}


int AsyncOneWire::wait (unsigned long us)
{
    if (queue->state != ONEWIRE_STATE_BEGIN || (queue->bufwrptr + sizeof (unsigned long)) >= queue->bufend) {
        return -1;
    }

    *(queue->bufwrptr++) = ONEWIRE_CMD_WAIT;
    memcpy (queue->bufwrptr, &us, sizeof (unsigned long));
    queue->bufwrptr += sizeof (unsigned long);

    return 0;
}


int AsyncOneWire::callback (AsyncOneWireCallback callback, void *param)
{
    if (queue->state != ONEWIRE_STATE_BEGIN || (queue->bufwrptr + sizeof (AsyncOneWireCallback) + sizeof (void *)) >= queue->bufend) {
        return -1;
    }

    *(queue->bufwrptr++) = ONEWIRE_CMD_CALLBACK;
    memcpy (queue->bufwrptr, &callback, sizeof (AsyncOneWireCallback));
    queue->bufwrptr += sizeof (AsyncOneWireCallback);
    memcpy (queue->bufwrptr, &param, sizeof (void *));
    queue->bufwrptr += sizeof (void *);

    return 0;
}


int AsyncOneWire::commit (void)
{
    if (queue->state != ONEWIRE_STATE_BEGIN) {
        return -1;
    }

    queue->state = ONEWIRE_STATE_IN_PROGRESS;

    return 0;
}


int AsyncOneWire::inProgress (void)
{
    return queue->state;
}


int AsyncOneWire::doReset (void)
{
    static unsigned long resetTxStart = 0;
    static unsigned long resetRxStart = 0;
    unsigned long now = micros ();

    if (resetTxStart == 0) {

    	ONEWIRE_SET_LOW ();
        resetTxStart = micros ();

    } else {

        if (resetRxStart == 0) {

            if ((now - resetTxStart) > 480) {
            	ONEWIRE_RELEASE ();
                resetRxStart = micros ();
            	delayMicroseconds (65); // 60us - 240us
                queue->presence = ONEWIRE_READ () ? 0 : 1;
            }

        } else {

            if ((now - resetRxStart) > 480) {
                resetTxStart = 0;
                resetRxStart = 0;
            }

        }

    }

    return (resetTxStart != 0);
}


int AsyncOneWire::doWrite (uint8_t bit)
{
	ONEWIRE_SET_LOW ();

    if (bit) {
    	delayMicroseconds (2); // 1us - 15us
    	ONEWIRE_RELEASE ();
    	delayMicroseconds (63);
    } else {
    	delayMicroseconds (63); // 60us - 120us
    	ONEWIRE_RELEASE ();
    	delayMicroseconds (2);
    }

    return 0;
}


int AsyncOneWire::doWriteAndPower (uint8_t bit)
{
	ONEWIRE_SET_LOW ();

    if (bit) {
    	delayMicroseconds (2); // 1us - 15us
    	ONEWIRE_SET_HIGH ();
    	delayMicroseconds (63);
    } else {
    	delayMicroseconds (63); // 60us - 120us
    	ONEWIRE_SET_HIGH ();
    	delayMicroseconds (2);
    }

    return 0;
}


int AsyncOneWire::doRead (void)
{
	ONEWIRE_SET_LOW ();
	delayMicroseconds (2); // >1us
	ONEWIRE_RELEASE ();
	delayMicroseconds (5); // 1us - 15us from pull
    queue->data >>= 1;
	queue->data |= (ONEWIRE_READ () ? 0x80 : 0);
	delayMicroseconds (58);

    return 0;
}


int AsyncOneWire::doSearch (uint8_t search_cmd)
{
    static int8_t step = -1;
    static uint8_t cmd = 0xF0;
    static uint8_t cmdpos;
    static uint8_t bit, bit_comp, bit_nr, dir;
    static uint8_t rom_byte, rom_mask;
    static uint8_t last_zero;
    static uint8_t last_discrepancy = 0;
    static uint8_t last_family_discrepancy = 0;
    static uint8_t last_device = 0;
    static uint8_t *rom = queue->last_search_rom;
    int search_result = 0;
    int res, i;

    if (search_cmd) {
        if (search_cmd == 0xF0 || search_cmd == 0xEC) {
            cmd = search_cmd;
            last_discrepancy = 0;
            last_device = 0;
            last_family_discrepancy = 0;
            for (i = 0; i < 8; i++) {
                rom[i] = 0;
            }
            step = 0;
        } else {
            step = -1;
        }
    } else {
        if (step == -1 && cmd && !last_device) {
            step = 0;
        }
    }

    queue->last_search_result = -1;

    switch (step) {

        case 0:
            
            cmdpos = 0;
            bit_nr = 1;
            rom_byte = 0;
            rom_mask = 1;
            last_zero = 0;
            step++;
            // no break

            
        case 1: // Step 1 - reset

            res = doReset ();
            if (res == 0) {
                if (!queue->presence) {
                    step = -1;
                    break;
                }
                step++;
            }
            break;

        case 2: // Step 2 - write command

            res = doWrite ((cmd >> cmdpos) & 0x01);
            if (res == 0) {
                cmdpos++;
                if (cmdpos == 8) {
                    step++;
                }
            }
            break;

        case 3: // Step 3 - read bit

            res = doRead ();
            if (res == 0) {
                bit = (getData () ? 1 : 0);
                step++;
            }
            break;

        case 4: // Step 4 - read complement of the bit

            res = doRead ();

            if (res == 0) {

                bit_comp = (getData () ? 1 : 0);

                // No devices left in the search
                if (bit && bit_comp) {
                    step = -1;
                    break;
                }

                // There are only 0-s or only 1-s as the next bit
                if (bit != bit_comp) {

                   dir = bit;  // bit write value for search

                } else {

                    // if this discrepancy is before the last_discrepancy
                    // on a previous next then pick the same as last time
                    if (bit_nr < last_discrepancy) {
                        dir = ((rom[rom_byte] & rom_mask) > 0);
                    } else {
                        // if equal to last pick 1, if not then pick 0
                        dir = (bit_nr == last_discrepancy);
                    }
                    
                    // if 0 was picked then record its position in last_zero
                    if (dir == 0) {
                        // check for Last discrepancy in family
                        last_zero = bit_nr;
                        if (last_zero < 9) {
                            last_family_discrepancy = last_zero;
                        }
                    }
                }

                // set or clear the bit in the ROM byte rom_byte with mask rom_mask
                if (dir == 1) {
                    rom[rom_byte] |= rom_mask;
                } else {
                    rom[rom_byte] &= ~rom_mask;
                }

                // increment the bit_nr and shift the rom mask
                bit_nr++;
                rom_mask <<= 1;

                // if the mask is 0 then switch to the next rom byte and reset rom mask
                if (rom_mask == 0) {
                    rom_byte++;
                    rom_mask = 1;
                }

                step++;
            }
            break;

        case 5: // Step 5 - write bit

            res = doWrite (dir);

            if (res == 0) {

                if (rom_byte < 8) {
                    step = 3;
                    break;
                }

                // if the search was successful
                if (bit_nr >= 64) {

                    // search successful so set last_discrepancy,last_device,search_result
                    last_discrepancy = last_zero;

                    // check for last device
                    if (last_discrepancy == 0) {
                        last_device = 1;
                    }

                    search_result = 1;
                
                }

                step++;

            }
            // no break
            
        case 6: // Step 6 - conclusions

            if (search_result && rom[0]) {
                queue->last_search_result = 0;
            }

            step = -1;
            break;
    }
    
    return (step != -1);
}


int AsyncOneWire::doPower (void)
{
	ONEWIRE_SET_HIGH ();
    return 0;
}


int AsyncOneWire::doUnpower (void)
{
	ONEWIRE_RELEASE ();
    return 0;
}


int AsyncOneWire::doWait (void *data)
{
    static unsigned long waitStart = 0;
    static unsigned long waitTime = 0;
    unsigned long now = micros ();

    if (waitStart == 0) {
        if (data) {
            waitStart = now;
            memcpy (&waitTime, data, sizeof (unsigned long ));
        }
    } else {
        if ((now - waitStart) >= waitTime) {
            waitStart = 0;
        }
    }

    return (waitStart != 0);
}


int AsyncOneWire::doCallback (void *data)
{
    AsyncOneWireCallback callback;
    void *param;
    
    if (data) {
        memcpy (&callback, data, sizeof (AsyncOneWireCallback));
        memcpy (&param, (void *)((uintptr_t)data + sizeof (AsyncOneWireCallback)), sizeof (void *));
        callback (this, param);
    }

    return 0;
}


uint8_t AsyncOneWire::getData (void)
{
    uint8_t res = queue->data;
    queue->data = 0;
    return res;
}


int AsyncOneWire::getLastSearchResult (uint8_t *rom)
{
    if (queue->last_search_result == 0) {
        memcpy (rom, queue->last_search_rom, 8);
    }

    return queue->last_search_result;
}


enumOneWireCmd AsyncOneWire::getNextCommand (void **data)
{
    enumOneWireCmd cmd;

    if (queue->state == ONEWIRE_STATE_IN_PROGRESS) {

        if (queue->bufrdptr < queue->bufwrptr) {

            cmd = (enumOneWireCmd)(*(queue->bufrdptr++));
            if (cmd == ONEWIRE_CMD_WAIT) {
                *data = queue->bufrdptr;
                if ((queue->bufrdptr + sizeof (unsigned long)) <= queue->bufwrptr) {
                    queue->bufrdptr += sizeof (unsigned long);
                } else {
                    cmd = ONEWIRE_CMD_NONE;
                }
            } else if (cmd == ONEWIRE_CMD_CALLBACK) {
                *data = queue->bufrdptr;
                if ((queue->bufrdptr + sizeof (AsyncOneWireCallback) + sizeof (void *)) <= queue->bufwrptr) {
                    queue->bufrdptr += sizeof (AsyncOneWireCallback) + sizeof (void *);
                } else {
                    cmd = ONEWIRE_CMD_NONE;
                }
            } else {
                *data = NULL;
            }

            return cmd;

        } else {

            queue->state = ONEWIRE_STATE_NONE;

        }

    }

    return ONEWIRE_CMD_NONE;
}


void AsyncOneWire::onCommandFinished (enumOneWireCmd cmd)
{
}


void AsyncOneWire::handle (void)
{
    static enumOneWireCmd cmd = ONEWIRE_CMD_NONE;
    int status;
    void *data = NULL;

    if (cmd == ONEWIRE_CMD_NONE) {
        cmd = getNextCommand (&data);
    }

    if (cmd != ONEWIRE_CMD_NONE) {

        switch (cmd) {
            case ONEWIRE_CMD_RESET:
                status = doReset ();
                break;
            case ONEWIRE_CMD_WRITE0:
                status = doWrite (0);
                break;
            case ONEWIRE_CMD_WRITE1:
                status = doWrite (1);
                break;
            case ONEWIRE_CMD_PWRITE0:
                status = doWriteAndPower (0);
                break;
            case ONEWIRE_CMD_PWRITE1:
                status = doWriteAndPower (1);
                break;
            case ONEWIRE_CMD_READ:
                status = doRead ();
                break;
            case ONEWIRE_CMD_SEARCH_FIRST:
                status = doSearch (0xF0);
                cmd = ONEWIRE_CMD_SEARCH_NEXT;
                break;
            case ONEWIRE_CMD_SEARCH_NEXT:
                status = doSearch (0x00);
                break;
            case ONEWIRE_CMD_POWER:
                status = doPower ();
                break;
            case ONEWIRE_CMD_UNPOWER:
                status = doUnpower ();
                break;
            case ONEWIRE_CMD_WAIT:
                status = doWait (data);
                break;
            case ONEWIRE_CMD_CALLBACK:
                status = doCallback (data);
                break;
            case ONEWIRE_CMD_NONE:
                break;
        }

        if (status == 0) {
            onCommandFinished (cmd);
            cmd = ONEWIRE_CMD_NONE;
        }

    }
}


uint8_t AsyncOneWire::crc (const uint8_t *data, uint8_t len)
{
    static const uint8_t table[] = {
          0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
        157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
         35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
        190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
         70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
        219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
        101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
        248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
        140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
         17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
        175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
         50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
        202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
         87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
        233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
        116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
    };
	uint8_t crc = 0;

	while (len--) {
		crc = table[crc ^ *data++];
	}

	return crc;
}

