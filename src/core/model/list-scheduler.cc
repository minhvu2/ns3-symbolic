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
  m_currNode = 0;		
  NS_LOG_FUNCTION (this);
}
ListScheduler::~ListScheduler ()
{
}

// <M>
uint32_t ListScheduler::m_numpackets = 0;
uint64_t ListScheduler::m_interval = 1024;
//uint64_t ListScheduler::m_symTime = 0;
uint64_t ListScheduler::m_firstSymPacket = 0;
uint32_t ListScheduler::m_symLink[2] = {0xffffffff, 0xffffffff};
uint64_t ListScheduler::m_packetId = 0;
std::vector<Scheduler::Node> ListScheduler::m_nodes;

bool ListScheduler::m_usePathReduction = true;
bool ListScheduler::m_useWaitingList = true;
bool ListScheduler::m_useLocalList = true;
bool ListScheduler::m_useEndOfSim = true;
bool ListScheduler::m_useLocalListv2 = true;

bool ListScheduler::m_isTransmitEvent = false;
uint32_t ListScheduler::m_numNodes = 2;
uint32_t ListScheduler::m_currPacketSize = 0;
Scheduler::EventSchedulers_t ListScheduler::m_currEventType = UNDEFINED;
bool ListScheduler::m_isNodeVectorInitialized = false;
bool ListScheduler::debug = false;

void
ListScheduler::SetPacketId (uint64_t packetId)
{
  m_packetId = packetId;
}

void
ListScheduler::SetSymLink (uint32_t node1, uint32_t node2)
{
  m_symLink[0] = node1;
  m_symLink[1] = node2;
}

void
ListScheduler::SetFirstSymPacket (uint64_t firstSymPacket)
{
  m_firstSymPacket = firstSymPacket;
}

void
ListScheduler::SetInterval (uint64_t interval)
{
  m_interval = interval;
}

void
ListScheduler::SetNumberSymPackets (uint32_t numpackets)
{
  m_numpackets = numpackets;
}

void
ListScheduler::SetInterfaceInfo (std::vector<std::vector<uint32_t> > interfaces)
{
  m_nodes.resize (interfaces.size ());	
  for (unsigned  i = 0; i < interfaces.size (); i++)
    {
	  m_nodes.at (i).m_context = i;
	  m_nodes.at (i).m_isWaiting = false;
	  m_nodes.at (i).m_interface = interfaces.at (i);
	  m_nodes.at (i).m_inPackets. assign (interfaces.at (i).size (), 0);        	
    }
  //if (true)
    //{
	  //for (unsigned  i = 0; i < m_nodes.size (); i++)
		//{
		  //printf ("Node %u: ", m_nodes.at (i).m_context);
		  //uint32_t interfaceNum = 1;
		  //for (unsigned  j = 0; j < m_nodes.at (i).m_interface.size (); j++)
		    //{
			  //printf ("interface %u - node %u", interfaceNum, m_nodes.at (i).m_interface.at (j));	
			//}
		  //printf ("\n");       	
		//}	
    //}	  			
}

void
ListScheduler::SetPathReduction (bool value)
{
  m_usePathReduction = value;
}

void
ListScheduler::SetWaitingList (bool value)
{
  m_useWaitingList = value;
}

void
ListScheduler::SetLocalList (bool value)
{
  m_useLocalList = value;
}

void
ListScheduler::SetEndOfSim (bool value)
{
  m_useEndOfSim = value;	
}

void
ListScheduler::SetLocalListv2 (bool value)
{
  m_useLocalListv2 = value;	
}

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

// Reduce the number of comparisions when inserting events
bool
ListScheduler::InsertPathReduction (EventsI i, const Event &ev)
{
  // m_uid are used as a tiebreaker when m_ts are equal 		
  if (ev.key.m_uid < i->key.m_uid)
	{
      if (ev.key.m_ts <= i->key.m_ts)
        {		  	
	      return true;	
	    }	
    }
  else // m_uid of ev is larger than that of i
	{
      if (ev.key.m_ts < i->key.m_ts)
        {
	      return true;	
	    }		
    }
  return false;         	
}

