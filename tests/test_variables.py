import os, sys
import logging
import unittest
import numpy as np
import sympy as sp

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.circuit import CunqaCircuit
from cunqa.circuit.parameter import Variable, variables
from cunqa.circuit.converters import convert
from cunqa.qutils import get_QPUs, qraise, qdrop
from cunqa.backend import Backend
from cunqa.qiskit_deps.transpiler import transpiler
from cunqa.logger import logger

from qiskit.circuit import QuantumCircuit, Parameter

class TestVariables(unittest.TestCase):
    
    @classmethod
    def setUpClass(cls):
        cls.job_to_qdrop = qraise(3, "00:10:00", co_located = True)
        cls.qpus = get_QPUs(on_node = False)

        cls.fakeqmio_backend = Backend({
            "name": "FakeQmio",
            "version": "/opt/cesga/qmio/hpc/calibrations/2025_05_15__12_41_26.json",
            "n_qubits": 32,
            "description": "FakeQmio backend",
            "simulator": None,
            "coupling_map": [[0,1],[2,1],[2,3],[4,3],[5,4],[6,3],[6,12],[7,0],[7,9],[9,10],[11,10],[11,12],[13,21],[14,11],[14,18],[15,8],[15,16],[18,17],[18,19],[20,19],[22,21],[22,31],[23,20],[23,30],[24,17],[24,27],[25,16],[25,26],[26,27],[28,27],[28,29],[30,29],[30,31]],
            "basis_gates": ["sx","x","rz","ecr"],
            "custom_instructions": "",
            "gates": [],
            "noise_model": {"errors": []},
            "noise_properties_path": "last_calibrations",
            "noise_path": "/opt/cesga/qmio/hpc/calibrations/2025_05_15__12_41_26.json"
        })

    @classmethod
    def tearDownClass(cls):
        qdrop(cls.job_to_qdrop)

    def test_non_assigned_variables_error(self):  
        """Checks that we get an error if we try to run a variable circuit without assigning parameters.""" 

        # Create circuit with Variable parameters
        var_circuit = CunqaCircuit(3)
        var_circuit.rx(Variable("x"), 0)

        return self.assertRaises(SystemExit, self.qpus[-1].run, var_circuit)
    

    def test_variable_expressions(self):
        """Checks wether expressions constructed from Variable parameters, which are `sympy` objects, are allowed in CunqaCircuits and evaluate correctly."""
        x = variables('x:4')

        expression = (sp.sin(x[0] + x[2]) + sp.exp(-1j*sp.pi*x[1])) / x[3]
        expr_circ = CunqaCircuit(1)
        expr_circ.rz(expression, 0)
        expr_circ.rx((sp.sin(x[0] + x[2]) + sp.exp(1j*sp.pi*x[1])) / x[3], 0)

        expr_circ.assign_parameters({x[0]: 0., x[1]: 1, x[2]: np.pi/2, x[3]: 1})
        return [self.assertAlmostEqual(instr["params"][0], 0.) for instr in expr_circ.instructions]

    def test_assign_works(self):
        """Test that if we assign parameters to a circuit parameters are correctly placed on instructions."""

        circ_assign = CunqaCircuit(3)

        circ_assign.rx(Variable("x"), 0)
        circ_assign.ry(Variable("y"), 1)
        circ_assign.rz(Variable("z"), 2)

        assign_values = {Variable("x"): 4, Variable("y"): 5.7, Variable("z"): 109.89}
        circ_assign.assign_parameters(assign_values)

        is_correct = []
        for i, instr in enumerate(circ_assign.instructions):
            variable = circ_assign.param_expressions[i]
            corresp_value = assign_values[variable]

            is_correct.append(corresp_value == instr["params"][0])
            #print(f"Varible: {variable}, corresponding value {corresp_value} and instruction {instr}.")

        return self.assertTrue(all(is_correct))


    def test_variables_n_convert(self):
        """Tests if convert correctly translates variable parameters between circuit formats
        CunqaCircuit         <-> qiskit.QuantumCircuit
        ---------------------------------------------------
        Variables            <-> qiskit.Parameters 
        Variable expressions <-> qiskit.ParameterExpressions"""

        # Translate minimal CunqaCircuit w Parameter(Expression)s to QuantumCircuit
        cunqac = CunqaCircuit(2)
        cunqac.rx(Variable("x")                                , 0) # x
        cunqac.rz(sp.sin(Variable("y")/2) + Variable("z") + 3.2, 1) # sin(y/2) + z +3.2
        qc = convert(cunqac, "QuantumCircuit")

        # Translate minimal QuantumCircuit w Parameter(Expression)s to CunqaCircuit
        qc2 = QuantumCircuit(2)
        qc2.rx(Parameter("x")                                 , 0) # x
        qc2.rz((np.sin(Parameter("y")/2) + Parameter("z") + 3.2), 1) # sin(y/2) + z +3.2
        cunqac2 = convert(qc2, "CunqaCircuit")

        
        # Process QuantumCircuits to check equality. 
        # str needed, apparently qiskit Parameter types lack .__eq__() method
        params1 = [str(instr[0].params) for instr in qc.data]
        params2 = [str(instr[0].params) for instr in qc2.data]

        return self.assertListEqual(cunqac.instructions, cunqac2.instructions), self.assertListEqual(params1, params2)


    def test_compatible_variables_n_transpile(self):
        """Checks if no error is raised during transpilation, and wether assigned parameters are preserved. Depends on assign_parameters"""

        # Create arbitrary circuit with Variables
        circ_param = CunqaCircuit(2, id="parametric")
        x = variables('x:2'); y = variables('y:2')

        circ_param.rz(x[0], 0); circ_param.rz(y[0], 0)
        circ_param.rx(x[1], 1); circ_param.rx(y[1], 1)

        circ_param.cx(0,1)

        circ_param.ry(x[0], 0); circ_param.rx(y[0], 0)
        circ_param.rz(x[1], 1); circ_param.ry(y[1], 1)
        
        circ_param.measure_all()
        circ_param.assign_parameters({x[0]: np.pi, x[1]: 0, y[0]: np.pi/2, y[1]: 5})

        circ_transpiled = transpiler(circ_param, backend = self.fakeqmio_backend, opt_level = 3, initial_layout = None)

        # Circuit was modified in transpilation so new fixed parameters appeared. I extract the dictionaries in current, which are variable parameters
        # Additionally, I transform the dictionaries to tuples, that are hashable and can be contained in a set
        transpiled_current = set([tuple(sorted(e.items())) for e in circ_transpiled.current_params if isinstance(e, dict)])
        old_current = set([tuple(sorted(e.items())) for e in circ_param.current_params])

        return self.assertEqual(old_current, transpiled_current)


    def test_upgrade_variable_parameters(self):
        """Checks wether upgrade_parameters behaves as expected when upgrading Variable Parameters. Depends on assign_parameters."""

        circ_upgrade = CunqaCircuit(3)

        circ_upgrade.rx(Variable("x"), 0)
        circ_upgrade.rx(Variable("y"), 1)
        circ_upgrade.rx(Variable("z"), 2)

        circ_upgrade.measure_all()

        results = []
        # First, assign values to Parameters
        circ_upgrade.assign_parameters({Variable("x"): np.pi, Variable("y"): 0, Variable("z"): 0})
        qjob = self.qpus[-2].run(circ_upgrade, shots=1024, transpile = False)
        results.append(qjob.result.counts)

        # Upgrade with dicts
        qjob.upgrade_parameters({Variable("x"): 0, Variable("y"): 0, Variable("z"): 0})
        results.append(qjob.result.counts)

        # Upgrade with a dict with only some of the Variables (previous values are preserved)
        qjob.upgrade_parameters({Variable("x"): 0, Variable("y"): np.pi})
        results.append(qjob.result.counts)

        # Now with a list
        qjob.upgrade_parameters([0, 0, np.pi])
        results.append(qjob.result.counts)

        return self.assertListEqual(results, [{"001": 1024}, {"000": 1024}, {"010": 1024}, {"100": 1024}])
        

if __name__ == "__main__":
    unittest.main(verbosity=2)