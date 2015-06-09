/* 
 * File:   AW_device_print.hxx
 * Author: aboeckma
 *
 * Created on November 13, 2012, 12:21 PM
 */

#pragma once

#include "aw_device.hxx"
#include "aw_device_cairo.hxx"

class AW_device_print : public AW_device_cairo { // derived from a Noncopyable
private:
    class Pimpl;
    Pimpl *prvt;

protected:
    cairo_t *get_cr(int gc) OVERRIDE;

public:
    AW_device_print(AW_common *common_);
    ~AW_device_print();

    GB_ERROR open(const char *path) __ATTR__USERESULT;
    void close();

    // AW_device interface:
    AW_DEVICE_TYPE type() OVERRIDE;

    void set_color_mode(bool mode);

};