// First time an event is inserted into a list
void
ListScheduler::InsertMultiList (Events &subList, const Event &ev)
{
  //if (debug)
    //{
	  //printf ("InsertIntoSubList-1\n");  
	//}

  if (m_useWaitingList == false) // Local lists without waiting lists
    {
	  if (m_usePathReduction == true)
	    {	  
		  for (EventsI i = subList.begin (); i != subList.end (); i++)
			{
			  if (InsertPathReduction (i, ev))
				{
				   subList.insert (i, ev);
				   //PrintDebugInfo (subList, ev.key.m_uid, i->key.m_uid);
				   return;
				}
			  // If i is the end of simulation event and the above insertion condition fails,
              // then ev must happen after i.
              // Thus we push ev at the end of the list.	
			  if (m_useEndOfSim == true && i->key.m_eventType == STOP)
			    {
				  subList.push_back (ev);
				  return;	
				}	
			}
		  subList.push_back (ev);
		  //PrintDebugInfo (subList, ev.key.m_uid, ev.key.m_uid);
		  return;
		}  
			  
	  if (m_usePathReduction == false)
	    {
		  for (EventsI i = subList.begin (); i != subList.end (); i++)
		    {
		      if (ev.key < i->key)
		        {
		          subList.insert (i, ev);
		          return;
		        }
		        
		      if (m_useEndOfSim == true && i->key.m_eventType == STOP)
		        {
				  subList.push_back (ev);
				  return;	
				}    
		    }
		  subList.push_back (ev);
		  return;		
	    }	
    }
    
  if (m_useWaitingList == true) // With waiting list
    {
	  InsertWaitingList (subList, subList.begin (), ev);
	  return;	
	}  
}

// Only use when we try to remove the next event and it has a waiting list
// First we have to extract events in that waiting list and insert into regular list
void
ListScheduler::InsertBackToMainList_FrontIsTimeout (Events &subList, const Event &ev)
{
  Event next = subList.front ();
  if (m_usePathReduction == true)
    {
	  for (EventsI i = subList.begin (); i != subList.end (); i++)
	    {
		  //printf ("Current event iterator %u \n", i->key.m_uid);
		  
		  // Insert to pending list of timeout event
		  // If this timeout event is at the front, do not insert
		  // If we insert ev into pending list of a front timeout event,
		  // there will be infinite loop
		  if (i->key.m_eventType == TIMEOUT && i->key.m_uid != next.key.m_uid)
		    {
			  i->pendingEvents.push_back(ev);
			  //printf ("Pushing event %u into pending list of event %u, size %lu \n", 
			  //	           ev.key.m_uid, i->key.m_uid, i->pendingEvents.size());			  
			  return;
			}
		  // i is not a timeout event, compare timestamp		
	      if (InsertPathReduction (i, ev))
	        { 	
			  subList.insert (i, ev);
			  //printf ("Inserting event %u into main list\n", ev.key.m_uid);	
              //PrintDebugInfo (ev.key.m_uid, i->key.m_uid);
			  return;	                
	        }
	        
	      if (m_useEndOfSim == true && i->key.m_eventType == STOP)
		    {
			  subList.push_back (ev);
			  return;	
			}	   
	    }
	  subList.push_back (ev);
	  return;
    }
    
  if (m_usePathReduction == false)
    {
	  for (EventsI i = subList.begin (); i != subList.end (); i++)
	    {
		  if (i->key.m_eventType == TIMEOUT && i->key.m_uid != next.key.m_uid)
		    {
			  i->pendingEvents.push_back(ev);
			  //printf ("Pushing event %u into pending list of event %u, size %lu \n", 
			  //	           ev.key.m_uid, i->key.m_uid, i->pendingEvents.size());			  
			  return;
			}
						
	      if (ev.key < i->key)
	        {
	          subList.insert (i, ev);
	          return;
	        }
	      
	      if (m_useEndOfSim == true && i->key.m_eventType == STOP)
		    {
			  subList.push_back (ev);
			  return;	
			}	   
	    }
	  subList.push_back (ev);
	  return;		
    }
}	

