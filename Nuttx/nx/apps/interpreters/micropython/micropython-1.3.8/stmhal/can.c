/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "mpconfig.h"
#include "nlr.h"
#include "misc.h"
#include "qstr.h"
#include "obj.h"
#include "objtuple.h"
#include "runtime.h"
#include "bufhelper.h"
#include "can.h"
#include "pybioctl.h"
#include MICROPY_HAL_H

#if MICROPY_HW_ENABLE_CAN

#define MASK16 (0)
#define LIST16 (1)
#define MASK32 (2)
#define LIST32 (3)

/// \moduleref pyb
/// \class CAN - controller area network communication bus
///
/// CAN implements the standard CAN communications protocol.  At
/// the physical level it consists of 2 lines: RX and TX.  Note that
/// to connect the pyboard to a CAN bus you must use a CAN transceiver
/// to convert the CAN logic signals from the pyboard to the correct
/// voltage levels on the bus.
///
/// Note that this driver does not yet support filter configuration
/// (it defaults to a single filter that lets through all messages),
/// or bus timing configuration (except for setting the prescaler).
///
/// Example usage (works without anything connected):
///
///     from pyb import CAN
///     can = pyb.CAN(1, pyb.CAN.LOOPBACK)
///     can.send('message!', 123)   # send message with id 123
///     can.recv(0)                 # receive message on FIFO 0

typedef struct _pyb_can_obj_t {
    mp_obj_base_t base;
    mp_uint_t can_id : 8;
    bool is_enabled : 1;
    bool extframe : 1;
    CAN_HandleTypeDef can;
} pyb_can_obj_t;

STATIC uint8_t can2_start_bank = 14;

// assumes Init parameters have been set up correctly
STATIC bool can_init(pyb_can_obj_t *can_obj) {
    CAN_TypeDef *CANx = NULL;

    uint32_t GPIO_Pin = 0;
    uint8_t  GPIO_AF_CANx = 0;
    GPIO_TypeDef* GPIO_Port = NULL;

    switch (can_obj->can_id) {
        // CAN1 is on RX,TX = Y3,Y4 = PB9,PB9
        case PYB_CAN_1:
            CANx = CAN1;
            GPIO_AF_CANx = GPIO_AF9_CAN1;
            GPIO_Port = GPIOB;
            GPIO_Pin = GPIO_PIN_8 | GPIO_PIN_9;
            __CAN1_CLK_ENABLE();
            break;

        // CAN2 is on RX,TX = Y5,Y6 = PB12,PB13
        case PYB_CAN_2:
            CANx = CAN2;
            GPIO_AF_CANx = GPIO_AF9_CAN2;
            GPIO_Port = GPIOB;
            GPIO_Pin = GPIO_PIN_12 | GPIO_PIN_13;
            __CAN1_CLK_ENABLE(); // CAN2 is a "slave" and needs CAN1 enabled as well
            __CAN2_CLK_ENABLE();
            break;

        default:
            return false;
    }

    // init GPIO
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.Pin = GPIO_Pin;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Alternate = GPIO_AF_CANx;
    HAL_GPIO_Init(GPIO_Port, &GPIO_InitStructure);

    // init CANx
    can_obj->can.Instance = CANx;
    HAL_CAN_Init(&can_obj->can);

    can_obj->is_enabled = true;

    return true;
}

STATIC void can_deinit(pyb_can_obj_t *can_obj) {
    can_obj->is_enabled = false;
    CAN_HandleTypeDef *can = &can_obj->can;
    HAL_CAN_DeInit(can);
    if (can->Instance == CAN1) {
        __CAN1_FORCE_RESET();
        __CAN1_RELEASE_RESET();
        __CAN1_CLK_DISABLE();
    } else if (can->Instance == CAN2) {
        __CAN2_FORCE_RESET();
        __CAN2_RELEASE_RESET();
        __CAN2_CLK_DISABLE();
    }
}

STATIC void can_clearfilter(uint32_t f) {
    CAN_FilterConfTypeDef filter;

    filter.FilterIdHigh         = 0;
    filter.FilterIdLow          = 0;
    filter.FilterMaskIdHigh     = 0;
    filter.FilterMaskIdLow      = 0;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterNumber         = f;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterScale          = CAN_FILTERSCALE_16BIT;
    filter.FilterActivation     = DISABLE;
    filter.BankNumber           = can2_start_bank;

    HAL_CAN_ConfigFilter(NULL, &filter);
}

