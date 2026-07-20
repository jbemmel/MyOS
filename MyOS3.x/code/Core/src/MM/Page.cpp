/***************************************************************************
                          Page.cpp  -  description
                             -------------------
    begin                : Sat Apr 27 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/

#include "Page.h"
#include "PageDirectory.h"

namespace MyOS {
namespace MM {

physadr_t
Page::getMapping() const
{
   register linadr_t a = (linadr_t) this;
   return PageDirectory::getDir(a)[ getPDTIndex(a) ].getMapping() | getOffset(a);
}

pteflags_t
Page::getAttributes() const
{
   register linadr_t a = (linadr_t) this;
   return PageDirectory::getDir(a)[ getPDTIndex(a) ].getAttributes();
}

void
Page::setAttributes( pteflags_t flags )
{
   register linadr_t a = (linadr_t) this;
   PageDirectory::getDir(a)[ getPDTIndex(a) ].setAttributes( flags );
}


}} // namespace