// Inserting event when there is waiting lists
void
ListScheduler::InsertWaitingList (Events &subList, EventsI start, const Event &ev)
{
  //if (debug)	
    //{
      //printf("InsertIntoMainList-1-Start \n");
      //PrintDebugInfo (subList, 1, 1);
      //printf ("Inserting loop start at event id %u \n", start->key.m_uid);
    //}  
   
  if (m_usePathReduction == true)
    {   
	  for (EventsI i = start; i != subList.end (); i++)
	    {
		  // insert to pending list of timeout event	
		  if (i->key.m_eventType == TIMEOUT)
		    {
			  i->pendingEvents.push_back(ev);
			  //printf ("Pushing event %u into pending list of event %u, size %lu \n", 
			  //	           ev.key.m_uid, i->key.m_uid, i->pendingEvents.size());			  
			  return;
			}		
		  //printf ("Current event iterator %u \n", i->key.m_uid);
		  if (InsertPathReduction (i, ev))
			{
			  subList.insert (i, ev);
			  //printf ("Inserting event %u into sub list\n", ev.key.m_uid);
			  //PrintDebugInfo (subList, ev.key.m_uid, i->key.m_uid);
			  return;
			}
			
		  if (m_useEndOfSim == true && i->key.m_eventType == STOP)
			{
			  subList.push_back (ev);
			  return;	
			}				
	    }
	  subList.push_back (ev);
	  return;
    }
 
  if (m_usePathReduction == false)
    {
	  for (EventsI i = start; i != subList.end (); i++)
	    {
		  if (i->key.m_eventType == TIMEOUT)
		    {
			  i->pendingEvents.push_back(ev);
			  //printf ("Pushing event %u into pending list of event %u, size %lu \n", 
			  //	           ev.key.m_uid, i->key.m_uid, i->pendingEvents.size());			  
			  return;
			}
						
	      if (ev.key < i->key)
	        {
	          subList.insert (i, ev);
	          //PrintDebugInfo (subList, ev.key.m_uid, i->key.m_uid);
	          return;
	        }
	        
	      if (m_useEndOfSim == true && i->key.m_eventType == STOP)
		    {
			  subList.push_back (ev);
			  return;	
			}  
	    }
	  subList.push_back (ev);
	  return;		
    }
}			

