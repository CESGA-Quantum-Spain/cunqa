/**
 * @file SimpleHost.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * A simple host class, implementing the host interface.
 *
 * The hosts have each a number of qubits, a range of the total ones are assigned to each host (in order).
 * For example, if the first host has 3 qubits, it gets the id 0 and the assigned qubits are 0, 1, 2. If the second host has 2 qubits, it gets the id 1 and the assigned qubits are 3, 4.
 * Similarly, for each qubit there is a corresponding classical bit.
 * Each host also has a qubit which is used for entanglement with other hosts.
 * Those qubits are assigned in order one for each host, after the qubits for all hosts were assigned.
 * For those also there is a corresponding classical bit.
 */

#pragma once

#ifndef _SIMPLE_HOST_H_
#define _SIMPLE_HOST_H_

#include <numeric>

#include "Network.h"
#include "Host.h"

namespace Network {

	/**
	 * @class SimpleHost
	 * @brief The simple host implementation.
	 *
	 * A simple host, having just a single qubit for entanglement between hosts, the qubits and classical bits are contiguous.
	 *
	 * @tparam Time The time type for the execution time.
	 * @sa SimpleController
	 * @sa SimpleNetwork
	 */
	template<typename Time = Types::time_type> class SimpleHost : public IHost<Time> {
	public:
		/**
		 * @brief The constructor.
		 * 
		 * The host will have one qubit more than the specified number, for entanglement with other hosts.
		 * It will also have one classical bit more than the specified number, for the measurement of the entangled qubit.
		 *
		 * @param hostId The id of the host.
		 * @param startQubitId The id of the first qubit assigned to the host.
		 * @param numQubits The number of qubits assigned to the host.
		 * @param startClassicalBitId The id of the first classical bit assigned to the host.
		 * @param numClassicalBits The number of classical bits assigned to the host.
		 */
		SimpleHost(size_t hostId, size_t startQubitId, size_t numQubits, size_t startClassicalBitId, size_t numClassicalBits)
			: id(hostId), startQubitId(startQubitId), numQubits(numQubits), startClassicalBitId(startClassicalBitId), numClassicalBits(numClassicalBits)
		{
		}

		/**
		 * @brief Get the host id.
		 *
		 * Obtain a host id used to identify the host in the network.
		 * @return The host id.
		 */
		size_t GetId() const override
		{
			return id;
		}

		/**
		 * @brief Get the number of qubits.
		 *
		 * Obtain the number of qubits in the host, excluding the one(s) used only for entanglement with other hosts.
		 * @return The number of qubits.
		 */
		size_t GetNumQubits() const override
		{
			return numQubits;
		}

		/**
		 * @brief Get the number of network entangled qubits.
		 *
		 * Obtain the number of qubits in the host used for entanglement with other hosts.
		 * The simple hosts have only one such qubit.
		 *
		 * @return The number of network entangled qubits, in this case, one.
		 */
		size_t GetNumNetworkEntangledQubits() const override
		{
			if (std::numeric_limits<size_t>::max() == entangledQubitId)
				return 0;

			return 1;
		}

		/**
		 * @brief Get the number of classical bits.
		 *
		 * Obtain the number of classical bits in the host.
		 * This does not include the bits used for measurement of the network entangled qubits.
		 * @return The number of classical bits.
		 */
		size_t GetNumClassicalBits() const override
		{
			return numClassicalBits;
		}

		/**
		 * @brief Check if a qubit is in the host.
		 *
		 * Checks if a qubit with the specified id is in the host, excluding the one(s) used only for entanglement with other hosts.
		 * @return True if the qubit is on this host, false otherwise.
		 */
		bool IsQubitOnHost(size_t qubitId) const override
		{
			return qubitId >= startQubitId && qubitId < startQubitId + numQubits;
		}

		/**
		 * @brief Check if two qubits are in the same host.
		 *
		 * Checks if two qubits with the specified ids are in the same host, excluding the one(s) used only for entanglement with other hosts.
		 * @return True if the qubits are on the same host, false otherwise.
		 */
		bool AreQubitsOnSameHost(size_t qubitId1, size_t qubitId2) const override
		{
			return IsQubitOnHost(qubitId1) && IsQubitOnHost(qubitId2);
		}

		/**
		 * @brief Check if a classical bit is in the host.
		 *
		 * Checks if a classical bit with the specified id is in the host.
		 * @return True if the classical bit is on this host, false otherwise.
		 */
		bool IsClassicalBitOnHost(size_t cbitId) const override
		{
			return (cbitId >= startClassicalBitId && cbitId < startClassicalBitId + numClassicalBits) || cbitId == entangledQubitMeasurementBit;
		}

		/**
		 * @brief Check if a qubit used for entanglement between hosts is in the host.
		 *
		 * Checks if a qubit used for entanglement between hosts with the specified id is in the host.
		 * @return True if the network entangled qubit is on this host, false otherwise.
		 */
		bool IsEntangledQubitOnHost(size_t qubitId) const override
		{
			if (GetNumNetworkEntangledQubits() == 0) return false;

			return qubitId == entangledQubitId;
		}

		/**
		 * @brief Check if two classical bits are in the same host.
		 *
		 * Checks if two classical bits with the specified ids are in the same host.
		 * @return True if the classical bits are on the same host, false otherwise.
		 */
		bool AreCbitsOnSameHost(size_t cbitId1, size_t cbitId2) const override
		{
			return IsClassicalBitOnHost(cbitId1) && IsClassicalBitOnHost(cbitId2);
		}

		/**
		 * @brief Get the ids of the qubits in the host.
		 *
		 * Obtain the ids of the qubits in the host, excluding the one(s) used only for entanglement with other hosts.
		 * @return The ids of the qubits in the host.
		 */
		std::vector<size_t> GetQubitsIds() const override
		{
			std::vector<size_t> res(numQubits);

			std::iota(res.begin(), res.end(), startQubitId);

			return res;
		}

		/**
		 * @brief Get the ids of the qubits used for entanglement between hosts in the host.
		 *
		 * Obtain the ids of the qubits used for entanglement between hosts in the host.
		 * @return The ids of the qubits used for entanglement between hosts in the host.
		 */
		std::vector<size_t> GetNetworkEntangledQubitsIds() const override
		{
			if (GetNumNetworkEntangledQubits() == 0) return {};

			return { entangledQubitId };
		}

		/**
		 * @brief Get the ids of the classical bits in the host.
		 *
		 * Obtain the ids of the classical bits in the host.
		 * @return The ids of the classical bits in the host.
		 */
		std::vector<size_t> GetClassicalBitsIds() const override
		{
			std::vector<size_t> res(numClassicalBits);

			std::iota(res.begin(), res.end(), startClassicalBitId);

			return res;
		}

		/**
		 * @brief Get the ids of the classical bits used for measurement of the qubits used for entanglement between hosts present in the host.
		 *
		 * Obtain the ids of the classical bits used for measurement of the qubits used for entanglement between hosts present in the host.
		 * @return The ids of the classical bits used for measurement of the qubits used for entanglement between hosts present in the host.
		 */
		std::vector<size_t> GetEntangledQubitMeasurementBitIds() const override
		{
			if (std::numeric_limits<size_t>::max() == entangledQubitMeasurementBit)
				return {};

			return { entangledQubitMeasurementBit };
		}

		/**
		 * @brief Send a packet to a host.
		 *
		 * Send a packet to a host.
		 * Not used in this simple host implementation, classical packets sending is not simulated, the classical bits are implicitely shared using the classical state for the quantum computing simulation for the network.
		 * 
		 * @param hostId The id of the host to send the packet to.
		 * @param packet The packet to send.
		 * @return False.
		 */
		bool SendPacketToHost(size_t hostId, const std::vector<uint8_t>& packet) override
		{
			return false;
		}

		/**
		 * @brief Receive a packet from a host.
		 *
		 * Not used in this simple host implementation, classical packets receiving is not simulated, the classical bits are implicitely shared using the classical state for the quantum computing simulation for the network.
		 *
		 * @param hostId The id of the host to receive the packet from.
		 * @param packet The packet to receive.
		 * @return False.
		 */
		bool RecvPacketFromHost(size_t hostId, const std::vector<uint8_t>& packet) override
		{
			return false;
		}

		/**
		 * @brief Get the id of the first qubit assigned to the host.
		 * 
		 * Obtain the id of the first qubit assigned to the host, the other ones are assigned contiquously.
		 * 
		 * @return The id of the first qubit assigned to the host.
		 */
		size_t GetStartQubitId() const override
		{
			return startQubitId;
		}

		/**
		 * @brief Get the id of the first classical bit assigned to the host.
		 *
		 * Obtain the id of the first classical bit assigned to the host, the other ones are assigned contiquously.
		 *
		 * @return The id of the first classical bit assigned to the host.
		 */
		size_t GetStartClassicalBitId() const override
		{
			return startClassicalBitId;
		}

		/**
		 * @brief Set the id of the qubit used for entanglement between hosts.
		 * 
		 * Sets the id of the qubit used for entanglement between hosts.
		 * Typically the convention is to allocate them in the simulators after all the qubits for all the hosts were allocated,
		 * so they are assigned in order, one for each host, with id number after the last qubit assigned to all hosts.
		 */
		void SetEntangledQubitId(size_t id)
		{
			entangledQubitId = id;
		}

		/**
		 * @brief Set the id of the classical bit used for measurement of the qubit used for entanglement between hosts.
		 *
		 * Sets the id of the classical bit used for measurement of the qubit used for entanglement between hosts.
		 * Typically the convention is to allocate them in the classical state of the quantum simulators after all the classical bits for all the hosts were allocated,
		 * so they are assigned in order, one for each host, with id number after the last classical bit assigned to all hosts.
		 */
		void SetEntangledQubitMeasurementBit(size_t id)
		{
			entangledQubitMeasurementBit = id;
		}

	private:
		size_t id;                               /**< The host id */
		
		size_t startQubitId = 0;                 /**< The id of the first qubit assigned to the host */
		size_t numQubits = 0;                    /**< The number of qubits assigned to the host */

		size_t startClassicalBitId = 0;          /**< The id of the first classical bit assigned to the host */
		size_t numClassicalBits = 0;             /**< The number of classical bits assigned to the host */

		size_t entangledQubitId = std::numeric_limits<size_t>::max();             /**< The id of the qubit used for entanglement between hosts */
		size_t entangledQubitMeasurementBit = std::numeric_limits<size_t>::max(); /**< The id of the classical bit used for measurement of the qubit used for entanglement between hosts */
	};

}

#endif // ! _SIMPLE_HOST_H_
