{# Filtra 'object' y 'builtins.object' de la lista de bases #}
{% set ns = namespace(new_bases=[]) %}
{% for b in bases %}
  {% if b != 'object' and b != 'builtins.object' %}
    {% set ns.new_bases = ns.new_bases + [b] %}
  {% endif %}
{% endfor %}
{% set bases = ns.new_bases %}
{% include "!autodoc/class.rst" %}
