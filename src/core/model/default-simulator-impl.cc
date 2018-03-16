/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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

#include "simulator.h"
#include "default-simulator-impl.h"
#include "scheduler.h"
#include "event-impl.h"

#include "ptr.h"
#include "pointer.h"
#include "assert.h"
#include "log.h"

#include <cmath>

// <M>
#include "list-scheduler.h"
// <M>


/**
 * \file
 * \ingroup simulator
 * Implementation of class ns3::DefaultSimulatorImpl.
 */

namespace ns3 {

// Note:  Logging in this file is largely avoided due to the
// number of calls that are made to these functions and the possibility
// of causing recursions leading to stack overflow
NS_LOG_COMPONENT_DEFINE ("DefaultSimulatorImpl");

NS_OBJECT_ENSURE_REGISTERED (DefaultSimulatorImpl);

TypeId
DefaultSimulatorImpl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DefaultSimulatorImpl")
    .SetParent<SimulatorImpl> ()
    .SetGroupName ("Core")
    .AddConstructor<DefaultSimulatorImpl> ()
  ;
  return tid;
}

DefaultSimulatorImpl::DefaultSimulatorImpl ()
{
  NS_LOG_FUNCTION (this);
  m_stop = false;
  // uids are allocated from 4.
  // uid 0 is "invalid" events
  // uid 1 is "now" events
  // uid 2 is "destroy" events
  m_uid = 4;
  // before ::Run is entered, the m_currentUid will be zero
  m_currentUid = 0;
  m_currentTs = 0;
  // <M>
  m_numNodes = 2;
  for (uint32_t i=0; i < m_numNodes; i++)
    {
	  m_localCurrentTs.push_back (0);	
    }
  m_implementations.push_back (true); // three to two paths per iteration
  m_implementations.push_back (true); // remove inactive events
  m_implementations.push_back (true); // waiting list
  m_implementations.push_back (true); // local lists and clocks
  ListScheduler::SetPathReduction (m_implementations.at (0));
  ListScheduler::SetWaitingList (m_implementations.at (2));
  ListScheduler::SetLocalList (m_implementations.at (3));  	 
  // <M>
  m_currentContext = 0xffffffff;
  m_unscheduledEvents = 0;
  m_eventsWithContextEmpty = true;
  m_main = SystemThread::Self();
}

DefaultSimulatorImpl::~DefaultSimulatorImpl ()
{
  NS_LOG_FUNCTION (this);
}

void
DefaultSimulatorImpl::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  ProcessEventsWithContext ();

  while (!m_events->IsEmpty ())
    {
      Scheduler::Event next = m_events->RemoveNext ();
      next.impl->Unref ();
    }
  m_events = 0;
  SimulatorImpl::DoDispose ();
}
void
DefaultSimulatorImpl::Destroy ()
{
  NS_LOG_FUNCTION (this);
  while (!m_destroyEvents.empty ()) 
    {
      Ptr<EventImpl> ev = m_destroyEvents.front ().PeekEventImpl ();
      m_destroyEvents.pop_front ();
      NS_LOG_LOGIC ("handle destroy " << ev);
      if (!ev->IsCancelled ())
        {
          ev->Invoke ();
        }
    }
}

void
DefaultSimulatorImpl::SetScheduler (ObjectFactory schedulerFactory)
{
  NS_LOG_FUNCTION (this << schedulerFactory);
  Ptr<Scheduler> scheduler = schedulerFactory.Create<Scheduler> ();

  // <M>
//  printf ("Setting new scheduler !!!!!!!!");
  // <M>

  if (m_events != 0)
    {
      while (!m_events->IsEmpty ())
        {
          Scheduler::Event next = m_events->RemoveNext ();
          scheduler->Insert (next);
        }
    }
  m_events = scheduler;
}

// System ID for non-distributed simulation is always zero
uint32_t 
DefaultSimulatorImpl::GetSystemId (void) const
{
  return 0;
}

