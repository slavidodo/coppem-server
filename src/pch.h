

#ifndef _PCH_DEFINED
#define _PCH_DEFINED

#include <iostream>
#include <stdio.h>

#include <assert.h>
#include <string>
#include <algorithm>
#include <cmath>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <list>

#include <lua.hpp>
#include <boost\utility.hpp>
#include <boost\asio.hpp>

#define LIB

#ifdef _MSC_VER
#ifdef NDEBUG
#define _SECURE_SCL 0
#define HAS_ITERATOR_DEBUGGING 0
#endif

#pragma warning(disable:4127) // conditional expression is constant
#pragma warning(disable:4244) // 'argument' : conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4250) // 'class1' : inherits 'class2::member' via dominance
#pragma warning(disable:4267) // 'var' : conversion from 'size_t' to 'type', possible loss of data
#pragma warning(disable:4351) // new behavior: elements of array will be default initialized
#pragma warning(disable:4458) // declaration hides class member
#endif

typedef std::string String;

#endif