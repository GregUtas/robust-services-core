//==============================================================================
//
//  ClassRegistry.h
//
//  Copyright (C) 2017  Greg Utas
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
#ifndef CLASSREGISTRY_H_INCLUDED
#define CLASSREGISTRY_H_INCLUDED

#include "Immutable.h"
#include "NbTypes.h"
#include "Registry.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for classes.
//
class ClassRegistry : public Immutable
{
   friend class Singleton< ClassRegistry >;
public:
   //  Adds CLS to the registry.
   //
   bool BindClass(Class& cls);

   //  Removes CLS from the registry.
   //
   void UnbindClass(Class& cls);

   //  Returns the class registered against CID.
   //
   Class* Lookup(ClassId cid) const;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   ClassRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~ClassRegistry();

   //  The global registry of classes.
   //
   Registry< Class > classes_;
};
}
#endif