void
ListScheduler::Insert (const Event &ev)
{
  NS_LOG_FUNCTION (this << &ev);
  if (!m_isNodeVectorInitialized)
    {
	  //uint64_t inf = 100000000;	
	  // tcp-bulk-send
	  uint64_t impactLatencyMatrix [2][2] = {{0, 0}, 
	                                         {0, 0}};
	                                         	                                        	                                         
	  // rip-simple-network
	  //uint64_t impactLatencyMatrix [6][6] = {{0, 8, 2, 4, 4, 6}, 
	                                         //{8, 0, 6, 4, 4, 2},
	                                         //{2, 6, 0, 2, 2, 4},
	                                         //{4, 4, 2, 0, 2, 2},
	                                         //{4, 4, 2, 2, 0, 2},
	                                         //{6, 2, 4, 2, 2, 0}};
	                                         
	  // simple-error-model
	  //uint64_t impactLatencyMatrix [4][4] = {{0, inf, 2, inf}, 
	                                         //{inf, 0, 2, 13},
	                                         //{2, 2, 0, 11},
	                                         //{inf, 13, 11, 0}};
	  //uint64_t impactLatencyMatrix [4][4] = {{0, inf, 0, inf}, 
	                                         //{inf, 0, 0, 0},
	                                         //{0, 0, 0, 0},
	                                         //{inf, 0, 0, 0}};                                                                                 			  
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
      if ((m_symLink[0] == 0 && m_symLink[1] == 2) || (m_symLink[0] == 2 && m_symLink[1] == 0))
        {
          m_packetId++;
          //if (ev.key.m_packetSize > 58 && m_numSymEvents < 1000)
          if (m_packetId >= m_firstSymPacket && m_numpackets > 0)
            {
			  m_numpackets--;	
              snprintf (buf, sizeof(buf), "Setting event id %u to symbolic !!!!!", ev.key.m_uid);
              s2e_warning (buf);
              memset (buf, 0, sizeof(buf));
              // New symbolic delay for each data packet
              uint64_t sym_ts;
              s2e_enable_forking ();
              s2e_make_symbolic (&sym_ts, sizeof(uint64_t), "Symbolic Delay");
              if (sym_ts > m_interval)
                {
                  s2e_kill_state (0, "Out of Range");
                }
              (const_cast<Event&>(ev)).key.m_ts += sym_ts;
              s2e_print_expression ("SymTime", ev.key.m_ts);
            }
        }
	  SetSymLink (0xffffffff, 0xffffffff);  
      SetTransmitEvent (false);
//    printf("Event to be inserted is a transmit event \n");
    }
  else
    {
      (const_cast<Event&>(ev)).key.m_isTransEvent = false;
      (const_cast<Event&>(ev)).key.m_packetSize = 0;
    } 
  //snprintf (buf, sizeof(buf), "New event %u to be inserted", ev.key.m_uid);
  //s2e_warning (buf);
  //memset (buf, 0, sizeof(buf));

  const_cast<Event&>(ev).key.m_eventType = m_currEventType;
  SetEventType (UNDEFINED);
  
  // Insert events into right list if local lists are used  
  if (m_useLocalList == true)
    {
	  if (m_useLocalListv2 == true && ev.key.m_eventType == OUTGOING)
	    {
		  InsertMultiList (m_nodesEvents.at (ev.key.m_prevContext), ev);
		  return;
		}	
      if (ev.key.m_context != 0xffffffff)
	    {
	      InsertMultiList (m_nodesEvents.at (ev.key.m_context), ev);
	      return;	
	    }
      else
        {	
	      InsertMultiList (m_simEvents, ev);
	      return;					
        }
    }
  
  // Waiting lists without local lists  
  if (m_useWaitingList == true)
    {
	  InsertWaitingList (m_events, m_events.begin (), ev);
	  return;	
	}
	
  // No waiting list or local list used but with path reduction	
  if (m_usePathReduction == true)
    {
	  for (EventsI i = m_events.begin (); i != m_events.end (); i++)
	    {
		  if (InsertPathReduction (i, ev))
		    {
			  m_events.insert (i, ev);
			  return;	
			}			
		  // use end of simulation	
		  if (m_useEndOfSim == true && i->key.m_eventType == STOP)
		    {
			  m_events.push_back (ev);
			  return;	
			}		
		}
	  m_events.push_back (ev);
	  return;		
    }
  
  // No technique used except end of simulation
  //if (m_usePathReduction == false)
    //{
	  for (EventsI i = m_events.begin (); i != m_events.end (); i++)
	    {
		  if (ev.key < i->key)
		    {
			  m_events.insert (i, ev);
			  return;	
			}
		  // use end of simulation	
		  if (m_useEndOfSim == true && i->key.m_eventType == STOP)
		    {
			  m_events.push_back (ev);
			  return;	
			}		
		}
	  m_events.push_back (ev);
	  return;		
	//}  	 

  //snprintf (buf, sizeof (buf), "Fail to insert event id %u", ev.key.m_uid);
  //s2e_warning (buf);
  //memset (buf, 0, sizeof (buf));
}

bool
ListScheduler::IsEmpty (void) const
{
  NS_LOG_FUNCTION (this);
  
  // Check all lists
  if (!m_simEvents.empty ())
    {
	  return false;
	}
  
  for (unsigned j = 0; j < m_nodesEvents.size (); j++)
    {
      if (!(m_nodesEvents.at (j)).empty ())
        {
		  return false;
		}  	 	
	}
	
  if (!m_events.empty ())
    {
	  return false;	
	}	
  	  	 		  	
  return true;

//  return m_events.empty(); 
}

Scheduler::Event
ListScheduler::PeekNext (void) const
{
  NS_LOG_FUNCTION (this);
//  printf ("PeekNext-1-Start \n");
  
  Event next;
  if (m_useLocalList == true)
    {
	  Event i;
	  
	  // Last event should be in simulator list
	  // Todo: double check this
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
    }
    
  if (m_useLocalList == false)
    {
	  next = m_events.front ();	
	}
  
  return next;		
}