/******************************************************************************/
// Micro Python bindings

STATIC void pyb_can_print(void (*print)(void *env, const char *fmt, ...), void *env, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_can_obj_t *self = self_in;
    if (!self->is_enabled) {
        print(env, "CAN(%u)", self->can_id);
    } else {
        print(env, "CAN(%u, CAN.", self->can_id);
        qstr mode;
        switch (self->can.Init.Mode) {
            case CAN_MODE_NORMAL: mode = MP_QSTR_NORMAL; break;
            case CAN_MODE_LOOPBACK: mode = MP_QSTR_LOOPBACK; break;
            case CAN_MODE_SILENT: mode = MP_QSTR_SILENT; break;
            case CAN_MODE_SILENT_LOOPBACK: default: mode = MP_QSTR_SILENT_LOOPBACK; break;
        }
        print(env, "%s, extframe=", qstr_str(mode));
        if (self->extframe) {
            mode = MP_QSTR_True;
        } else {
            mode = MP_QSTR_False;
        }
        print(env, "%s)", qstr_str(mode));
    }
}

// init(mode, extframe=False, prescaler=100, *, sjw=1, bs1=6, bs2=8)
STATIC mp_obj_t pyb_can_init_helper(pyb_can_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,         MP_ARG_REQUIRED | MP_ARG_INT,   {.u_int  = CAN_MODE_NORMAL} },
        { MP_QSTR_extframe,     MP_ARG_BOOL,                    {.u_bool = false} },
        { MP_QSTR_prescaler,    MP_ARG_INT,                     {.u_int  = 100} },
        { MP_QSTR_sjw,          MP_ARG_KW_ONLY | MP_ARG_INT,    {.u_int = 1} },
        { MP_QSTR_bs1,          MP_ARG_KW_ONLY | MP_ARG_INT,    {.u_int = 6} },
        { MP_QSTR_bs2,          MP_ARG_KW_ONLY | MP_ARG_INT,    {.u_int = 8} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->extframe = args[1].u_bool;

    // set the CAN configuration values
    memset(&self->can, 0, sizeof(self->can));
    CAN_InitTypeDef *init = &self->can.Init;
    init->Mode = args[0].u_int << 4; // shift-left so modes fit in a small-int
    init->Prescaler = args[2].u_int;
    init->SJW = ((args[3].u_int - 1) & 3) << 24;
    init->BS1 = ((args[4].u_int - 1) & 0xf) << 16;
    init->BS2 = ((args[5].u_int - 1) & 7) << 20;
    init->TTCM = DISABLE;
    init->ABOM = DISABLE;
    init->AWUM = DISABLE;
    init->NART = DISABLE;
    init->RFLM = DISABLE;
    init->TXFP = DISABLE;

    // init CAN (if it fails, it's because the port doesn't exist)
    if (!can_init(self)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "CAN port %d does not exist", self->can_id));
    }

    return mp_const_none;
}

/// \classmethod \constructor(bus, ...)
///
/// Construct a CAN object on the given bus.  `bus` can be 1-2, or 'YA' or 'YB'.
/// With no additional parameters, the CAN object is created but not
/// initialised (it has the settings from the last initialisation of
/// the bus, if any).  If extra arguments are given, the bus is initialised.
/// See `init` for parameters of initialisation.
///
/// The physical pins of the CAN busses are:
///
///   - `CAN(1)` is on `YA`: `(RX, TX) = (Y3, Y4) = (PB8, PB9)`
///   - `CAN(2)` is on `YB`: `(RX, TX) = (Y5, Y6) = (PB12, PB13)`
STATIC mp_obj_t pyb_can_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // create object
    pyb_can_obj_t *o = m_new_obj(pyb_can_obj_t);
    o->base.type = &pyb_can_type;
    o->is_enabled = false;

    // work out port
    o->can_id = 0;
    if (MP_OBJ_IS_STR(args[0])) {
        const char *port = mp_obj_str_get_str(args[0]);
        if (0) {
        #if defined(PYBV10)
        } else if (strcmp(port, "YA") == 0) {
            o->can_id = PYB_CAN_YA;
        } else if (strcmp(port, "YB") == 0) {
            o->can_id = PYB_CAN_YB;
        #endif
        } else {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "CAN port %s does not exist", port));
        }
    } else {
        o->can_id = mp_obj_get_int(args[0]);
    }

    if (n_args > 1 || n_kw > 0) {
        // start the peripheral
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        pyb_can_init_helper(o, n_args - 1, args + 1, &kw_args);
    }

    return o;
}

