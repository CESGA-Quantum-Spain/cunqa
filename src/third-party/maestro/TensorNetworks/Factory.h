/**
 * @file Factory.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The factory for gates tensors.
 * 
 * Creates the tensors for the gates.
 */

#pragma once

#ifndef  __TENSORS_FACTORY_H_
#define __TENSORS_FACTORY_H_ 1	

#include "../Utils/Tensor.h"
#include "QubitRegister.h"

namespace TensorNetworks {

	/**
	 * @brief The factory for gates tensors.
	 *
	 * Creates the tensors for the gates.
	 */
	class Factory {
	public:
		using MatrixClass = Eigen::MatrixXcd;

		static std::shared_ptr<Utils::Tensor<>> CreateTensorFromGate(const QC::Gates::QuantumGateWithOp<MatrixClass>& gate)
		{
			if (gate.getQubitsNumber() > 2)
				throw std::invalid_argument("The gate has more than 2 qubits. Decompose gates on a higher number of qubits in gates on 1 and 2 qubits only.");
			
			if (gate.getQubitsNumber() == 1)
				return CreateTensorFromGateOnOneQubit(gate);
			
			return CreateTensorFromGateOnTwoQubits(gate);
		}

		static std::shared_ptr<Utils::Tensor<>> CreateProjectionTensor(bool onZero = true)
		{
			static const std::vector<size_t> dims{ 2, 2 };
			auto tensor = std::make_shared<Utils::Tensor<>>(dims);

			std::vector<size_t> indices{ 0, 0 };
			(*tensor)(indices) = onZero ? 1. : 0.;
			indices[1] = 1;
			(*tensor)(indices) = 0;
			indices[0] = 1;
			(*tensor)(indices) = onZero ? 0. : 1.;
			indices[1] = 0;
			(*tensor)(indices) = 0;

			return tensor;
		}

		static std::shared_ptr<Utils::Tensor<>> CreateQubit0Tensor()
		{
			static const std::vector<size_t> dims{ 2 };
			auto tensor = std::make_shared<Utils::Tensor<>>(dims);

			std::vector<size_t> indices{ 0 };
			(*tensor)(indices) = 1.0;
			indices[0] = 1;
			(*tensor)(indices) = 0.0;

			return tensor;
		}

		static std::shared_ptr<Utils::Tensor<>> CreateQubit1Tensor()
		{
			static const std::vector<size_t> dims{ 2 };
			auto tensor = std::make_shared<Utils::Tensor<>>(dims);

			std::vector<size_t> indices{ 0 };
			(*tensor)(indices) = 0.0;
			indices[0] = 1;
			(*tensor)(indices) = 1.0;

			return tensor;
		}

		static std::shared_ptr<Utils::Tensor<>> CreateTensorFromGateOnOneQubit(const QC::Gates::QuantumGateWithOp<MatrixClass>& gate)
		{
			static const std::vector<size_t> dims{ 2, 2 };
			auto tensor = std::make_shared<Utils::Tensor<>>(dims);

			const MatrixClass& mat = gate.getRawOperatorMatrix();


			std::vector<size_t> indices{ 0, 0 };
			(*tensor)(indices) = mat(0, 0);
			indices[1] = 1;
			(*tensor)(indices) = mat(1, 0);
			indices[0] = 1;
			(*tensor)(indices) = mat(1, 1);
			indices[1] = 0;
			(*tensor)(indices) = mat(0, 1);

			return tensor;
		}

		static std::shared_ptr<Utils::Tensor<>> CreateTensorFromGateOnTwoQubits(const QC::Gates::QuantumGateWithOp<MatrixClass>& gate)
		{
			static const std::vector<size_t> dims{ 2, 2, 2, 2 };
			auto tensor = std::make_shared<Utils::Tensor<>>(dims);

			const MatrixClass& mat = gate.getRawOperatorMatrix();

			std::vector<size_t> indices;
			indices.resize(4);

			//q1l, q0l, q1c, q0c
			for (int q0l = 0; q0l < 2; ++q0l) // ctrl qubit
			{
				const int l0 = q0l << 1;
				indices[2] = q0l;
				for (int q0c = 0; q0c < 2; ++q0c) // ctrl qubit
				{
					indices[0] = q0c;
					const int c0 = q0c << 1;
					for (int q1l = 0; q1l < 2; ++q1l)
					{
						indices[3] = q1l;
						for (int q1c = 0; q1c < 2; ++q1c)
						{
							indices[1] = q1c;
							(*tensor)(indices) = mat(l0 | q1l, c0 | q1c);
						}
					}
				}
			}

			return tensor;
		}
	};

}

#endif // ! __TENSORS_FACTORY_H_
