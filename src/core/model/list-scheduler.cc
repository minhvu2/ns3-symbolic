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

#include "list-scheduler.h"
#include "event-impl.h"
#include "log.h"
#include <utility>
#include <string>
#include "assert.h"
#include <stdio.h>
// <M>
#include "s2e.h"
// <M>


/**
 * \file
 * \ingroup scheduler
 * Implementation of ns3::ListScheduler class.
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ListScheduler");

NS_OBJECT_ENSURE_REGISTERED (ListScheduler);

TypeId
ListScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ListScheduler")
    .SetParent<Scheduler> ()
    .SetGroupName ("Core")
    .AddConstructor<ListScheduler> ()
  ;
  return tid;
}

ListScheduler::ListScheduler ()
{
  NS_LOG_FUNCTION (this);
}
ListScheduler::~ListScheduler ()
{
}
// <M>
bool ListScheduler::m_isTransmitEvent = false;
uint32_t ListScheduler::m_numSymEvents = 0;
uint32_t ListScheduler::m_currPacketSize = 0;
uint64_t ListScheduler::m_maxBound = 1200;
uint64_t ListScheduler::m_symTime = 0;
// Set m_isTransmitEvent to true if next inserted event is transmitting event
void
ListScheduler::SetTransmitEvent (bool value)
{
  m_isTransmitEvent = value;
}

void
ListScheduler::PrintDebugInfo (uint32_t ev_id, uint32_t i_id)
{
  char buf[64];
  memset (buf, 0 ,sizeof(buf));

  snprintf (buf, sizeof(buf), "Inserting event %u before %u", ev_id, i_id);
  s2e_warning (buf);
  memset (buf, 0, sizeof(buf));
  
  s2e_warning ("Current event queue------");
  for (EventsI j = m_events.begin (); j != m_events.end (); j++)
    {
      j->key.m_originalTs = j->key.m_ts;

      if (s2e_is_symbolic (&(j->key.m_originalTs), sizeof(uint64_t)))
        {
          s2e_get_example (&(j->key.m_originalTs), sizeof(uint64_t));
        }

      if (j->key.m_isTransEvent)
        {
          snprintf (buf, sizeof(buf), "Event %u : %llu ms - Transmission Event",
            j->key.m_uid, j->key.m_originalTs);
        }
      else
        {
          snprintf (buf, sizeof(buf), "Event %u : %llu ms",
            j->key.m_uid, j->key.m_originalTs);
        }

      s2e_warning (buf);
      memset (buf, 0, sizeof(buf));

      if (s2e_is_symbolic (&(j->key.m_ts), sizeof(uint64_t)))
        {
          snprintf (buf, sizeof(buf), "Symbolic event");
          s2e_warning (buf);
          memset (buf, 0, sizeof(buf));
        }
    }
  s2e_warning ("End of queue--------");
}
// <M>

void
ListScheduler::Insert (const Event &ev)
{
  NS_LOG_FUNCTION (this << &ev);

  // <M>
  // <M>
//  uint64_t limit_ts = ev.key.m_ts;
//  if (s2e_is_symbolic (&((const_cast<Event&>(ev)).key.m_ts),
//        sizeof(uint64_t)))
//    {
//      s2e_get_example(&((const_cast<Event&>(ev)).key.m_ts), sizeof(uint64_t));
//      (const_cast<Event&>(ev)).key.m_originalTs = 0;
//    }
//  else
//    {
//      (const_cast<Event&>(ev)).key.m_originalTs = ev.key.m_ts;
//    }
  char buf[64];
  memset (buf, 0, sizeof(buf));

  if (!s2e_is_symbolic (&m_symTime, sizeof (uint64_t)))
    {
      s2e_make_symbolic (&m_symTime, sizeof (uint64_t), "Symbolic delay");
      if (m_symTime > m_maxBound)
      {
        s2e_kill_state (0, "Out of range");
      }
    }

  // Only make m_ts of transmit events symbolic
  if (m_isTransmitEvent)
    {
          (const_cast<Event&>(ev)).key.m_isTransEvent = true;
          (const_cast<Event&>(ev)).key.m_packetSize = m_currPacketSize;
          if (ev.key.m_packetSize > 58 && m_numSymEvents < 1)
            {
              m_numSymEvents++;
              snprintf (buf, sizeof(buf), "Setting event id %u to symbolic !!!!!",
                ev.key.m_uid);
              s2e_warning (buf);
              memset (buf, 0, sizeof(buf));
//              uint64_t sym_ts;
              s2e_enable_forking ();
//              s2e_make_symbolic (&sym_ts, sizeof(uint64_t), "Time Stamp");
              (const_cast<Event&>(ev)).key.m_ts += m_symTime;
//            s2e_make_symbolic(const_cast<uint64_t *>(&(ev.key.m_ts)),
//                                sizeof(uint64_t), "Timestamp");
            }
          SetTransmitEvent (false);
//        printf("Event to be inserted is a transmit event \n");
    }
  else
    {
      (const_cast<Event&>(ev)).key.m_isTransEvent = false;
      (const_cast<Event&>(ev)).key.m_packetSize = 0;
    }

      snprintf (buf, sizeof(buf), "Event %u to be inserted", ev.key.m_uid);
      s2e_warning (buf);
      memset (buf, 0, sizeof(buf));
      for (EventsI i = m_events.begin (); i != m_events.end (); i++)
        {
		  if (ev.key.m_uid < i->key.m_uid)
            {
              if (ev.key.m_ts <= i->key.m_ts)
                {
                  m_events.insert (i, ev);
                  PrintDebugInfo (ev.key.m_uid, i->key.m_uid);
                  return;
                }
            }
          else // ev.id > i.id
            {
              if (ev.key.m_ts < i->key.m_ts)
                {
                  m_events.insert (i, ev);
                  PrintDebugInfo (ev.key.m_uid, i->key.m_uid);
                  return;
                }
            }
        }
//    snprintf (buf, sizeof(buf), "Inserting event id %u at the end of the queue",
//      ev.key.m_uid);
//    s2e_warning (buf);
//    memset (buf, 0, sizeof(buf));
                    
  // <M>


// Original code
/*  for (EventsI i = m_events.begin (); i != m_events.end (); i++)
    {
      if (ev.key < i->key)
        {
          m_events.insert (i, ev);
          return;
        }
    }
*/

  m_events.push_back (ev);
}
bool
ListScheduler::IsEmpty (void) const
{
  NS_LOG_FUNCTION (this);
  return m_events.empty ();
}
Scheduler::Event
ListScheduler::PeekNext (void) const
{
  NS_LOG_FUNCTION (this);
  return m_events.front ();
}

Scheduler::Event
ListScheduler::RemoveNext (void)
{
  NS_LOG_FUNCTION (this);
  Event next = m_events.front ();
  m_events.pop_front ();
//  printf("Removing next event: %u, time: %llu \n", next.key.m_uid, next.key.m_ts);
  return next;
}

void
ListScheduler::Remove (const Event &ev)
{
  NS_LOG_FUNCTION (this << &ev);

//  printf("Removing event: %u, time: %llu \n", ev.key.m_uid, ev.key.m_ts);

  for (EventsI i = m_events.begin (); i != m_events.end (); i++)
    {
      if (i->key.m_uid == ev.key.m_uid)
        {
          NS_ASSERT (ev.impl == i->impl);
          m_events.erase (i);
          return;
        }
    }
  NS_ASSERT (false);
}

} // namespace ns3
