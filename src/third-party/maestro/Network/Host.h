/*******************************************************

Copyright (C) 2023 2639731 ONTARIO INC. <joe.diadamo.dryrock@gmail.com>

The files in this repository make up the Codebase.

All files in this Codebase are owned by 2639731 ONTARIO INC..

Any files within this Codebase can not be copied and/or distributed without the express permission of 2639731 ONTARIO INC.

*******************************************************/

/**
 * @file Host.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The host interface.
 *
 * Must be derived from by the particular host implementations.
 */

#pragma once

#ifndef _HOST_H_
#define _HOST_H_

#include <vector>

namespace Network {

	/**
	 * @class IHost
	 * @brief The host interface.
	 *
	 * The interface for the hosts in the network.
	 * Must be derived from by the particular host implementations.
	 * Most of the hosts will be quantum computers but classical computers are also possible.
	 * One that is possible is a controller, a classical computer responsible with the network management,
	 * circuit distribution and so on.
	 * 
	 * @tparam Time The time type for the execution time.
	 * @sa IController
	 * @sa INetwork
	 */
	template<typename Time = Types::time_type> class IHost : public std::enable_shared_from_this<IHost<Time>> {
	public:
		/**
		 * @brief The destructor.
		 *
		 * The destructor. Virtual since this is an interface.
		 */
		virtual ~IHost() = default;

		/**
		 * @brief Get the host id.
		 *
		 * Obtain a host id used to identify the host in the network.
		 * @return The host id.
		 */
		virtual size_t GetId() const = 0;

		/**
		 * @brief Get the number of qubits.
		 * 
		 * Obtain the number of qubits in the host, excluding the one(s) used only for entanglement with other hosts.
		 * @return The number of qubits.
		 */
		virtual size_t GetNumQubits() const = 0;

		/**
		 * @brief Get the number of network entangled qubits.
		 * 
		 * Obtain the number of qubits in the host used for entanglement with other hosts.
		 * 
		 * @return The number of network entangled qubits.
		 */
		virtual size_t GetNumNetworkEntangledQubits() const = 0;

		/**
		 * @brief Get the number of classical bits.
		 * 
		 * Obtain the number of classical bits in the host.
		 * This does not include the bits used for measurement of the network entangled qubits.
		 * @return The number of classical bits.
		 */
		virtual size_t GetNumClassicalBits() const = 0;

		/**
		 * @brief Check if a qubit is in the host.
		 * 
		 * Checks if a qubit with the specified id is in the host, excluding the one(s) used only for entanglement with other hosts.
		 * @return True if the qubit is on this host, false otherwise.
		 */
		virtual bool IsQubitOnHost(size_t qubitId) const = 0;

		/**
		 * @brief Check if two qubits are in the same host.
		 *
		 * Checks if two qubits with the specified ids are in the same host, excluding the one(s) used only for entanglement with other hosts.
		 * @return True if the qubits are on the same host, false otherwise.
		 */
		virtual bool AreQubitsOnSameHost(size_t qubitId1, size_t qubitId2) const = 0;

		/**
		 * @brief Check if a classical bit is in the host.
		 *
		 * Checks if a classical bit with the specified id is in the host.
		 * @return True if the classical bit is on this host, false otherwise.
		 */
		virtual bool IsClassicalBitOnHost(size_t cbitId) const = 0;

		/**
		 * @brief Check if two classical bits are in the same host.
		 *
		 * Checks if two classical bits with the specified ids are in the same host.
		 * @return True if the classical bits are on the same host, false otherwise.
		 */
		virtual bool AreCbitsOnSameHost(size_t cbitId1, size_t cbitId2) const = 0;

		/**
		 * @brief Check if a qubit used for entanglement between hosts is in the host.
		 *
		 * Checks if a qubit used for entanglement between hosts with the specified id is in the host.
		 * @return True if the network entangled qubit is on this host, false otherwise.
		 */
		virtual bool IsEntangledQubitOnHost(size_t qubitId) const = 0;

		/**
		 * @brief Get the ids of the qubits in the host.
		 *
		 * Obtain the ids of the qubits in the host, excluding the one(s) used only for entanglement with other hosts.
		 * @return The ids of the qubits in the host.
		 */
		virtual std::vector<size_t> GetQubitsIds() const = 0;

		/**
		 * @brief Get the ids of the classical bits in the host.
		 *
		 * Obtain the ids of the classical bits in the host.
		 * @return The ids of the classical bits in the host.
		 */
		virtual std::vector<size_t> GetClassicalBitsIds() const = 0;

		/**
		 * @brief Get the ids of the qubits used for entanglement between hosts in the host.
		 *
		 * Obtain the ids of the qubits used for entanglement between hosts in the host.
		 * @return The ids of the qubits used for entanglement between hosts in the host.
		 */
		virtual std::vector<size_t> GetNetworkEntangledQubitsIds() const = 0;

		/**
		 * @brief Get the ids of the classical bits used for measurement of the qubits used for entanglement between hosts present in the host.
		 *
		 * Obtain the ids of the classical bits used for measurement of the qubits used for entanglement between hosts present in the host.
		 * @return The ids of the classical bits used for measurement of the qubits used for entanglement between hosts present in the host.
		 */
		virtual std::vector<size_t> GetEntangledQubitMeasurementBitIds() const = 0;

		/**
		 * @brief Send a packet to a host.
		 *
		 * Send a packet to a host.
		 * Usually called by the host implementation or perhaps in some cases by the network implementation.
		 * @param hostId The id of the host to send the packet to.
		 * @param packet The packet to send.
		 * @return True if the packet was sent successfully, false otherwise.
		 */
		virtual bool SendPacketToHost(size_t hostId, const std::vector<uint8_t>& packet) = 0;

		/**
		 * @brief Receive a packet from a host.
		 *
		 * Receive a packet from a host.
		 * Usually called by the network implementation.
		 * @param hostId The id of the host to receive the packet from.
		 * @param packet The packet to receive.
		 * @return True if the packet was received successfully, false otherwise.
		 */
		virtual bool RecvPacketFromHost(size_t hostId, const std::vector<uint8_t>& packet) = 0;

		/**
         * @brief Get the id of the first qubit assigned to the host.
         *
         * Obtain the id of the first qubit assigned to the host, the other ones are assigned contiquously.
         *
         * @return The id of the first qubit assigned to the host.
         */
		virtual size_t GetStartQubitId() const = 0;

		/**
         * @brief Get the id of the first classical bit assigned to the host.
         *
         * Obtain the id of the first classical bit assigned to the host, the other ones are assigned contiquously.
         *
         * @return The id of the first classical bit assigned to the host.
         */
		virtual size_t GetStartClassicalBitId() const = 0;

		/**
		 * @brief Get a shared pointer to this object.
		 *
		 * Returns a shared pointer to this object.
		 * The object needs to be already wrapped in a shared pointer.
		 * @return A shared pointer to this object.
		 */
		std::shared_ptr<IHost<Time>> getptr()
		{
			return std::enable_shared_from_this<IHost<Time>>::shared_from_this();
		}
	};

}

#endif // !_HOST_H_