void
ListScheduler::CheckFrontEvent (Events &subList)
{
  //if (debug)
    //{
      //printf ("---RemoveNext-1-Start \n");
      //printf ("Main list before removing next event size %u \n", subList.size());  
      //PrintDebugInfo (subList, 1, 1);
    //}
    
  //if (subList.empty())
    //{
	  //return;	
	//}
	  
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
		  //printf ("-RemoveNext-2-Whileloop \n");
		  //printf ("-next event id %u pending list in RemoveNext---", nextI->key.m_uid);
		  //PrintDebugInfo (nextI->pendingEvents, 1, 1);	
          Event ev = nextI->pendingEvents.front ();
	      nextI->pendingEvents.pop_front();
	      //printf ("About to insert event id %u- %lu ms to main list \n",
		  //         ev.key.m_uid, ev.key.m_ts);
	      InsertBackToMainList_FrontIsTimeout (subList, ev);		      		      	
        }   				
    }
    
  //if (debug)
    //{
	  //printf ("Main list after extracting events from waiting list size %u \n", subList.size());  
      //PrintDebugInfo (subList, 1, 1); 	
	//}	      	
}

bool
ListScheduler::IsDeadLock ()
{
  for (unsigned i = 0; i < m_nodes.size (); i++)
    {
	  if (m_nodes.at (i).m_isWaiting == false)
	    {
		  return false;	
		}	
	}
  return true;		
}

void
ListScheduler::DecrementInPacket (const Event &ev)
{
  uint32_t interface = GetInterface (ev.key.m_prevContext, ev.key.m_context);
  m_nodes.at (ev.key.m_context).m_inPackets.at (interface) -= 1; 	
}

bool
ListScheduler::HasIncomingOnAllInterfaces (uint32_t nodeID)
{
  for (unsigned i = 0; i < m_nodes.at (nodeID).m_inPackets.size (); i++)
    {
	  if (m_nodes.at (nodeID).m_inPackets.at (i) == 0)
	    {
		  return false;	
		}	
	}
  return true;		
}

uint32_t
ListScheduler::GetInterface (uint32_t src, uint32_t dst)
{
  uint32_t interface = 100;	
  for (unsigned i = 0; i < m_nodes.at (dst).m_interface.size (); i++)
    {
	  if (m_nodes.at (dst).m_interface. at(i) == src)
	    {	
		  interface = i;
		  break;	
		}	
	}
  return interface;		
}

