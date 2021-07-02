// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef DB_STORE_LOG_H
#define DB_STORE_LOG_H

#include <errno.h>
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "common/dout.h" 

using namespace std;

#define dout_subsys ceph_subsys_rgw
#undef dout_prefix
#define dout_prefix *_dout << "rgw dbstore: "

#define dbout_prefix(cct, v) dout_impl(cct, dout_subsys, v) dout_prefix
#define dbendl_impl  dendl      

#define dbout(cct, v) dbout_prefix(cct, v)
#define dbendl dbendl_impl

#endif
