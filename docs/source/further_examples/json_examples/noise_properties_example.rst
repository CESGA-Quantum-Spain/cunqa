Noise properties JSON
======================

The noise properties are specified as in the following example:

.. code-block:: json
    
    {
        "Qubits": 
        {
            "q[0]": 
            {
                "T1 (s)": 4.1215e-05,
                "T2 (s)": 4.3997e-05,
                "Drive Frequency (Hz)": 4358600000.0,
                "Measuring frequency (Hz)": 10276000000.0,
                "Readout duration (s)": 2.8599e-06,
                "Readout fidelity (RB)": 0.813,
                "Fidelity readout": 0.813,
                "T1 error (s)": 2.5871e-06,
                "T2 error (s)": 3.8227e-06
            },
            "q[1]": 
            {
                "T1 (s)": 7.0842e-05,
                "T2 (s)": 0.0001731,
                "Drive Frequency (Hz)": 4251600000.0,
                "Measuring frequency (Hz)": 9652700000.0,
                "Readout duration (s)": 5.7492e-06,
                "Readout fidelity (RB)": 0.9241,
                "Fidelity readout": 0.9241,
                "T1 error (s)": 7.5137e-06,
                "T2 error (s)": 7.411e-05
            }
        },
        "Q1Gates": 
        {
            "q[0]": 
            {
                "SX": 
                    {
                        "Gate duration (s)": 6.4e-08,
                        "Fidelity(RB)": 0.98992,
                        "Fidelity error": 0.0014552
                    },
                "Rz": 
                    {
                        "Gate duration (s)": 0,
                        "Fidelity(RB)": 1.0
                    }
            },
            "q[1]": 
            {
                "SX": 
                    {
                        "Gate duration (s)": 3.2e-08,
                        "Fidelity(RB)": 0.99802,
                        "Fidelity error": 0.00035564
                    },
                "Rz": 
                    {
                        "Gate duration (s)": 0,
                        "Fidelity(RB)": 1.0
                    }
            },
        },
        "Q2Gates(RB)": 
        {
            "0-1": 
            {
                "ECR": 
                {
                    "Control": 0,
                    "Target": 1,
                    "Duration (s)": 4.16e-07,
                    "Fidelity(RB)": 0.96657,
                    "Fidelity error": 0.0016152
                }
            }
        }
    }       