void
DefaultSimulatorImpl::ProcessOneEvent (void)
{
  Scheduler::Event next = m_events->RemoveNext ();
  // <M>
//  printf ("After removing next event id %u - %lu ms in ProcessOneEvent-DefaultSimulatorImpl, current ts %lu ms\n", 
//          next.key.m_uid, next.key.m_ts, m_currentTs);
  // <M>

//  NS_ASSERT (next.key.m_ts >= m_currentTs);
  m_unscheduledEvents--;

  NS_LOG_LOGIC ("handle " << next.key.m_ts);
//  m_currentTs = next.key.m_ts;
  
  // Set current timestamp for corresponding list
  if (next.key.m_context != 0xffffffff)
    {
      m_localCurrentTs.at (next.key.m_context) = next.key.m_ts;		  	
	}
  else //next event is in the simulator list
    {
	  m_currentTs = next.key.m_ts;
	}	
  m_currentContext = next.key.m_context;
  m_currentUid = next.key.m_uid;
//  printf ("ProcessOneEvent-1\n");
  next.impl->Invoke ();
  next.impl->Unref ();

//  printf ("ProcessOneEvent-2\n");

  ProcessEventsWithContext ();
}

bool 
DefaultSimulatorImpl::IsFinished (void) const
{
  return m_events->IsEmpty () || m_stop;
}

void
DefaultSimulatorImpl::ProcessEventsWithContext (void)
{
  if (m_eventsWithContextEmpty)
    {
      return;
    }

//  printf ("ProcessEventsWithContext-1 \n");

  // swap queues
  EventsWithContext eventsWithContext;
  {
    CriticalSection cs (m_eventsWithContextMutex);
    m_eventsWithContext.swap(eventsWithContext);
    m_eventsWithContextEmpty = true;
  }
  while (!eventsWithContext.empty ())
    {
       EventWithContext event = eventsWithContext.front ();
       eventsWithContext.pop_front ();
       Scheduler::Event ev;
       ev.impl = event.event;
       ev.key.m_ts = m_currentTs + event.timestamp;
       ev.key.m_context = event.context;
       ev.key.m_uid = m_uid;
       m_uid++;
       m_unscheduledEvents++;
       m_events->Insert (ev);
    }
    
//  printf ("ProcessEventsWithContext-2 \n");  
}

void
DefaultSimulatorImpl::Run (void)
{
  NS_LOG_FUNCTION (this);
  // Set the current threadId as the main threadId
  m_main = SystemThread::Self();
  ProcessEventsWithContext ();
  m_stop = false;

  while (!m_events->IsEmpty () && !m_stop) 
    {
      ProcessOneEvent ();
    }

  // If the simulator stopped naturally by lack of events, make a
  // consistency test to check that we didn't lose any events along the way.
  NS_ASSERT (!m_events->IsEmpty () || m_unscheduledEvents == 0);
}

void 
DefaultSimulatorImpl::Stop (void)
{
  NS_LOG_FUNCTION (this);
  m_stop = true;
}

void 
DefaultSimulatorImpl::Stop (Time const &delay)
{
  NS_LOG_FUNCTION (this << delay.GetTimeStep ());
  Simulator::Schedule (delay, &Simulator::Stop);
}

//
// Schedule an event for a _relative_ time in the future.
//
EventId
DefaultSimulatorImpl::Schedule (Time const &delay, EventImpl *event)
{
  NS_LOG_FUNCTION (this << delay.GetTimeStep () << event);
  NS_ASSERT_MSG (SystemThread::Equals (m_main), "Simulator::Schedule Thread-unsafe invocation!");

//  Time tAbsolute = delay + TimeStep (m_currentTs);
// <M>
// Set scheduled timestamp based on local clocks
  Time tAbsolute;
  if (GetContext () != 0xffffffff)
    {
	  tAbsolute = delay + TimeStep (m_localCurrentTs.at (GetContext ()));	
	  NS_ASSERT (tAbsolute >= TimeStep (m_localCurrentTs.at (GetContext ())));
	}
  else
    {
	  tAbsolute = delay + TimeStep (m_currentTs);
	  NS_ASSERT (tAbsolute >= TimeStep (m_currentTs));	
	}	

  NS_ASSERT (tAbsolute.IsPositive ());
//  NS_ASSERT (tAbsolute >= TimeStep (m_currentTs));
  Scheduler::Event ev;
  ev.impl = event;
  ev.key.m_ts = (uint64_t) tAbsolute.GetTimeStep ();
  ev.key.m_context = GetContext ();
  ev.key.m_uid = m_uid;
  
  // <M>
  ev.key.m_eventType = ListScheduler::GetCurrEventType ();
  // <M>
  
  m_uid++;
  m_unscheduledEvents++;
  m_events->Insert (ev);
  return EventId (event, ev.key.m_ts, ev.key.m_context, ev.key.m_uid, ev.key.m_eventType);
}

