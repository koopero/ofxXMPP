/*
 * ofxXMPP.h
 *
 *  Created on: Aug 24, 2013
 *      Author: arturo
 */

#ifndef OFXXMPP_H_
#define OFXXMPP_H_

#include "ofThread.h"
#include "ofConstants.h"
#include "strophe.h"
#include <map>
#include <queue>
#include "ofEvents.h"
#include "ofLog.h"

#include "Poco/Condition.h"


enum ofxXMPPChatState{
	ofxXMPPChatStateActive,
	ofxXMPPChatStateInactive,
	ofxXMPPChatStateGone,
	ofxXMPPChatStateComposing,
	ofxXMPPChatStatePaused
};

enum ofxXMPPShowState{
	ofxXMPPShowAvailable,
	ofxXMPPShowAway,
	ofxXMPPShowDnd,
	ofxXMPPShowXA
};

enum ofxXMPPConnectionState{
	ofxXMPPConnected,
	ofxXMPPConnecting,
	ofxXMPPDisconnected
};

enum ofxXMPPTerminateReason{
	ofxXMPPTerminateSuccess,
	ofxXMPPTerminateBusy,
	ofxXMPPTerminateDecline,
	ofxXMPPTerminateUnkown
};

struct ofxXMPPUser{
	std::string userName;
	std::string status;
	ofxXMPPShowState show;
	ofxXMPPChatState chatState;
	int priority;
	std::string resource;
	std::vector<std::string> capabilities;
};

struct ofxXMPPMessage{
	std::string from;
	std::string type;
	std::string body;
};

struct ofxXMPPPayload{
	int id;  //typically 96 - 127
	std::string name;
	int clockrate;
};

#ifndef USE_OFX_NICE
struct ofxICECandidate{
	int component;
	std::string foundation;
	int generation;
	std::string id;
	std::string ip;
	int network;
	int port;
	int priority;
	std::string protocol;
	std::string type;
};
#endif

struct ofxXMPPICETransport{
	std::string pwd;
	std::string ufrag;
	std::vector<ofxICECandidate> candidates;
};

struct ofxXMPPJingleContent{
	std::string name;
	std::string media;
	std::vector<ofxXMPPPayload> payloads;
	ofxXMPPICETransport transport;
};

struct ofxXMPPJingleInitiation{
	std::string from;
	std::string sid;
	std::vector<ofxXMPPJingleContent> contents;
};


struct ofxXMPPJingleFileInitiation{
	std::string fid;
	std::string from;
	std::string sid;
	std::string name;
	std::string date;
	std::string desc;
	size_t size;
	std::string hash;  // file hash as sha-1
	ofxXMPPICETransport transport;
};

struct ofxXMPPJingleHash{
	std::string sid;
	std::string fid;
	std::string from;
	std::string hash;
	std::string algo;
};

/// XMPP is a protocol that allows sending messages among to peers through a server
/// It's used among others by jabber or google talk. Apart from chat like messages
/// it allows to send metadata, through the subprotocol jingle, that enables both
/// peers to start a connection to send video, audio... among each other.
/// This class makes it easy to use XMPP and also implements part of the jingle protocol
/// allowing other applications or addons like ofxGstRTP to start a video/audio conference
class ofxXMPP: public ofThread {
public:
	ofxXMPP();
	virtual ~ofxXMPP();

	/// sets the show state of the client, this will tipically change the state icon in clients
	/// as available, away, do not disturb...
	void setShow(ofxXMPPShowState showState);

	/// string that sets the status of the client, will show to other clients as a text message
	/// near the client name
	void setStatus(const std::string & status);

	/// capabilities string that signals to other clients if this client has camera, microphone...
	/// although there are some standard capabilities defined in the jingle RFC 2 clients can use
	/// any string that they agree on to signal that they can start a connection for certain stream
	/// types
	void setCapabilities(const std::string & capabilities);

	/// starts a connection to an XMPP server, passing the host to connect to, the username or jid
	/// and the password
	void connect(const std::string & host, const std::string & jid, const std::string & pass);

	/// stops the connection
	void stop();

	/// sends a message to an specific client, to has to be the username or the specific id of a client
	/// which is usually the username + a hash that we get calling getFriends()
	void sendMessage(const std::string & to, const std::string & message);

	/// returns a vector with all the users in our contact list from which we can know their state,
	/// capabilities...
	std::vector<ofxXMPPUser> getFriends();

	/// returns a vector with all the users in our contact list from which we can know their state,
	/// capabilities...
	std::vector<ofxXMPPUser> getFriendsWithCapability(const std::string & capability);

	/// returns the connection state of this client
	ofxXMPPConnectionState getConnectionState();

	/// returns the JID of this client as returned by the server usually the username + a hash
    std::string getBoundJID();

	// general events

	/// ofAddListener to this event to receive notifications about changes in the connection state
	/// of this client
	ofEvent<ofxXMPPConnectionState> connectionStateChanged;

	/// ofAddListener to this event to receive new messages from other clients
	ofEvent<ofxXMPPMessage> newMessage;

	/// ofAddListener to this event to receive notifications about users in ourcontact list that
	/// connect while we are connected
	ofEvent<ofxXMPPUser> userConnected;

