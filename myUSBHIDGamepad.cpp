// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// #define DEBUG
#if defined(DEBUG)
#include <Arduino.h>
#endif

#include "myUSBHID.h"

#if CONFIG_TINYUSB_HID_ENABLED

#include "myUSBHIDGamepad.h"

static const uint8_t report_descriptor[] = {
    TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(HID_REPORT_ID_GAMEPAD))
};

myUSBHIDGamepad::myUSBHIDGamepad(): hid(), _x(0), _y(0), _z(0), _rz(0), _rx(0), _ry(0), _hat(0), _buttons(0){
    static bool initialized = false;
    if(!initialized){
        initialized = true;
        hid.addDevice(this, sizeof(report_descriptor));
    }
}

uint16_t myUSBHIDGamepad::_onGetDescriptor(uint8_t* dst){
    memcpy(dst, report_descriptor, sizeof(report_descriptor));
    return sizeof(report_descriptor);
}

void myUSBHIDGamepad::begin(){
    hid.begin();
}

void myUSBHIDGamepad::end(){

}

bool myUSBHIDGamepad::write(){
    hid_gamepad_report_t report = {
        .x       = _x,
        .y       = _y,
        .z       = _z,
        .rz      = _rz,
        .rx      = _rx,
        .ry      = _ry,
        .hat     = _hat,
        .buttons = _buttons
    };
    return hid.SendReport(HID_REPORT_ID_GAMEPAD, &report, sizeof(report));
}

bool myUSBHIDGamepad::leftStick(int8_t x, int8_t y){
    _x = x;
    _y = y;
    return write();
}

bool myUSBHIDGamepad::rightStick(int8_t z, int8_t rz){
    _z = z;
    _rz = rz;
    return write();
}

bool myUSBHIDGamepad::leftTrigger(int8_t rx){
    _rx = rx;
    return write();
}

bool myUSBHIDGamepad::rightTrigger(int8_t ry){
    _ry = ry;
    return write();
}

bool myUSBHIDGamepad::hat(uint8_t hat){
    if(hat > 9){
        return false;
    }
    _hat = hat;
    return write();
}

bool myUSBHIDGamepad::pressButton(uint8_t button){
    if(button > 63){
        return false;
    }
    #ifdef DEBUG
    Serial.print("pressButton received: ");
    Serial.println(button);
    Serial.print("1 << button equals: ");
    Serial.println(1LL << button, HEX);
    Serial.print("_buttons starts as :");
    Serial.println(_buttons, HEX);
    #endif
    // _buttons |= (1 << button);
    _buttons |= (1LL << button);
    #ifdef DEBUG
    Serial.print("_buttons after |=: ");
    Serial.println(_buttons, HEX);
    #endif
    return write();
}

bool myUSBHIDGamepad::releaseButton(uint8_t button){
    if(button > 63){
        return false;
    }
    _buttons &= ~(1LL << button);
    return write();
}

bool myUSBHIDGamepad::send(int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint64_t buttons){
    if(hat > 9){
        return false;
    }
    _x = x;
    _y = y;
    _z = z;
    _rz = rz;
    _rx = rx;
    _ry = ry;
    _hat = hat;
    _buttons = buttons;
    return write();
}


#endif /* CONFIG_TINYUSB_HID_ENABLED */
