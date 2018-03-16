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
  static void SetPacketSize (uint32_t packetSize);
  static void SetTransmitEvent (bool value);  
  static void SetEventType (EventSchedulers_t eventType);
  static EventSchedulers_t GetCurrEventType (void);   
  static void SetNumNodes (uint32_t n); 
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
  /** Parameters for symbolic execution. */
  static uint32_t m_numSymEvents;
  static uint64_t m_symTime;
  static uint64_t m_maxBound;
  
  static uint32_t m_currPacketSize;
  static bool m_isTransmitEvent;
  static uint32_t m_numNodes;
  static Scheduler::EventSchedulers_t m_currEventType;
  static bool m_isNodeVectorInitialized;
  static bool debug;
  
  typedef std::vector<uint64_t> nodeImpactLatency;
  /** The impact latency matrix. */
  std::vector<nodeImpactLatency> m_impactLatency;
  /** The simulator event list. */
  Events m_simEvents;
  /** The node event lists, each node has its own list. */
  std::vector<Events> m_nodesEvents;
  
  void InsertBackToMainList_FrontIsTimeout (Events &subList, const Scheduler::Event &ev);
  void InsertWaitingList (Events &subList, EventsI start, const Scheduler::Event &ev);
  void CheckFrontEvent (Events &subList);
  
  void InsertMultiList (Events &subList, const Scheduler::Event &ev);
  void RemoveFromSubList (Events &subList, const Scheduler::Event &ev);
  void PrintDebugInfo (Events &subList, uint32_t ev_id, uint32_t i_id);
  // <M>
};

} // namespace ns3

#endif /* LIST_SCHEDULER_H */
