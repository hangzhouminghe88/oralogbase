//**************************************************************
//*
//* 修 改 者：wqy 日期：2012-5-24:14
//* 从fd库里的dlist修改而来
//*
//****************************************************************
/*
   Netpro庐 - The Network Backup Solution

   Copyright (C) 2004-2008 Free Software Foundation Europe e.V.

   The main author of Netpro is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Netpro庐 is a registered trademark of Kern Sibbald.
   The licensor of Netpro is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Z眉rich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *  Written by Kern Sibbald MMIV
 *
 *   Version $Id: dlist.h 7380 2008-07-14 10:42:59Z kerns $
 */

#define M_ABORT 1

/* In case you want to specifically specify the offset to the link */
#define OFFSET(item, link) (int)((char *)(link) - (char *)(item))
/*
 * There is a lot of extra casting here to work around the fact
 * that some compilers (Sun and Visual C++) do not accept
 * (void *) as an lvalue on the left side of an equal.
 *
 * Loop var through each member of list
 */
#ifdef HAVE_TYPEOF
#define foreach_dlist(var, list) \
        for((var)=NULL; ((var)=(typeof(var))(list)->next(var)); )
#else
#define foreach_dlist(var, list) \
    for((var)=NULL; (*((void **)&(var))=(void*)((list)->next(var))); )
#endif

#include <stdio.h>

struct dlink {
   void *next;
   void *prev;
};

class dlist {
protected:
   void *head;
   void *tail;
   int	loffset;
   unsigned int num_items;
public:
// 增加拷贝构造函数
   dlist(const dlist& list);
   dlist& operator = (const dlist& list);
   void SetVals(void *pHead,void *pTail,unsigned int num,int offset);
   void SetOffset(int offset);
   int  GetOffset();
   

   dlist(void *item, dlink *link);
   dlist(void);
   ~dlist() { destroy(); }
   void init(void *item, dlink *link);
   void init();
   void prepend(void *item);
   void append(void *item);
   void set_prev(void *item, void *prev);
   void set_next(void *item, void *next);
   void *get_prev(void *item);
   void *get_next(void *item);
   dlink *get_link(void *item);
   void insert_before(void *item, void *where);
   void insert_after(void *item, void *where);
   void *binary_insert(void *item, int compare(void *item1, void *item2));
   void *binary_search(void *item, int compare(void *item1, void *item2));
   void binary_insert_multiple(void *item, int compare(void *item1, void *item2));
   void remove(void *item);
   bool empty() const;
   int  size() const;
   void *next(void *item);      
   void *prev(void *item);
   void destroy();
   void *first() const;
   void *last() const;
};

inline dlist::dlist(const dlist& list)
{
	this->head = list.head;
	this->tail = list.tail;
	this->loffset = list.loffset;
	this->num_items = list.num_items;
}

inline dlist& dlist::operator = (const dlist& list)
{
	this->head = list.head;
	this->tail = list.tail;
	this->loffset = list.loffset;
	this->num_items = list.num_items;
	return *this;
}

inline void dlist::SetVals(void *pHead,void *pTail,unsigned int num,int offset)
{
	head = pHead;
	tail = pTail;
	num_items = num;
	loffset = offset;
}

inline void dlist::SetOffset(int offset)
{
	loffset = offset;
}

inline int dlist::GetOffset()
{
	return loffset;
}

/*
 * This allows us to do explicit initialization,
 *   allowing us to mix C++ classes inside malloc'ed
 *   C structures. Define before called in constructor.
 */
inline void dlist::init(void *item, dlink *link)
{
   head = tail = NULL;
   loffset = (int)((char *)link - (char *)item);
   if (loffset < 0 || loffset > 5000) {
      loffset = 0;
   }
   num_items = 0;
}

inline void dlist::init()
{
   head = tail = NULL;
   loffset = 0;
   num_items = 0;
}


/*
 * Constructor called with the address of a
 *   member of the list (not the list head), and
 *   the address of the link within that member.
 * If the link is at the beginning of the list member,
 *   then there is no need to specify the link address
 *   since the offset is zero.
 */
inline dlist::dlist(void *item, dlink *link)
{
   init(item, link);
}

/* Constructor with link at head of item */
inline dlist::dlist(void) : head(0), tail(0), loffset(0), num_items(0)
{
}

inline void dlist::set_prev(void *item, void *prev)
{
   ((dlink *)(((char *)item)+loffset))->prev = prev;
}

inline void dlist::set_next(void *item, void *next)
{
   ((dlink *)(((char *)item)+loffset))->next = next;
}

inline void *dlist::get_prev(void *item)
{
   return ((dlink *)(((char *)item)+loffset))->prev;
}

inline void *dlist::get_next(void *item)
{
   return ((dlink *)(((char *)item)+loffset))->next;
}


inline dlink *dlist::get_link(void *item)
{
   return (dlink *)(((char *)item)+loffset);
}



inline bool dlist::empty() const
{
   return head == NULL;
}

inline int dlist::size() const
{
   return num_items;
}



inline void * dlist::first() const
{
   return head;
}

inline void * dlist::last() const
{
   return tail;
}

