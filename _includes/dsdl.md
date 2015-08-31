{% if include.header_prefix == null %} {% assign HEADER_PREFIX = '##' %}
{% else %}                             {% assign HEADER_PREFIX = include.header_prefix %}
{% endif %}
{% if include.namespace == null %} {% assign TARGET_NAMESPACE = 'uavcan' %}
{% else %}                         {% assign TARGET_NAMESPACE = include.namespace %}
{% endif %}

{% assign dsdl_files = site.static_files | where: 'extname', '.uavcan' | sort: 'path' %}

{% for x in dsdl_files %}
{% include dsdl_parse_path.md input_path=x.path %}
{% if NAMESPACE contains TARGET_NAMESPACE %}
{% if NAMESPACE != prev_namespace %}
{{HEADER_PREFIX}} `{{NAMESPACE}}`
{% assign prev_namespace = NAMESPACE %}
{% endif %}
#{{HEADER_PREFIX}} `{{TYPE_NAME}}`
Full name: `{{FULL_TYPE_NAME}}`
{% if DEFAULT_DTID != null %}
Default data type ID: {{DEFAULT_DTID}}
{% endif %}
```python
{% include_relative {{RELATIVE_PATH}} %}
```
{% endif %}
{% endfor %}
