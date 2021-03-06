//------------------------------------------------------------------------------
// Copyright (C) 2019 Intel Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//------------------------------------------------------------------------------

#ifndef EXTRA_FEATURES_TEST_HPP
#define EXTRA_FEATURES_TEST_HPP

#include "../../qureg/qureg.hpp"
#include "../../util/extra_features/qaoa_features.hpp"

//////////////////////////////////////////////////////////////////////////////
// Test fixture class.

class ExtraFeaturesTest : public ::testing::Test
{
 protected:

  ExtraFeaturesTest()
  { }

  // just after the 'constructor'
  void SetUp() override
  {
    // All tests are skipped if the rank is dummy.
    if (qhipster::mpi::Environment::IsUsefulRank() == false)
        GTEST_SKIP();

    // All tests are skipped if the 6-qubit state is distributed in more than 2^5 ranks.
    // In fact the MPI version needs to allocate half-the-local-storage for communication.
    // If the local storage is a single amplitude, this cannot be further divided.
    if (qhipster::mpi::Environment::GetStateSize() > 32)
        GTEST_SKIP();
  }

  const std::size_t num_qubits_ = 6;
  double accepted_error_ = 1e-15;
};

//////////////////////////////////////////////////////////////////////////////
// Functions developed to facilitate the simulation of QAOA circuits.
//////////////////////////////////////////////////////////////////////////////

TEST_F(ExtraFeaturesTest, qaoa_maxcut)
{
  // Instance of the max-cut problem provided as adjacency matrix.
  // It is a ring of 6 vertices:
  //
  //   0--1--2
  //   |     |
  //   5--4--3
  //
  std::vector<int> adjacency = {0, 1, 0, 0, 0, 1,
                                1, 0, 1, 0, 0, 0,
                                0, 1, 0, 1, 0, 0,
                                0, 0, 1, 0, 1, 0,
                                0, 0, 0, 1, 0, 1,
                                1, 0, 0, 0, 1, 0};
  QubitRegister<ComplexDP> diag (num_qubits_,"base",0);
  int max_cut_value;
  max_cut_value = qaoa::InitializeVectorAsMaxCutCostFunction(diag,adjacency);

  // Among other properties, only two bipartition has cut=0.
  ComplexDP amplitude;
  amplitude = { 0, 0 };
  ASSERT_COMPLEX_NEAR(diag.GetGlobalAmplitude(0), amplitude, accepted_error_);
  ASSERT_COMPLEX_NEAR(diag.GetGlobalAmplitude(diag.GlobalSize()-1), amplitude,
                      accepted_error_);
  // No bipartition can cut a single edge.
  for (size_t j=0; j<diag.LocalSize(); ++j)
      ASSERT_GT( std::abs(diag[j].real()-1.), accepted_error_);

  // Perform QAOA simulation (p=1).
  QubitRegister<ComplexDP> psi  (num_qubits_,"++++",0);
  double gamma = 0.4;
  double beta  = 0.3;
  // Emulation of the layer based on the cost function: 
  qaoa::ImplementQaoaLayerBasedOnCostFunction(psi, diag, gamma);
  // Simulation of the layer based on the local transverse field: 
  for (int qubit=0; qubit<num_qubits_; ++qubit)
      psi.ApplyRotationX(qubit, beta);
  // Get average of cut value:
  double expectation = qaoa::GetExpectationValueFromCostFunction( psi, diag);
  
  // Get histogram of the cut values:
  std::vector<double> histo = qaoa::GetHistogramFromCostFunction(psi, diag, max_cut_value);
  ASSERT_EQ(histo.size(), max_cut_value+1);
  double average=0;
  for (int j=0; j<histo.size(); ++j)
      average += double(j)*histo[j];
  ASSERT_DOUBLE_EQ(expectation, average);
}

//////////////////////////////////////////////////////////////////////////////

#endif	// header guard EXTRA_FEATURES_TEST_HPP
