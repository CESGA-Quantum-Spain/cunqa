Infrastructure JSON 
====================

Here we present the JSON format for deploying heterogeneous vQPU infrastructures. It is a very simple JSON, with a single key, _backends_, whose argument is a list of JSON paths to backends as specified in :doc:`backend_json_example`.

.. code-block:: json

    {
        "backends":[
            "/path/to/vqpu1.json",
            "/path/to/vqpu2.json"
        ]
    }