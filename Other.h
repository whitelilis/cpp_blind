#ifndef ROOTHEADERS_HPP_INCLUDED
#define ROOTHEADERS_HPP_INCLUDED

#ifdef __GNUC__
// Avoid tons of warnings with root code
#pragma GCC system_header
#endif

#include <fstream>
#include <iostream>
#include <vector>
#include <deque>
#include <set>
#include <string>
#include <sstream>

#include "IWCStrategy.h"
#include "time.h"
#include "stdio.h"

#include <longfist/LFDataStruct.h>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <longfist/LFDataStruct.h>
#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>

#endif