STATIC mp_obj_t pyb_can_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_can_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_init_obj, 1, pyb_can_init);

/// \method deinit()
/// Turn off the CAN bus.
STATIC mp_obj_t pyb_can_deinit(mp_obj_t self_in) {
    pyb_can_obj_t *self = self_in;
    can_deinit(self);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_can_deinit_obj, pyb_can_deinit);

/// \method any(fifo)
/// Return `True` if any message waiting on the FIFO, else `False`.
STATIC mp_obj_t pyb_can_any(mp_obj_t self_in, mp_obj_t fifo_in) {
    pyb_can_obj_t *self = self_in;
    mp_int_t fifo = mp_obj_get_int(fifo_in);
    if (fifo == 0) {
        if (__HAL_CAN_MSG_PENDING(&self->can, CAN_FIFO0) != 0) {
            return mp_const_true;
        }
    } else {
        if (__HAL_CAN_MSG_PENDING(&self->can, CAN_FIFO1) != 0) {
            return mp_const_true;
        }
    }
    return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_can_any_obj, pyb_can_any);

/// \method send(send, addr, *, timeout=5000)
/// Send a message on the bus:
///
///   - `send` is the data to send (an integer to send, or a buffer object).
///   - `addr` is the address to send to
///   - `timeout` is the timeout in milliseconds to wait for the send.
///
/// Return value: `None`.
STATIC mp_obj_t pyb_can_send(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_send,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    pyb_can_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[0].u_obj, &bufinfo, data);

    if (bufinfo.len > 8) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "CAN data field too long"));
    }

    // send the data
    CanTxMsgTypeDef tx_msg;
    if (self->extframe){
        tx_msg.ExtId = args[1].u_int & 0x1FFFFFFF;
        tx_msg.IDE = CAN_ID_EXT;
    } else {
        tx_msg.StdId = args[1].u_int & 0x7FF;
        tx_msg.IDE = CAN_ID_STD;
    }
    tx_msg.RTR = CAN_RTR_DATA;
    tx_msg.DLC = bufinfo.len;
    for (mp_uint_t i = 0; i < bufinfo.len; i++) {
        tx_msg.Data[i] = ((byte*)bufinfo.buf)[i]; // Data is uint32_t but holds only 1 byte
    }
    self->can.pTxMsg = &tx_msg;
    HAL_StatusTypeDef status = HAL_CAN_Transmit(&self->can, args[2].u_int);

    if (status != HAL_OK) {
        mp_hal_raise(status);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_send_obj, 1, pyb_can_send);

/// \method recv(fifo, *, timeout=5000)
///
/// Receive data on the bus:
///
///   - `fifo` is an integer, which is the FIFO to receive on
///   - `timeout` is the timeout in milliseconds to wait for the receive.
///
/// Return value: buffer of data bytes.
STATIC mp_obj_t pyb_can_recv(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_fifo,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    pyb_can_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // receive the data
    CanRxMsgTypeDef rx_msg;
    self->can.pRxMsg = &rx_msg;
    HAL_StatusTypeDef status = HAL_CAN_Receive(&self->can, args[0].u_int, args[1].u_int);

    if (status != HAL_OK) {
        mp_hal_raise(status);
    }

    // return the received data
    // TODO use a namedtuple (when namedtuple types can be stored in ROM)
    mp_obj_tuple_t *tuple = mp_obj_new_tuple(4, NULL);
    if (rx_msg.IDE == CAN_ID_STD) {
        tuple->items[0] = MP_OBJ_NEW_SMALL_INT(rx_msg.StdId);
    } else {
        tuple->items[0] = MP_OBJ_NEW_SMALL_INT(rx_msg.ExtId);
    }
    tuple->items[1] = MP_OBJ_NEW_SMALL_INT(rx_msg.RTR);
    tuple->items[2] = MP_OBJ_NEW_SMALL_INT(rx_msg.FMI);
    byte *data;
    tuple->items[3] = mp_obj_str_builder_start(&mp_type_bytes, rx_msg.DLC, &data);
    for (mp_uint_t i = 0; i < rx_msg.DLC; i++) {
        data[i] = rx_msg.Data[i]; // Data is uint32_t but holds only 1 byte
    }
    tuple->items[3] = mp_obj_str_builder_end(tuple->items[3]);
    return tuple;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_recv_obj, 1, pyb_can_recv);