	/// ofAddListener to this event to receive notifications about users in out conatct list
	/// that disconnect while we are connected
	ofEvent<ofxXMPPUser> userDisconnected;

	/// jingle rtp events
	/// this events are mostly used by ofxGstRTP to initiate video/audio connections
	/// with other clients using the jingle protocol but can be used manually:
	///
	/// TODO: check this is correct
	/// usually subscribe to all this events to implement the full jingle workflow,
	/// when a user wants to start a connection with us we'll receive a jingleInitiationReceived
	/// event then we need to call ack() passing the initiation that we just received wait for the
	/// jingleRing event and then the application should do something to notify the user of the
	/// incomming call if the user accepts the call we shoukd call ackRing
	ofEvent<ofxXMPPJingleInitiation> jingleInitiationReceived;
	ofEvent<std::string> jingleInitiationACKd;
	ofEvent<ofxXMPPJingleInitiation> jingleInitiationAccepted;
	ofEvent<ofxXMPPTerminateReason> jingleTerminateReceived;
	ofEvent<std::string> jingleRing;

	/// jingle file transfer events
	ofEvent<ofxXMPPJingleFileInitiation> jingleFileInitiationReceived;
	ofEvent<ofxXMPPJingleFileInitiation> jingleFileInitiationAccepted;
	ofEvent<ofxXMPPJingleHash> hashReceived;
	ofEvent<ofxXMPPJingleHash> hashACKd;



	// RTP-ICE using Jingle  xmpp.org/extensions/xep-0167.html

	/// returns sid for the initiated session
	void initiateRTP(const std::string & to, ofxXMPPJingleInitiation & jingle);
	void ack(const ofxXMPPJingleInitiation & jingle);
	void ring(const ofxXMPPJingleInitiation & jingle);
	void ackRing(const std::string & to, const std::string & sid);
	void acceptRTPSession(const std::string & to, ofxXMPPJingleInitiation & jingle);
	void terminateRTPSession(ofxXMPPJingleInitiation & jingle, ofxXMPPTerminateReason reason);

	// File transfer based on Jingle file transfer but using libnice tcp over udp
	void initiateFileTransfer(const std::string & to, ofxXMPPJingleFileInitiation & jingle);
	void ack(const ofxXMPPJingleFileInitiation & jingle);
	void acceptFileTransfer(ofxXMPPJingleFileInitiation & jingle);
	void sendFileHash(const std::string & to, const ofxXMPPJingleHash & hash);
	void ack(const ofxXMPPJingleHash & hash);

	static std::string toString(ofxXMPPShowState showState);
	static ofxXMPPShowState fromString(std::string showState);


    enum JingleState{
    	Disconnected=0,
    	SessionAccepted,

    	//initiator
    	InitiatingRTP,
    	InitiationACKd,
    	WaitingSessionAccept,

    	// responder
    	GotInitiate,
    	Ringing,
    	RingACKd,
    	AcceptingRTP
    };

    /*enum JingleFileTransferState{
    	FileDisconnected=0,
    	FileSessionAccepted,

    	//initiator
    	FileInitiatingRTP,
    	FileInitiationACKd,
    	FileWaitingSessionAccept,

    	// responder
    	FileWaitingFile,
    	FileGotInitiate,
    	FileRinging,
    	FileRingACKd,
    	FileAcceptingRTP
    };*/

    JingleState getJingleState();


    static std::string LOG_NAME;

private:
	void threadedFunction();

	void sendPressence();

	void update(ofEventArgs & args);
	void fileInitiationReceived(xmpp_stanza_t * iq);
	void rtpInitiationReceived(xmpp_stanza_t * iq);
	void fileInitiationAccepted(xmpp_stanza_t * iq);
	void rtpInitiationAccepted(xmpp_stanza_t * iq);

	ofxXMPPJingleInitiation jingleInititationFromStanza(xmpp_stanza_t * jingle);
	ofxXMPPJingleFileInitiation jingleFileInititationFromStanza(xmpp_stanza_t * iq);

	xmpp_stanza_t * stanzaFromICETransport(const ofxXMPPICETransport & transport);

	static void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status,
            const int error, xmpp_stream_error_t * const stream_error,
            void * const userdata);

	static int presence_handler(xmpp_conn_t * const conn,
				     xmpp_stanza_t * const stanza,
				     void * const userdata);

	static int message_handler(xmpp_conn_t * const conn,
				     xmpp_stanza_t * const stanza,
				     void * const userdata);

	static int iq_handler(xmpp_conn_t * const conn,
				     xmpp_stanza_t * const stanza,
				     void * const userdata);

    static xmpp_ctx_t *ctx;
    xmpp_conn_t *conn;

    ofxXMPPShowState currentShow;
    std::string currentStatus;
    std::string capabilities;

    std::map<std::string, ofxXMPPUser> friends;
    std::queue<ofxXMPPMessage> messageQueue;

    ofxXMPPConnectionState connectionState;
    std::string userName;

    JingleState jingleState;
    //JingleFileTransferState jingleFileTransferState;

    Poco::Condition disconnection;
    bool disconnecting;

    static std::string toString(JingleState state);
    //static string toString(JingleFileTransferState state);
    void addTextChild(xmpp_stanza_t * stanza, const std::string & textstr);

};

#endif /* OFXXMPP_H_ */
