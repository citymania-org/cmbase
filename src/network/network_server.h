/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file network_server.h Server part of the network protocol. */

#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include "network_internal.h"
#include "core/tcp_listen.h"
#include "../saveload/saveload.h"

class ServerNetworkGameSocketHandler;
/** Make the code look slightly nicer/simpler. */
typedef ServerNetworkGameSocketHandler NetworkClientSocket;
/** Pool with all client sockets. */
using NetworkClientSocketPool = Pool<NetworkClientSocket, ClientPoolID, 8, PoolType::NetworkClient>;
extern NetworkClientSocketPool _networkclientsocket_pool;

/** Class for handling the server side of the game connection. */
class ServerNetworkGameSocketHandler : public NetworkClientSocketPool::PoolItem<&_networkclientsocket_pool>, public NetworkGameSocketHandler, public TCPListenHandler<ServerNetworkGameSocketHandler, PACKET_SERVER_FULL, PACKET_SERVER_BANNED> {
protected:
	std::unique_ptr<class NetworkAuthenticationServerHandler> authentication_handler = nullptr; ///< The handler for the authentication.
	std::string peer_public_key{}; ///< The public key of our client.

	NetworkRecvStatus Receive_CLIENT_JOIN(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_IDENTIFY(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_GAME_INFO(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_AUTH_RESPONSE(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_GETMAP(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_MAP_OK(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_ACK(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_COMMAND(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_CHAT(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_SET_NAME(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_QUIT(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_ERROR(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_RCON(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_NEWGRFS_CHECKED(Packet &p) override;
	NetworkRecvStatus Receive_CLIENT_MOVE(Packet &p) override;

	NetworkRecvStatus SendGameInfo();
	NetworkRecvStatus SendNewGRFCheck();
	NetworkRecvStatus SendWelcome();
	NetworkRecvStatus SendAuthRequest();
	NetworkRecvStatus SendEnableEncryption();

public:
	/** Status of a client */
	enum ClientStatus : uint8_t {
		STATUS_INACTIVE,      ///< The client is not connected nor active.
		STATUS_AUTH_GAME,     ///< The client is authorizing with game (server) password.
		STATUS_IDENTIFY,      ///< The client is identifying itself.
		STATUS_NEWGRFS_CHECK, ///< The client is checking NewGRFs.
		STATUS_AUTHORIZED,    ///< The client is authorized.
		STATUS_MAP_WAIT,      ///< The client is waiting as someone else is downloading the map.
		STATUS_MAP,           ///< The client is downloading the map.
		STATUS_DONE_MAP,      ///< The client has downloaded the map.
		STATUS_PRE_ACTIVE,    ///< The client is catching up the delayed frames.
		STATUS_ACTIVE,        ///< The client is active within in the game.
		STATUS_END,           ///< Must ALWAYS be on the end of this list!! (period).
	};

	uint8_t lag_test = 0; ///< Byte used for lag-testing the client
	uint8_t last_token = 0; ///< The last random token we did send to verify the client is listening
	uint32_t last_token_frame = 0; ///< The last frame we received the right token
	ClientStatus status = STATUS_INACTIVE; ///< Status of this client
	CommandQueue outgoing_queue{}; ///< The command-queue awaiting delivery; conceptually more a bucket to gather commands in, after which the whole bucket is sent to the client.
	size_t receive_limit = 0; ///< Amount of bytes that we can receive at this moment

	std::shared_ptr<struct PacketWriter> savegame = nullptr; ///< Writer used to write the savegame.
	NetworkAddress client_address{}; ///< IP-address of the client (so they can be banned)

	citymania::SavePreset cm_preset; ///< Preset to use for the savegame

	ServerNetworkGameSocketHandler(SOCKET s);
	~ServerNetworkGameSocketHandler();

	std::unique_ptr<Packet> ReceivePacket() override;
	NetworkRecvStatus CloseConnection(NetworkRecvStatus status) override;
	std::string GetClientName() const;

	void CheckNextClientToSendMap(NetworkClientSocket *ignore_cs = nullptr);

	NetworkRecvStatus SendWait();
	NetworkRecvStatus SendMap();
	NetworkRecvStatus SendErrorQuit(ClientID client_id, NetworkErrorCode errorno);
	NetworkRecvStatus SendQuit(ClientID client_id);
	NetworkRecvStatus SendShutdown();
	NetworkRecvStatus SendNewGame();
	NetworkRecvStatus SendRConResult(uint16_t colour, const std::string &command);
	NetworkRecvStatus SendMove(ClientID client_id, CompanyID company_id);

	NetworkRecvStatus SendClientInfo(NetworkClientInfo *ci);
	NetworkRecvStatus SendError(NetworkErrorCode error, const std::string &reason = {});
	NetworkRecvStatus SendChat(NetworkAction action, ClientID client_id, bool self_send, const std::string &msg, int64_t data);
	NetworkRecvStatus SendExternalChat(const std::string &source, TextColour colour, const std::string &user, const std::string &msg);
	NetworkRecvStatus SendJoin(ClientID client_id);
	NetworkRecvStatus SendFrame();
	NetworkRecvStatus SendSync();
	NetworkRecvStatus SendCommand(const CommandPacket &cp);
	NetworkRecvStatus SendConfigUpdate();

	static void Send();
	static void AcceptConnection(SOCKET s, const NetworkAddress &address);
	static bool AllowConnection();

	/**
	 * Get the name used by the listener.
	 * @return the name to show in debug logs and the like.
	 */
	static const char *GetName()
	{
		return "server";
	}

	const std::string &GetClientIP();
	std::string_view GetPeerPublicKey() const { return this->peer_public_key; }

	static ServerNetworkGameSocketHandler *GetByClientID(ClientID client_id);
};

void NetworkServer_Tick(bool send_frame);
void ChangeNetworkRestartTime(bool reset);

#endif /* NETWORK_SERVER_H */
