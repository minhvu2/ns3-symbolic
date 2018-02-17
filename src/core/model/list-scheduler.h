/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef LIST_SCHEDULER_H
#define LIST_SCHEDULER_H

#include "scheduler.h"
#include <list>
#include <utility>
#include <stdint.h>

/**
 * \file
 * \ingroup scheduler
 * Declaration of ns3::ListScheduler class.
 */

namespace ns3 {

class EventImpl;

/**
 * \ingroup scheduler
 * \brief a std::list event scheduler
 *
 * This class implements an event scheduler using an std::list
 * data structure, that is, a double linked-list.
 */
class ListScheduler : public Scheduler
{
public:
  /**
   *  Register this type.
   *  \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Constructor. */
  ListScheduler ();
  /** Destructor. */
  virtual ~ListScheduler ();

  // <M>
  static bool m_isTransmitEvent;
  static void SetTransmitEvent (bool value);
  void PrintDebugInfo (uint32_t ev_id, uint32_t i_id);
  static uint32_t m_numSymEvents;
  static uint32_t m_currPacketSize;
  static uint64_t m_symTime;
  static uint64_t m_maxBound;

  static void SetEventType (EventSchedulers_t eventType);
  static EventSchedulers_t GetCurrEventType (void);
  // <M>

  // Inherited
  virtual void Insert (const Scheduler::Event &ev);
  virtual bool IsEmpty (void) const;
  virtual Scheduler::Event PeekNext (void) const;
  virtual Scheduler::Event RemoveNext (void);
  virtual void Remove (const Scheduler::Event &ev);

private:
  /** Event list type: a simple list of Events. */
  typedef std::list<Scheduler::Event> Events;
  /** Events iterator. */
  typedef std::list<Scheduler::Event>::iterator EventsI;

  /** The event list. */
  Events m_events;
  
  // <M>
  static bool debug;
  static Scheduler::EventSchedulers_t m_currEventType;

  void InsertIntoMainList (EventsI start, const Scheduler::Event &ev);
  void InsertIntoMainList_NextIsTimeout (const Scheduler::Event &ev);
  // <M>  
};

} // namespace ns3

#endif /* LIST_SCHEDULER_H */
