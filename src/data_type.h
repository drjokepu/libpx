//
//  data_type.h
//  libpx
//
//  Created by Tamas Czinege on 30/04/2012.
//  Copyright (c) 2012 Tamas Czinege. All rights reserved.
//

#ifndef libpx_data_type__h
#define libpx_data_type__h

typedef enum px_datatype
{
    px_data_type_bool              = 16,
    px_data_type_byte              = 17,
    px_data_type_char              = 18,
    px_data_type_name              = 19,
    px_data_type_int64             = 20,
    px_data_type_int16             = 21,
    px_data_type_int16a            = 22,
    px_data_type_int32             = 23,
    px_data_type_regproc           = 24,
    px_data_type_varcharu          = 25,
    px_data_type_oid               = 26,
    px_data_type_tid               = 27,
    px_data_type_xid               = 28,
    px_data_type_cid               = 29,
    px_data_type_oida              = 30,
    px_data_type_xml               = 142,
    px_data_type_nodetree          = 194,
    px_data_type_storagemanager    = 210,
    px_data_type_point             = 600,
    px_data_type_linesegment       = 601,
    px_data_type_box               = 603,
    px_data_type_polygon           = 604,
    px_data_type_line              = 628,
    px_data_type_single            = 700,
    px_data_type_double            = 701,
    px_data_type_datetime          = 702,
    px_data_type_timespan          = 703,
    px_data_type_timerange         = 704,
    px_data_type_circle            = 718,
    px_data_type_money             = 790,
    px_data_type_macaddress        = 829,
    px_data_type_inet              = 869,
    px_data_type_net               = 650,
    px_data_type_int16au           = 1005,
    px_data_type_int32a            = 1007,
    px_data_type_texta             = 1009,
    px_data_type_floata            = 1021,
    px_data_type_oidau             = 1028,
    px_data_type_acl               = 1033,
    px_data_type_acla              = 1034,
    px_data_type_cstring           = 1263,
    px_data_type_charn             = 1042,
    px_data_type_varcharn          = 1043,
    px_data_type_date              = 1082,
    px_data_type_time              = 1083,
    px_data_type_timestamp         = 1114,
    px_data_type_timestampz        = 1184,
    px_data_type_interval          = 1186,
    px_data_type_timez             = 1266,
    px_data_type_fixbitstring      = 1560,
    px_data_type_varbitstring      = 1562,
    px_data_type_numeric           = 1700,
    px_data_type_refcursor         = 2202,
    px_data_type_regop             = 2203,
    px_data_type_regopwitharg      = 2204,
    px_data_type_regclass          = 2205,
    px_data_type_regtype           = 2206,
    px_data_type_regtypea          = 2211,
    px_data_type_uuid              = 2950,
    // To be continued ...
} px_datatype;

#endif
