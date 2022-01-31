//
// Created by Dieter Baron on 22/01/31.
//

#include "OutputContextHeader.h"

bool OutputContextHeader::close() {
    // TODO: error: no header
    return header_set;
}


bool OutputContextHeader::header(DatEntry *dat) {
    if (header_set) {
	// TODO: error: duplicate header
	return false;
    }
    header_set = true;
    header_data = *dat;
}
