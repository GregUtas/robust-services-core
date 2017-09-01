//==============================================================================
//
//  PooledClass.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PooledClass.h"
#include <bitset>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "NbTypes.h"
#include "ObjectPool.h"
#include "Pooled.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name PooledClass_ctor = "PooledClass.ctor";

PooledClass::PooledClass(ClassId cid, size_t size) :
   Class(cid, size),
   pool_(nullptr)
{
   Debug::ft(PooledClass_ctor);
}

//------------------------------------------------------------------------------

fn_name PooledClass_dtor = "PooledClass.dtor";

PooledClass::~PooledClass()
{
   Debug::ft(PooledClass_dtor);
}

//------------------------------------------------------------------------------

void PooledClass::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Class::Display(stream, prefix, options);

   stream << prefix << "pool : ";

   if((pool_ != nullptr) && options.test(DispVerbose))
   {
      stream << CRLF;
      pool_->Display(stream, prefix + spaces(2), options);
   }
   else
   {
      stream << strObj(pool_) << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name PooledClass_New = "PooledClass.New";

Object* PooledClass::New(size_t size)
{
   Debug::ft(PooledClass_New);

   return pool_->DeqBlock(size);
}

//------------------------------------------------------------------------------

void PooledClass::Patch(sel_t selector, void* arguments)
{
   Class::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name PooledClass_SetPool = "PooledClass.SetPool";

bool PooledClass::SetPool(ObjectPool& pool)
{
   Debug::ft(PooledClass_SetPool);

   //  Set POOL as this class's pool unless it has already registered another
   //  pool.
   //
   if((pool_ != nullptr) && (pool_ != &pool))
   {
      Debug::SwErr(PooledClass_SetPool, pool_->Pid(), pool.Pid());
      return false;
   }

   pool_ = &pool;
   return true;
}
}
