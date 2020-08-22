//==============================================================================
//
//  PooledClass.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "PooledClass.h"
#include <bitset>
#include <ostream>
#include <string>
#include "Algorithms.h"
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
   Debug::ftnt(PooledClass_dtor);
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
      Debug::SwLog(PooledClass_SetPool,
         "pool already set", pack2(pool_->Pid(), pool.Pid()));
      return false;
   }

   pool_ = &pool;
   return true;
}
}
