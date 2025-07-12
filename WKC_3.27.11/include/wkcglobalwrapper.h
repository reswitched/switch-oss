/*
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef _WKCGLOBALWRAPPER_H_
#define _WKCGLOBALWRAPPER_H_

#include <wkc/wkcheappeer.h>

typedef struct WKCStaticLocalBool_ {
    WKCStaticLocalBool_() {
        fFlag = true;
        wkcMemoryRegisterGlobalObjPeer(&fFlag, sizeof(bool), 0);
    }
    ~WKCStaticLocalBool_(){};
    bool fFlag;
} WKCStaticLocalBool;

typedef struct WKCGlobalObjResetter_ {
    WKCGlobalObjResetter_(void* in_obj, int in_size, void* in_init) {
        wkcMemoryRegisterGlobalObjPeer(in_obj, in_size, in_init);
    }
    ~WKCGlobalObjResetter_(){};
} WKCGlobalObjResetter;

#define WKC_DEFINE_STATIC_TYPE(type, name, initval) \
    static type name = initval; \
    { \
        static WKCStaticLocalBool name##Inited; \
        if (!name##Inited.fFlag) [[unlikely]] { \
            new (&name) type(initval); \
            name##Inited.fFlag = true; \
        } \
    }

#define WKC_DEFINE_STATIC_TYPE_WITH_DEFAULT_CONSTRUCT(type, name) \
    static type name; \
    { \
        static WKCStaticLocalBool name##Inited; \
        if (!name##Inited.fFlag) [[unlikely]] { \
            new (&name) type; \
            name##Inited.fFlag = true; \
        } \
    }

#define WKC_DEFINE_STATIC_INT(name, initval) WKC_DEFINE_STATIC_TYPE(int, name, initval)
#define WKC_DEFINE_STATIC_UINT(name, initval) WKC_DEFINE_STATIC_TYPE(unsigned int, name, initval)
#define WKC_DEFINE_STATIC_LONGLONG(name, initval) WKC_DEFINE_STATIC_TYPE(long long, name, initval)
#define WKC_DEFINE_STATIC_INT64T(name, initval) WKC_DEFINE_STATIC_TYPE(int64_t, name, initval)
#define WKC_DEFINE_STATIC_BOOL(name, initval) WKC_DEFINE_STATIC_TYPE(bool, name, initval)
#define WKC_DEFINE_STATIC_DOUBLE(name, initval) WKC_DEFINE_STATIC_TYPE(double, name, initval)

#define WKC_DEFINE_STATIC_HASHMAP_PTR(type1, type2, name, initval) \
    static HashMap<type1, type2>* name = 0; \
    { \
        static WKCStaticLocalBool name##Inited; \
        if (!name##Inited.fFlag) { \
            name = initval; \
            name##Inited.fFlag = true; \
        } \
    }

#define WKC_DEFINE_GLOBAL_TYPE(type, name, initval) \
    type name = initval; \
    static const type name##InitVal = initval; \
    static const WKCGlobalObjResetter cReset##name((void*)&name, sizeof(type), (void*)&name##InitVal);

#define WKC_DEFINE_GLOBAL_TYPE_ZERO(type, name) \
    type name = 0; \
    static const WKCGlobalObjResetter cReset##name((void*)&name, sizeof(type), 0);

#define WKC_DEFINE_GLOBAL_TYPE_DEFAULT_ZERO(type, name) \
    type name = { }; \
    static const WKCGlobalObjResetter cReset##name((void*)&name, sizeof(type), 0);

#define WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(type, name) \
    static type name; \
    static const type name##InitVal; \
    static const WKCGlobalObjResetter cReset##name;

#define WKC_DEFINE_GLOBAL_CLASS_OBJ(type, classname, name, initval) \
    type classname::name = initval; \
    const type classname::name##InitVal = initval; \
    const WKCGlobalObjResetter classname::cReset##name((void*)&name, sizeof(type), (void*)&name##InitVal);

#define WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(type, name) \
    static type name; \
    static const WKCGlobalObjResetter cReset##name;

#define WKC_DEFINE_GLOBAL_CLASS_OBJ_ZERO(type, classname, name) \
    type classname::name = 0; \
    const WKCGlobalObjResetter classname::cReset##name((void*)&name, sizeof(type), 0);

#define WKC_DEFINE_GLOBAL_CLASS_OBJ_DEFAULT_ZERO(type, classname, name) \
    type classname::name = { }; \
    const WKCGlobalObjResetter classname::cReset##name((void*)&name, sizeof(type), 0);

#define WKC_DEFINE_GLOBAL_DOUBLE(name, initval) WKC_DEFINE_GLOBAL_TYPE(double, name, initval)
#define WKC_DEFINE_GLOBAL_FLOAT(name, initval) WKC_DEFINE_GLOBAL_TYPE(float, name, initval)
#define WKC_DEFINE_GLOBAL_SHORT(name, initval) WKC_DEFINE_GLOBAL_TYPE(short, name, initval)
#define WKC_DEFINE_GLOBAL_USHORT(name, initval) WKC_DEFINE_GLOBAL_TYPE(unsigned short, name, initval)
#define WKC_DEFINE_GLOBAL_INT(name, initval) WKC_DEFINE_GLOBAL_TYPE(int, name, initval)
#define WKC_DEFINE_GLOBAL_UINT(name, initval) WKC_DEFINE_GLOBAL_TYPE(unsigned int, name, initval)
#define WKC_DEFINE_GLOBAL_SIZET(name, initval) WKC_DEFINE_GLOBAL_TYPE(size_t, name, initval)
#define WKC_DEFINE_GLOBAL_LONG(name, initval) WKC_DEFINE_GLOBAL_TYPE(long, name, initval)
#define WKC_DEFINE_GLOBAL_ULONG(name, initval) WKC_DEFINE_GLOBAL_TYPE(unsigned long, name, initval)
#define WKC_DEFINE_GLOBAL_UINT64T(name, initval) WKC_DEFINE_GLOBAL_TYPE(uint64_t, name, initval)
#define WKC_DEFINE_GLOBAL_BOOL(name, initval) WKC_DEFINE_GLOBAL_TYPE(bool, name, initval)

#define WKC_DEFINE_GLOBAL_DOUBLE_ZERO(name) WKC_DEFINE_GLOBAL_TYPE_ZERO(double, name)
#define WKC_DEFINE_GLOBAL_FLOAT_ZERO(name) WKC_DEFINE_GLOBAL_TYPE_ZERO(float, name)
#define WKC_DEFINE_GLOBAL_SHORT_ZERO(name) WKC_DEFINE_GLOBAL_TYPE_ZERO(short, name)
#define WKC_DEFINE_GLOBAL_USHORT_ZERO(name) WKC_DEFINE_GLOBAL_TYPE_ZERO(unsigned short, name)
#define WKC_DEFINE_GLOBAL_INT_ZERO(name) WKC_DEFINE_GLOBAL_TYPE_ZERO(int, name)
#define WKC_DEFINE_GLOBAL_UINT_ZERO(name) WKC_DEFINE_GLOBAL_TYPE_ZERO(unsigned int, name)
#define WKC_DEFINE_GLOBAL_SIZET_ZERO(name) WKC_DEFINE_GLOBAL_TYPE_ZERO(size_t, name)
#define WKC_DEFINE_GLOBAL_LONG_ZERO(name) WKC_DEFINE_GLOBAL_TYPE_ZERO(long, name)
#define WKC_DEFINE_GLOBAL_ULONG_ZERO(name) WKC_DEFINE_GLOBAL_TYPE_ZERO(unsigned long, name)
#define WKC_DEFINE_GLOBAL_UINT64T_ZERO(name) WKC_DEFINE_GLOBAL_TYPE_ZERO(uint64_t, name)
#define WKC_DEFINE_GLOBAL_BOOL_ZERO(name) WKC_DEFINE_GLOBAL_TYPE_ZERO(bool, name)

#define WKC_DEFINE_GLOBAL_THREADSPECIFIC_PTR(type, name) \
    ThreadSpecific<type, CanBeGCThread::True>* name = 0; \
    static const WKCGlobalObjResetter cReset##name(&name, sizeof(ThreadSpecific<type, CanBeGCThread::True>*), 0);

#define WKC_DEFINE_GLOBAL_HASHSET_PTR(type1, type2, name) \
    HashSet<type1, type2>* name = 0; \
    static const WKCGlobalObjResetter cReset##name(&name, sizeof(HashSet<type1, type2>*), 0);

#endif // _WKCGLOBALWRAPPER_H_
