//
// Created by Dieter Baron on 22/01/31.
//

#ifndef HAD_OUTPUT_CONTEXT_HEADER_H
#define HAD_OUTPUT_CONTEXT_HEADER_H

#include "OutputContext.h"

class OutputContextHeader : public OutputContext {
  public:
    explicit OutputContextHeader() : header_set(false) { }

    bool close() override;
    bool game(GamePtr game) override { return false; };
    bool header(DatEntry *dat) override;

    const DatEntry &get_header() const { return header_data; }

  private:
    DatEntry header_data;
    bool header_set;
};


#endif // HAD_OUTPUT_CONTEXT_HEADER_H