/// \class method initfilterbanks
///
/// Set up the filterbanks. All filter will be disabled and set to their reset states.
///
///   - `banks` is an integer that sets how many filter banks that are reserved for CAN1.
///     0  -> no filters assigned for CAN1
///     28 -> all filters are assigned to CAN1
///     CAN2 will get the rest of the 28 available.
///
/// Return value: none.
STATIC mp_obj_t pyb_can_initfilterbanks(mp_obj_t self, mp_obj_t bank_in) {
    can2_start_bank = mp_obj_get_int(bank_in);

    for (int f = 0; f < 28; f++) {
        can_clearfilter(f);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_can_initfilterbanks_fun_obj, pyb_can_initfilterbanks);
STATIC MP_DEFINE_CONST_CLASSMETHOD_OBJ(pyb_can_initfilterbanks_obj, (const mp_obj_t)&pyb_can_initfilterbanks_fun_obj);

STATIC mp_obj_t pyb_can_clearfilter(mp_obj_t self_in, mp_obj_t bank_in) {
    pyb_can_obj_t *self = self_in;
    mp_int_t f = mp_obj_get_int(bank_in);
    if (self->can_id == 2) {
        f += can2_start_bank;
    }
    can_clearfilter(f);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_can_clearfilter_obj, pyb_can_clearfilter);

/// Configures a filterbank
/// Return value: `None`.
#define EXTENDED_ID_TO_16BIT_FILTER(id) (((id & 0xC00000) >> 13) | ((id & 0x38000) >> 15)) | 8
STATIC mp_obj_t pyb_can_setfilter(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_bank,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_mode,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_fifo,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = CAN_FILTER_FIFO0} },
        { MP_QSTR_params,   MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };

    // parse args
    pyb_can_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_uint_t len;
    mp_obj_t *params;
    mp_obj_get_array(args[3].u_obj, &len, &params);

    CAN_FilterConfTypeDef filter;
    if (args[1].u_int == MASK16 || args[1].u_int == LIST16) {
        if (len != 4) {
            goto error;
        }
        filter.FilterScale = CAN_FILTERSCALE_16BIT;
        if (self->extframe) {
            filter.FilterIdLow      = EXTENDED_ID_TO_16BIT_FILTER(mp_obj_get_int(params[0])); // id1
            filter.FilterMaskIdLow  = EXTENDED_ID_TO_16BIT_FILTER(mp_obj_get_int(params[1])); // mask1
            filter.FilterIdHigh     = EXTENDED_ID_TO_16BIT_FILTER(mp_obj_get_int(params[2])); // id2
            filter.FilterMaskIdHigh = EXTENDED_ID_TO_16BIT_FILTER(mp_obj_get_int(params[3])); // mask2
        } else {
            filter.FilterIdLow      = mp_obj_get_int(params[0]) << 5; // id1
            filter.FilterMaskIdLow  = mp_obj_get_int(params[1]) << 5; // mask1
            filter.FilterIdHigh     = mp_obj_get_int(params[2]) << 5; // id2
            filter.FilterMaskIdHigh = mp_obj_get_int(params[3]) << 5; // mask2
        }
        if (args[1].u_int == MASK16) {
            filter.FilterMode  = CAN_FILTERMODE_IDMASK;
        }
        if (args[1].u_int == LIST16) {
            filter.FilterMode  = CAN_FILTERMODE_IDLIST;
        }
    }
    else if (args[1].u_int == MASK32 || args[1].u_int == LIST32) {
        if (len != 2) {
            goto error;
        }
        filter.FilterScale      = CAN_FILTERSCALE_32BIT;

        filter.FilterIdHigh     = (mp_obj_get_int(params[0]) & 0xFF00)  >> 13;
        filter.FilterIdLow      = ((mp_obj_get_int(params[0]) & 0x00FF) << 3) | 4;
        filter.FilterMaskIdHigh = (mp_obj_get_int(params[1]) & 0xFF00 ) >> 13;
        filter.FilterMaskIdLow  = ((mp_obj_get_int(params[1]) & 0x00FF) << 3) | 4;
        if (args[1].u_int == MASK32) {
            filter.FilterMode  = CAN_FILTERMODE_IDMASK;
        }
        if (args[1].u_int == LIST32) {
            filter.FilterMode  = CAN_FILTERMODE_IDLIST;
        }
    } else {
        goto error;
    }

    filter.FilterFIFOAssignment = args[2].u_int; // fifo
    filter.FilterNumber = args[0].u_int; // bank
    if (self->can_id == 1) {
        if (filter.FilterNumber >= can2_start_bank) {
            goto error;
        }
    } else {
        filter.FilterNumber = filter.FilterNumber + can2_start_bank;
        if (filter.FilterNumber > 27) {
            goto error;
        }
    }
    filter.FilterActivation = ENABLE;
    filter.BankNumber = can2_start_bank;
    HAL_CAN_ConfigFilter(&self->can, &filter);

    return mp_const_none;