Scheduler::Event
ListScheduler::RemoveNextLocalList (void)
{
  // If using waiting list, first check if any front event is timeout one
  // If there is, extract events from its waiting list
  if (m_useWaitingList == true) // local lists and waiting lists
    {
	  if (!m_simEvents.empty () && !m_simEvents.begin ()->pendingEvents.empty())
	    {	
	      CheckFrontEvent (m_simEvents);
	    }  
	  for (unsigned i = 0; i < m_nodesEvents.size(); i++)
	    {
		  if (!m_nodesEvents.at (i).empty () && !m_nodesEvents.at (i).begin ()->pendingEvents.empty())
		    {	
		      CheckFrontEvent (m_nodesEvents.at (i));	
		    }
		}
	}

  // After extracting events from waiting list of all front events,
  // find the next event to execute
  Event next;
  bool foundNext = true;
  if (!m_simEvents.empty ())
    {	  
	  next = m_simEvents.front();
	  //printf ("Front simulator event id %u - %lu ms - node %u\n",
	  //        next.key.m_uid, next.key.m_ts, next.key.m_context);	
	  for (unsigned i = 0; i < m_nodesEvents.size (); i++)
	    {
		  // simulator events have impact latency of 0 to other nodes
		  if (!m_nodesEvents.at (i).empty ())
		    {
			  // if simulator event id is smaller	
		      if (next.key.m_uid < m_nodesEvents.at (i).front ().key.m_uid)
		        {	
		          if (next.key.m_ts > m_nodesEvents.at (i).front ().key.m_ts)
		            {
			          //if (debug)
			            //{	
			              //printf ("Next event is not a simulator one.\n");
			              //printf ("Front event id %u - %lu ms at node %u.\n", m_nodesEvents.at (i).front ().key.m_uid,
			              //        m_nodesEvents.at (i).front ().key.m_ts, m_nodesEvents.at (i).front ().key.m_context);
			          //}	
			          foundNext = false;
			          break;
				    }
			    }
			  else // simulator event id is larger thus its timestamp must be strictly lesser
			    {
				  if (next.key.m_ts >= m_nodesEvents.at (i).front ().key.m_ts)
				    {
					  foundNext = false;
					  break;	 
					} 	
				}    	
			} 	
		}
	}

  if (foundNext && !m_simEvents.empty ())
    {
      //printf ("Removing next simulator event: %u - %llu ms \n", next.key.m_uid, next.key.m_ts);
      //PrintDebugInfo (m_simEvents, 1, 1);
      m_simEvents.pop_front ();
      return next;
    }
    
  if (m_useLocalListv2 == true)
  {   
    uint32_t nodeID = 0;
    while (true)
      {
		//front event at this node can safely execute  
		if (HasIncomingOnAllInterfaces (nodeID))
		  {
			m_nodes.at (nodeID).m_isWaiting = false;
			next = m_nodesEvents.at (nodeID).front ();
			if (next.key.m_eventType == OUTGOING)
			  {
				if (nodeID != next.key.m_prevContext)
				  {
				    printf ("Error: Local list v2 !!!!!!!!");
				  }  
				uint32_t interface = GetInterface (next.key.m_prevContext, next.key.m_context);
				m_nodes.at (next.key.m_context).m_inPackets.at (interface) += 1;
				m_nodesEvents.at (nodeID).pop_front ();
				next.key.m_eventType = INCOMING;
				InsertMultiList (m_nodesEvents.at (next.key.m_context), next);
				m_nodes.at (next.key.m_context).m_isWaiting = false;  
			  }
			else
			  {
				m_nodesEvents.at (nodeID).pop_front ();
				return next;  
			  }    
		  }
		//set current nodes to wait status and move to next node 
		//check if there is deadlock  
		else
		  {
			m_nodes.at (nodeID).m_isWaiting = true;
			nodeID = (nodeID + 1) % m_nodes.size ();
			if (IsDeadLock ())
			  {
				break;  
			  }  
		  }    
	  }
  }    
    
// Next eligible event is in one of the node lists  
  for (unsigned i = 0; i < m_nodesEvents.size (); i++)
    {
	  foundNext = true;	
      if (!m_nodesEvents.at (m_currNode).empty ())
        {
		  next = m_nodesEvents.at (m_currNode).front ();
		  // Pick the front event of a non-empty node list
		  // Compare it to the front event of other node lists
		  for (unsigned j = 0; j < m_nodesEvents.size (); j++)
		    {
			  if (next.key.m_eventType == STOP)
			    {
				  foundNext = false;
				  break;	
				}		
			  if (m_currNode != j && !m_nodesEvents.at (j). empty ()
			      && next.key.m_ts > m_nodesEvents.at (j).front ().key.m_ts + m_impactLatency.at (m_currNode).at (j))
			  //if  (m_currNode != j && !m_nodesEvents.at (j). empty ()
			       //&& !InsertPathReduction (m_nodesEvents.at (j).begin (), next))    
			    {
				  foundNext = false;
				  break;	
				}    	
			}
		  if (foundNext)
		    {
			  //printf ("Removing next event: %u - %llu ms, i = %u \n", next.key.m_uid, next.key.m_ts, i);
			  //printf ("Node list %u size %lu", i, m_nodesEvents.at (i).size ());
			  m_nodesEvents.at (m_currNode).pop_front ();
			  m_currNode = (m_currNode + 1) % m_nodesEvents.size();	
			  return next;	
			}		
		}
	  m_currNode = (m_currNode + 1) % m_nodesEvents.size();	  	 	
	}
  	  	 		 	    
  //printf ("Error: Shoud not reach this statement \n");
  return next;
}

Scheduler::Event
ListScheduler::RemoveNext (void)
{
  NS_LOG_FUNCTION (this);
  
  Event next;
  if (m_useLocalList == true)
    {
	  next = RemoveNextLocalList ();
	  return next; 	
	}
	
  if (m_useWaitingList == true)
    {
	  CheckFrontEvent (m_events);
	  next = m_events.front ();
	  m_events.pop_front ();
	  return next;	
	}

  //if (m_useWaitingList == false)
    //{
	  next = m_events.front ();
	  m_events.pop_front ();
	  return next;			
	//}		

  //printf ("Error: Should not reach this statement \n");
  //return next;  
}

