/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2009:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
		Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any other version as decided by the two original authors
	Stephane Magnenat and Valentin Longchamp.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ASEBA_MEDULLA
#define ASEBA_MEDULLA

#include <dashel/dashel.h>
#include <QThread>
#include <QStringList>
#include <QDBusObjectPath>
#include <QDBusAbstractAdaptor>
#include <QDBusMessage>
#include <QMetaType>
#include <QList>
#include "../msg/msg.h"
#include "../msg/descriptions-manager.h"

typedef QList<qint16> Values;

namespace Aseba
{
	/**
	\defgroup medulla Software router of messages on TCP and D-Bus.
	*/
	/*@{*/
	
	class Hub;
	class AsebaNetworkInterface;
	
	//! DBus interface for an event filter
	class EventFilterInterface: public QObject
	{
		Q_OBJECT
		Q_CLASSINFO("D-Bus Interface", "ch.epfl.mobots.EventFilter")
		
		public:
			EventFilterInterface(AsebaNetworkInterface* network) : network(network) { ListenEvent(0); }
			void emitEvent(const quint16 id, const QString& name, const Values& data);
			
		public slots:
			Q_SCRIPTABLE Q_NOREPLY void ListenEvent(const quint16 event);
			Q_SCRIPTABLE Q_NOREPLY void ListenEventName(const QString& name, const QDBusMessage &message);
			Q_SCRIPTABLE Q_NOREPLY void IgnoreEvent(const quint16 event);
			Q_SCRIPTABLE Q_NOREPLY void IgnoreEventName(const QString& name, const QDBusMessage &message);
			Q_SCRIPTABLE Q_NOREPLY void Free();
		
		signals:
			Q_SCRIPTABLE void Event(const quint16, const QString& name, const Values& );
		
		protected:
			AsebaNetworkInterface* network;
	};
	
	//! DBus interface for aseba network
	class AsebaNetworkInterface: public QDBusAbstractAdaptor, public DescriptionsManager
	{
		Q_OBJECT
		Q_CLASSINFO("D-Bus Interface", "ch.epfl.mobots.AsebaNetwork")
		
		protected:
			struct RequestData
			{
				unsigned nodeId;
				unsigned pos;
				QDBusMessage reply;
			};
			
		public:
			AsebaNetworkInterface(Hub* hub);
		
		private slots:
			friend class Hub;
			void processMessage(Message *message, Dashel::Stream* sourceStream);
			friend class EventFilterInterface;
			void sendEventOnDBus(const quint16 event, const Values& data);
			void listenEvent(EventFilterInterface* filter, quint16 event);
			void ignoreEvent(EventFilterInterface* filter, quint16 event);
			void filterDestroyed(EventFilterInterface* filter);
		
		public slots:
			Q_NOREPLY void LoadScripts(const QString& fileName, const QDBusMessage &message);
			QStringList GetNodesList() const;
			QStringList GetVariablesList(const QString& node) const;
			Q_NOREPLY void SetVariable(const QString& node, const QString& variable, const Values& data, const QDBusMessage &message) const;
			Values GetVariable(const QString& node, const QString& variable, const QDBusMessage &message);
			Q_NOREPLY void SendEvent(const quint16 event, const Values& data);
			Q_NOREPLY void SendEventName(const QString& name, const Values& data, const QDBusMessage &message);
			QDBusObjectPath CreateEventFilter();
		
		protected:
			virtual void nodeDescriptionReceived(unsigned nodeId);
			
		protected:
			Hub* hub;
			CommonDefinitions commonDefinitions;
			typedef QMap<QString, unsigned> NodesNamesMap;
			NodesNamesMap nodesNames;
			typedef QList<RequestData*> RequestsList;
			RequestsList pendingReads;
			typedef QMultiMap<quint16, EventFilterInterface*> EventsFiltersMap;
			EventsFiltersMap eventsFilters;
			unsigned eventsFiltersCounter;
	};
	
	/*!
		Route Aseba messages on the TCP part of the network.
		
		This thread only *receives* messages.
		All dispatch, including forwarding, is done in the main thread called by
		the AsebaNetworkInterface class.
	*/
	class Hub: public QThread, public Dashel::Hub
	{
		Q_OBJECT
		
		public:
			/*! Creates the hub, listen to TCP on port, and creates a DBus interace.
				@param verbose should we print a notification on each message
				@param dump should we dump content of each message
				@param forward should we only forward messages instead of transmit them back to the sender
				@param rawTime should the time be printed as integer
			*/
			Hub(unsigned port, bool verbose, bool dump, bool forward, bool rawTime);
			
			/*! Sends a message to Dashel peers.
				Does not delete the message, should be called by the main thread.
				@param message aseba message to send
				@param sourceStream originate of the message, if from Dashel.
			*/
			void sendMessage(Message *message, Dashel::Stream* sourceStream = 0);
			/*! Sends a message to Dashel peers.
				Convenience overload
			*/
			void sendMessage(Message& message, Dashel::Stream* sourceStream = 0);
			
		signals:
			void messageAvailable(Message *message, Dashel::Stream* sourceStream);
			
		private:
			virtual void run();
			virtual void connectionCreated(Dashel::Stream *stream);
			virtual void incomingData(Dashel::Stream *stream);
			virtual void connectionClosed(Dashel::Stream *stream, bool abnormal);
			
		private:
			bool verbose; //!< should we print a notification on each message
			bool dump; //!< should we dump content of CAN messages
			bool forward; //!< should we only forward messages instead of transmit them back to the sender
			bool rawTime; //!< should displayed timestamps be of the form sec:usec since 1970
	};
	
	/*@}*/
};

Q_DECLARE_METATYPE(Values);

#endif
