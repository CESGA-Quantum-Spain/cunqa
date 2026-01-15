import os, sys
import logging
import unittest
import numpy as np

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.circuit import CunqaCircuit
from cunqa.qutils import get_QPUs, qraise, qdrop
from cunqa.result import ResultError
from cunqa.logger import logger

class TestState(unittest.TestCase):
    """Checks the correct functioning on the `save_state` instruction and the `statevector` and `density_matrix` properties of Result()"""
    @classmethod
    def setUpClass(cls):
        cls.job_to_qdrop = qraise(2, "00:10:00", co_located = True) #Important: Aer simulator
        cls.qpus = get_QPUs(on_node = False)

        cls.circ = CunqaCircuit(2)
        cls.circ.h(0)
        cls.circ.cx(0, 1)
        cls.circ.save_state() # Key instruction for the test
        cls.circ.measure_all()

        # Another circuit with several save_states
        cls.circ_sev = CunqaCircuit(2)
        cls.circ_sev.h(0)
        cls.circ_sev.save_state(label="after_h") # Key instruction for the test
        cls.circ_sev.cx(0, 1)
        cls.circ_sev.save_state() # Key instruction for the test
        cls.circ_sev.measure_all()

        # A circuit without save states
        cls.circ_no_state = CunqaCircuit(2)
        cls.circ_no_state.h(0); cls.circ_no_state.cx(0,1)
        cls.circ_no_state.measure_all()

    @classmethod
    def tearDownClass(cls):
        qdrop(cls.job_to_qdrop)    

    # TESTS --------------------------------------------------
    def test_statevector(self):
        """Checks wether the statevector can be retrieved if a circuit with save_state is simulated with statevector method."""
        qjob = self.qpus[-1].run(self.circ, method="statevector")
        statevec = qjob.result.statevector

        statevec_exact = np.array([[0.70710678+0.j], [0.        +0.j], [0.        +0.j], [0.70710678+0.j]])

        return np.testing.assert_allclose(statevec, statevec_exact)
    
    def test_density_matrix(self):
        """Checks wether the density matrix can be retrieved if a circuit with save_state is simulated with density_matrix method."""
        qjob = self.qpus[-2].run(self.circ, method="density_matrix")
        densmat = qjob.result.density_matrix

        densmat_exact = np.array([[[0.5+0.j], [0. +0.j], [0. +0.j], [0.5+0.j]],

                                [[0. +0.j], [0. +0.j], [0. +0.j], [0. +0.j]],

                                [[0. +0.j], [0. +0.j], [0. +0.j], [0. +0.j]],

                                [[0.5+0.j], [0. +0.j], [0. +0.j], [0.5+0.j]]])
        
        return np.testing.assert_allclose(densmat, densmat_exact)

    def test_several_statevector(self):
        """Checks that several statevectors can be correctly retrieved with multiple save_state() with different labels."""
        qjob = self.qpus[-1].run(self.circ_sev, method="statevector")
        statevecs = qjob.result.statevector

        statevecs_exact = {'after_h':      np.array([[0.70710678+0.j], [0.70710678+0.j], [0.        +0.j], [0.        +0.j]]),
                            'statevector': np.array([[0.70710678+0.j], [0.        +0.j], [0.        +0.j], [0.70710678+0.j]])}
        
        [self.assertIn(key, statevecs) for key in statevecs_exact.keys()]
        [self.assertIn(key, statevecs_exact) for key in statevecs.keys()] # Test that statevecs doesn't have extra keys

        return [np.testing.assert_allclose(statevecs[k], statevecs_exact[k]) for k in statevecs_exact.keys()]
    
    def test_several_density_matrix(self):
        """Checks that several density matrices can be correctly retrieved with multiple save_state() with different labels."""
        qjob = self.qpus[-2].run(self.circ_sev, method="density_matrix")
        densmats = qjob.result.density_matrix

        densmats_exact = {'after_h': np.array([[[0.5+0.j], [0.5+0.j], [0. +0.j], [0. +0.j]],

                                               [[0.5+0.j], [0.5+0.j], [0. +0.j], [0. +0.j]],

                                               [[0. +0.j], [0. +0.j], [0. +0.j], [0. +0.j]],

                                               [[0. +0.j], [0. +0.j], [0. +0.j], [0. +0.j]]]), 
       
       'density_matrix': np.array([[[0.5+0.j], [0. +0.j], [0. +0.j], [0.5+0.j]],

                                   [[0. +0.j], [0. +0.j], [0. +0.j], [0. +0.j]],

                                   [[0. +0.j], [0. +0.j], [0. +0.j], [0. +0.j]],

                                   [[0.5+0.j], [0. +0.j], [0. +0.j], [0.5+0.j]]])}
        
        [self.assertIn(key, densmats) for key in densmats_exact.keys()]
        [self.assertIn(key, densmats_exact) for key in densmats.keys()] # Test that densmats doesn't have extra keys

        return [np.testing.assert_allclose(densmats[k], densmats_exact[k]) for k in densmats_exact.keys()]
    
    # ERROR TESTS ----------------------------------
    def test_error_if_repeated_label(self):
        """Checks that an error is raised if several states are stored with the same label."""
        wrong_circ = CunqaCircuit(2)
        wrong_circ.h(0)
        wrong_circ.save_state(label="same_label")
        wrong_circ.rz(np.pi/4, 1)
        wrong_circ.save_state(label="same_label")
        wrong_circ.measure_all()

        job = self.qpus[-1].run(wrong_circ)

        with self.assertRaises(SystemExit):
            job.result
    
    def test_no_statevector_error(self):
        """Checks that an error is raised if the user tries to retrieve an statevector which is not available."""
        result = self.qpus[-1].run(self.circ_no_state, method="statevector").result

        with self.assertRaises(ResultError):
            result.statevector

    def test_no_density_matrix_error(self):
        """Checks that an error is raised if the user tries to retrieve an density_matrix which is not available."""
        result = self.qpus[-2].run(self.circ_no_state, method="density_matrix").result

        with self.assertRaises(ResultError):
            result.density_matrix