void
ListScheduler::RemoveWaitingList (Events &subList, const Event &ev)
{
  // Event to be removed is intially in the main list	
  for (EventsI i = subList.begin (); i != subList.end (); i++)
    {		
      if (i->key.m_uid == ev.key.m_uid)
        {
          NS_ASSERT (ev.impl == i->impl);
          //NS_ASSERT (i->key.m_eventType == TIMEOUT);
                  
          while (!i->pendingEvents.empty ())
            {
			  //printf ("Remove-1, i id %u - %lu ms, pending list size %lu \n",
			  //        i->key.m_uid, i->key.m_ts, i->pendingEvents.size());
			  //if (debug)
			  //{        
			    //printf ("-next pending list in Remove---");
		        //PrintDebugInfo (i->pendingEvents, 1, 1);
		      //}        	
		      Event pendingEv = i->pendingEvents.front ();
		      i->pendingEvents.pop_front ();
		      // Event ev is removed, insert pending events from ev++
		      // Do not need to compare pendingEv with ev
		      for (EventsI j = subList.begin (); j != subList.end (); j++)
		        {
				  if (j->key.m_uid == ev.key.m_uid)
				    {
					  j++;
					  InsertWaitingList (subList, j, pendingEv);
					  break;	
					}	  
		        }	  		      
		      //printf ("i id %u - %lu ms \n", i->key.m_uid, i->key.m_ts);
		      //printf ("i id %u - %lu ms, pending list size %lu \n", 
		      //        i->key.m_uid, i->key.m_ts, i->pendingEvents.size());	        
		    }	
          
          subList.erase (i);
          //printf ("Finish Removing cancelled event: %u, time: %lu \n", i->key.m_uid, i->key.m_ts);
          return;
        }
    }
    
  // Event to be removed is initially not in the main list
  for (EventsI i = subList.begin (); i != subList.end (); i++)
    {
	  // Event to be removed is just inserted into main list by above loop
	  // Todo: check whether this if is redundant  
	  if (i->key.m_uid == ev.key.m_uid)
		{
		  subList.erase (i);
		  return;  
		}
	  // Event to be removed is in some waiting list  
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
ListScheduler::RemoveLocalList (Events &subList, const Event &ev)
{
  if (m_useWaitingList == true)
    {
	  RemoveWaitingList (subList, ev);
	  return;	
	}

  if (m_useWaitingList == false)		
    {	
	  for (EventsI i = subList.begin (); i != subList.end (); i++)
	    {
	      if (i->key.m_uid == ev.key.m_uid)
	        {
	          NS_ASSERT (ev.impl == i->impl);
	          subList.erase (i);
	          //printf ("Removing event: %u, time: %lu \n", ev.key.m_uid, ev.key.m_ts);
	          return;
	        }
	    }
    }  
}

void
ListScheduler::Remove (const Event &ev)
{
  NS_LOG_FUNCTION (this << &ev);
  
  // Local lists are used, remove the event from the corresponding local list
  if (m_useLocalList == true)
    {
	  if (ev.key.m_context != 0xffffffff)
	    {	  		
		  RemoveLocalList (m_nodesEvents.at (ev.key.m_context), ev);
		  return;	
		}
	  else
	    {	
		  RemoveLocalList (m_simEvents, ev);
		  return;					
	    }
    }
  
  // No local list, only waiting list  
  if (m_useWaitingList == true)
    {
	  RemoveWaitingList (m_events, ev);
	  return;	
	}    

  // At this point, local lists and waiting lists are not used
  //if (m_useWaitingList == false)
    //{
	  for (EventsI i = m_events.begin (); i != m_events.end (); i++)
		{
		  if (i->key.m_uid == ev.key.m_uid)
			{
			  NS_ASSERT (ev.impl == i->impl);
			  m_events.erase (i);
			  return;
			}
		}
	//}  
     
  //NS_ASSERT (false);
}

uint32_t
ListScheduler::RemoveAll (uint32_t context)
{
  uint32_t num = 0;	
  while (!m_nodesEvents.at (context). empty())
    {
	  Event ev = m_nodesEvents.at (context).front ();
	  m_nodesEvents.at (context).pop_front ();
	  ev.impl->Unref ();
	  num++;	
	}
  return num;		
}

} // namespace ns3