void
DefaultSimulatorImpl::ScheduleWithContext (uint32_t context, Time const &delay, EventImpl *event)
{
  NS_LOG_FUNCTION (this << context << delay.GetTimeStep () << event);

  if (SystemThread::Equals (m_main))
    {
//      Time tAbsolute = delay + TimeStep (m_currentTs);
// <M>
      Time tAbsolute;
      if (GetContext () != 0xffffffff)
        {
	      tAbsolute = delay + TimeStep (m_localCurrentTs.at (context));	
	      NS_ASSERT (tAbsolute >= TimeStep (m_localCurrentTs.at (context)));
	    }
      else
        {
	      tAbsolute = delay + TimeStep (m_currentTs);
	      NS_ASSERT (tAbsolute >= TimeStep (m_currentTs));	
	    }	
      Scheduler::Event ev;
      ev.impl = event;
      ev.key.m_ts = (uint64_t) tAbsolute.GetTimeStep ();
      ev.key.m_context = context;
      ev.key.m_uid = m_uid;
      
      m_uid++;
      m_unscheduledEvents++;
      m_events->Insert (ev);
    }
  else
    {
      EventWithContext ev;
      ev.context = context;
      // Current time added in ProcessEventsWithContext()
      ev.timestamp = delay.GetTimeStep ();
      ev.event = event;
      {
        CriticalSection cs (m_eventsWithContextMutex);
        m_eventsWithContext.push_back(ev);
        m_eventsWithContextEmpty = false;
      }
    }
}

// <M>
// For events that change their context, use local clock of previous node
void
DefaultSimulatorImpl::ScheduleWithContext (uint32_t prevContext, uint32_t context, Time const &delay, EventImpl *event)
{
  NS_LOG_FUNCTION (this << context << delay.GetTimeStep () << event);

  if (SystemThread::Equals (m_main))
    {
//      Time tAbsolute = delay + TimeStep (m_currentTs);
// <M>
      Time tAbsolute;
      if (prevContext != 0xffffffff)
        {
	      tAbsolute = delay + TimeStep (m_localCurrentTs.at (prevContext));	
	      NS_ASSERT (tAbsolute >= TimeStep (m_localCurrentTs.at (prevContext)));
	    }
      else
        {
	      tAbsolute = delay + TimeStep (m_currentTs);
	      NS_ASSERT (tAbsolute >= TimeStep (m_currentTs));	
	    }
      Scheduler::Event ev;
      ev.impl = event;
      ev.key.m_ts = (uint64_t) tAbsolute.GetTimeStep ();
      ev.key.m_context = context;
      ev.key.m_uid = m_uid;
      
      m_uid++;
      m_unscheduledEvents++;
      m_events->Insert (ev);
    }
  else
    {
      EventWithContext ev;
      ev.context = context;
      // Current time added in ProcessEventsWithContext()
      ev.timestamp = delay.GetTimeStep ();
      ev.event = event;
      {
        CriticalSection cs (m_eventsWithContextMutex);
        m_eventsWithContext.push_back(ev);
        m_eventsWithContextEmpty = false;
      }
    }
}
// <M>

EventId
DefaultSimulatorImpl::ScheduleNow (EventImpl *event)
{
  NS_ASSERT_MSG (SystemThread::Equals (m_main), "Simulator::ScheduleNow Thread-unsafe invocation!");

  Scheduler::Event ev;
  ev.impl = event;
//  ev.key.m_ts = m_currentTs;
// <M>
  if (GetContext () != 0xffffffff)
    {
	  ev.key.m_ts = m_localCurrentTs.at (GetContext());	
	}
  else
    {
	  ev.key.m_ts = m_currentTs;
	}
  ev.key.m_context = GetContext ();
  ev.key.m_uid = m_uid;
  
  // <M>
  ev.key.m_eventType = ListScheduler::GetCurrEventType ();
  // <M>

  m_uid++;
  m_unscheduledEvents++;
  m_events->Insert (ev);
  return EventId (event, ev.key.m_ts, ev.key.m_context, ev.key.m_uid, ev.key.m_eventType);
}

