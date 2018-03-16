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
uint32_t ListScheduler::m_numSymEvents = 0;
uint64_t ListScheduler::m_maxBound = 1200;
uint64_t ListScheduler::m_symTime = 0;

bool ListScheduler::m_isTransmitEvent = false;
uint32_t ListScheduler::m_numNodes = 2;
uint32_t ListScheduler::m_currPacketSize = 0;
Scheduler::EventSchedulers_t ListScheduler::m_currEventType = UNDEFINED;
bool ListScheduler::m_isNodeVectorInitialized = false;
bool ListScheduler::debug = false;

void
ListScheduler::SetPacketSize (uint32_t packetSize)
{
  m_currPacketSize = packetSize;		
}	

void
ListScheduler::SetNumNodes (uint32_t n)
{
  m_numNodes = n;		
}	

void
ListScheduler::SetTransmitEvent (bool value)
{
  m_isTransmitEvent = value;
}

void 
ListScheduler::SetEventType (EventSchedulers_t eventType)
{
  m_currEventType = eventType;
}

Scheduler::EventSchedulers_t
ListScheduler::GetCurrEventType (void)
{
  return m_currEventType;	
}	


void
ListScheduler::PrintDebugInfo (Events &subList, uint32_t ev_id, uint32_t i_id)
{
  char buf[64];
  memset (buf, 0 ,sizeof(buf));

  snprintf (buf, sizeof(buf), "Inserting event %u before %u", ev_id, i_id);
  s2e_warning (buf);
  memset (buf, 0, sizeof(buf));

  s2e_warning ("Current event queue------");
  for (EventsI j = subList.begin (); j != subList.end (); j++)
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

// First time an event is inserted into a list
void
ListScheduler::InsertMultiList (Events &subList, const Event &ev)
{
  if (debug)
    {
	  printf ("InsertIntoSubList-1\n");  
	}

  bool waitingList = true;	
  if (waitingList == false) //multi list without waiting list
    {	  
	  for (EventsI i = subList.begin (); i != subList.end (); i++)
        {
          if (ev.key.m_uid < i->key.m_uid)
            {
              if (ev.key.m_ts <= i->key.m_ts)
               {
                 subList.insert (i, ev);
//                 PrintDebugInfo (subList, ev.key.m_uid, i->key.m_uid);              
                 return;
               }
            }
          else // ev.id > i.id
            {
              if (ev.key.m_ts < i->key.m_ts)
                {
                  subList.insert (i, ev);
//                  PrintDebugInfo (subList, ev.key.m_uid, i->key.m_uid);
                  return;
                }
            }
        }
      subList.push_back (ev);
//      PrintDebugInfo (subList, ev.key.m_uid, ev.key.m_uid);
      return;
    }

  if (waitingList == true) //with waiting list
    {
	  InsertWaitingList (subList, subList.begin (), ev);
	  return;	
	}            
}

// Only use when we try to remove the next event and it has a waiting list
// Have to extract events in that waiting list and insert into regular list
void
ListScheduler::InsertBackToMainList_FrontIsTimeout (Events &subList, const Event &ev)
{
	Event next = subList.front ();
    for (EventsI i = subList.begin (); i != subList.end (); i++)
    {
//	  printf ("Current event iterator %u \n", i->key.m_uid);	
      if (ev.key.m_uid < i->key.m_uid)
        {
		  // insert to pending list of timeout event
		  // if this timeout event is at the front, go to else
		  // if we insert ev into pending list of a front timeout event,
		  // there will be infinite loop
		  if (i->key.m_eventType == TIMEOUT && i->key.m_uid != next.key.m_uid)
		    {
			  i->pendingEvents.push_back(ev);
//			  printf ("Pushing event %u into pending list of event %u, size %lu \n", 
//			           ev.key.m_uid, i->key.m_uid, i->pendingEvents.size());			  
			  return;
		    }
		  else // i is not a timeout event, compare timestamp
		    {  	
              if (ev.key.m_ts <= i->key.m_ts)
                {	
                  subList.insert (i, ev);
//                  printf ("Inserting event %u into main list\n", ev.key.m_uid);	
//                  PrintDebugInfo (ev.key.m_uid, i->key.m_uid);
                  return;
                }
            }  
        }
      else // ev.id > i.id
        {
		  if (i->key.m_eventType == TIMEOUT && i->key.m_uid != next.key.m_uid)
		    {
			  i->pendingEvents.push_back(ev);
//			  printf ("Pushing event %u into pending list of event %u, size %lu \n", 
//			          ev.key.m_uid, i->key.m_uid, i->pendingEvents.size());
			  return;
		    }
		  else
		    {  
              if (ev.key.m_ts < i->key.m_ts)
                {
                  subList.insert (i, ev);
//                  printf ("Inserting event %u into main list\n", ev.key.m_uid);
//                 PrintDebugInfo (ev.key.m_uid, i->key.m_uid);
                  return;
                }
            }    
        }
    }
  subList.push_back (ev);
}	

// Inserting event when there is waiting lists
void
ListScheduler::InsertWaitingList (Events &subList, EventsI start, const Event &ev)
{
  if (debug)	
    {
      printf("InsertIntoMainList-1-Start \n");
      PrintDebugInfo (subList, 1, 1);
      printf ("Inserting loop start at event id %u \n", start->key.m_uid);
    }  
   
  for (EventsI i = start; i != subList.end (); i++)
    {
//	  printf ("Current event iterator %u \n", i->key.m_uid);	
      if (ev.key.m_uid < i->key.m_uid)
        {
		  // insert to pending list of timeout event	
		  if (i->key.m_eventType == TIMEOUT)
		    {
			  i->pendingEvents.push_back(ev);
//			  printf ("Pushing event %u into pending list of event %u, size %lu \n", 
//			           ev.key.m_uid, i->key.m_uid, i->pendingEvents.size());			  
			  return;
		    }
		  else // i is not a timeout event, compare timestamp
		    {  	
              if (ev.key.m_ts <= i->key.m_ts)
                {	
                  subList.insert (i, ev);
//                  printf ("Inserting event %u into main list\n", ev.key.m_uid);	
//                  PrintDebugInfo (ev.key.m_uid, i->key.m_uid);
                  return;
                }
            }  
        }
      else // ev.m_uid > i.m_uid
        {
		  if (i->key.m_eventType == TIMEOUT)
		    {
			  i->pendingEvents.push_back(ev);
//			  printf ("Pushing event %u into pending list of event %u, size %lu \n", 
//			          ev.key.m_uid, i->key.m_uid, i->pendingEvents.size());
			  return;
		    }
		  else
		    {  
              if (ev.key.m_ts < i->key.m_ts)
                {
                  subList.insert (i, ev);
//                  printf ("Inserting event %u into main list\n", ev.key.m_uid);
//                 PrintDebugInfo (ev.key.m_uid, i->key.m_uid);
                  return;
                }
            }    
        }
    }
  subList.push_back (ev);
}			

void
ListScheduler::Insert (const Event &ev)
{
  NS_LOG_FUNCTION (this << &ev);
  if (!m_isNodeVectorInitialized)
    {
	  //simple-global-routing	
	  //uint64_t impactLatencyMatrix [4][4] = {{0, 10, 5, 15}, 
	                                         //{10, 0, 5, 15},
	                                         //{5, 5, 0, 10},
	                                         //{15, 15, 10, 0}};
	  //uint64_t impactLatencyMatrix [4][4] = {{0, 5, 100000, 100000}, 
	                                         //{5, 0, 100000, 100000},
	                                         //{100000, 100000, 0, 5},
	                                         //{100000, 100000, 5, 0}};
	  uint64_t impactLatencyMatrix [2][2] = {{0, 5}, 
	                                         {5, 0},};   			  
	  m_impactLatency.resize (m_numNodes);
	  for (unsigned i = 0; i < m_numNodes; i++)
	    {
		  m_impactLatency.at (i).resize (m_numNodes);
		  for (unsigned j = 0; j < m_numNodes; j++)
		    {
			  m_impactLatency.at (i).at (j) = impactLatencyMatrix [i][j];	
			}	
		}                           	
	  m_nodesEvents.resize (m_numNodes);
	  m_isNodeVectorInitialized = true;	
	}
 	
  char buf[64];
  memset (buf, 0, sizeof(buf));

  // Only make m_ts of transmit events symbolic
  if (m_isTransmitEvent)
    {
          (const_cast<Event&>(ev)).key.m_isTransEvent = true;
          (const_cast<Event&>(ev)).key.m_packetSize = m_currPacketSize;
          if (ev.key.m_packetSize > 58 && m_numSymEvents < 1000)
            {
              m_numSymEvents++;
              snprintf (buf, sizeof(buf), "Setting event id %u to symbolic !!!!!",
                ev.key.m_uid);
              s2e_warning (buf);
              memset (buf, 0, sizeof(buf));
              // New symbolic delay for each data packet
              uint64_t sym_ts;
              s2e_enable_forking ();
              s2e_make_symbolic (&sym_ts, sizeof(uint64_t), "Symbolic Delay");
              if (sym_ts > m_maxBound)
              {
                s2e_kill_state (0, "Out of Range");
              }
              (const_cast<Event&>(ev)).key.m_ts += sym_ts;
            }
          SetTransmitEvent (false);
//        printf("Event to be inserted is a transmit event \n");
    }
  else
    {
      (const_cast<Event&>(ev)).key.m_isTransEvent = false;
      (const_cast<Event&>(ev)).key.m_packetSize = 0;
    } 
  snprintf (buf, sizeof(buf), "New event %u to be inserted", ev.key.m_uid);
  s2e_warning (buf);
  memset (buf, 0, sizeof(buf));

  // insert event into right list
  const_cast<Event&>(ev).key.m_eventType = m_currEventType;
  SetEventType (UNDEFINED);
  if (ev.key.m_context != 0xffffffff)
    {
	  InsertMultiList (m_nodesEvents.at(ev.key.m_context), ev);
	  return;	
	}
  else
    {	
	  InsertMultiList (m_simEvents, ev);
	  return;					
    }
//  printf ("Fail to insert event %u", ev.key.m_uid);

/*// Use in waiting list without multi list 
  const_cast<Event&>(ev).key.m_eventType = m_currEventType;
  SetEventType (UNDEFINED);
  InsertIntoMainList (m_events.begin (), ev);
  */ 
}

bool
ListScheduler::IsEmpty (void) const
{
  NS_LOG_FUNCTION (this);
  
  // Check all lists
  if (!m_simEvents.empty())
    {
	  return false;
	}
  
  for (unsigned j = 0; j < m_nodesEvents.size(); j++)
    {
      if (!(m_nodesEvents.at(j)).empty())
        {
		  return false;
		}  	 	
	}
  	  	 		  	
  return true;

//  return m_events.empty(); 
}

Scheduler::Event
ListScheduler::PeekNext (void) const
{
  NS_LOG_FUNCTION (this);
//  printf ("PeekNext-1-Start \n");
  
  Event i, next;
  
  // Last event should be in simulator list
  if (!m_simEvents.empty ())
    {
	  next = m_simEvents.front();	
    }	
  
  for (unsigned j = 0; j < m_nodesEvents.size(); j++)
    {
      if (!(m_nodesEvents.at(j)).empty())
      {
	    i = (m_nodesEvents.at(j)).front ();
	    if (next.key.m_ts > i.key.m_ts)
	    {
	      next = i; 	   
	    }	  	
	  }		  
    }	       
  return next;
}

void
ListScheduler::CheckFrontEvent (Events &subList)
{
  if (debug)
    {
      printf ("---RemoveNext-1-Start \n");
      printf ("Main list before removing next event size %u \n", subList.size());  
      PrintDebugInfo (subList, 1, 1);
    }
    
  if (subList.empty())
    {
	  return;	
	}  
  EventsI nextI = subList.begin ();
	
  if (!nextI->pendingEvents.empty ()) // pending list is not empty, should be a timeout event
    {
      NS_ASSERT (nextI->key.m_eventType == TIMEOUT);

	  uint64_t size = nextI->pendingEvents.size();	
	  // insert events in pending list into main list until the next timeout event
	  // use for instead of while because of infinite loop
	  // when a pending event is poped and pushed into the same pending list infinitely
      for (uint64_t i = 0; i < size; i++)
	    {
//		  printf ("-RemoveNext-2-Whileloop \n");
//		  printf ("-next event id %u pending list in RemoveNext---", nextI->key.m_uid);
//		  PrintDebugInfo (nextI->pendingEvents, 1, 1);	
          Event ev = nextI->pendingEvents.front ();
	      nextI->pendingEvents.pop_front();
//	      printf ("About to insert event id %u- %lu ms to main list \n",
//		           ev.key.m_uid, ev.key.m_ts);
	      InsertBackToMainList_FrontIsTimeout (subList, ev);		      		      	
        }   				
    }
    
  if (debug)
    {
	  printf ("Main list after extracting events from waiting list size %u \n", subList.size());  
      PrintDebugInfo (subList, 1, 1); 	
	}	      	
}

Scheduler::Event
ListScheduler::RemoveNext (void)
{
  NS_LOG_FUNCTION (this);
  
  bool waitingList = true;
  // if using waiting list, first check if any front event is timeout one
  // if there is, extract events from its waiting list
  if (waitingList) // multi list and waiting list
    {
	  CheckFrontEvent (m_simEvents);
	  for (unsigned i = 0; i < m_nodesEvents.size(); i++)
	    {
		  CheckFrontEvent (m_nodesEvents.at (i));	
		}	
	}      

  Event next;
  bool foundNext = true;
  if (!m_simEvents.empty ())
    {	  
	  next = m_simEvents.front();
//	  printf ("Front simulator event id %u - %lu ms - node %u\n",
//	          next.key.m_uid, next.key.m_ts, next.key.m_context);	
	  for (unsigned i = 0; i < m_nodesEvents.size (); i++)
	    {
		  // simulator events have impact latency of 0	
		  if (!m_nodesEvents.at (i).empty () &&
		      next.key.m_ts > m_nodesEvents.at (i).front ().key.m_ts + 0)
		    {
			  if (debug)
			    {	
			      printf ("Next event is not a simulator one.\n");
			      printf ("Front event id %u - %llu ms at node %u.\n", m_nodesEvents.at (i).front ().key.m_uid,
			              m_nodesEvents.at (i).front ().key.m_ts, m_nodesEvents.at (i).front ().key.m_context);
			    }	
			  foundNext = false;
			  break;	
			} 	
		}
	}

  if (foundNext)
    {
//      printf ("Removing next event: %u, time: %lu \n", next.key.m_uid, next.key.m_ts);
      m_simEvents.pop_front ();
      return next;
    }
// foundNext is false here so set it to true at the beginning of for loop    		
// next eligible event is in one of the node lists  
  for (unsigned i = 0; i < m_nodesEvents.size (); i++)
    {
	  foundNext = true;	
      if (!m_nodesEvents.at (i).empty ())
        {
		  next = m_nodesEvents.at (i).front ();
		  // pick the front event of a non-empty node list
		  // compare it to the front event of other node lists
		  for (unsigned j = 0; j < m_nodesEvents.size (); j++)
		    {
			  if (i != j && !m_nodesEvents.at (j). empty ()
			      && next.key.m_ts >= m_nodesEvents.at (j).front ().key.m_ts + m_impactLatency.at (i).at (j))
			    {
				  foundNext = false;
				  break;	
				}    	
			}
		  if (foundNext)
		    {
//			  printf ("Removing next event: %u, time: %lu \n", next.key.m_uid, next.key.m_ts);
			  m_nodesEvents.at (i).pop_front ();	
			  return next;	
			}		
		}  	 	
	}
  	  	 		  	    
//  printf ("Error: Shoud not reach this statement \n");
  return next;
}

void
ListScheduler::RemoveFromSubList (Events &subList, const Event &ev)
{
  // If there is no waiting list, use this	
/*
  for (EventsI i = subList.begin (); i != subList.end (); i++)
    {
      if (i->key.m_uid == ev.key.m_uid)
        {
          NS_ASSERT (ev.impl == i->impl);
          subList.erase (i);
//          printf ("Removing event: %u, time: %lu \n", ev.key.m_uid, ev.key.m_ts);
          return;
        }
    }
*/    
  //Event to be removed is intially in the main list	
  for (EventsI i = subList.begin (); i != subList.end (); i++)
    {		
      if (i->key.m_uid == ev.key.m_uid)
        {
          NS_ASSERT (ev.impl == i->impl);
//          NS_ASSERT (i->key.m_eventType == TIMEOUT);
                  
          while (!i->pendingEvents.empty ())
            {
//			  printf ("Remove-1, i id %u - %lu ms, pending list size %lu \n",
//			          i->key.m_uid, i->key.m_ts, i->pendingEvents.size());
			  if (debug)
			  {        
			    printf ("-next pending list in Remove---");
		        PrintDebugInfo (i->pendingEvents, 1, 1);
		      }        	
		      Event pendingEv = i->pendingEvents.front ();
		      i->pendingEvents.pop_front ();
		      // event ev is removed, insert pending events from ev++
		      // do not need to compare pendingEv with ev
		      for (EventsI j = subList.begin (); j != subList.end (); j++)
		        {
				  if (j->key.m_uid == ev.key.m_uid)
				    {
					  j++;
					  InsertWaitingList (subList, j, pendingEv);
					  break;	
					}	  
		        }	  
		      
//		      printf ("i id %u - %lu ms \n", i->key.m_uid, i->key.m_ts);
//		      printf ("i id %u - %lu ms, pending list size %lu \n", 
//		              i->key.m_uid, i->key.m_ts, i->pendingEvents.size());	        
		    }	
          
          subList.erase (i);
//          printf ("Finish Removing cancelled event: %u, time: %lu \n", i->key.m_uid, i->key.m_ts);
          return;
        }
    }
    
    // Event to be removed is initially not in the main list
    for (EventsI i = subList.begin (); i != subList.end (); i++)
      {
		// Event to be removed is just inserted into main list by above loop  
		if (i->key.m_uid == ev.key.m_uid)
		  {
			subList.erase (i);
			return;  
		  }
		// Event to be removed is in some pending list  
		if (!i->pendingEvents.empty())
		  {
			for (EventsI j = i->pendingEvents.begin (); j != i->pendingEvents.end(); j++)
			  {
				if (j->key.m_uid == ev.key.m_uid)
				  {
					i->pendingEvents.erase(j); 
					return; 
				  }	    
			  }	    
		  }	   	    
	  }    
}

void
ListScheduler::Remove (const Event &ev)
{
  NS_LOG_FUNCTION (this << &ev);
  
  if (ev.key.m_context != 0xffffffff)
    {	  		
	  RemoveFromSubList (m_nodesEvents.at(ev.key.m_context), ev);
	  return;	
	}
  else
    {	
	  RemoveFromSubList (m_simEvents, ev);
	  return;					
    }
     
  NS_ASSERT (false);
}

} // namespace ns3