error:
    nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "CAN filter parameter error"));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_setfilter_obj, 1, pyb_can_setfilter);

STATIC const mp_map_elem_t pyb_can_locals_dict_table[] = {
    // instance methods
    { MP_OBJ_NEW_QSTR(MP_QSTR_init), (mp_obj_t)&pyb_can_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_deinit), (mp_obj_t)&pyb_can_deinit_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_any), (mp_obj_t)&pyb_can_any_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_send), (mp_obj_t)&pyb_can_send_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_recv), (mp_obj_t)&pyb_can_recv_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_initfilterbanks), (mp_obj_t)&pyb_can_initfilterbanks_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_setfilter), (mp_obj_t)&pyb_can_setfilter_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_clearfilter), (mp_obj_t)&pyb_can_clearfilter_obj },

    // class constants
    // Note: we use the ST constants >> 4 so they fit in a small-int.  The
    // right-shift is undone when the constants are used in the init function.
    { MP_OBJ_NEW_QSTR(MP_QSTR_NORMAL), MP_OBJ_NEW_SMALL_INT(CAN_MODE_NORMAL >> 4) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_LOOPBACK), MP_OBJ_NEW_SMALL_INT(CAN_MODE_LOOPBACK >> 4) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SILENT), MP_OBJ_NEW_SMALL_INT(CAN_MODE_SILENT >> 4) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SILENT_LOOPBACK), MP_OBJ_NEW_SMALL_INT(CAN_MODE_SILENT_LOOPBACK >> 4) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_MASK16), MP_OBJ_NEW_SMALL_INT(MASK16) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_LIST16), MP_OBJ_NEW_SMALL_INT(LIST16) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MASK32), MP_OBJ_NEW_SMALL_INT(MASK32) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_LIST32), MP_OBJ_NEW_SMALL_INT(LIST32) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_can_locals_dict, pyb_can_locals_dict_table);

mp_uint_t can_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    pyb_can_obj_t *self = self_in;
    mp_uint_t ret;
    if (request == MP_IOCTL_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_IOCTL_POLL_RD)
            && ((__HAL_CAN_MSG_PENDING(&self->can, CAN_FIFO0) != 0)
                || (__HAL_CAN_MSG_PENDING(&self->can, CAN_FIFO1) != 0))) {
            ret |= MP_IOCTL_POLL_RD;
        }
        if ((flags & MP_IOCTL_POLL_WR) && (self->can.Instance->TSR & CAN_TSR_TME)) {
            ret |= MP_IOCTL_POLL_WR;
        }
    } else {
        *errcode = EINVAL;
        ret = -1;
    }
    return ret;
}

STATIC const mp_stream_p_t can_stream_p = {
    //.read = can_read, // is read sensible for CAN?
    //.write = can_write, // is write sensible for CAN?
    .ioctl = can_ioctl,
    .is_text = false,
};

const mp_obj_type_t pyb_can_type = {
    { &mp_type_type },
    .name = MP_QSTR_CAN,
    .print = pyb_can_print,
    .make_new = pyb_can_make_new,
    .stream_p = &can_stream_p,
    .locals_dict = (mp_obj_t)&pyb_can_locals_dict,
};

#endif // MICROPY_HW_ENABLE_CAN