class TestProbabilities(unittest.TestCase):
    """Checks the correct functioning of the `probabilities()` function in `Result()` with all of its options."""
    @classmethod
    def setUpClass(cls):
        cls.job_to_qdrop = qraise(3, "00:10:00", co_located = True)
        cls.qpus = get_QPUs(on_node = False)

        state_circ = CunqaCircuit(3)
        state_circ.h(0); state_circ.h(1); state_circ.h(2)
        state_circ.rx(np.pi/4, 0); state_circ.cx(0, 1); state_circ.rx(np.pi/4, 2)
        state_circ.save_state()
        state_circ.measure_all()

        # Circuit without save_state() for the case where probabilities are estimated from counts
        circ = CunqaCircuit(3)
        circ.h(0); circ.h(1); circ.h(2)
        circ.rx(np.pi/4, 0); circ.cx(0, 1); circ.rx(np.pi/4, 2)
        circ.measure_all()

        cls.result_statevec = cls.qpus[-1].run(state_circ, method="statevector").result
        cls.result_densmat  = cls.qpus[-2].run(state_circ, method="density_matrix").result
        cls.result_counts   = cls.qpus[-3].run(circ, seed = 18).result
        

    @classmethod
    def tearDownClass(cls):
        qdrop(cls.job_to_qdrop)    

    # TESTS --------------------------------------------------
    def test_statevector_probs(self):
        """Test that the correct probabilities are returned using statevector"""
        probs = self.result_statevec.probabilities()
        
        return np.testing.assert_allclose(probs, np.array([0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125]))
    
    def test_density_matrix_probs(self):
        """Test that the correct probabilities are returned using density_matrix"""
        probs = self.result_densmat.probabilities()
        
        return np.testing.assert_allclose(probs, np.array([0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125]))

    def test_estimate_probs(self):
        """Test that the correct probabilities are returned estimating by counts with seed=18 and default shots=1024"""
        probs = self.result_counts.probabilities()
        
        return np.testing.assert_allclose(probs, np.array([0.11816406, 0.12988281, 0.11914062, 0.12402344, 0.12890625, 0.12207031, 0.13085938, 0.12695312]))


    # PER QUBIT probabilities
    def test_statevector_per_qubit_probs(self):
        """Test that the correct per qubit probabilities are returned using statevector"""
        probs = self.result_statevec.probabilities(per_qubit=True)
        
        return np.testing.assert_allclose(probs, np.array([[0.5, 0.5], [0.5, 0.5], [0.5, 0.5]]))
    
    def test_density_matrix_per_qubit_probs(self):
        """Test that the correct per qubit probabilities are returned using density_matrix"""
        probs = self.result_densmat.probabilities(per_qubit=True)
        
        return np.testing.assert_allclose(probs, np.array([[0.5, 0.5], [0.5, 0.5], [0.5, 0.5]]))

    def test_estimate_per_qubit_probs(self):
        """Test that the correct per qubit probabilities are returned estimating by counts with seed=18 and default shots=1024"""
        probs = self.result_counts.probabilities(per_qubit=True)
        
        return np.testing.assert_allclose(probs, np.array([[0.49121094, 0.50878906], [0.49902344, 0.50097656], [0.49707031, 0.50292969]]))


    # PARTIAL probabilities
    def test_statevector_partial_probs(self):
        """Test that the correct partial probabilities are returned using statevector"""
        probs = self.result_statevec.probabilities(partial = [1,2])
        
        return self.assertDictEqual(probs, {'00': 0.25, '01': 0.2499999999999999, '10': 0.24999999999999992, '11': 0.24999999999999994})
    
    def test_density_matrix_partial_probs(self):
        """Test that the correct partial probabilities are returned using density_matrix"""
        probs = self.result_densmat.probabilities(partial = [1,2])
        
        return self.assertDictEqual(probs, {'00': 0.2500000000000001, '01': 0.2499999999999999, '10': 0.25, '11': 0.25})

    def test_estimate_partial_probs(self):
        """Test that the correct partial probabilities are returned estimating by counts with seed=18 and default shots=1024"""
        probs = self.result_counts.probabilities(partial = [1,2])
        
        return self.assertDictEqual(probs, {'00': 0.2470703125, '01': 0.251953125, '10': 0.25, '11': 0.2509765625})


    # PER QUBIT AND PARTIAL probabilities
    def test_statevector_per_qubit_partial_probs(self):
        """Test that the correct partial per qubit probabilities are returned using statevector"""
        probs = self.result_statevec.probabilities(per_qubit=True, partial = [1,2])
        
        return np.testing.assert_allclose(probs, np.array([[0.5, 0.5], [0.5, 0.5]]))
    
    def test_density_matrix_per_qubit_partial_probs(self):
        """Test that the correct partial per qubit probabilities are returned using density_matrix"""
        probs = self.result_densmat.probabilities(per_qubit=True, partial = [1,2])
        
        return np.testing.assert_allclose(probs, np.array([[0.5, 0.5], [0.5, 0.5]]))

    def test_estimate_per_qubit_partial_probs(self):
        """Test that the correct partial per qubit probabilities are returned estimating by counts with seed=18 and default shots=1024"""
        probs = self.result_counts.probabilities(per_qubit=True, partial = [1,2])
        
        return np.testing.assert_allclose(probs, np.array([[0.49902344, 0.50097656], [0.49707031, 0.50292969]]))


if __name__ == "__main__":
    unittest.main(verbosity=2)