EventId
DefaultSimulatorImpl::ScheduleDestroy (EventImpl *event)
{
  NS_ASSERT_MSG (SystemThread::Equals (m_main), "Simulator::ScheduleDestroy Thread-unsafe invocation!");

  EventId id (Ptr<EventImpl> (event, false), m_currentTs, 0xffffffff, 2);
  m_destroyEvents.push_back (id);
  m_uid++;
  return id;
}

Time
DefaultSimulatorImpl::Now (void) const
{
  // Do not add function logging here, to avoid stack overflow
//  return TimeStep (m_currentTs);
// <M>
  if (GetContext() != 0xffffffff)
    {
	  return TimeStep (m_localCurrentTs.at (GetContext ()));	
	}
  else
    {
      return TimeStep (m_currentTs);
	}	
}

Time 
DefaultSimulatorImpl::GetDelayLeft (const EventId &id) const
{
  if (IsExpired (id))
    {
      return TimeStep (0);
    }
  else
    {
//      return TimeStep (id.GetTs () - m_currentTs);
// <M>
      if (id.GetContext () != 0xffffffff)
        {
	      return TimeStep (id.GetTs () - m_localCurrentTs.at (id.GetContext ()));	
	    }
      else
        {
          return TimeStep (id.GetTs() - m_currentTs);
	    }	
    }
}

void
DefaultSimulatorImpl::Remove (const EventId &id)
{
  if (id.GetUid () == 2)
    {
      // destroy events.
      for (DestroyEvents::iterator i = m_destroyEvents.begin (); i != m_destroyEvents.end (); i++)
        {
          if (*i == id)
            {
              m_destroyEvents.erase (i);
              break;
            }
        }
      return;
    }
//  if (IsExpired (id))
//    {
//      return;
//    }
  Scheduler::Event event;
  event.impl = id.PeekEventImpl ();
  event.key.m_ts = id.GetTs ();
  event.key.m_context = id.GetContext ();
  event.key.m_uid = id.GetUid ();
  
  // <M>
  event.key.m_eventType = id.GetEventType ();
  // <M>
  m_events->Remove (event);
  event.impl->Cancel ();
  // whenever we remove an event from the event list, we have to unref it.
  event.impl->Unref ();

  m_unscheduledEvents--;
}

void
DefaultSimulatorImpl::Cancel (const EventId &id)
{
  if (!IsExpired (id))
    {
	  if (m_implementations.at (1)) // remove inactive events
	    {
//		  printf ("Cancelling and removing event id %u - %lu ms in DefaultSimulatorImpl\n", id.GetUid(), id.GetTs());
          Remove (id);
		}
	  else
	    {
		  id.PeekEventImpl ()->Cancel ();	
		}	
    }
}

bool
DefaultSimulatorImpl::IsExpired (const EventId &id) const
{
  if (id.GetUid () == 2)
    {
      if (id.PeekEventImpl () == 0 ||
          id.PeekEventImpl ()->IsCancelled ())
        {
          return true;
        }
      // destroy events.
      for (DestroyEvents::const_iterator i = m_destroyEvents.begin (); i != m_destroyEvents.end (); i++)
        {
          if (*i == id)
            {
              return false;
            }
        }
      return true;
    }
// <M>
  uint64_t currentTs;
  if (GetContext () != 0xffffffff)
    {
	  currentTs = m_localCurrentTs.at (GetContext ());	
	}
  else
    {
	  currentTs = m_currentTs;	
	}
	
  if (id.PeekEventImpl () == 0 ||
      id.GetTs () < currentTs ||
      (id.GetTs () == currentTs &&
       id.GetUid () <= m_currentUid) ||
      id.PeekEventImpl ()->IsCancelled ()) 
    {
      return true;
    }
  else
    {
      return false;
    }
}

Time 
DefaultSimulatorImpl::GetMaximumSimulationTime (void) const
{
  /// \todo I am fairly certain other compilers use other non-standard
  /// post-fixes to indicate 64 bit constants.
  return TimeStep (0x7fffffffffffffffLL);
}

uint32_t
DefaultSimulatorImpl::GetContext (void) const
{
  return m_currentContext;
}

} // namespace ns3
