CUNQA module
================
.. autosummary::
   :toctree: _autosummary
   :template: cunqa-module-template.rst
   :recursive:
   
   .. automodule:: {{ fullname }}

   {% block modules %}
   {% if modules %}
   .. raw:: html

      <h2> {{ _('Modules') }} </h2>

   .. autosummary::
      :toctree:
      :template: custom-module-template.rst
      :recursive:
   {% for item in modules %}
      {{ item }}
   {%- endfor %}
   {% endif %}
   {% endblock %}