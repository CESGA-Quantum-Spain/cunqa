/**
 * @file TensorNode.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * Tensor Node class.
 *
 * A class a tensor node that's part of a tensor network.
 * It contains a tensor and its connections to other tensor nodes, also the qubits on which the tensor acts.
 */

#pragma once

#ifndef __TENSOR_NODE_H_
#define __TENSOR_NODE_H_ 1

#include <Eigen/Eigen>
#include <memory>
#include "../Utils/Tensor.h"
#include "../Types.h"
#include "Factory.h"

namespace TensorNetworks {

	class TensorNode {
	public:
		using Index = Eigen::Index;
		using MatrixClass = Eigen::MatrixXcd;

		void SetGate(const QC::Gates::QuantumGateWithOp<MatrixClass>& gate, Types::qubit_t q1, Types::qubit_t q2 = 0)
		{
			const size_t qubitsNumber = gate.getQubitsNumber();

			Clear();

			if (qubitsNumber > 2)
				throw std::invalid_argument("The gate has more than 2 qubits. Decompose gates on a higher number of qubits in gates on 1 and 2 qubits only.");

			qubits.push_back(q1);

			if (qubitsNumber > 1)
				qubits.push_back(q2);

			qubits.push_back(q1);
			if (qubitsNumber > 1)
				qubits.push_back(q2);

			tensor = Factory::CreateTensorFromGate(gate);

			connections.resize(qubitsNumber * 2, NotConnected);
			connectionsIndices.resize(qubitsNumber * 2, NotConnected);
		}

		void SetQubit(Types::qubit_t q, bool zero = true)
		{
			Clear();

			qubits.push_back(q);

			if (zero)
				tensor = Factory::CreateQubit0Tensor();
			else
				tensor = Factory::CreateQubit1Tensor();

			connections.push_back(NotConnected);
			connectionsIndices.push_back(NotConnected);
		}

		void SetProjector(Types::qubit_t q, bool zero = true)
		{
			Clear();

			qubits.push_back(q);
			qubits.push_back(q);

			tensor = Factory::CreateProjectionTensor(zero);

			connections.resize(2, NotConnected);
			connectionsIndices.resize(2, NotConnected);
		}

		void SetSuper()
		{
			if (tensor) tensor->Conj();
		}

		void SetId(Index Id)
		{
			id = Id;
		}

		Index GetId() const
		{
			return id;
		}

		size_t GetQubitsNumber() const
		{
			return qubits.size();
		}

		size_t GetRank() const
		{
			if (!tensor) return 0;

			return tensor->GetRank();
		}

		void Clear()
		{
			id = 0;

			tensor.reset();
			qubits.clear();
			connections.clear();
			connectionsIndices.clear();
		}

		// this is needed for tensor contraction, since connections are modified in the process
		std::shared_ptr<TensorNode> CloneWithoutTensorCopy() const
		{
			auto cloned = std::make_shared<TensorNode>();
			cloned->id = id;
			cloned->qubits = qubits;
			cloned->connections = connections;
			cloned->connectionsIndices = connectionsIndices;
			cloned->tensor = tensor;
			
			//cloned->contractsTheNeededQubit = contractsTheNeededQubit;

			return cloned;
		}

		// Useful for contracting the tensor network without actually contracting the tensors
		// can be used in algorithms that find an (sub)optimal contraction order
		std::shared_ptr<TensorNode> CloneWithADummyTensor() const
		{
			auto cloned = std::make_shared<TensorNode>();
			cloned->id = id;
			cloned->qubits = qubits;
			cloned->connections = connections;
			cloned->connectionsIndices = connectionsIndices;
			cloned->tensor = std::make_shared<Utils::Tensor<>>(tensor->GetDims(), true);

			//cloned->contractsTheNeededQubit = contractsTheNeededQubit;

			return cloned;
		}

		std::shared_ptr<TensorNode> Clone() const
		{
			auto cloned = std::make_shared<TensorNode>();
			cloned->id = id;
			cloned->qubits = qubits;
			cloned->connections = connections;
			cloned->connectionsIndices = connectionsIndices;
			cloned->tensor = std::make_shared<Utils::Tensor<>>(*tensor);

			//cloned->contractsTheNeededQubit = contractsTheNeededQubit;

			return cloned;
		}

		constexpr static Index NotConnected = -1;

		Index id = 0;
		
		std::vector<Types::qubit_t> qubits; // each dimension/index of the tensor corresponds to a qubit
		std::vector<Index> connections; // the ids of the connected tensors in the network (equal with the index in the tensors vector)
		std::vector<Index> connectionsIndices; // the indices of the connected indices in the connected tensor on that position
		std::shared_ptr<Utils::Tensor<>> tensor; // the actual tensor

		// this is only needed for contraction algorithm
		// it will be set to true only on contraction result nodes, not on the original tensor nodes
		bool contractsTheNeededQubit = false;
	};

}

#endif // __TENSOR_NODE_H_
