// Copyright 2021 Xanadu Quantum Technologies Inc.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/**
 * @file
 * \rst
 * Handles computing statistics using a statevector like basis state probabilities.
 * \endrst
 */
#pragma once

#include "lightning_qubit.hpp"
#include <unordered_set>

inline VectorXcd all_probs ( Ref<VectorXcd> state) {
    return (state * state.conjugate().transpose()).diagonal();
}

vector<int> map_wires(const vector<int>& wires, const int num_qubits);

std::unordered_set<int> get_inactive_wires(const vector<int>& wires, const int Dim);

VectorXcd marginal_probs(Ref<VectorXcd> state, const int qubits, const vector<int> &wires, 
        const int num_wires);

template <int Dim, int M, typename... Shape>
VectorXcd compute_marginal(Ref<VectorXcd> state, const vector<int>& wires, Shape... shape){

        vector<int> mapped_wires = map_wires(wires, Dim);

        // Determine which subsystems are to be summed over
        std::unordered_set<int> inactive_wires = get_inactive_wires(mapped_wires, Dim);

        const int LenInactiveWires = Dim-M;
        Array_Xq<LenInactiveWires> dims;
        std::copy_n(std::make_move_iterator(inactive_wires.begin()), LenInactiveWires, dims.begin());

        // Faster not to store the intermediate tensor but compute with ref
        State_Xq<M> marginal_probs_tensor = TensorMap<State_Xq<Dim>>(state.data(), shape...).sum(dims);

        VectorXcd result = Map<VectorXcd> (marginal_probs_tensor.data(), marginal_probs_tensor.size(), 1);
        return result;
}


// Stopping template
//
template<int Dim, int M, int ValueIdx>
class DynamicWiresGenerator
{
public:
    template<typename... Shape>
    static inline VectorXcd marginal_probs(
        Ref<VectorXcd> state,
        const vector<int>& wires
        )
    {
        return DynamicWiresGenerator<Dim, M, ValueIdx -1>::marginal_probs(state, wires);
    }
};

// Valid stopping template: Dim>=M
template<int Dim, int M>
class DynamicWiresGenerator<Dim, M, M>
{

public:
    template<typename... Shape>
    static inline VectorXcd marginal_probs(
        Ref<VectorXcd> state,
        const vector<int>& wires
        )
    {
        // The correct size of wires have been generated
        return QubitOperations<Dim>::template marginal_probs<M>(state, wires);
    }
};

// Invalid stopping template: Dim<M
template<int Dim, int M>
class DynamicWiresGenerator<Dim, M, -1>
{

public:
    template<typename... Shape>
    static inline VectorXcd marginal_probs(
        Ref<VectorXcd> state,
        const vector<int>& wires
        )
    {
        throw std::invalid_argument("Must specify fewer wires than the number of overall qubits.");
    }
};

// Init template
template<int Dim, int M>
class DynamicWiresInit
{

public:
    template<typename... Shape>
    static inline VectorXcd marginal_probs(
        Ref<VectorXcd> state,
        const vector<int>& wires
        )
    {
        return DynamicWiresGenerator<Dim, M, Dim>::marginal_probs(state, wires);
    }
};
